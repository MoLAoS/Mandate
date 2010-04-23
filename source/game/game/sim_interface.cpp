// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "network_util.h"

#include "sim_interface.h"

#include "network_interface.h"
#include "client_interface.h"
#include "server_interface.h"

#include "profiler.h"

using namespace Glest::Net;
using namespace Glest::Game;

namespace Glest { namespace Sim {

const int speedValues[GameSpeed::COUNT] = {
	0,
	10,
	20,
	30,
	40,
	50,
	60,
	70
};

// =====================================================
//	class SkillCycleTable
// =====================================================

void SkillCycleTable::create(const TechTree *techTree) {
	_TRACE_FUNCTION();
	numEntries = theWorld.getSkillTypeFactory()->getSkillTypeCount();
	assert(numEntries);

	cycleTable = new CycleInfo[numEntries];
	for (int i=0; i < numEntries; ++i) {
		cycleTable[i] = theWorld.getSkillTypeFactory()->getType(i)->calculateCycleTime();
	}
}

struct SkillCycleTableMsgHeader {
	int32  type		:  8;
	uint32 count	: 24;
};

void SkillCycleTable::send(Socket *socket) const {
	_TRACE_FUNCTION();
	size_t dataSize = numEntries * sizeof(CycleInfo);

	assert(dataSize < 16777216);
	assert( (1<<24) == 16777216);

	SkillCycleTableMsgHeader hdr;
	hdr.type = MessageType::SKILL_CYCLE_TABLE;
	hdr.count = numEntries;
	Message::send(socket, &hdr, sizeof(SkillCycleTableMsgHeader));
	Message::send(socket, cycleTable, dataSize);
}

bool SkillCycleTable::receive(Socket *socket) {
	_TRACE_FUNCTION();
	delete cycleTable;
	cycleTable = 0;

	SkillCycleTableMsgHeader hdr;
	const size_t &hdrSize = sizeof(SkillCycleTableMsgHeader);
	if (!socket->peek(&hdr, hdrSize)) {
		return false;
	}
	const size_t payloadSize = hdr.count * sizeof(CycleInfo);
	if (socket->getDataToRead() >= hdrSize + payloadSize) {
		cycleTable = new CycleInfo[hdr.count];
		if (!socket->receive(&hdr, hdrSize)
		|| !socket->receive(cycleTable, payloadSize)) {
			delete cycleTable;
			cycleTable = 0;
			throw std::runtime_error(
				"SkillCycleTable::receive() : Socket lied, getDataToRead() said ok, receive() couldn't deliver");
		}
		return true;
	}
	return false;
}

// =====================================================
//	class SimulationInterface
// =====================================================

SimulationInterface::SimulationInterface(Program &program)
		: program(program)
		, game(0)
		, world(0)
		, stats(0)
		, savedGame(0)
		, paused(false)
		, gameOver(false)
		, quit(false)
		, commander(0)
		, speed(GameSpeed::NORMAL) {
}

SimulationInterface::~SimulationInterface() {
	_TRACE_FUNCTION();
	delete stats;
	stats = 0;
}


void SimulationInterface::constructGameWorld(GameState *g) {
	_TRACE_FUNCTION();
	delete stats;
	game = g;
	world = new World(this);
	stats = new Stats(this);
	commander = new Commander(this);
}

void SimulationInterface::destroyGameWorld() {
	_TRACE_FUNCTION();
	deleteValues(aiInterfaces.begin(), aiInterfaces.end());
	aiInterfaces.clear();
	delete world;
	world = 0;
	delete commander;
	commander = 0;

	// game was deleted, that's why we're here, null our ptr
	game = 0;
}

NetworkInterface* SimulationInterface::asNetworkInterface() {
	if (getNetworkRole() != GameRole::LOCAL) {
		return static_cast<NetworkInterface*>(this);
	}
	return 0;
}

ClientInterface* SimulationInterface::asClientInterface() {
	if (getNetworkRole() == GameRole::CLIENT) {
		return static_cast<ClientInterface*>(this);
	}
	return 0;
}

ServerInterface* SimulationInterface::asServerInterface() {
	if (getNetworkRole() == GameRole::SERVER) {
		return static_cast<ServerInterface*>(this);
	}
	return 0;
}

void SimulationInterface::loadWorld() {
	_TRACE_FUNCTION();
	const string &scenarioPath = gameSettings.getScenarioPath();
	string scenarioName = basename(scenarioPath);
	//preload
	world->preload();
	//tileset
	if (!world->loadTileset()) {
		throw runtime_error ( "The tileset could not be loaded. See glestadv-error.log" );
	}
	//tech, load before map because of resources
	if (!world->loadTech()) {
		throw runtime_error ( "The techtree could not be loaded. See glestadv-error.log" );
	}
	//map
	world->loadMap();

	//scenario
	if (!scenarioName.empty()) {
		theLang.loadScenarioStrings(scenarioPath, scenarioName);
		world->loadScenario(scenarioPath + "/" + scenarioName + ".xml");
	}
}

void SimulationInterface::initWorld() {
	_TRACE_FUNCTION();
	commander->init(world);
	world->init(savedGame ? savedGame->getChild("world") : NULL);

	// create (or receive) random number seeds for AIs
	int aiCount = 0;
	for (int i=0; i < world->getFactionCount(); ++i) {
		if (world->getFaction(i)->getCpuControl()) {
			++aiCount;
		}
	}
	int32 *seeds = (aiCount ? new int32[aiCount] : NULL);
	if (seeds) {
		syncAiSeeds(aiCount, seeds);
	}
	//create AIs
	int seedCount = 0;
	aiInterfaces.resize(world->getFactionCount());
	for (int i=0; i < world->getFactionCount(); ++i) {
		Faction *faction = world->getFaction(i);
		if (faction->getCpuControl() && ScriptManager::getPlayerModifiers(i)->getAiEnabled()) {
			aiInterfaces[i] = new AiInterface(*game, i, faction->getTeam(), seeds[seedCount++]);
			theLogger.add("Creating AI for faction " + intToStr(i), true);
		} else {
			aiInterfaces[i] = 0;
		}
	}
	delete [] seeds;
	createSkillCycleTable(world->getTechTree());
	ScriptManager::init(game);
}

int SimulationInterface::launchGame() {
	_TRACE_FUNCTION();
	Checksum checksum;
	world->getTileset()->doChecksum(checksum);
	world->getTechTree()->doChecksum(checksum);
	world->getMap()->doChecksum(checksum);

	// ready ?
	theLogger.add("Waiting for players...", true);
	waitUntilReady(checksum);
	startGame();
	world->activateUnits();
	return getNetworkRole() == GameRole::LOCAL ? 2 : -1;
}

void SimulationInterface::updateWorld() {
	// Ai-Interfaces
	for (int i = 0; i < world->getFactionCount(); ++i) {
		if (world->getFaction(i)->getCpuControl()
		&& ScriptManager::getPlayerModifiers(i)->getAiEnabled()) {
			aiInterfaces[i]->update();
		}
	}
	// World
	world->processFrame();
	frameProccessed();

	// give pending commands
	foreach (Commands, it, pendingCommands) {
		commander->giveCommand(it->toCommand());
	}
	pendingCommands.clear();
}

GameStatus SimulationInterface::checkWinner(){
	if (!gameOver) {
		if (gameSettings.getDefaultVictoryConditions()) {
			return checkWinnerStandard();
		} else {
			return checkWinnerScripted();
		}
	}
	return GameStatus::NO_CHANGE;
}

GameStatus SimulationInterface::checkWinnerStandard(){
	//lose
	bool lose= false;
	if(!hasBuilding(world->getThisFaction())){
		lose= true;
		for(int i=0; i<world->getFactionCount(); ++i){
			if(!world->getFaction(i)->isAlly(world->getThisFaction())){
				stats->setVictorious(i);
			}
		}
		gameOver= true;
		return GameStatus::LOST;
	}

	//win
	if(!lose){
		bool win= true;
		for(int i=0; i<world->getFactionCount(); ++i){
			if(i!=world->getThisFactionIndex()){
				if(hasBuilding(world->getFaction(i)) && !world->getFaction(i)->isAlly(world->getThisFaction())){
					win= false;
				}
			}
		}

		//if win
		if(win){
			for(int i=0; i< world->getFactionCount(); ++i){
				if(world->getFaction(i)->isAlly(world->getThisFaction())){
					stats->setVictorious(i);
				}
			}
			gameOver= true;
			return GameStatus::WON;
		}
	}
	return GameStatus::NO_CHANGE;
}

GameStatus SimulationInterface::checkWinnerScripted(){
	if (ScriptManager::getGameOver()) {
		gameOver = true;
		for (int i= 0; i<world->getFactionCount(); ++i) {
			if (ScriptManager::getPlayerModifiers(i)->getWinner()) {
				stats->setVictorious(i);
			}
		}
		if (ScriptManager::getPlayerModifiers(world->getThisFactionIndex())->getWinner()) {
			return GameStatus::WON;
		} else {
			return GameStatus::LOST;
		}
	}
	return GameStatus::NO_CHANGE;
}


bool SimulationInterface::hasBuilding(const Faction *faction){
	for(int i=0; i<faction->getUnitCount(); ++i){
		Unit *unit = faction->getUnit(i);
		if(unit->getType()->hasSkillClass(SkillClass::BE_BUILT) && unit->isAlive()){
			return true;
		}
	}
	return false;
}

GameSpeed SimulationInterface::pause() {
	if (speed != GameSpeed::PAUSED) {
		prevSpeed = speed;
		speed = GameSpeed::PAUSED;
	}
	return speed;
}

GameSpeed SimulationInterface::resume() {
	if (speed == GameSpeed::PAUSED) {
		speed = prevSpeed;
	}
	return speed;
}

GameSpeed SimulationInterface::incSpeed() {
	if (speed < GameSpeed::FASTEST) {
		++speed;
	}
	return speed;
}

GameSpeed SimulationInterface::decSpeed() {
	if (speed > GameSpeed::SLOWEST) {
		--speed;
	}
	return speed;
}

GameSpeed SimulationInterface::resetSpeed() {
	if (speed != GameSpeed::NORMAL) {
		speed = GameSpeed::NORMAL;
	}
	return speed;
}

int SimulationInterface::getUpdateLoops() {
	return 1;///@todo fix: using speedValues[speed]; NO floating point...
}

void SimulationInterface::doQuitGame(QuitSource source) {
	_TRACE_FUNCTION();
	quitGame(source);
	quit = true;
}

void SimulationInterface::requestCommand(Command *command) {
	Unit *unit = command->getCommandedUnit();

	if (command->isAuto()) {
		// Auto & AI generated commands go straight though
		pendingCommands.push_back(command);
	} else {
		// else add to request queue
		if (command->getArchetype() == CommandArchetype::GIVE_COMMAND) {
			requestedCommands.push_back(NetworkCommand(command));
		} else if (command->getArchetype() == CommandArchetype::CANCEL_COMMAND) {
			requestedCommands.push_back(NetworkCommand(NetworkCommandType::CANCEL_COMMAND, unit, Vec2i(-1)));
		}
	}
}

void SimulationInterface::doUpdateUnitCommand(Unit *unit) {
	const SkillType *old_st = unit->getCurrSkill();

	// if unit has command process it
	if (unit->anyCommand()) {
		// check if a command being 'watched' has finished
		if (unit->getCommandCallback() && unit->getCommandCallback() != unit->getCurrCommand()) {
			ScriptManager::commandCallback(unit);
		}
		unit->getCurrCommand()->getType()->update(unit);
	}
	// if no commands, add stop (or guard for pets) command
	if (!unit->anyCommand() && unit->isOperative()) {
		const UnitType *ut = unit->getType();
		unit->setCurrSkill(SkillClass::STOP);
		if (unit->getMaster() && ut->hasCommandClass(CommandClass::GUARD)) {
			unit->giveCommand(new Command(ut->getFirstCtOfClass(CommandClass::GUARD),
							CommandFlags(CommandProperties::AUTO), unit->getMaster()));
		} else {
			if (ut->hasCommandClass(CommandClass::STOP)) {
				unit->giveCommand(new Command(ut->getFirstCtOfClass(CommandClass::STOP), CommandFlags()));
			}
		}
	}
	//if unit is out of EP, it stops
	if (unit->computeEp()) {
		if (unit->getCurrCommand()) {
			unit->cancelCurrCommand();
		}
		unit->setCurrSkill(SkillClass::STOP);
	}
	updateSkillCycle(unit);

	if (unit->getCurrSkill() != old_st) {	// if starting new skill
		unit->resetAnim(theWorld.getFrameCount() + 1); // reset animation cycle for next frame
	}
	IF_DEBUG_EDITION(
		UnitStateRecord usr(unit);
		worldLog.addUnitRecord(usr);
	)
	postCommandUpdate(unit);
}

void SimulationInterface::updateSkillCycle(Unit *unit) {
	if (unit->getCurrSkill()->getClass() == SkillClass::MOVE) {
		unit->updateMoveSkillCycle();
	} else {
		unit->updateSkillCycle(skillCycleTable.lookUp(unit).getSkillFrames());
	}
}

void SimulationInterface::startFrame(int frame) {
	IF_DEBUG_EDITION(
		assert(frame);
		worldLog.newFrame(frame);
	)
}

void SimulationInterface::doUpdateAnim(Unit *unit) {
	if (unit->getCurrSkill()->getClass() == SkillClass::DIE) {
		unit->updateAnimDead();
	} else {
		const CycleInfo &inf = skillCycleTable.lookUp(unit);
		unit->updateAnimCycle(inf.getAnimFrames(), inf.getSoundOffset(), inf.getAttackOffset());
	}
	postAnimUpdate(unit);
}

void SimulationInterface::doUnitBorn(Unit *unit) {
	const CycleInfo inf = skillCycleTable.lookUp(unit);
	unit->updateSkillCycle(inf.getSkillFrames());
	unit->updateAnimCycle(inf.getAnimFrames());
	postUnitBorn(unit);
}

void SimulationInterface::doUpdateProjectile(Unit *u, Projectile pps, const Vec3f &start, const Vec3f &end) {
	updateProjectilePath(u, pps, start, end);
	postProjectileUpdate(u, pps->getEndFrame());
}

void SimulationInterface::changeRole(GameRole role) {
	_TRACE_FUNCTION();
	SimulationInterface *newThis = 0;
	switch (role) {
		case GameRole::LOCAL:
			newThis = new SimulationInterface(program);
			break;
		case GameRole::CLIENT:
			newThis = new ClientInterface(program);
			break;
		case GameRole::SERVER:
			newThis = new ServerInterface(program);
			break;
		default:
			throw std::runtime_error("Invalid GameRole passed to SimulationInterface::changeRole()");
	}
	program.setSimInterface(newThis);
}

} // namespace Sim

#if _GAE_DEBUG_EDITION_

namespace Debug {

UnitStateRecord::UnitStateRecord(Unit *unit) {
	this->unit_id = unit->getId();
	this->cmd_id = unit->anyCommand() ? unit->getCurrCommand()->getType()->getId() : -1;
	this->skill_id = unit->getCurrSkill()->getId();
	this->curr_pos_x = unit->getPos().x;
	this->curr_pos_y = unit->getPos().y;
	this->next_pos_x = unit->getNextPos().x;
	this->next_pos_y = unit->getNextPos().y;
	this->targ_pos_x = unit->getTargetPos().x;
	this->targ_pos_y = unit->getTargetPos().y;
	this->target_id  = unit->getTarget() ? unit->getTarget()->getId() : -1;
}

ostream& operator<<(ostream &lhs, const UnitStateRecord& state) {
	return lhs
		<< "Unit: " << state.unit_id
		<< ", CommandId: " << state.cmd_id
		<< ", SkillId: " << state.skill_id
		<< "\n\tCurr Pos: " << Vec2i(state.curr_pos_x, state.curr_pos_y)
		<< ", Next Pos: " << Vec2i(state.next_pos_x, state.next_pos_y)
		<< "\n\tTarg Pos: " << Vec2i(state.targ_pos_x, state.targ_pos_y)
		<< ", Targ Id: " << state.target_id
		<< endl;
}

ostream& operator<<(ostream &lhs, const FrameRecord& record) {
	if (record.frame != -1) {
		lhs	<< "Frame Record, frame number: " << record.frame << endl;
		foreach_const (FrameRecord, unitRecord, record) {
			lhs << *unitRecord;
		}
	}
	return lhs;
}

const char *gs_datafile = "game_state.gsd";
const char *gs_indexfile = "game_state.gsi";

WorldLog::WorldLog() {
	currFrame.frame = 0;
	// clear old data and index files
	indexFile = theFileFactory.getFileOps();
	indexFile->openWrite(gs_indexfile);
	dataFile = theFileFactory.getFileOps();
	dataFile->openWrite(gs_datafile);
}
WorldLog::~WorldLog() {
	delete indexFile;
	delete dataFile;
}

struct StateLogIndexEntry {
	int32	frame;
	int32	start;
	int32	end;
};

void WorldLog::writeFrame() {
	StateLogIndexEntry ndxEntry;
	ndxEntry.start = dataFile->tell();
	foreach (FrameRecord, it, currFrame) {
		dataFile->write(&(*it), sizeof(UnitStateRecord), 1);
	}
	ndxEntry.end = dataFile->tell();
	ndxEntry.frame = currFrame.frame;
	indexFile->write(&ndxEntry, sizeof(StateLogIndexEntry), 1);
}

void WorldLog::logFrame(int frame) {
	if (frame == -1) {
		stringstream ss;
		ss << currFrame;
		LOG_NETWORK( ss.str() );
	} else {
		assert(frame > 0);
		FileOps *f = theFileFactory.getFileOps();
		f->openRead(gs_indexfile);
		long pos = (frame - 1) * sizeof(StateLogIndexEntry);
		StateLogIndexEntry ndxEntry;
		f->seek(pos, SEEK_SET);
		f->read(&ndxEntry, sizeof(StateLogIndexEntry), 1);
		delete f;
		f = theFileFactory.getFileOps();
 		f->openRead(gs_datafile);
 		f->seek(ndxEntry.start, SEEK_SET);

		FrameRecord record;
		record.frame = frame;
		int numUpdates = (ndxEntry.end - ndxEntry.start) / sizeof(UnitStateRecord);
		assert((ndxEntry.end - ndxEntry.start) % sizeof(UnitStateRecord) == 0);

		if (numUpdates) {
			UnitStateRecord *unitRecords = new UnitStateRecord[numUpdates];
			f->read(unitRecords, sizeof(UnitStateRecord), numUpdates);
			for (int i=0; i < numUpdates; ++i) {
				record.push_back(unitRecords[i]);
			}
			delete [] unitRecords;
			stringstream ss;
			ss << record;
			LOG_NETWORK( ss.str() );
		} else {
			LOG_NETWORK( "Frame " + intToStr(frame) + " has no updates." );
		}
		delete f;
	}
}

} // namespace Debug

#endif // _GAE_DEBUG_EDITION_

} // namespace Game

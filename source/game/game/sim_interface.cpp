// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  Based on code Copyright (C) 2001-2008 Martiño Figueroa
//
//  GPL V2, see source/liscence.txt
// ==============================================================

#include "pch.h"

#include "client_interface.h"
#include "server_interface.h"

#include "profiler.h"

using namespace Glest::Net;

namespace Glest { namespace Sim {

/* if speed values are changed, lcm must be recalculated. Try to keep the lcm low, 
 * don't use values with lots of different factors */
const int GameSpeeds_lcm = GameConstants::updateFps; // least common multiple of speed values
const int speedValues[GameSpeed::COUNT] = {
	0,	// PAUSED
	20,	// SLOWEST
	30, // SLOW
	40, // NORMAL
	60, // FAST
	80	// FASTEST
};

const int speedIntervals[GameSpeed::COUNT] = {
	0,
	GameSpeeds_lcm / speedValues[GameSpeed::SLOWEST],	// 12,
	GameSpeeds_lcm / speedValues[GameSpeed::SLOW],		//  8,
	GameSpeeds_lcm / speedValues[GameSpeed::NORMAL],	//  6,
	GameSpeeds_lcm / speedValues[GameSpeed::FAST],		//  4,
	GameSpeeds_lcm / speedValues[GameSpeed::FASTEST]	//  3
};

int SimulationInterface::getUpdateInterval() const {
	return speedIntervals[speed];
}

// =====================================================
//	class SkillCycleTable
// =====================================================

SkillCycleTable::SkillCycleTable(RawMessage raw) {
	numEntries = raw.size / sizeof(CycleInfo);
	cycleTable = reinterpret_cast<CycleInfo*>(raw.data);
}

SkillCycleTable::~SkillCycleTable(){
	delete[] cycleTable;
}

void SkillCycleTable::create(const TechTree *techTree) {
	numEntries = g_world.getSkillTypeFactory().getSkillTypeCount();
	header.messageSize = numEntries * sizeof(CycleInfo);
	NETWORK_LOG( "SkillCycleTable built, numEntries = " << numEntries 
		<< ", messageSize = " << header.messageSize << " (@" << sizeof(CycleInfo) << ")" );
	//assert(numEntries);
	if(!numEntries){
		cycleTable = NULL;
		return;
	}

	cycleTable = new CycleInfo[numEntries];
	for (int i=0; i < numEntries; ++i) {
		cycleTable[i] = g_world.getSkillTypeFactory().getType(i)->calculateCycleTime();
	}
}

void SkillCycleTable::send(NetworkConnection* connection) const {
	Message::send(connection, &header, sizeof(MsgHeader));
	Message::send(connection, cycleTable, header.messageSize);
	NETWORK_LOG( "SkillCycleTable sent." );
}

bool SkillCycleTable::receive(NetworkConnection* connection) {
	delete[] cycleTable;
	cycleTable = 0;

	Socket *socket = connection->getSocket();
	if (!socket->peek(&header, sizeof(MsgHeader))) {
		return false;
	}
	if (socket->getDataToRead() >= sizeof(MsgHeader) + header.messageSize) {
		cycleTable = new CycleInfo[header.messageSize / sizeof(CycleInfo)];
		if (!socket->receive(&header, sizeof(MsgHeader))
		|| !socket->receive(cycleTable, header.messageSize)) {
			delete[] cycleTable;
			cycleTable = 0;
			throw std::runtime_error(
				"SkillCycleTable::receive() : Socket lied, getDataToRead() said ok, receive() couldn't deliver");
		}
		NETWORK_LOG( __FUNCTION__ << "(): got message, type: " << MessageTypeNames[MessageType(header.messageType)]
			<< ", messageSize: " << header.messageSize
		);
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
		, m_gaia(0)
		, commander(0)
		, speed(GameSpeed::NORMAL)
		, skillCycleTable(0) {
}

SimulationInterface::~SimulationInterface() {
	delete stats;
	stats = 0;
	delete m_gaia;
	m_gaia = 0;
	delete skillCycleTable;
	skillCycleTable = 0;
}

void SimulationInterface::constructGameWorld(GameState *g) {
	NETWORK_LOG( __FUNCTION__ );
	delete stats;
	game = g;
	world = new World(this);
	stats = new Stats(this);
	commander = new Commander(this);
	IF_DEBUG_EDITION(
		worldLog = new WorldLog();
	);
}

void SimulationInterface::destroyGameWorld() {
	NETWORK_LOG( __FUNCTION__ );
	deleteValues(aiInterfaces.begin(), aiInterfaces.end());
	aiInterfaces.clear();
	delete world;
	world = 0;
	delete commander;
	commander = 0;
	game = 0; // game was deleted, that's why we're here, null our ptr
	IF_DEBUG_EDITION(
		delete worldLog;
	);
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
	NETWORK_LOG( __FUNCTION__ );
	const string &scenarioPath = gameSettings.getScenarioPath();
	string scenarioName = basename(scenarioPath);
	//preload
	world->preload();
	//tech, load before map because of resources
	if (!world->loadTech()) {
		throw runtime_error("The techtree could not be loaded. See glestadv-error.log");
	}
	//tileset
	if (!world->loadTileset()) {
		throw runtime_error("The tileset could not be loaded. See glestadv-error.log");
	}
	//map
	world->loadMap();

	//scenario
	if (!scenarioName.empty()) {
		g_lang.loadScenarioStrings(scenarioPath, scenarioName);
		world->loadScenario(scenarioPath + "/" + scenarioName + ".xml");
	}
}

void SimulationInterface::initWorld() {
	NETWORK_LOG( __FUNCTION__ );
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
	// create AIs
	int seedCount = 0;
	aiInterfaces.resize(world->getFactionCount());
	for (int i=0; i < world->getFactionCount(); ++i) {
		Faction *faction = world->getFaction(i);
		if (faction->getCpuControl() && ScriptManager::getPlayerModifiers(i)->getAiEnabled()) {
			ControlType ctrl = faction->getControlType();
			if (ctrl >= ControlType::CPU_EASY || ctrl <= ControlType::CPU_MEGA) {
				aiInterfaces[i] = new Plan::GlestAiInterface(faction, seeds[seedCount++]);
			} //else if (ctrl == ControlType::PROTO_AI) {
				//aiInterfaces[i] = new AdvancedAiInterface(*game, i, faction->getTeam(), seeds[seedCount++]);
			//}
			g_logger.add("Creating AI for faction " + intToStr(i), true);
		} else {
			aiInterfaces[i] = 0;
		}
	}
	delete [] seeds;

	m_gaia = new Plan::Gaia(world->getGlestimals());
	m_gaia->init();

	doDataSync();

	createSkillCycleTable(world->getTechTree());
}

/** @return maximum update backlog (must be -1 for multiplayer) */
int SimulationInterface::launchGame() {
	NETWORK_LOG( __FUNCTION__ );
	// ready ?
	g_logger.add("Waiting for players...", true);
	try {
		waitUntilReady();
	} catch (NetworkError &e) {
		LOG_NETWORK( e.what() );
		throw e;
	}

	startGame();
	world->activateUnits();
	return getNetworkRole() == GameRole::LOCAL ? 12 : -1;
	//
}

void SimulationInterface::updateWorld() {
	// Ai-Interfaces
	for (int i = 0; i < world->getFactionCount(); ++i) {
		if (world->getFaction(i)->getCpuControl()
		&& ScriptManager::getPlayerModifiers(i)->getAiEnabled()) {
			aiInterfaces[i]->update();
		}
	}
	m_gaia->update();

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

GameStatus SimulationInterface::checkWinnerStandard() {
	//lose
	bool lose = false;
	if (!hasBuilding(world->getThisFaction())) {
		lose = true;
		for (int i=0; i < world->getFactionCount(); ++i) {
			if (!world->getFaction(i)->isAlly(world->getThisFaction())) {
				stats->setVictorious(i);
			}
		}
		gameOver = true;
		return GameStatus::LOST;
	}

	//win
	if (!lose) {
		bool win = true;
		for (int i=0; i < world->getFactionCount(); ++i) {
			if (i != world->getThisFactionIndex()) {
				if (hasBuilding(world->getFaction(i)) 
				&& !world->getFaction(i)->isAlly(world->getThisFaction())) {
					win = false;
				}
			}
		}

		//if win
		if (win) {
			for (int i=0; i < world->getFactionCount(); ++i) {
				if (world->getFaction(i)->isAlly(world->getThisFaction())) {
					stats->setVictorious(i);
				}
			}
			gameOver = true;
			return GameStatus::WON;
		}
	}
	return GameStatus::NO_CHANGE;
}

GameStatus SimulationInterface::checkWinnerScripted() {
	if (ScriptManager::getGameOver()) {
		gameOver = true;
		for (int i= 0; i < world->getFactionCount(); ++i) {
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
	for (int i=0; i < faction->getUnitCount(); ++i) {
		Unit *unit = faction->getUnit(i);
		if (unit->getType()->hasSkillClass(SkillClass::BE_BUILT)
		&& unit->isAlive() && !unit->getType()->getProperty(Property::WALL)) {
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

void SimulationInterface::doQuitGame(QuitSource source) {
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
		if (unit->getCommandCallback() != unit->getCurrCommand()->getId()) {
			int last = unit->getCommandCallback();
			ScriptManager::commandCallback(unit);
			// if the callback set a new callback we don't want to clear it
			// only clear if the callback id's are the same
			if (last == unit->getCommandCallback()) {
				unit->clearCommandCallback();
			}
		}
		unit->getCurrCommand()->getType()->update(unit);
	}
	// if no commands, add stop (or guard for pets) command
	if (!unit->anyCommand() && unit->isOperative()) {
		const UnitType *ut = unit->getType();
		unit->setCurrSkill(SkillClass::STOP);
		if (ut->hasCommandClass(CommandClass::STOP)) {
			unit->giveCommand(new Command(ut->getFirstCtOfClass(CommandClass::STOP), CommandFlags()));
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
		unit->resetAnim(g_world.getFrameCount() + 1); // reset animation cycle for next frame
	}
	IF_DEBUG_EDITION(
		UnitStateRecord usr(unit);
		worldLog->addUnitRecord(usr);
	)
	IF_MAD_SYNC_CHECKS(
		postCommandUpdate(unit);
	)
}

void SimulationInterface::updateSkillCycle(Unit *unit) {
	if (unit->getCurrSkill()->getClass() == SkillClass::MOVE) {
		unit->updateMoveSkillCycle();
	} else {
		unit->updateSkillCycle(skillCycleTable->lookUp(unit).getSkillFrames());
	}
}

void SimulationInterface::startFrame(int frame) {
	IF_DEBUG_EDITION(
		assert(frame);
		worldLog->newFrame(frame);
	)
}

void SimulationInterface::doUpdateAnim(Unit *unit) {
	if (unit->getCurrSkill()->getClass() == SkillClass::DIE) {
		unit->updateAnimDead();
	} else {
		const CycleInfo &inf = skillCycleTable->lookUp(unit);
		unit->updateAnimCycle(inf.getAnimFrames(), inf.getSoundOffset(), inf.getAttackOffset());
	}
	IF_MAD_SYNC_CHECKS(
		postAnimUpdate(unit);
	)
}

void SimulationInterface::doUnitBorn(Unit *unit) {
	const CycleInfo &inf = skillCycleTable->lookUp(unit);
	unit->updateSkillCycle(inf.getSkillFrames());
	unit->updateAnimCycle(inf.getAnimFrames());
	IF_MAD_SYNC_CHECKS(
		postUnitBorn(unit);
	)
}

void SimulationInterface::doUpdateProjectile(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end) {
	updateProjectilePath(u, pps, start, end);
	IF_MAD_SYNC_CHECKS(
		postProjectileUpdate(u, pps->getEndFrame());
	)
}

void SimulationInterface::changeRole(GameRole role) {
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

#if _GAE_DEBUG_EDITION_ && LOG_NETWORKING

namespace Debug {

UnitStateRecord::UnitStateRecord(Unit *unit) {
	unit_id = unit->getId();
	cmd_id = unit->anyCommand() ? unit->getCurrCommand()->getType()->getId() : -1;
	skill_id = unit->getCurrSkill()->getId();
	curr_pos_x = unit->getPos().x;
	curr_pos_y = unit->getPos().y;
	next_pos_x = unit->getNextPos().x;
	next_pos_y = unit->getNextPos().y;
	targ_pos_x = unit->getTargetPos().x;
	targ_pos_y = unit->getTargetPos().y;
	target_id  = unit->getTarget();
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
	indexFile = g_fileFactory.getFileOps();
	indexFile->openWrite(gs_indexfile);
	dataFile = g_fileFactory.getFileOps();
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
		NETWORK_LOG( currFrame );
		return;
	}
	assert(frame > 0);
	FileOps *f = g_fileFactory.getFileOps();
	f->openRead(gs_indexfile);
	long pos = (frame - 1) * sizeof(StateLogIndexEntry);
	StateLogIndexEntry ndxEntry;
	f->seek(pos, SEEK_SET);
	f->read(&ndxEntry, sizeof(StateLogIndexEntry), 1);
	delete f;
	f = g_fileFactory.getFileOps();
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
		NETWORK_LOG( record );
	} else {
		NETWORK_LOG( "Frame " << frame << " has no updates." );
	}
	delete f;
}

} // namespace Debug

#endif // _GAE_DEBUG_EDITION_ && LOG_NETWORKING

} // namespace Game

// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti?o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "faction.h"

#include <cassert>

#include "unit.h"
#include "world.h"
#include "upgrade.h"
#include "map.h"
#include "command.h"
#include "object.h"
#include "config.h"
#include "skill_type.h"
#include "core_data.h"
#include "renderer.h"
#include "script_manager.h"
#include "cartographer.h"
#include "game.h"
#include "earthquake_type.h"
#include "sound_renderer.h"
#include "sim_interface.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Net;

namespace Glest { namespace Entities {

// =====================================================
//  class Vec2iList, UnitPath & WaypointPath
// =====================================================

void Vec2iList::read(const XmlNode *node) {
	clear();
	stringstream ss(node->getStringValue());
	Vec2i pos;
	ss >> pos;
	while (pos != Vec2i(-1)) {
		push_back(pos);
		ss >> pos;
	}
}

void Vec2iList::write(XmlNode *node) const {
	stringstream ss;
	foreach_const(Vec2iList, it, (*this)) {
		ss << *it;
	}
	ss << Vec2i(-1);
	node->addAttribute("value", ss.str());
}

void UnitPath::read(const XmlNode *node) {
	Vec2iList::read(node);
	blockCount = node->getIntAttribute("blockCount");
}

void UnitPath::write(XmlNode *node) const {
	Vec2iList::write(node);
	node->addAttribute("blockCount", blockCount);
}

void WaypointPath::condense() {
	if (size() < 2) {
		return;
	}
	iterator prev, curr;
	prev = curr = begin();
	while (++curr != end()) {
		if (prev->dist(*curr) < 3.f) {
			prev = erase(prev);
		} else {
			++prev;
		}
	}
}

// =====================================================
// 	class Unit
// =====================================================

// ============================ Constructor & destructor =============================

/** Construct Unit object */
Unit::Unit(int id, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, Unit* master)
		: lastCommandUpdate(0)
		, nextCommandUpdate(0)
		, pos(pos)
		, lastPos(pos)
		, nextPos(pos)
		, targetPos(0)
		, targetVec(0.0f)
		, meetingPos(pos)
		, commandCallback(0)
		, hp_below_trigger(0)
		, hp_above_trigger(0)
		, attacked_trigger(false) {
	this->faction = faction;
	this->map = map;

	this->id = id;
	hp = 1;
	ep = 0;
	loadCount = 0;
	deadCount = 0;
	highlight = 0.f;
	progress2 = 0;
	kills = 0;

	targetRef = -1;
	targetField = Field::LAND;		// init just to keep it pretty in memory

	level= NULL;

	Random random(id);
	float rot = 0.f;
	//rot += random.randRange(-5, 5);

	lastRotation = rot;
	targetRotation = rot;
	rotation = rot;

	this->type = type;
	loadType = NULL;
	currSkill = getType()->getFirstStOfClass(SkillClass::STOP);	//starting skill
	UNIT_LOG(theWorld.getFrameCount() << "::Unit:" << id << " constructed at pos" << pos );

	toBeUndertaken = false;
	autoRepairEnabled = true;

	computeTotalUpgrade();
	hp = type->getMaxHp() / 20;

	fire = NULL;
}


Unit::Unit(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld)
		: targetRef(node->getOptionalIntValue("targetRef", -1))
		, effects(node->getChild("effects"))
		, effectsCreated(node->getChild("effectsCreated")) {
	this->faction = faction;
	this->map = map;

	id = node->getChildIntValue("id");

	string s;
	//hp loaded after recalculateStats()
	ep = node->getChildIntValue("ep");
	loadCount = node->getChildIntValue("loadCount");
	deadCount = node->getChildIntValue("deadCount");
	kills = node->getChildIntValue("kills");
	type = faction->getType()->getUnitType(node->getChildStringValue("type"));

	s = node->getChildStringValue("loadType");
	loadType = s == "null_value" ? NULL : tt->getResourceType(s);

	lastRotation = node->getChildFloatValue("lastRotation");
	targetRotation = node->getChildFloatValue("targetRotation");
	rotation = node->getChildFloatValue("rotation");

	progress2 = node->getChildIntValue("progress2");
	targetField = (Field)node->getChildIntValue("targetField");

	pos = node->getChildVec2iValue("pos"); //map->putUnitCells() will set this, so we reload it later
	lastPos = node->getChildVec2iValue("lastPos");
	nextPos = node->getChildVec2iValue("nextPos");
	targetPos = node->getChildVec2iValue("targetPos");
	targetVec = node->getChildVec3fValue("targetVec");
	// meetingPos loaded after map->putUnitCells()
	faceTarget = node->getChildBoolValue("faceTarget");
	useNearestOccupiedCell = node->getChildBoolValue("useNearestOccupiedCell");
	s = node->getChildStringValue("currSkill");
	currSkill = s == "null_value" ? NULL : type->getSkillType(s);

	nextCommandUpdate = node->getChildIntValue("nextCommandUpdate");
	lastCommandUpdate = node->getChildIntValue("lastCommandUpdate");
	nextAnimReset = node->getChildIntValue("nextAnimReset");
	lastAnimReset = node->getChildIntValue("lastAnimReset");

	highlight = node->getChildFloatValue("highlight");
	toBeUndertaken = node->getChildBoolValue("toBeUndertaken");
	autoRepairEnabled = node->getChildBoolValue("autoRepairEnabled");

	if (type->hasMeetingPoint()) {
		meetingPos = node->getChildVec2iValue("meeting-point");
	}

	XmlNode *n = node->getChild("commands");
	for(int i = 0; i < n->getChildCount(); ++i) {
		commands.push_back(new Command(n->getChild("command", i), type, faction->getType()));
	}

	unitPath.read(node->getChild("unitPath"));
	waypointPath.read(node->getChild("waypointPath"));

	totalUpgrade.reset();
	computeTotalUpgrade();

	fire = NULL;
	faction->add(this);

	recalculateStats();
	hp = node->getChildIntValue("hp"); // HP will be at max due to recalculateStats

	if (hp) {
		map->putUnitCells(this, node->getChildVec2iValue("pos"));
		meetingPos = node->getChildVec2iValue("meetingPos"); // putUnitCells sets this, so we reset it here
	}
	if(type->hasSkillClass(SkillClass::BE_BUILT) && !type->hasSkillClass(SkillClass::MOVE)) {
		map->flatternTerrain(this);
	}
	if(node->getChildBoolValue("fire")) {
		decHp(0); // trigger logic to start fire system
	}
}

/** delete stuff */
Unit::~Unit() {
	//remove commands
	while (!commands.empty()) {
		delete commands.back();
		commands.pop_back();
	}
	UNIT_LOG(theWorld.getFrameCount() << "::Unit:" << id << " deleted." );
}

void Unit::save(XmlNode *node) const {
	XmlNode *n;
	node->addChild("id", id);
	node->addChild("hp", hp);
	node->addChild("ep", ep);
	node->addChild("loadCount", loadCount);
	node->addChild("deadCount", deadCount);
	node->addChild("nextCommandUpdate", nextCommandUpdate);
	node->addChild("lastCommandUpdate", lastCommandUpdate);
	node->addChild("nextAnimReset", nextAnimReset);
	node->addChild("lastAnimReset", lastAnimReset);
	node->addChild("highlight", highlight);
	node->addChild("progress2", progress2);
	node->addChild("kills", kills);
	node->addChild("targetRef", targetRef);
	node->addChild("targetField", targetField);
	node->addChild("pos", pos);
	node->addChild("lastPos", lastPos);
	node->addChild("nextPos", nextPos);
	node->addChild("targetPos", targetPos);
	node->addChild("targetVec", targetVec);
	node->addChild("meetingPos", meetingPos);
	node->addChild("faceTarget", faceTarget);
	node->addChild("useNearestOccupiedCell", useNearestOccupiedCell);	
	node->addChild("lastRotation", lastRotation);
	node->addChild("targetRotation", targetRotation);
	node->addChild("rotation", rotation);
	node->addChild("type", type->getName());
	node->addChild("loadType", loadType ? loadType->getName() : "null_value");
	node->addChild("currSkill", currSkill ? currSkill->getName() : "null_value");

	node->addChild("toBeUndertaken", toBeUndertaken);
//	node->addChild("alive", alive);
	node->addChild("autoRepairEnabled", autoRepairEnabled);

	if (type->hasMeetingPoint()) {
		node->addChild("meeting-point", meetingPos);
	}

	effects.save(node->addChild("effects"));
	effectsCreated.save(node->addChild("effectsCreated"));

	node->addChild("fire", fire ? true : false);

	unitPath.write(node->addChild("unitPath"));
	waypointPath.write(node->addChild("waypointPath"));

	n = node->addChild("commands");
	for(Commands::const_iterator i = commands.begin(); i != commands.end(); ++i) {
		(*i)->save(n->addChild("command"));
	}
}


// ====================================== get ======================================

/** @param from position to search from
  * @return nearest cell to from that is occuppied
  * @todo re-implement with Dijkstra search
  */
Vec2i Unit::getNearestOccupiedCell(const Vec2i &from) const {
	int size = type->getSize();

	if(size == 1) {
		return pos;
	} else {
		float nearestDist = 100000.f;
		Vec2i nearestPos(-1);

		for (int x = 0; x < size; ++x) {
			for (int y = 0; y < size; ++y) {
				if (!type->hasCellMap() || type->getCellMapCell(x, y)) {
					Vec2i currPos = pos + Vec2i(x, y);
					float dist = from.dist(currPos);
					if (dist < nearestDist) {
						nearestDist = dist;
						nearestPos = currPos;
					}
				}
			}
		}
		// check for empty cell maps
		assert(nearestPos != Vec2i(-1));
		return nearestPos;
	}
}
/* *
 * If the unit has a cell map, then return the nearest position to the center that is a free cell
 * in the cell map or pos if no cell map.
 */
/*
Vec2i Unit::getCellPos() const {

	if (type->hasCellMap()) {
		//find nearest pos to center that is free
		Vec2i centeredPos = getCenteredPos();
		float nearestDist = 100000.f;
		Vec2i nearestPos = pos;

		for (int i = 0; i < type->getSize(); ++i) {
			for (int j = 0; j < type->getSize(); ++j) {
				if (type->getCellMapCell(i, j)) {
					Vec2i currPos = pos + Vec2i(i, j);
					float dist = currPos.dist(centeredPos);
					if (dist < nearestDist) {
						nearestDist = dist;
						nearestPos = currPos;
					}
				}
			}
		}
		return nearestPos;
	}
	return pos;
}*/

/*
float Unit::getVerticalRotation() const{
	/ *if(type->getProperty(UnitType::pRotatedClimb) && currSkill->getClass()==SkillClass::MOVE){
		float heightDiff= map->getCell(pos)->getHeight() - map->getCell(targetPos)->getHeight();
		float dist= pos.dist(targetPos);
		return radToDeg(atan2(heightDiff, dist));
	}* /
	return 0.f;
}
*/
/** query completeness of thing this unit is producing
  * @return percentage complete, or -1 if not currently producing anything */
int Unit::getProductionPercent() const {
	if(anyCommand()) {
		const ProducibleType *produced = commands.front()->getType()->getProduced();
		if(produced) {
			return clamp(progress2 * 100 / produced->getProductionTime(), 0, 100);
		}
	}
	return -1;
}

/** query next availale level @return next level, or NULL */
const Level *Unit::getNextLevel() const{
	if(level==NULL && type->getLevelCount()>0){
		return type->getLevel(0);
	}
	else{
		for(int i=1; i<type->getLevelCount(); ++i){
			if(type->getLevel(i-1)==level){
				return type->getLevel(i);
			}
		}
	}
	return NULL;
}

/** retrieve name description, levelName + unitTypeName */
string Unit::getFullName() const{
	string str;
	if(level!=NULL){
		str+= level->getName() + " ";
	}
	str+= type->getName();
	return str;
}

// ====================================== is ======================================

/** query unit interestingness
  * @param iut the type of interestingness you're interested in
  * @return true if this unit is interesting in the way you're interested in
  */
bool Unit::isInteresting(InterestingUnitType iut) const{
	switch(iut){
	case InterestingUnitType::IDLE_HARVESTER:
		if(type->hasCommandClass(CommandClass::HARVEST)) {
			if(!commands.empty()) {
				const CommandType *ct = commands.front()->getType();
				if(ct) {
					return ct->getClass() == CommandClass::STOP;
				}
			}
		}
		return false;

	case InterestingUnitType::BUILT_BUILDING:
		return type->hasSkillClass(SkillClass::BE_BUILT) && isBuilt();
	case InterestingUnitType::PRODUCER:
		return type->hasSkillClass(SkillClass::PRODUCE);
	case InterestingUnitType::DAMAGED:
		return isDamaged();
	case InterestingUnitType::STORE:
		return type->getStoredResourceCount() > 0;
	default:
		return false;
	}
}

/** find a repair command type that can repair a unit with
  * @param u the unit in need of repairing 
  * @return a RepairCommandType that can repair u, or NULL
  */
const RepairCommandType * Unit::getRepairCommandType(const Unit *u) const {
	for(int i = 0; i < type->getCommandTypeCount<RepairCommandType>(); i++) {
		const RepairCommandType *rct = type->getCommandType<RepairCommandType>(i);
		const RepairSkillType *rst = rct->getRepairSkillType();
		if( (!rst->isSelfOnly() || this == u)
		&& (rst->isSelfAllowed() || this != u)
		&& (rct->canRepair(u->type))) {
			return rct;
		}
	}
	return 0;
}

float Unit::getProgress() const {
	return float(theWorld.getFrameCount() - lastCommandUpdate)
			/	float(nextCommandUpdate - lastCommandUpdate);
}

float Unit::getAnimProgress() const {
	return float(theWorld.getFrameCount() - lastAnimReset)
			/	float(nextAnimReset - lastAnimReset);
}

// ====================================== set ======================================

void Unit::setCommandCallback() {
	commandCallback = commands.front()->getId();
}

/** sets the current skill */
void Unit::setCurrSkill(const SkillType *newSkill) {
	assert(newSkill);
	//COMMAND_LOG(theWorld.getFrameCount() << "::Unit:" << id << " skill set => " << SkillClassNames[currSkill->getClass()] );
	if (newSkill->getClass() == SkillClass::STOP && currSkill->getClass() == SkillClass::STOP) {
		return;
	}
	if (newSkill != currSkill) {
		while(!eyeCandy.empty()){
			eyeCandy.back()->fade();
			eyeCandy.pop_back();
		}
	}
	progress2 = 0;
	currSkill = newSkill;
	notifyObservers(UnitObserver::eStateChange);
	for (unsigned i = 0; i < currSkill->getEyeCandySystemCount(); ++i) {
		UnitParticleSystem *ups = currSkill->getEyeCandySystem(i)->createUnitParticleSystem();
		ups->setPos(getCurrVector());
		//ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
		eyeCandy.push_back(ups);
		theRenderer.manageParticleSystem(ups, ResourceScope::GAME);
	}
}

/** sets unit's target */
void Unit::setTarget(const Unit *unit, bool faceTarget, bool useNearestOccupiedCell) {
	if(!unit) {
		targetRef = -1;
		return;
	}
	targetRef = unit->getId();
	this->faceTarget = faceTarget;
	this->useNearestOccupiedCell = useNearestOccupiedCell;
	updateTarget(unit);
}

/** sets unit's position @param pos position to set 
  * @warning sets Unit data members only, does not place/move on map */
void Unit::setPos(const Vec2i &pos){
	this->lastPos = this->pos;
	this->pos = pos;
	this->meetingPos = pos - Vec2i(1);
	
	// make sure it's not invalid if they build at 0,0
	if(pos.x == 0 && pos.y == 0) {
		this->meetingPos = pos + Vec2i(type->getSize());
	}
}

/** sets targetRotation */
void Unit::face(const Vec2i &nextPos) {
	Vec2i relPos = nextPos - pos;
	Vec2f relPosf = Vec2f((float)relPos.x, (float)relPos.y);
	targetRotation = radToDeg(atan2f(relPosf.x, relPosf.y));
}

void Unit::startAttackSystems(const AttackSkillType *ast) {
	Renderer &renderer = Renderer::getInstance();

	Projectile *psProj = 0;
	Splash *psSplash;

	ProjectileType *pstProj = ast->getProjParticleType();
	SplashType *pstSplash = ast->getSplashParticleType();

	Vec3f startPos = this->getCurrVector();
	Vec3f endPos = this->getTargetVec();

	//make particle system
	const Tile *sc = map->getTile(Map::toTileCoords(this->getPos()));
	const Tile *tsc = map->getTile(Map::toTileCoords(this->getTargetPos()));
	bool visible = sc->isVisible(theWorld.getThisTeamIndex()) || tsc->isVisible(theWorld.getThisTeamIndex());

	//projectile
	if (pstProj != NULL) {
		psProj = pstProj->createProjectileParticleSystem();

		switch (pstProj->getStart()) {
			case ProjectileStart::SELF:
				break;

			case ProjectileStart::TARGET:
				startPos = this->getTargetVec();
				break;

			case ProjectileStart::SKY: {
					Random random(id);
					float skyAltitude = 30.f;
					startPos = endPos;
					startPos.x += random.randRange(-skyAltitude / 8.f, skyAltitude / 8.f);
					startPos.y += skyAltitude;
					startPos.z += random.randRange(-skyAltitude / 8.f, skyAltitude / 8.f);
				}
				break;
		}

		theSimInterface->doUpdateProjectile(this, psProj, startPos, endPos);
		// game network interface calls setPath() on psProj, differently for clients/servers
		//theNetworkManager.getNetworkInterface()->doUpdateProjectile(this, psProj, startPos, endPos);
		
		if(pstProj->isTracking() && targetRef != -1) {
			Unit *target = theSimInterface->getUnitFactory().getUnit(targetRef);
			psProj->setTarget(target);
			psProj->setDamager(new ParticleDamager(this, target, &theWorld, theGame.getGameCamera()));
		} else {
			psProj->setDamager(new ParticleDamager(this, NULL, &theWorld, theGame.getGameCamera()));
		}
		psProj->setVisible(visible);
		renderer.manageParticleSystem(psProj, ResourceScope::GAME);
	} else {
		theWorld.hit(this);
	}

	//splash
	if (pstSplash != NULL) {
		psSplash = pstSplash->createSplashParticleSystem();
		psSplash->setPos(endPos);
		psSplash->setVisible(visible);
		renderer.manageParticleSystem(psSplash, ResourceScope::GAME);
		if (pstProj != NULL) {
			psProj->link(psSplash);
		}
	}
#ifdef EARTHQUAKE_CODE
	const EarthquakeType *et = ast->getEarthquakeType();
	if (et) {
		et->spawn(*map, this, this->getTargetPos(), 1.f);
		if (et->getSound()) {
			// play rather visible or not
			theSoundRenderer.playFx(et->getSound(), getTargetVec(), theGame.getGameCamera()->getPos());
		}
		// FIXME: hacky mechanism of keeping attackers from walking into their own earthquake :(
		this->finishCommand();
	}
#endif
}

// =============================== Render related ==================================
/*
Vec3f Unit::getCurrVectorFlat() const {
	Vec3f v(static_cast<float>(pos.x),  computeHeight(pos), static_cast<float>(pos.y));

	if (currSkill->getClass() == SkillClass::MOVE) {
		Vec3f last(static_cast<float>(lastPos.x),
				computeHeight(lastPos),
				static_cast<float>(lastPos.y));
		v = v.lerp(progress, last);
	}

	float halfSize = type->getSize() / 2.f;
	v.x += halfSize;
	v.z += halfSize;

	return v;
}
*/
// =================== Command list related ===================

/** query first available (and currently executable) command type of a class 
  * @param commandClass CommandClass of interest
  * @return the first executable CommandType matching commandClass, or NULL
  */
const CommandType *Unit::getFirstAvailableCt(CommandClass commandClass) const {
	typedef vector<CommandType*> CommandTypes;
	const  CommandTypes &cmdTypes = type->getCommandTypes(commandClass);
	foreach_const (CommandTypes, it, cmdTypes) {
		if (faction->reqsOk(*it)) {
			return *it;
		}
	}
	return 0;
	/*
	for(int i = 0; i < type->getCommandTypeCount(); ++i) {
		const CommandType *ct = type->getCommandType(i);
		if(ct && ct->getClass() == commandClass && faction->reqsOk(ct)) {
			return ct;
		}
	}
	return NULL;
	*/
}

/**get Number of commands
 * @return the size of the commands
 */
unsigned int Unit::getCommandSize() const{
	return commands.size();
}

/** give one command, queue or clear command queue and push back (depending on flags)
  * @param command the command to execute
  * @return a CommandResult describing success or failure
  */
CommandResult Unit::giveCommand(Command *command) {
	const CommandType *ct = command->getType();
	COMMAND_LOG(
		theWorld.getFrameCount() << "::Unit:" << id << " command given: " 
		<< CommandClassNames[command->getType()->getClass()]
	);
	if(ct->getClass() == CommandClass::SET_MEETING_POINT) {
		if(command->isQueue() && !commands.empty()) {
			commands.push_back(command);
		} else {
			meetingPos = command->getPos();
			delete command;
		}
		return CommandResult::SUCCESS;
	}

	if(ct->isQueuable() || command->isQueue()) {
		//cancel current command if it is not queuable or marked to be queued
		if(!commands.empty() && !commands.front()->getType()->isQueuable() && !command->isQueue()) {
			cancelCommand();
			unitPath.clear();
		}
	} else {
		//empty command queue
		clearCommands();
		unitPath.clear();

		//for patrol commands, remember where we started from
		if(ct->getClass() == CommandClass::PATROL) {
			command->setPos2(pos);
		}
	}

	//check command
	CommandResult result = checkCommand(*command);
	//COMMAND_LOG( "NO_RESERVE_RESOURCES flag is " << (command->isReserveResources() ? "not " : "" ) << "set,"
	//	<< " command result = " << CommandResultNames[result] );
	if(result == CommandResult::SUCCESS){
		applyCommand(*command);
		commands.push_back(command);

		if(faction->isThisFaction() && command->getUnit() && !command->isAuto()) {
			command->getUnit()->resetHighlight();
		}
	} else {
		delete command;
	}
	if (commands.empty() || commands.front()->getType()->getClass() == CommandClass::STOP) {
		notifyObservers(UnitObserver::eStateChange);
	}	
	return result;
}

/** removes current command (and any queued Set meeting point commands) 
  * @return the command now at the head of the queue (the new current command) */
Command *Unit::popCommand() {
	//pop front
	//COMMAND_LOG( theWorld.getFrameCount() << "::Unit:" << id << " " 
	//	<< CommandClassNames[commands.front()->getType()->getClass()] << " command popped." );
	delete commands.front();
	commands.erase(commands.begin());
	unitPath.clear();

	Command *command = commands.empty() ? NULL : commands.front();

	// we don't let hacky set meeting point commands actually get anywhere
	while(command && command->getType()->getClass() == CommandClass::SET_MEETING_POINT) {
		setMeetingPos(command->getPos());
		delete command;
		commands.erase(commands.begin());
		command = commands.empty() ? NULL : commands.front();
	}
	//if ( command ) {
	//	COMMAND_LOG(theWorld.getFrameCount() << "::Unit:" << id << " " 
	//		<< CommandClassNames[commands.front()->getType()->getClass()] << " command now front of queue." );
	//}
	if (commands.empty() || commands.front()->getType()->getClass() == CommandClass::STOP) {
		notifyObservers(UnitObserver::eStateChange);
	}
	return command;
}
/** pop current command (used when order is done) 
  * @return CommandResult::SUCCESS, or CommandResult::FAIL_UNDEFINED on catastrophic failure
  */
CommandResult Unit::finishCommand() {
	//is empty?
	if(commands.empty()) {
		return CommandResult::FAIL_UNDEFINED;
	}
	//COMMAND_LOG( theWorld.getFrameCount() << "::Unit:" << intToStr(id) << " " 
	//	<< CommandClassNames[commands.front()->getType()->getClass()] << " command finished." );

	Command *command = popCommand();

	//for patrol command, remember where we started from
	if(command && command->getType()->getClass() == CommandClass::PATROL) {
		command->setPos2(pos);
	}
	if (commands.empty() || commands.front()->getType()->getClass() == CommandClass::STOP) {
		notifyObservers(UnitObserver::eStateChange);
	}
	return CommandResult::SUCCESS;
}

/** cancel command on back of queue */
CommandResult Unit::cancelCommand() {

	//is empty?
	if(commands.empty()){
		return CommandResult::FAIL_UNDEFINED;
	}
	//COMMAND_LOG(theWorld.getFrameCount() << "::Unit:" << id << " queued " 
	//	<< CommandClassNames[commands.front()->getType()->getClass()] << " command cancelled." );

	//undo command
	undoCommand(*commands.back());

	//delete ans pop command
	delete commands.back();
	commands.pop_back();

	//clear routes
	unitPath.clear();
	if (commands.empty() || commands.front()->getType()->getClass() == CommandClass::STOP) {
		notifyObservers(UnitObserver::eStateChange);
	}
	return CommandResult::SUCCESS;
}

/** cancel current command */
CommandResult Unit::cancelCurrCommand() {
	//is empty?
	if(commands.empty()) {
		return CommandResult::FAIL_UNDEFINED;
	}
	//COMMAND_LOG(theWorld.getFrameCount() << "::Unit:" << intToStr(id) << " current " 
	//	<< CommandClassNames[commands.front()->getType()->getClass()] << " command cancelled." );

	//undo command
	undoCommand(*commands.front());

	Command *command = popCommand();
	if (commands.empty() || commands.front()->getType()->getClass() == CommandClass::STOP) {
		notifyObservers(UnitObserver::eStateChange);
	}
	return CommandResult::SUCCESS;
}

// =================== route stack ===================

/** Creates a unit, places it on the map and applies static costs for starting units
  * @param startingUnit true if this is a starting unit.
  */
void Unit::create(bool startingUnit) {
	UNIT_LOG( theWorld.getFrameCount() << "::Unit:" << id << " created." );
	faction->add(this);
	lastPos = Vec2i(-1);
	map->putUnitCells(this, pos);
	if(startingUnit){
		faction->applyStaticCosts(type);
	}
	nextCommandUpdate = -1;
	setCurrSkill(type->getStartSkill());
}

/** Give a unit life. Called when a unit becomes 'operative'
  */
void Unit::born(){
	UNIT_LOG(theWorld.getFrameCount() << "::Unit:" << id + " born." );
	faction->addStore(type);
	faction->applyStaticProduction(type);
	setCurrSkill(SkillClass::STOP);
	computeTotalUpgrade();
	recalculateStats();
	hp= type->getMaxHp();
	faction->checkAdvanceSubfaction(type, true);
	theWorld.getCartographer()->applyUnitVisibility(this);
	theSimInterface->doUnitBorn(this);
}

void checkTargets(const Unit *dead) {
	typedef list<ParticleSystem*> psList;
	const psList &list = theRenderer.getParticleManager()->getList();
	foreach_const (psList, it, list) {
		if (*it && (*it)->isProjectile()) {
			Projectile* pps = static_cast<Projectile*>(*it);
			if (pps->getTarget() == dead) {
				pps->setTarget(NULL);
			}
		}
	}
}


/**
 * Do everything that should happen when a unit dies, except remove them from the faction.  Should
 * only be called when a unit's HPs are zero or less.
 */
void Unit::kill(const Vec2i &lastPos, bool removeFromCells) {
	assert(hp <= 0);
	UNIT_LOG(theWorld.getFrameCount() << "::Unit:" << id + " killed." );
	hp = 0;

	World::getCurrWorld()->hackyCleanUp(this);
	theWorld.getCartographer()->removeUnitVisibility(this);

	if (fire != NULL) {
		fire->fade();
		fire = NULL;
	}

	// do the cleaning
	if (removeFromCells) {
		map->clearUnitCells(this, lastPos);
	}

	if (!isBeingBuilt()) {
		faction->removeStore(type);
	}
	setCurrSkill(SkillClass::DIE);

	// no longer needs static resources
	if (isBeingBuilt()) {
		faction->deApplyStaticConsumption(type);
	} else {
		faction->deApplyStaticCosts(type);
	}

	Died(this);

	notifyObservers(UnitObserver::eKill);
	clearCommands();

	// hack... 'tracking' particle systems might reference this, 'this' will soon be deleted...
	checkTargets(this);

	// random decay time
	deadCount = Random(id).randRange(-256, 256);
	//deadCount = 0;
}

// =================== Referencers ===================



// =================== Other ===================

/** Deduce a command based on a location and/or unit clicked
  * @param pos position clicked (or position of targetUnit)
  * @param targetUnit unit clicked or NULL
  * @return the CommandType to execute
  */
const CommandType *Unit::computeCommandType(const Vec2i &pos, const Unit *targetUnit) const{
	const CommandType *commandType = NULL;
	Tile *sc = map->getTile(Map::toTileCoords(pos));

	if (targetUnit) {
		//attack enemies
		if (!isAlly(targetUnit)) {
			commandType = type->getAttackCommand(targetUnit->getCurrZone());

		//repair allies
		} else {
			commandType = getRepairCommandType(targetUnit);
		}
	} else {
		//check harvest command
		Resource *resource = sc->getResource();
		if (resource != NULL) {
			commandType = type->getHarvestCommand(resource->getType());
		}
	}

	//default command is move command
	if (!commandType) {
		commandType = type->getFirstCtOfClass(CommandClass::MOVE);
	}

	if (!commandType && type->hasMeetingPoint()) {
		commandType = type->getFirstCtOfClass(CommandClass::SET_MEETING_POINT);
	}

	return commandType;
}

/** called to update animation cycle on a dead unit */
void Unit::updateAnimDead() {
	assert(currSkill->getClass() == SkillClass::DIE);
	// if dead, set startFrame to last frame, endFrame to this frame
	// to keep the cycle at the 'end' so getAnimProgress() always returns 1.f
	const int &frame = theWorld.getFrameCount();
	this->lastAnimReset = frame - 1;
	this->nextAnimReset = frame;
}

/** called at the end of an animation cycle, or on anim reset, sets next cycle end frame,
  * sound start time and attack start time
  * @param frameOffset the number of frames the new anim cycle with take
  * @param soundOffset the number of frames from now to start the skill sound (or -1 if no sound)
  * @param attackOffset the number  of frames from now to start attack systems (or -1 if no attack)*/
void Unit::updateAnimCycle(int frameOffset, int soundOffset, int attackOffset) {
	assert(currSkill->getClass() != SkillClass::DIE);
	if (frameOffset == -1) { // hacky handle move skill
		assert(currSkill->getClass() == SkillClass::MOVE);
		static const float speedModifier = 1.f / GameConstants::speedDivider / float(WORLD_FPS);
		float animSpeed = currSkill->getAnimSpeed() * speedModifier;
		//if moving to a higher cell move slower else move faster
		float heightDiff = map->getCell(lastPos)->getHeight() - map->getCell(pos)->getHeight();
		float heightFactor = clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
		animSpeed *= heightFactor;

		// calculate skill cycle length
		frameOffset = int(1.0000001f / animSpeed);
	}
	const int &frame = theWorld.getFrameCount();
	assert(frameOffset > 0);
	this->lastAnimReset = frame;
	this->nextAnimReset = frame + frameOffset;
	if (soundOffset > 0) {
		this->soundStartFrame = frame + soundOffset;
	} else {
		this->soundStartFrame = -1;
	}
	if (attackOffset > 0) {
		this->attackStartFrame = frame + attackOffset;
	} else {
		this->attackStartFrame = -1;
	}

}

/** called after a command is updated and a skill is selected
  * @param frameOffset the number of frames the next skill cycle with take */
void Unit::updateSkillCycle(int frameOffset) {
	//assert(currSkill->getClass() != SkillClass::MOVE || isNetworkClient());
	lastCommandUpdate = theWorld.getFrameCount();
	nextCommandUpdate = theWorld.getFrameCount() + frameOffset;
}

/** called by the server only, updates a skill cycle for the move skill */
void Unit::updateMoveSkillCycle() {
//	assert(!isNetworkClient());
	assert(currSkill->getClass() == SkillClass::MOVE);
	static const float speedModifier = 1.f / GameConstants::speedDivider / float(WORLD_FPS);

	float progressSpeed = getSpeed() * speedModifier;
	if (pos.x != nextPos.x && pos.y != nextPos.y) { //if moving in diagonal move slower
		progressSpeed *= 0.71f;
	}
	//if moving to a higher cell move slower else move faster
	float heightDiff = map->getCell(lastPos)->getHeight() - map->getCell(pos)->getHeight();
	float heightFactor = clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
	progressSpeed *= heightFactor;

	// reset lastCommandUpdate and calculate next skill cycle length	
	lastCommandUpdate = theWorld.getFrameCount();
	nextCommandUpdate = theWorld.getFrameCount() + int(1.0000001f / progressSpeed) + 1;
}

/** @return true when the current skill has completed a cycle */
bool Unit::update() {
	_PROFILE_FUNCTION();
	SoundRenderer &soundRenderer = SoundRenderer::getInstance();

	// start skill sound ?
	if (currSkill->getSound()) {
		if (theWorld.getFrameCount() == getSoundStartFrame()) {
			if (map->getTile(Map::toTileCoords(getPos()))->isVisible(theWorld.getThisTeamIndex())) {
				soundRenderer.playFx(currSkill->getSound(), getCurrVector(), theGame.getGameCamera()->getPos());
			}
		}
	}

	// start attack systems ?
	if (currSkill->getClass() == SkillClass::ATTACK
	&& getNextAttackFrame() == theWorld.getFrameCount()) {
		startAttackSystems(static_cast<const AttackSkillType*>(currSkill));
	}

	// update anim cycle ?
	if (theWorld.getFrameCount() >= getNextAnimReset()) {
		// new anim cycle (or reset)
		theSimInterface->doUpdateAnim(this);
	}

	// update emanations every 8 frames
	if (this->getEmanations().size() && !((theWorld.getFrameCount() + id) % 8) && isOperative()) {
		updateEmanations();
	}

	//highlight
	if(highlight > 0.f) {
		highlight -= 1.f / (GameConstants::highlightTime * WORLD_FPS);
	}
	//update target
	updateTarget();
	//rotation
	bool moved = currSkill->getClass() == SkillClass::MOVE;
	bool rotated = false;
	if(currSkill->getClass() != SkillClass::STOP) {
		const int rotFactor = 2;
		if(getProgress() < 1.f / rotFactor) {
			if(type->getFirstStOfClass(SkillClass::MOVE)) {
				rotated = true;
				if(abs(lastRotation - targetRotation) < 180) {
					rotation = lastRotation + (targetRotation - lastRotation) * getProgress() * rotFactor;
				} else {
					float rotationTerm = targetRotation > lastRotation ? -360.f : + 360.f;
					rotation = lastRotation + (targetRotation - lastRotation + rotationTerm) 
						* getProgress() * rotFactor;
				}
			}
		}
	}

	if (fire && moved) {
		fire->setPos(getCurrVector());
	}
	if (moved || rotated) {
		foreach (UnitParticleSystems, it, eyeCandy) {
			if (moved) (*it)->setPos(getCurrVector());
			if (rotated) (*it)->setRotation(getRotation());
		}
	}

	// check for cycle completion	
	// '>=' because nextCommandUpdate can be < frameCount if unit is dead
	if (theWorld.getFrameCount() >= getNextCommandUpdate()) {
		lastRotation = targetRotation;
		if (currSkill->getClass() != SkillClass::DIE) {
			return true;
		} else {
			deadCount++;
			if (deadCount >= GameConstants::maxDeadCount) {
				toBeUndertaken = true;
			}
		}
	}
	return false;
}

//REFACOR: to Emanation::update() called from Unit::update()
void Unit::updateEmanations() {
	// This is a little hokey, but probably the best way to reduce redundant code
	static EffectTypes singleEmanation(1);
	foreach_const (Emanations, i, getEmanations()) {
		singleEmanation[0] = *i;
		theWorld.applyEffects(this, singleEmanation, pos, Field::LAND, (*i)->getRadius());
		theWorld.applyEffects(this, singleEmanation, pos, Field::AIR, (*i)->getRadius());
	}
}

/**
 * Do positive or negative Hp and Ep regeneration. This method is
 * provided to reduce redundant code in a number of other places, mostly in
 * UnitUpdater.
 *
 * @returns true if the unit dies
 */
bool Unit::doRegen(int hpRegeneration, int epRegeneration) {
	if(hp < 1) {
		// dead people don't regenerate
		return true;
	}

	//hp regen/degen
	if(hpRegeneration > 0)
		repair(hpRegeneration);
	else if(hpRegeneration < 0) {
		if(decHp(-hpRegeneration))
			return true;
	}

	//ep regen/degen
	ep += epRegeneration;
	if(ep > getMaxEp()) {
		ep = getMaxEp();
	} else if(ep < 0) {
		ep = 0;
	}
	return false;
}

/**
 * Update the unit by one tick.
 * @returns if the unit died during this call, the killer is returned, NULL otherwise.
 */
Unit* Unit::tick() {
	Unit *killer = NULL;

	//replace references to dead units with their dying position prior to their
	//deletion for some commands
	for(Commands::iterator i = commands.begin(); i != commands.end(); i++) {
		switch((*i)->getType()->getClass()) {
			case CommandClass::MOVE:
			case CommandClass::REPAIR:
			case CommandClass::GUARD:
			case CommandClass::PATROL: {
					const Unit* unit1 = (*i)->getUnit();
					if(unit1 && unit1->isDead()) {
						(*i)->setUnit(NULL);
						(*i)->setPos(unit1->getPos());
					}
					const Unit* unit2 = (*i)->getUnit2();
					if(unit2 && unit2->isDead()) {
						(*i)->setUnit2(NULL);
						(*i)->setPos2(unit2->getPos());
					}
				}
				break;
			default:
				break;
		}
	}
	if(isAlive()) {
		if(doRegen(getHpRegeneration(), getEpRegeneration())) {
			if(!(killer = effects.getKiller())) {
				// if no killer, then this had to have been natural degeneration
				killer = this;
			}
		}
	}

	effects.tick();
	if(effects.isDirty()) {
		recalculateStats();
	}

	return killer;
}

/** Evaluate current skills energy requirements, subtract from current energy 
  *	@return false if the skill can commence, true if energy requirements are not met
  */
bool Unit::computeEp() {

	//if not enough ep
	if (currSkill->getEpCost() > 0 && ep - currSkill->getEpCost() < 0) {
		return true;
	}

	//decrease ep
	ep -= currSkill->getEpCost();
	if (ep > getMaxEp()) {
		ep = getMaxEp();
	}

	return false;
}

/** Repair this unit
  * @param amount amount of HP to restore
  * @param multiplier a multiplier for amount
  * @return true if this unit is now at max hp
  */
bool Unit::repair(int amount, fixed multiplier) {
	if (!isAlive()) {
		return false;
	}

	//if not specified, use default value
	if (!amount) {
		amount = getType()->getMaxHp() / type->getProductionTime() + 1;
	}
	amount = (amount * multiplier).intp();

	//increase hp
	hp += amount;
	if ( hp_above_trigger && hp > hp_above_trigger ) {
		hp_above_trigger = 0;
		ScriptManager::onHPAboveTrigger(this);
	}
	if (hp > getMaxHp()) {
		hp = getMaxHp();
		if (!isBuilt()) {
			faction->checkAdvanceSubfaction(type, true);
			born();
		}
		return true;
	}

	//stop fire
	if (hp > type->getMaxHp() / 2 && fire != NULL) {
		fire->fade();
		fire = NULL;
	}
	return false;
}

/** Decrements HP by the specified amount
  * @param i amount of HP to remove
  * @return true if unit is now dead
  */
bool Unit::decHp(int i) {
	assert(i >= 0);
	if (hp == 0) {
		World::getCurrWorld()->hackyCleanUp(this);
		return false;
	}
	// we shouldn't ever go negative
	assert(hp > 0);
	hp -= i;
	if ( hp_below_trigger && hp < hp_below_trigger ) {
		hp_below_trigger = 0;
		ScriptManager::onHPBelowTrigger(this);
	}

	//fire
	if (type->getProperty(Property::BURNABLE) && hp < type->getMaxHp() / 2 && fire == NULL) {
		FireParticleSystem *fps;
		fps = new FireParticleSystem(200);
		fps->setSpeed(2.5f / Config::getInstance().getGsWorldUpdateFps());
		fps->setPos(getCurrVector());
		fps->setRadius(type->getSize() / 3.f);
		fps->setTexture(CoreData::getInstance().getFireTexture());
		fps->setSize(type->getSize() / 3.f);
		fire = fps;
		Renderer::getInstance().manageParticleSystem(fps, ResourceScope::GAME);
	}

	//stop fire on death
	if (hp <= 0) {
		World::getCurrWorld()->hackyCleanUp(this);
//		alive = false;
		hp = 0;
		if (fire) {
			fire->fade();
			fire = NULL;
		}
		return true;
	}
	return false;
}

string Unit::getDesc(bool full) const {

	Lang &lang = Lang::getInstance();
	int armorBonus = getArmor() - type->getArmor();
	int sightBonus = getSight() - type->getSight();

	//pos
	//str+="Pos: "+v2iToStr(pos)+"\n";

	//hp
	string str = "\n" + lang.get("Hp") + ": " + intToStr(hp) + "/" + intToStr(getMaxHp());
	if (getHpRegeneration() != 0) {
		str += " (" + lang.get("Regeneration") + ": " + intToStr(getHpRegeneration()) + ")";
	}

	//ep
	if (getMaxEp() != 0) {
		str += "\n" + lang.get("Ep") + ": " + intToStr(ep) + "/" + intToStr(getMaxEp());
		if (getEpRegeneration() != 0) {
			str += " (" + lang.get("Regeneration") + ": " + intToStr(getEpRegeneration()) + ")";
		}
	}

	if (!full) {
		// Show only current command being executed and effects
		if (!commands.empty()) {
			str += "\n" + commands.front()->getType()->getName();
		}
		effects.getDesc(str);
		return str;
	}

	//armor
	str += "\n" + lang.get("Armor") + ": " + intToStr(type->getArmor());
	if (armorBonus) {
		if (armorBonus > 0) {
			str += "+";
		}
		str += intToStr(armorBonus);
	}
	str += " (" + type->getArmourType()->getName() + ")";

	//sight
	str += "\n" + lang.get("Sight") + ": " + intToStr(type->getSight());
	if (sightBonus) {
		if (sightBonus > 0) {
			str += "+";
		}
		str += intToStr(sightBonus);
	}

	//kills
	const Level *nextLevel = getNextLevel();
	if (kills > 0 || nextLevel != NULL) {
		str += "\n" + lang.get("Kills") + ": " + intToStr(kills);
		if (nextLevel != NULL) {
			str += " (" + nextLevel->getName() + ": " + intToStr(nextLevel->getKills()) + ")";
		}
	}

	//str+= "\nskl: "+scToStr(currSkill->getClass());

	//load
	if (loadCount) {
		str += "\n" + lang.get("Load") + ": " + intToStr(loadCount) + "  " + loadType->getName();
	}

	//consumable production
	for (int i = 0; i < type->getCostCount(); ++i) {
		const Resource *r = getType()->getCost(i);
		if (r->getType()->getClass() == ResourceClass::CONSUMABLE) {
			str += "\n";
			str += r->getAmount() < 0 ? lang.get("Produce") + ": " : lang.get("Consume") + ": ";
			str += intToStr(abs(r->getAmount())) + " " + r->getType()->getName();
		}
	}

	//command info
	if (!commands.empty()) {
		str += "\n" + commands.front()->getType()->getName();
		if (commands.size() > 1) {
			str += "\n" + lang.get("OrdersOnQueue") + ": " + intToStr(commands.size());
		}
	} else {
		//can store
		if (type->getStoredResourceCount() > 0) {
			for (int i = 0; i < type->getStoredResourceCount(); ++i) {
				const Resource *r = type->getStoredResource(i);
				str += "\n" + lang.get("Store") + ": ";
				str += intToStr(r->getAmount()) + " " + r->getType()->getName();
			}
		}
	}

	//effects
	effects.getDesc(str);

	return str;
}

/** Apply effects of an UpgradeType 
  * @param upgradeType the type describing the Upgrade to apply*/
void Unit::applyUpgrade(const UpgradeType *upgradeType){
	if(upgradeType->isAffected(type)){
		totalUpgrade.sum(upgradeType);
		recalculateStats();
	}
}

/** recompute stats, re-evaluate upgrades & level and recalculate totalUpgrade */
void Unit::computeTotalUpgrade() {
	faction->getUpgradeManager()->computeTotalUpgrade(this, &totalUpgrade);
	level = NULL;
	for(int i = 0; i < type->getLevelCount(); ++i){
		const Level *level = type->getLevel(i);
		if(kills >= level->getKills()) {
			totalUpgrade.sum(level);
			this->level = level;
		} else {
			break;
		}
	}
	recalculateStats();
}


/**
 * Recalculate the unit's stats (contained in base class
 * EnhancementType) to take into account changes in the effects and/or
 * totalUpgrade objects.
 */
void Unit::recalculateStats() {
	int oldMaxHp = getMaxHp();
	int oldHp = hp;

	reset();
	setValues(*type); // => setValues(UnitStats &)

	// add up all multipliers first and then apply (multiply) once.
	// See EnhancementType::addMultipliers() for the 'adding' strategy
	addMultipliers(totalUpgrade);
	for(Effects::const_iterator i = effects.begin(); i != effects.end(); i++) {
		addMultipliers(*(*i)->getType(), (*i)->getStrength());
	}
	applyMultipliers(*this);

	addStatic(totalUpgrade);
	for(Effects::const_iterator i = effects.begin(); i != effects.end(); i++) {
		addStatic(*(*i)->getType(), (*i)->getStrength());

		// take care of effect damage type
		hpRegeneration += (*i)->getActualHpRegen() - (*i)->getType()->getHpRegeneration();
	}

	effects.clearDirty();

	if(getMaxHp() > oldMaxHp) {
		hp += getMaxHp() - oldMaxHp;
	} else if(hp > getMaxHp()) {
		hp = getMaxHp();
	}
	// correct nagatives
	if(sight < 0) {
		sight = 0;
	}
	if(maxEp < 0) {
		maxEp = 0;
	}
	if(maxHp < 0) {
		maxHp = 0;
	}
	// If this guy is dead, make sure they stay dead
	if(oldHp < 1) {
		hp = 0;
	}
}

/**
 * Adds an effect to this unit
 * @returns true if this effect had an immediate regen/degen that killed the unit.
 */
bool Unit::add(Effect *e) {
	if(!isAlive() && !e->getType()->isEffectsNonLiving()){
		delete e;
		return false;
	}

	if(e->getType()->isTickImmediately()) {
		if(doRegen(e->getType()->getHpRegeneration(), e->getType()->getEpRegeneration())) {
			delete e;
			return true;
		}
		if(e->tick()) {
			// single tick, immediate effect
			delete e;
			return false;
		}
	}

	effects.add(e);

	if(effects.isDirty()) {
		recalculateStats();
	}

	return false;
}

/**
 * Cancel/remove the effect from this unit, if it is present.  This is usualy called because the
 * originator has died.  The caller is expected to clean up the Effect object.
 */
void Unit::remove(Effect *e) {
	effects.remove(e);

	if(effects.isDirty()) {
		recalculateStats();
	}
}

/**
 * Notify this unit that the effect they gave to somebody else has expired. This effect will
 * (should) have been one that this unit caused.
 */
void Unit::effectExpired(Effect *e){
	e->clearSource();
	effectsCreated.remove(e);
	effects.clearRootRef(e);

	if(effects.isDirty()) {
		recalculateStats();
	}
}

/** Another one bites the dust. Increment 'kills' & check level */
void Unit::incKills() {
	++kills;

	const Level *nextLevel = getNextLevel();
	if (nextLevel != NULL && kills >= nextLevel->getKills()) {
		level = nextLevel;
		totalUpgrade.sum(level);
		recalculateStats();
	}
}

/** Perform a morph @param mct the CommandType describing the morph @return true if successful */
bool Unit::morph(const MorphCommandType *mct) {
	const UnitType *morphUnitType = mct->getMorphUnit();
	Field newField = morphUnitType->getField();
	if (map->areFreeCellsOrHasUnit(pos, morphUnitType->getSize(), newField, this)) {
		map->clearUnitCells(this, pos);
		faction->deApplyStaticCosts(type);
		type = morphUnitType;
		computeTotalUpgrade();
		map->putUnitCells(this, pos);
		faction->applyDiscount(morphUnitType, mct->getDiscount());
		
		// reprocess commands
		Commands newCommands;
		Commands::const_iterator i;

		// add current command, which should be the morph command
		assert(commands.size() > 0 && commands.front()->getType()->getClass() == CommandClass::MORPH);
		newCommands.push_back(commands.front());
		i = commands.begin();
		++i;

		// add (any) remaining if possible
		for(; i != commands.end(); ++i) {
			// first see if the new unit type has a command by the same name
			const CommandType *newCmdType = type->getCommandType((*i)->getType()->getName());
			// if not, lets see if we can find any command of the same class
			if(!newCmdType) {
				newCmdType = type->getFirstCtOfClass((*i)->getType()->getClass());
			}
			// if still not found, we drop the comand, otherwise, we add it to the new list
			if(newCmdType) {
				(*i)->setType(newCmdType);
				newCommands.push_back(*i);
			}
		}
		commands = newCommands;
		return true;
	} else {
		return false;
	}
}

// ==================== PRIVATE ====================

/** calculate unit height
  * @param pos location ground reference
  * @return the height this unit 'stands' at
  */
inline float Unit::computeHeight(const Vec2i &pos) const {
	const Cell *const &cell = map->getCell(pos);
	switch (type->getField()) {
		case Field::LAND:
			return cell->getHeight();
		case Field::AIR:
			return cell->getHeight() + World::airHeight;
		case Field::AMPHIBIOUS:
			if (!cell->isSubmerged()) {
				return cell->getHeight();
			}
			// else on water, fall through
		case Field::ANY_WATER:
		case Field::DEEP_WATER:
			return map->getWaterLevel();
		default:
			throw runtime_error("Unhandled Field in Unit::computeHeight()");
	}
}

/** updates target information, (targetPos, targetField & tagetVec) and resets targetRotation 
  * @param target the unit we are tracking */
void Unit::updateTarget(const Unit *target) {
	if(!target) {
		target = theSimInterface->getUnitFactory().getUnit(targetRef);
	}

	//find a free pos in cellmap
//	setTargetPos(unit->getCellPos());
	
	if(target) {
		targetPos = useNearestOccupiedCell
				? target->getNearestOccupiedCell(pos)
				: targetPos = target->getCenteredPos();
		targetField = target->getCurrField();
		targetVec = target->getCurrVector();
		
		if(faceTarget) {
			face(target->getCenteredPos());
		}
	}
}

/** clear command queue */
void Unit::clearCommands() {
	while(!commands.empty()) {
		undoCommand(*commands.back());
		delete commands.back();
		commands.pop_back();
	}
}

/** Check if a command can be executed
  * @param command the command to check
  * @return a CommandResult describing success or failure
  */
CommandResult Unit::checkCommand(const Command &command) const {
	const CommandType *ct = command.getType();

	if (command.getArchetype() != CommandArchetype::GIVE_COMMAND) {
		return CommandResult::SUCCESS;
	}
	
	if(ct->getClass() == CommandClass::SET_MEETING_POINT) {
		return type->hasMeetingPoint() ? CommandResult::SUCCESS : CommandResult::FAIL_UNDEFINED;
	}

	//if not operative or has not command type => fail
	if (!isOperative() || command.getUnit() == this || !getType()->hasCommandType(ct)) {
		return CommandResult::FAIL_UNDEFINED;
	}

	//if pos is not inside the world
	if (command.getPos() != Command::invalidPos && !map->isInside(command.getPos())) {
		return CommandResult::FAIL_UNDEFINED;
	}

	//check produced
	const ProducibleType *produced = ct->getProduced();
	if (produced) {
		if (!faction->reqsOk(produced)) {
			return CommandResult::FAIL_REQUIREMENTS;
		}
		if (!faction->checkCosts(produced)) {
			return CommandResult::FAIL_RESOURCES;
		}
	}
	
	//build command specific, check resources and requirements for building
	if(ct->getClass() == CommandClass::BUILD) {
		const UnitType *builtUnit = command.getUnitType();
		if(!faction->reqsOk(builtUnit)) {
			return CommandResult::FAIL_REQUIREMENTS;
		}
		if(command.isReserveResources() && !faction->checkCosts(builtUnit)) {
			return CommandResult::FAIL_RESOURCES;
		}

	//upgrade command specific, check that upgrade is not upgraded
	} else if(ct->getClass() == CommandClass::UPGRADE){
		const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(ct);
		if(faction->getUpgradeManager()->isUpgradingOrUpgraded(uct->getProducedUpgrade())) {
			return CommandResult::FAIL_UNDEFINED;
		}
	}

	return CommandResult::SUCCESS;
}

/** Apply costs for a command.
  * @param command the command to apply costs for
  */
void Unit::applyCommand(const Command &command) {
	const CommandType *ct = command.getType();

	//check produced
	const ProducibleType *produced = ct->getProduced();
	if(produced) {
		faction->applyCosts(produced);
	}

	//build command specific
	if(ct->getClass() == CommandClass::BUILD && command.isReserveResources()) {
		faction->applyCosts(command.getUnitType());

	//upgrade command specific
	} else if(ct->getClass() == CommandClass::UPGRADE) {
		const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(ct);
		faction->startUpgrade(uct->getProducedUpgrade());
	}
}

/** De-Apply costs for a command
  * @param command the command to cancel
  * @return CommandResult::SUCCESS
  */
CommandResult Unit::undoCommand(const Command &command) {
	const CommandType *ct = command.getType();

	//return cost
	const ProducibleType *produced = ct->getProduced();
	if(produced) {
		faction->deApplyCosts(produced);
	}

	//return building cost if not already building it or dead
	if(ct->getClass() == CommandClass::BUILD && command.isReserveResources()) {
		if(currSkill->getClass() != SkillClass::BUILD && currSkill->getClass() != SkillClass::DIE){
			faction->deApplyCosts(command.getUnitType());
		}
	}

	//upgrade command cancel from list
	if(ct->getClass() == CommandClass::UPGRADE) {
		const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(ct);
		faction->cancelUpgrade(uct->getProducedUpgrade());
	}

	return CommandResult::SUCCESS;
}

/** query the speed at which a skill type is executed 
  * @param st the SkillType
  * @return the speed value this unit would execute st at
  */
int Unit::getSpeed(const SkillType *st) const {
	fixed speed = st->getSpeed();
	switch(st->getClass()) {
		case SkillClass::MOVE:
			speed = speed * moveSpeedMult + moveSpeed;
			break;

		case SkillClass::ATTACK:
			speed =  speed * attackSpeedMult + attackSpeed;
			break;

		case SkillClass::PRODUCE:
		case SkillClass::UPGRADE:
		case SkillClass::MORPH:
			speed =  speed * prodSpeedMult + prodSpeed;
			break;

		case SkillClass::BUILD:
		case SkillClass::REPAIR:
			speed =  speed * repairSpeedMult + repairSpeed;
			break;

		case SkillClass::HARVEST:
			speed =  speed * harvestSpeedMult + harvestSpeed;
			break;

		case SkillClass::BE_BUILT:
		case SkillClass::STOP:
		case SkillClass::DIE:
		case SkillClass::COUNT:
			break;
		default:
			throw runtime_error("unhandled SkillClass");
	}
	return (speed > 0 ? speed.intp() : 0);
}

// =====================================================
//  class UnitFactory
// =====================================================

UnitFactory::UnitFactory() 
		: idCounter(0) {
}

UnitFactory::~UnitFactory() {
	foreach (UnitMap, it, unitMap) {
		delete it->second;
	}
	foreach (Units, it, deadList) {
		delete *it;
	}
}

Unit* UnitFactory::newInstance(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld) {
	Unit *unit = new Unit(node, faction, map, tt, putInWorld);
	if (unit->isAlive()) {
		if (unitMap.find(unit->getId()) != unitMap.end()) {
			throw runtime_error("Error: duplicate Unit id.");
		}
		unitMap[unit->getId()] = unit;
	} else {
		deadList.push_back(unit);
	}
	return unit;
}

Unit* UnitFactory::newInstance(const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, Unit* master) {
	Unit *unit = new Unit(idCounter, pos, type, faction, map, master);
	unitMap[idCounter] = unit;
	//unitList.push_back(unit);
	
	unit->Died.connect(this, &UnitFactory::onUnitDied);

	// todo: more connect-o-rama

	++idCounter;
	return unit;
}

Unit* UnitFactory::getUnit(int id) {
	UnitMap::iterator it = unitMap.find(id);
	if (it != unitMap.end()) {
		return it->second;
	}
	return 0;
}

void UnitFactory::onUnitDied(Unit *unit) {
	unitMap.erase(unitMap.find(unit->getId()));
	deadList.push_back(unit);
}

void UnitFactory::update() {
	Units::iterator it = deadList.begin();
	while (it != deadList.end()) {
		if ((*it)->getToBeUndertaken()) {
			(*it)->undertake();
			delete *it;
			it = deadList.erase(it);

		} else {
			return;
		}
	}
}

void UnitFactory::deleteUnit(Unit *unit) {
	UnitMap::iterator it = unitMap.find(unit->getId());
	if (it == unitMap.end()) {
		throw runtime_error("Error: Unit not in unitMap");
	}
	delete unit;
	unitMap.erase(it);
}

}}//end namespace

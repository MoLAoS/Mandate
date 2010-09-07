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
#include "user_interface.h"

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

ostream& operator<<(ostream &stream,  Vec2iList &vec) {
	foreach_const (Vec2iList, it, vec) {
		if (it != vec.begin()) {
			stream << ", ";
		}
		stream << *it;
	}
	return stream;
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
Unit::Unit(int id, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, 
		   CardinalDir facing, Unit* master)
        : visible(true)
        , id(id)
        , hp(1)
        , ep(0)
        , loadCount(0)
        , deadCount(0)
        , lastAnimReset(0)
        , nextAnimReset(-1)
        , lastCommandUpdate(0)
        , nextCommandUpdate(-1)
        , attackStartFrame(-1)
        , soundStartFrame(-1)
        , progress2(0)
        , kills(0)
        , highlight(0.f)
        , targetRef(-1)
        , targetField(Field::LAND)
        , faceTarget(true)
        , useNearestOccupiedCell(true)
        , level(0)
		, pos(pos)
		, lastPos(pos)
		, nextPos(pos)
		, targetPos(0)
		, targetVec(0.0f)
		, meetingPos(0)
		, lastRotation(0.f)
        , targetRotation(0.f)
        , rotation(0.f)
		, m_facing(facing)
        , type(type)
        , loadType(0)
        , currSkill(0)
        , toBeUndertaken(false)
        , autoRepairEnabled(true)
        , carried(false)
        , faction(faction)
        , fire(0)
        , map(map)
        , commandCallback(0)
        , hp_below_trigger(0)
        , hp_above_trigger(0)
        , attacked_trigger(false) {
	Random random(id);
	currSkill = getType()->getFirstStOfClass(SkillClass::STOP);	//starting skill
	UNIT_LOG(g_world.getFrameCount() << "::Unit:" << id << " constructed at pos" << pos );

	computeTotalUpgrade();
	hp = type->getMaxHp() / 20;

	setModelFacing(m_facing);
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
	m_facing = enum_cast<CardinalDir>(node->getChildIntValue("facing"));

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
	removeCommands();
	UNIT_LOG(g_world.getFrameCount() << "::Unit:" << id << " deleted." );
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
	node->addChild("facing", int(m_facing));
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
  * @return nearest cell to 'from' that is occuppied
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
				if (!type->hasCellMap() || type->getCellMapCell(x, y, m_facing)) {
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

/** query completeness of thing this unit is producing
  * @return percentage complete, or -1 if not currently producing anything */
int Unit::getProductionPercent() const {
	if (anyCommand()) {
		CommandClass cmdClass = commands.front()->getType()->getClass();
		if (cmdClass == CommandClass::PRODUCE || cmdClass == CommandClass::MORPH
		|| cmdClass == CommandClass::GENERATE) {
			const ProducibleType *produced = commands.front()->getProdType();
			if (produced) {
				return clamp(progress2 * 100 / produced->getProductionTime(), 0, 100);
			}
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
	return float(g_world.getFrameCount() - lastCommandUpdate)
			/	float(nextCommandUpdate - lastCommandUpdate);
}

float Unit::getAnimProgress() const {
	return float(g_world.getFrameCount() - lastAnimReset)
			/	float(nextAnimReset - lastAnimReset);
}

// ====================================== set ======================================

void Unit::setCommandCallback() {
	commandCallback = commands.front()->getId();
}

/** sets the current skill */
void Unit::setCurrSkill(const SkillType *newSkill) {
	assert(newSkill);
	//COMMAND_LOG(g_world.getFrameCount() << "::Unit:" << id << " skill set => " << SkillClassNames[currSkill->getClass()] );
	if (newSkill->getClass() == SkillClass::STOP && currSkill->getClass() == SkillClass::STOP) {
		return;
	}
	if (newSkill != currSkill) {
		while(!skillParticleSystems.empty()){
			skillParticleSystems.back()->fade();
			skillParticleSystems.pop_back();
		}
	}
	progress2 = 0;
	currSkill = newSkill;
	StateChanged(this);
	for (unsigned i = 0; i < currSkill->getEyeCandySystemCount(); ++i) {
		UnitParticleSystem *ups = currSkill->getEyeCandySystem(i)->createUnitParticleSystem();
		ups->setPos(getCurrVector());
		//ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
		skillParticleSystems.push_back(ups);
		g_renderer.manageParticleSystem(ups, ResourceScope::GAME);
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

void Unit::setModelFacing(CardinalDir value) {
	m_facing = value;
	lastRotation = targetRotation = rotation = value * 90.f;
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
	bool visible = sc->isVisible(g_world.getThisTeamIndex()) || tsc->isVisible(g_world.getThisTeamIndex());

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

		g_simInterface->doUpdateProjectile(this, psProj, startPos, endPos);
		// game network interface calls setPath() on psProj, differently for clients/servers
		//theNetworkManager.getNetworkInterface()->doUpdateProjectile(this, psProj, startPos, endPos);

		if(pstProj->isTracking() && targetRef != -1) {
			Unit *target = g_simInterface->getUnitFactory().getUnit(targetRef);
			psProj->setTarget(target);
			psProj->setDamager(new ParticleDamager(this, target, &g_world, g_gameState.getGameCamera()));
		} else {
			psProj->setDamager(new ParticleDamager(this, NULL, &g_world, g_gameState.getGameCamera()));
		}
		psProj->setVisible(visible);
		renderer.manageParticleSystem(psProj, ResourceScope::GAME);
	} else {
		g_world.hit(this);
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
			g_soundRenderer.playFx(et->getSound(), getTargetVec(), g_gameState.getGameCamera()->getPos());
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
 * @return the number of commands on this unit's queue
 */
unsigned int Unit::getCommandCount() const{
	return commands.size();
}

/** give one command, queue or clear command queue and push back (depending on flags)
  * @param command the command to execute
  * @return a CommandResult describing success or failure
  */
CommandResult Unit::giveCommand(Command *command) {
	const CommandType *ct = command->getType();

	if (ct->getClass() == CommandClass::SET_MEETING_POINT) {
		if(command->isQueue() && !commands.empty()) {
			commands.push_back(command);
		} else {
			meetingPos = command->getPos();
			delete command;
		}
		COMMAND_LOG( __FUNCTION__ << "(): " << *this << ", " << *command << ", Result=" << CommandResultNames[CommandResult::SUCCESS] );
		return CommandResult::SUCCESS;
	}

	if (ct->isQueuable() || command->isQueue()) { // user wants this queued...
		// cancel current command if it is not queuable or marked to be queued
		if(!commands.empty() && !commands.front()->getType()->isQueuable() && !command->isQueue()) {
			COMMAND_LOG( __FUNCTION__ << "(): " << *this << ", " << " incoming command wants queue, but current is not queable. Cancel current command" );
			cancelCommand();
			unitPath.clear();
		}
	} else {
		// empty command queue
		COMMAND_LOG( __FUNCTION__ << "(): " << *this << ", " << " incoming command is not marked to queue, Clear command queue" );
		clearCommands();
		unitPath.clear();

		// for patrol commands, remember where we started from
		if(ct->getClass() == CommandClass::PATROL) {
			command->setPos2(pos);
		}
	}

	// check command
	CommandResult result = checkCommand(*command);
	//COMMAND_LOG( "NO_RESERVE_RESOURCES flag is " << (command->isReserveResources() ? "not " : "" ) << "set,"
	//	<< " command result = " << CommandResultNames[result] );
	if (result == CommandResult::SUCCESS) {
		applyCommand(*command);

		if (command->getType()->getClass() == CommandClass::LOAD) {
			if (std::find(unitsToCarry.begin(), unitsToCarry.end(), command->getUnit()) == unitsToCarry.end()) {
				unitsToCarry.push_back(command->getUnit());
				COMMAND_LOG( __FUNCTION__ << "() adding unit to load list " << *command->getUnit() )
				if (!commands.empty() && commands.front()->getType()->getClass() == CommandClass::LOAD) {
					COMMAND_LOG( __FUNCTION__ << "() deleting load command, already loading.")
					delete command;
					command = 0;
				}
			}
		} else if (command->getType()->getClass() == CommandClass::UNLOAD) {
			if (command->getUnit()) {
				if (std::find(unitsToUnload.begin(), unitsToUnload.end(), command->getUnit()) == unitsToUnload.end()) {
					assert(std::find(carriedUnits.begin(), carriedUnits.end(), command->getUnit()) != carriedUnits.end());
					unitsToUnload.push_back(command->getUnit());
					COMMAND_LOG( __FUNCTION__ << "() adding unit to unload list " << *command->getUnit() )
					if (!commands.empty() && commands.front()->getType()->getClass() == CommandClass::UNLOAD) {
						COMMAND_LOG( __FUNCTION__ << "() deleting unload command, already unloading.")
						delete command;
						command = 0;
					}
				}
			} else {
				unitsToUnload.clear();
				unitsToUnload = carriedUnits;
			}
		}
		if (command) {
			commands.push_back(command);
		}
	} else {
		delete command;
		command = 0;
	}
	if (commands.empty() || commands.front()->getType()->getClass() == CommandClass::STOP) {
		StateChanged(this);
	}
	if (command) {
		COMMAND_LOG( __FUNCTION__ << "(): " << *this << ", " << *command << ", Result=" << CommandResultNames[result] );
	}
	return result;
}

/** removes current command (and any queued Set meeting point commands)
  * @return the command now at the head of the queue (the new current command) */
Command *Unit::popCommand() {
	// pop front
	COMMAND_LOG(__FUNCTION__ << "() " << *this << " cancelling current " << commands.front()->getType()->getName() << " command." );

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
	if (command) {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " new current is " << command->getType()->getName() << " command." );
	} else {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " now has no commands." );
	}
	if (commands.empty() || commands.front()->getType()->getClass() == CommandClass::STOP) {
		StateChanged(this);
	}
	return command;
}
/** pop current command (used when order is done)
  * @return CommandResult::SUCCESS, or CommandResult::FAIL_UNDEFINED on catastrophic failure
  */
CommandResult Unit::finishCommand() {
	//is empty?
	if(commands.empty()) {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " no command to finish!" );
		return CommandResult::FAIL_UNDEFINED;
	}
	COMMAND_LOG(__FUNCTION__ << "() " << *this << ", " << commands.front()->getType()->getName() << " command finished." );

	Command *command = popCommand();

	//for patrol command, remember where we started from
	if(command && command->getType()->getClass() == CommandClass::PATROL) {
		command->setPos2(pos);
	}
	if (commands.empty() || commands.front()->getType()->getClass() == CommandClass::STOP) {
		StateChanged(this);
	}
	if (commands.empty()) {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " now has no commands." );
	} else {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << ", " << commands.front()->getType()->getName() << " command next on queue." );
	}

	return CommandResult::SUCCESS;
}

/** cancel command on back of queue */
CommandResult Unit::cancelCommand() {
	// is empty?
	if(commands.empty()){
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " No commands to cancel!");
		return CommandResult::FAIL_UNDEFINED;
	}

	//undo command
	const CommandType *ct = commands.back()->getType();
	undoCommand(*commands.back());

	//delete ans pop command
	delete commands.back();
	commands.pop_back();

	//clear routes
	unitPath.clear();
	if (commands.empty() || commands.front()->getType()->getClass() == CommandClass::STOP) {
		StateChanged(this);
	}
	if (commands.empty()) {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " current " << ct->getName() << " command cancelled.");
	} else {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " a queued " << ct->getName() << " command cancelled.");
	}
	return CommandResult::SUCCESS;
}

/** cancel current command */
CommandResult Unit::cancelCurrCommand() {
	//is empty?
	if(commands.empty()) {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " No commands to cancel!");
		return CommandResult::FAIL_UNDEFINED;
	}

	//undo command
	undoCommand(*commands.front());

	Command *command = popCommand();
	if (!command || command->getType()->getClass() == CommandClass::STOP) {
		StateChanged(this);
	}
	if (!command) {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " now has no commands." );
	} else {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << ", " << command->getType()->getName() << " command next on queue." );
	}
	return CommandResult::SUCCESS;
}

void Unit::removeCommands() {
	if (!g_program.isTerminating() && World::isConstructed()) {
		COMMAND_LOG(__FUNCTION__ << "() " << *this << " clearing all commands." );
	}
	while (!commands.empty()) {
		delete commands.back();
		commands.pop_back();
	}
}

// =================== route stack ===================

/** Creates a unit, places it on the map and applies static costs for starting units
  * @param startingUnit true if this is a starting unit.
  */
void Unit::create(bool startingUnit) {
	UNIT_LOG( g_world.getFrameCount() << "::Unit:" << id << " created." );
	faction->add(this);
	lastPos = Vec2i(-1);
	map->putUnitCells(this, pos);
	if (startingUnit) {
		faction->applyStaticCosts(type);
	}
	nextCommandUpdate = -1;
	setCurrSkill(type->getStartSkill());
}

/** Give a unit life. Called when a unit becomes 'operative'
  */
void Unit::born(){
	UNIT_LOG(g_world.getFrameCount() << "::Unit:" << id + " born." );
	faction->addStore(type);
	faction->applyStaticProduction(type);
	setCurrSkill(SkillClass::STOP);
	computeTotalUpgrade();
	recalculateStats();
	hp= type->getMaxHp();
	faction->checkAdvanceSubfaction(type, true);
	g_world.getCartographer()->applyUnitVisibility(this);
	g_simInterface->doUnitBorn(this);
}

void checkTargets(const Unit *dead) {
	typedef list<ParticleSystem*> psList;
	const psList &list = g_renderer.getParticleManager()->getList();
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
void Unit::kill() {
	assert(hp <= 0);
	UNIT_LOG(g_world.getFrameCount() << *this << " killed." );
	hp = 0;
	g_world.getCartographer()->removeUnitVisibility(this);

	if (!isCarried()) { // if not in transport, clear cells
		map->clearUnitCells(this, pos);
	}

	if (fire) {
		fire->fade();
		fire = 0;
	}

	if (isBeingBuilt()) { // no longer needs static resources
		faction->deApplyStaticConsumption(type);
	} else {
		faction->deApplyStaticCosts(type);
		faction->removeStore(type);
	}

	setCurrSkill(SkillClass::DIE);
	Died(this);
	clearCommands();
	checkTargets(this); // hack... 'tracking' particle systems might reference this
	deadCount = Random(id).randRange(-256, 256); // random decay time
}

void Unit::resetHighlight() {
	highlight= 1.f;
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
		} else if (targetUnit->getType()->isOfClass(UnitClass::CARRIER)) {
			//move to be loaded
			commandType = type->getFirstCtOfClass(CommandClass::MOVE);
		} else if (getType()->isOfClass(UnitClass::CARRIER)) {
			//load
			commandType = type->getFirstCtOfClass(CommandClass::LOAD);
		} else {
			//repair allies
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
	const int &frame = g_world.getFrameCount();
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
	const int &frame = g_world.getFrameCount();
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
  * @param frameOffset the number of frames the next skill cycle will take */
void Unit::updateSkillCycle(int frameOffset) {
	// assert server doesn't use this for move...
	assert(currSkill->getClass() != SkillClass::MOVE || g_simInterface->asClientInterface());

	// modify offset for upgrades/effects/etc
	if (currSkill->getClass() != SkillClass::MOVE) {
		fixed ratio = getBaseSpeed() / fixed(getSpeed());
		frameOffset = (frameOffset * ratio).round();
		if (frameOffset < 1) {
			frameOffset = 1;
		}
	}
	// else move skill, server has already modified speed for us

	lastCommandUpdate = g_world.getFrameCount();
	nextCommandUpdate = g_world.getFrameCount() + frameOffset;
}

/** called by the server only, updates a skill cycle for the move skill */
void Unit::updateMoveSkillCycle() {
	assert(!g_simInterface->asClientInterface());
	assert(currSkill->getClass() == SkillClass::MOVE);
	static const float speedModifier = 1.f / GameConstants::speedDivider / float(WORLD_FPS);

	float progressSpeed = getSpeed() * speedModifier;
	if (pos.x != nextPos.x && pos.y != nextPos.y) { // if moving in diagonal move slower
		progressSpeed *= 0.71f;
	}
	// if moving to a higher cell move slower else move faster
	float heightDiff = map->getCell(lastPos)->getHeight() - map->getCell(pos)->getHeight();
	float heightFactor = clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
	progressSpeed *= heightFactor;

	// reset lastCommandUpdate and calculate next skill cycle length
	lastCommandUpdate = g_world.getFrameCount();
	nextCommandUpdate = g_world.getFrameCount() + int(1.0000001f / progressSpeed) + 1;
}

/** @return true when the current skill has completed a cycle */
bool Unit::update() {
	_PROFILE_FUNCTION();
	const int &frame = g_world.getFrameCount();

	// start skill sound ?
	if (currSkill->getSound() && frame == getSoundStartFrame()) {
		if (map->getTile(Map::toTileCoords(getPos()))->isVisible(g_world.getThisTeamIndex())) {
			g_soundRenderer.playFx(currSkill->getSound(), getCurrVector(), g_gameState.getGameCamera()->getPos());
		}
	}

	// start attack systems ?
	if (currSkill->getClass() == SkillClass::ATTACK && frame == getNextAttackFrame()) {
		startAttackSystems(static_cast<const AttackSkillType*>(currSkill));
	}

	// update anim cycle ?
	if (frame >= getNextAnimReset()) {
		// new anim cycle (or reset)
		g_simInterface->doUpdateAnim(this);
	}

	// update emanations every 8 frames
	if (this->getEmanations().size() && !((frame + id) % 8) && isOperative()) {
		updateEmanations();
	}

	// fade highlight
	if (highlight > 0.f) {
		highlight -= 1.f / (GameConstants::highlightTime * WORLD_FPS);
	}

	// update target
	updateTarget();
	
	// rotation
	bool moved = currSkill->getClass() == SkillClass::MOVE;
	bool rotated = false;
	if (currSkill->getClass() != SkillClass::STOP) {
		const int rotFactor = 2;
		if (getProgress() < 1.f / rotFactor) {
			if (type->getFirstStOfClass(SkillClass::MOVE)) {
				rotated = true;
				if (abs(lastRotation - targetRotation) < 180) {
					rotation = lastRotation + (targetRotation - lastRotation) * getProgress() * rotFactor;
				} else {
					float rotationTerm = targetRotation > lastRotation ? -360.f : + 360.f;
					rotation = lastRotation + (targetRotation - lastRotation + rotationTerm)
						* getProgress() * rotFactor;
				}
			}
		}
	}

	// update particle system location/orientation
	if (fire && moved) {
		fire->setPos(getCurrVector());
	}
	if (moved || rotated) {
		foreach (UnitParticleSystems, it, skillParticleSystems) {
			if (moved) (*it)->setPos(getCurrVector());
			if (rotated) (*it)->setRotation(getRotation());
		}
		foreach (UnitParticleSystems, it, effectParticleSystems) {
			if (moved) (*it)->setPos(getCurrVector());
			if (rotated) (*it)->setRotation(getRotation());
		}
	}

	// check for cycle completion
	// '>=' because nextCommandUpdate can be < frameCount if unit is dead
	if (frame >= getNextCommandUpdate()) {
		lastRotation = targetRotation;
		if (currSkill->getClass() != SkillClass::DIE) {
			return true;
		} else {
			++deadCount;
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
		g_world.applyEffects(this, singleEmanation, pos, Field::LAND, (*i)->getRadius());
		g_world.applyEffects(this, singleEmanation, pos, Field::AIR, (*i)->getRadius());
	}
}

/**
 * Do positive or negative Hp and Ep regeneration. This method is
 * provided to reduce redundant code in a number of other places.
 *
 * @returns true if the unit dies
 */
bool Unit::doRegen(int hpRegeneration, int epRegeneration) {
	if (hp < 1) {
		// dead people don't regenerate
		return true;
	}

	// hp regen/degen
	if (hpRegeneration > 0) {
		repair(hpRegeneration);
	} else if (hpRegeneration < 0) {
		if (decHp(-hpRegeneration)) {
			return true;
		}
	}

	//ep regen/degen
	ep += epRegeneration;
	if (ep > getMaxEp()) {
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
	for (Commands::iterator i = commands.begin(); i != commands.end(); i++) {
		switch ((*i)->getType()->getClass()) {
			case CommandClass::MOVE:
			case CommandClass::REPAIR:
			case CommandClass::GUARD:
			case CommandClass::PATROL: {
					const Unit* unit1 = (*i)->getUnit();
					if (unit1 && unit1->isDead()) {
						(*i)->setUnit(NULL);
						(*i)->setPos(unit1->getPos());
					}
					const Unit* unit2 = (*i)->getUnit2();
					if (unit2 && unit2->isDead()) {
						(*i)->setUnit2(NULL);
						(*i)->setPos2(unit2->getPos());
					}
				}
				break;
			default:
				break;
		}
	}
	if (isAlive()) {
		if (doRegen(getHpRegeneration(), getEpRegeneration())) {
			if (!(killer = effects.getKiller())) {
				// if no killer, then this had to have been natural degeneration
				killer = this;
			}
		}
	}

	effects.tick();
	if (effects.isDirty()) {
		recalculateStats();
		checkEffectParticles();
	}

	return killer;
}

/** Evaluate current skills energy requirements, subtract from current energy
  *	@return false if the skill can commence, true if energy requirements are not met
  */
bool Unit::computeEp() {

	// if not enough ep
	if (currSkill->getEpCost() > 0 && ep - currSkill->getEpCost() < 0) {
		return true;
	}

	// decrease ep
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
	if (hp_above_trigger && hp > hp_above_trigger) {
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
		return false;
	}
	// we shouldn't ever go negative
	assert(hp > 0);
	hp -= i;
	if (hp_below_trigger && hp < hp_below_trigger) {
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
	int armorBonus = getArmor() - type->getArmor();
	int sightBonus = getSight() - type->getSight();

	stringstream ss;
	//pos
	//str+="Pos: "+v2iToStr(pos)+"\n";

	//hp
	ss << g_lang.get("Hp") << ": " << hp << "/" << getMaxHp();
	if (getHpRegeneration()) {
		ss << " (" << g_lang.get("Regeneration") << ": " << getHpRegeneration() << ")";
	}

	//ep
	if (getMaxEp()) {
		ss << endl << g_lang.get("Ep") << ": " << ep << "/" << getMaxEp();
		if (getEpRegeneration()) {
			ss << " (" << g_lang.get("Regeneration") << ": " << getEpRegeneration() << ")";
		}
	}

	if (!full) {
		// Show only current command being executed and effects
		if (!commands.empty()) {
			ss << endl << commands.front()->getType()->getName();
		}
		effects.streamDesc(ss);
		//effects.getDesc(str);
		return ss.str();
	}

	//armor
	ss << endl << g_lang.get("Armor") << ": " << type->getArmor();
	if (armorBonus) {
		ss << (armorBonus > 0 ? "+" : "-") << armorBonus;
	}
	ss << " (" << type->getArmourType()->getName() << ")";

	//sight
	ss << endl << g_lang.get("Sight") << ": " << type->getSight();
	if (sightBonus) {
		ss << (sightBonus > 0 ? "+" : "-") << sightBonus;
	}

	//kills
	const Level *nextLevel = getNextLevel();
	if (kills > 0 || nextLevel) {
		ss << endl << g_lang.get("Kills") << ": " << kills;
		if (nextLevel) {
			ss << " (" << nextLevel->getName() << ": " << nextLevel->getKills() << ")";
		}
	}

	//load
	if (loadCount) {
		ss << endl << g_lang.get("Load") << ": " << loadCount << "  " << loadType->getName();
	}

	//consumable production
	for (int i = 0; i < type->getCostCount(); ++i) {
		const Resource *r = getType()->getCost(i);
		if (r->getType()->getClass() == ResourceClass::CONSUMABLE) {
			ss << endl << (r->getAmount() < 0 ? g_lang.get("Produce") : g_lang.get("Consume"))
				<< ": " << abs(r->getAmount()) << " " << r->getType()->getName();
		}
	}

	//command info
	if (!commands.empty()) {
		ss << endl << commands.front()->getType()->getName();
		if (commands.size() > 1) {
			ss << endl << g_lang.get("OrdersOnQueue") << ": " << commands.size();
		}
	} else {
		//can store
		if (type->getStoredResourceCount() > 0) {
			for (int i = 0; i < type->getStoredResourceCount(); ++i) {
				const Resource *r = type->getStoredResource(i);
				ss << endl << g_lang.get("Store") << ": ";
				ss << r->getAmount() << " " << r->getType()->getName();
			}
		}
	}

	//effects
	effects.streamDesc(ss);

	return ss.str();
}

/** Apply effects of an UpgradeType
  * @param upgradeType the type describing the Upgrade to apply*/
void Unit::applyUpgrade(const UpgradeType *upgradeType) {
	if (upgradeType->isAffected(type)) {
		totalUpgrade.sum(upgradeType);
		recalculateStats();
	}
}

/** recompute stats, re-evaluate upgrades & level and recalculate totalUpgrade */
void Unit::computeTotalUpgrade() {
	faction->getUpgradeManager()->computeTotalUpgrade(this, &totalUpgrade);
	level = NULL;
	for (int i = 0; i < type->getLevelCount(); ++i) {
		const Level *level = type->getLevel(i);
		if (kills >= level->getKills()) {
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
	for (Effects::const_iterator i = effects.begin(); i != effects.end(); i++) {
		addMultipliers(*(*i)->getType(), (*i)->getStrength());
	}
	applyMultipliers(*this);

	addStatic(totalUpgrade);
	for (Effects::const_iterator i = effects.begin(); i != effects.end(); i++) {
		addStatic(*(*i)->getType(), (*i)->getStrength());

		// take care of effect damage type
		hpRegeneration += (*i)->getActualHpRegen() - (*i)->getType()->getHpRegeneration();
	}

	effects.clearDirty();

	if (getMaxHp() > oldMaxHp) {
		hp += getMaxHp() - oldMaxHp;
	} else if (hp > getMaxHp()) {
		hp = getMaxHp();
	}
	// correct nagatives
	if (sight < 0) {
		sight = 0;
	}
	if (maxEp < 0) {
		maxEp = 0;
	}
	if (maxHp < 0) {
		maxHp = 0;
	}
	// If this guy is dead, make sure they stay dead
	if (oldHp < 1) {
		hp = 0;
	}
}

/**
 * Adds an effect to this unit
 * @returns true if this effect had an immediate regen/degen that killed the unit.
 */
bool Unit::add(Effect *e) {
	if (!isAlive() && !e->getType()->isEffectsNonLiving()) {
		delete e;
		return false;
	}

	if (e->getType()->isTickImmediately()) {
		if (doRegen(e->getType()->getHpRegeneration(), e->getType()->getEpRegeneration())) {
			delete e;
			return true;
		}
		if (e->tick()) {
			// single tick, immediate effect
			delete e;
			return false;
		}
	}

	bool startParticles = true;
	foreach (Effects, it, effects) {
		if (e->getType() == (*it)->getType()) {
			startParticles = false;
			break;
		}
	}
	effects.add(e);

	const UnitParticleSystemTypes &particleTypes = e->getType()->getParticleTypes();
	if (!particleTypes.empty() && startParticles) {
		foreach_const (UnitParticleSystemTypes, it, particleTypes) {
			UnitParticleSystem *ups = (*it)->createUnitParticleSystem();
			ups->setPos(getCurrVector());
			//ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
			effectParticleSystems.push_back(ups);
			g_renderer.manageParticleSystem(ups, ResourceScope::GAME);
		}
	}

	if (effects.isDirty()) {
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

	if (effects.isDirty()) {
		recalculateStats();
	}
}

void Unit::checkEffectParticles() {
	set<const EffectType*> seenEffects;
	foreach (Effects, it, effects) {
		seenEffects.insert((*it)->getType());
	}
	set<const UnitParticleSystemType*> seenSystems;
	foreach_const (set<const EffectType*>, it, seenEffects) {
		const UnitParticleSystemTypes &types = (*it)->getParticleTypes();
		foreach_const (UnitParticleSystemTypes, it2, types) {
			seenSystems.insert(*it2);
		}
	}
	UnitParticleSystems::iterator psIt = effectParticleSystems.begin();
	while (psIt != effectParticleSystems.end()) {
		if (seenSystems.find((*psIt)->getType()) == seenSystems.end()) {
			(*psIt)->fade();
			psIt = effectParticleSystems.erase(psIt);
		} else {
			++psIt;
		}
	}
}

/**
 * Notify this unit that the effect they gave to somebody else has expired. This effect will
 * (should) have been one that this unit caused.
 */
void Unit::effectExpired(Effect *e) {
	e->clearSource();
	effectsCreated.remove(e);
	effects.clearRootRef(e);

	if (effects.isDirty()) {
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
bool Unit::morph(const MorphCommandType *mct, const UnitType *ut) {
	Field newField = ut->getField();
	if (map->areFreeCellsOrHasUnit(pos, ut->getSize(), newField, this)) {
		map->clearUnitCells(this, pos);
		faction->deApplyStaticCosts(type);
		type = ut;
		computeTotalUpgrade();
		map->putUnitCells(this, pos);
		faction->applyDiscount(ut, mct->getDiscount());

		// reprocess commands
		Commands newCommands;
		Commands::const_iterator i;

		// add current command, which should be the morph command
		assert(commands.size() > 0 && commands.front()->getType()->getClass() == CommandClass::MORPH);
		newCommands.push_back(commands.front());
		i = commands.begin();
		++i;

		// add (any) remaining if possible
		for (; i != commands.end(); ++i) {
			// first see if the new unit type has a command by the same name
			const CommandType *newCmdType = type->getCommandType((*i)->getType()->getName());
			// if not, lets see if we can find any command of the same class
			if (!newCmdType) {
				newCmdType = type->getFirstCtOfClass((*i)->getType()->getClass());
			}
			// if still not found, we drop the comand, otherwise, we add it to the new list
			if (newCmdType) {
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
float Unit::computeHeight(const Vec2i &pos) const {
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
	if (!target) {
		target = g_simInterface->getUnitFactory().getUnit(targetRef);
	}

	if (target) {
		targetPos = useNearestOccupiedCell
				? target->getNearestOccupiedCell(pos)
				: targetPos = target->getCenteredPos();
		targetField = target->getCurrField();
		targetVec = target->getCurrVector();

		if (faceTarget) {
			face(target->getCenteredPos());
		}
	}
}

/** clear command queue */
void Unit::clearCommands() {
	while (!commands.empty()) {
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

	if (ct->getClass() == CommandClass::SET_MEETING_POINT) {
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
	const ProducibleType *produced = command.getProdType();
	if (produced) {
		if (!faction->reqsOk(produced)) {
			return CommandResult::FAIL_REQUIREMENTS;
		}
		if (!faction->checkCosts(produced)) {
			return CommandResult::FAIL_RESOURCES;
		}
	}

	if (ct->getClass() == CommandClass::LOAD) { // check load command
		if (static_cast<const LoadCommandType*>(ct)->getLoadCapacity() > getCarriedCount()) {
			if (static_cast<const LoadCommandType*>(ct)->canCarry(command.getUnit()->getType())
			&& command.getUnit()->getFactionIndex() == getFactionIndex()) {
				return CommandResult::SUCCESS;
			}
			return CommandResult::FAIL_INVALID_LOAD;
		} else {
			return CommandResult::FAIL_LOAD_LIMIT;
		}
	}

	if (ct->getClass() == CommandClass::BUILD) { // build command specific
		const UnitType *builtUnit = static_cast<const UnitType*>(command.getProdType());
		const BuildCommandType *bct = static_cast<const BuildCommandType*>(ct);
		if (bct->isBlocked(builtUnit, command.getPos(), command.getFacing())) {
			return CommandResult::FAIL_BLOCKED;
		}
		if (!faction->reqsOk(builtUnit)) {
			return CommandResult::FAIL_REQUIREMENTS;
		}
		if (command.isReserveResources() && !faction->checkCosts(builtUnit)) {
			return CommandResult::FAIL_RESOURCES;
		}
	} else if (!produced  // multi-tier selected morph or produce
	&& (ct->getClass() == CommandClass::MORPH || ct->getClass() == CommandClass::PRODUCE)) {
		produced = command.getProdType();
		assert(produced);
		if (!faction->reqsOk(produced)) {
			return CommandResult::FAIL_REQUIREMENTS;
		}
		if (!faction->checkCosts(produced)) {
			return CommandResult::FAIL_RESOURCES;
		}
	} else if (ct->getClass() == CommandClass::UPGRADE) { // upgrade command specific
		const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(ct);
		if (faction->getUpgradeManager()->isUpgradingOrUpgraded(uct->getProducedUpgrade())) {
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

	// check produced
	const ProducibleType *produced = command.getProdType();
	if (produced) {
		faction->applyCosts(produced);
	}

	// todo multi-tier upgrades, and use CommandType::getProduced() and Command::prodType
	if (ct->getClass() == CommandClass::UPGRADE) { // upgrade (why not handled by getProduced() ???)
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

	// return building cost if not already building it or dead
	if (ct->getClass() == CommandClass::BUILD && command.isReserveResources()) {
		if (currSkill->getClass() != SkillClass::BUILD && currSkill->getClass() != SkillClass::DIE) {
			faction->deApplyCosts(command.getProdType());
		}
	} else { //return cost
		const ProducibleType *produced = command.getProdType();
		if (produced) {
			faction->deApplyCosts(produced);
		}
	}
	// upgrade command cancel from list
	if (ct->getClass() == CommandClass::UPGRADE) {
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
	switch (st->getClass()) {
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

		default:
			break;
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

Unit* UnitFactory::newInstance(const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, CardinalDir face,  Unit* master) {
	Unit *unit = new Unit(idCounter, pos, type, faction, map, face, master);
	unitMap[idCounter] = unit;
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
	deadList.push_back(unit);
}

void UnitFactory::update() {
	Units::iterator it = deadList.begin();
	while (it != deadList.end()) {
		if ((*it)->getToBeUndertaken()) {
			(*it)->undertake();
			unitMap.erase(unitMap.find((*it)->getId()));
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
	unitMap.erase(it);
	Units::iterator uit = std::find(deadList.begin(), deadList.end(), unit);
	if (uit != deadList.end()) {
		deadList.erase(uit);
	}
	delete unit;
}

}}//end namespace

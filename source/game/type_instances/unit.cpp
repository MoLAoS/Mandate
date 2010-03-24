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

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class Unit
// =====================================================

/** skill speed divider @see somewhere else */
const float Unit::speedDivider= 100.f;
/** number of frames until a corpse is removed */
const int Unit::maxDeadCount= 1000;	//time in until the corpse disapears
/** time (in seconds ?) of selection circle effect 'flashes' */
const float Unit::highlightTime= 0.5f;
/** the invalid unit ID */
const int Unit::invalidId= -1;

// ============================ Constructor & destructor =============================

/** Construct Unit object */
Unit::Unit(int id, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, Unit* master)
		: progressSpeed(0.f)
		, animProgressSpeed(0.f)
		, pos(pos)
		, lastPos(pos)
		, nextPos(pos)
		, targetPos(0)
		, targetVec(0.0f)
		, meetingPos(pos)
		, commandCallback(NULL)
		, hp_below_trigger(0)
		, hp_above_trigger(0)
		, attacked_trigger(false) {
	this->faction = faction;
	this->map = map;
	this->master = master;

	this->id = id;
	hp = 1;
	ep = 0;
	loadCount = 0;
	deadCount = 0;
	progress= 0;
	lastAnimProgress = 0;
	animProgress = 0;
	highlight = 0.f;
	progress2 = 0;
	kills = 0;

	// UnitType needs modifiying, the new pathfinder does not support
	// units with multiple fields (nor did the old one), 'switching' fields
	// will need to  be done with morphs. 
	if(type->getField(Field::LAND)) currField = Field::LAND;
	else if(type->getField(Field::AIR)) currField = Field::AIR;

	if (type->getField (Field::AMPHIBIOUS)) currField = Field::AMPHIBIOUS;
	else if (type->getField (Field::ANY_WATER)) currField = Field::ANY_WATER;
	else if (type->getField (Field::DEEP_WATER)) currField = Field::DEEP_WATER;

	targetField = Field::LAND;		// init just to keep it pretty in memory
	level= NULL;

	Random random(id);
	float rot = 0.f;
	rot += random.randRange(-5, 5);

	lastRotation = rot;
	targetRotation = rot;
	rotation = rot;

	this->type = type;
	loadType = NULL;
	currSkill = getType()->getFirstStOfClass(SkillClass::STOP);	//starting skill
//	lastSkill = currSkill;
	//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " constructed at pos(" + intToStr(pos.x) + "," + intToStr(pos.y) + ")" );

	toBeUndertaken = false;
//	alive= true;
	autoRepairEnabled = true;

	computeTotalUpgrade();
	hp = type->getMaxHp() / 20;

	fire = NULL;

	nextUpdateFrames = 1.f;
}

/** delete stuff */
Unit::~Unit() {
	//remove commands
	while (!commands.empty()) {
		delete commands.back();
		commands.pop_back();
	}
	// this shouldn't really be needed, but there seems to be some circumstances where deleted units
	// remain in a selection group.
	Gui::getCurrentGui()->makeSureImNotSelected(this);
//	World::getCurrWorld()->getMap()->makeSureImRemoved(this);
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

/** query if a unit is a pet of this unit
  * @param u potential pet
  * @return true if u is a pet of this unit
  */
bool Unit::isPet(const Unit *u) const {
	for(Pets::const_iterator i = pets.begin(); i != pets.end(); i++) {
		if(*i == u) {
			return true;
		}
	}
	return false;
}

/** find a repair command type that can repair a unit
  * @param u the unit in need of repairing 
  * @return a RepairCommandType that can repair u, or NULL
  */
const RepairCommandType * Unit::getRepairCommandType(const Unit *u) const {
	for(int i = 0; i < type->getCommandTypeCount(); i++) {
		const CommandType *ct = type->getCommandType(i);
		if(ct->getClass() == CommandClass::REPAIR) {
			const RepairCommandType *rct = (const RepairCommandType *)ct;
			const RepairSkillType *rst = rct->getRepairSkillType();
			if((!rst->isPetOnly() || isPet(u))
					&& (!rst->isSelfOnly() || this == u)
					&& (rst->isSelfAllowed() || this != u)
					&& (rct->isRepairableUnitType(u->type))) {
				return rct;
			}
		}
	}
	return NULL;
}

// ====================================== set ======================================

/** sets the current skill */
void Unit::setCurrSkill(const SkillType *currSkill) {
	assert(currSkill);
	//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " skill set => " + SkillClassNames[currSkill->getClass()] );
	if (currSkill->getClass() == SkillClass::STOP && this->currSkill->getClass() == SkillClass::STOP) {
		return;
	}
	if (currSkill->getClass() != this->currSkill->getClass()) {
		animProgress = 0;
		lastAnimProgress = 0;
	}
	progress2 = 0;
	this->currSkill = currSkill;
	notifyObservers(UnitObserver::eStateChange);
}

/** sets unit's target */
void Unit::setTarget(const Unit *unit, bool faceTarget, bool useNearestOccupiedCell) {
	targetRef = unit;
	if(!unit) {
		return;
	}
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
		this->meetingPos = pos + Vec2i(size);
	}
}

/** sets targetRotation */
void Unit::face(const Vec2i &nextPos) {
	Vec2i relPos = nextPos - pos;
	Vec2f relPosf = Vec2f((float)relPos.x, (float)relPos.y);
	targetRotation = radToDeg(atan2f(relPosf.x, relPosf.y));
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
	for(int i = 0; i < type->getCommandTypeCount(); ++i) {
		const CommandType *ct = type->getCommandType(i);
		if(ct && ct->getClass() == commandClass && faction->reqsOk(ct)) {
			return ct;
		}
	}
	return NULL;
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
	UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " command given: " + CommandClassNames[command->getType()->getClass()] );
	if(ct->getClass() == CommandClass::SET_MEETING_POINT) {
		if(command->isQueue() && !commands.empty()) {
			commands.push_back(command);
		} else {
			meetingPos = command->getPos();
			delete command;
		}
		return CommandResult::SUCCESS;
	}

	if(ct->getClass() == CommandClass::ATTACK && pets.size() > 0) {
		// pets don't attack the master's actual target, just to the pos
		Vec2i pos = command->getPos();
		if(pos.x == 0 && pos.y == 0) {
			assert(command->getUnit());
			pos = command->getUnit()->getPos();
		}
		for(Pets::const_iterator i = pets.begin(); i != pets.end(); i++) {
			Unit *pet = i->getUnit();
			const CommandType *ct = pet->getType()->getFirstCtOfClass(CommandClass::ATTACK);
			if(ct) {
				// ignore result
				pet->giveCommand(new Command(ct, CommandFlags(CommandProperties::QUEUE, command->isQueue()), pos));
			}
		}
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
	if(result == CommandResult::SUCCESS){
		applyCommand(*command);
		commands.push_back(command);

		if(faction->isThisFaction() && command->getUnit() && !command->isAuto()) {
			command->getUnit()->resetHighlight();
		}
	} else {
		delete command;
	}

	return result;
}

/** removes current command (and any queued Set meeting point commands) 
  * @return the command now at the head of the queue (the new current command) */
Command *Unit::popCommand() {
	//pop front
	//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " " 
	//	+ CommandClassNames[commands.front()->getType()->getClass()] + " command popped." );
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
	//	UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " " 
	//		+ CommandClassNames[commands.front()->getType()->getClass()] + " command now front of queue." );
	//}
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
	//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " " 
	//	+ CommandClassNames[commands.front()->getType()->getClass()] + " command finished." );

	Command *command = popCommand();

	//for patrol command, remember where we started from
	if(command && command->getType()->getClass() == CommandClass::PATROL) {
		command->setPos2(pos);
	}
	return CommandResult::SUCCESS;
}

/** cancel command on back of queue */
CommandResult Unit::cancelCommand() {

	//is empty?
	if(commands.empty()){
		return CommandResult::FAIL_UNDEFINED;
	}
	//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " queued " 
	//	+ CommandClassNames[commands.front()->getType()->getClass()] + " command cancelled." );

	//undo command
	undoCommand(*commands.back());

	//delete ans pop command
	delete commands.back();
	commands.pop_back();

	//clear routes
	unitPath.clear();

	return CommandResult::SUCCESS;
}

/** cancel current command */
CommandResult Unit::cancelCurrCommand() {
	//is empty?
	if(commands.empty()) {
		return CommandResult::FAIL_UNDEFINED;
	}
	//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " current " 
	//	+ CommandClassNames[commands.front()->getType()->getClass()] + " command cancelled." );

	//undo command
	undoCommand(*commands.front());

	Command *command = popCommand();

	return CommandResult::SUCCESS;
}

// =================== route stack ===================

/** Creates a unit, places it on the map and applies static costs for starting units
  * @param startingUnit true if this is a starting unit.
  */
void Unit::create(bool startingUnit) {
	//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " created." );
	faction->add(this);
	lastPos.x = lastPos.y = -1;
	map->putUnitCells(this, pos);
	if(startingUnit){
		faction->applyStaticCosts(type);
	}
	nextCommandUpdate = -1;
}

/** Give a unit life. Called when a unit becomes 'operative'
  */
void Unit::born(){
	//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " born." );
	faction->addStore(type);
	faction->applyStaticProduction(type);
	setCurrSkill(SkillClass::STOP);
	computeTotalUpgrade();
	recalculateStats();
	hp= type->getMaxHp();
	faction->checkAdvanceSubfaction(type, true);
	theWorld.getCartographer()->applyUnitVisibility(this);
	preProcessSkill();
	nextCommandUpdate--;
}

/**
 * Do everything that should happen when a unit dies, except remove them from the faction.  Should
 * only be called when a unit's HPs are zero or less.
 */
void Unit::kill(const Vec2i &lastPos, bool removeFromCells) {
	assert(hp <= 0);
	hp = 0;

	World::getCurrWorld()->hackyCleanUp(this);
	theWorld.getCartographer()->removeUnitVisibility(this);

	if(fire != NULL) {
		fire->fade();
		fire = NULL;
	}

	//do the cleaning
	if(removeFromCells) {
		map->clearUnitCells(this, lastPos);
	}

	if(!isBeingBuilt()) {
		faction->removeStore(type);
	}
	setCurrSkill(SkillClass::DIE);

	//no longer needs static resources
	if(isBeingBuilt()) {
		faction->deApplyStaticConsumption(type);
	} else {
		faction->deApplyStaticCosts(type);
	}

	Died(this);

	notifyObservers(UnitObserver::eKill);
	clearCommands();
	//kill or free pets
	killPets();
	//kiss mom of the cheek
	if(master.getUnit()) {
		master.getUnit()->petDied(this);
	}
	// hack... 'tracking' particle systems might reference this, 'this' will soon be deleted...
	Renderer::getInstance().getParticleManager()->checkTargets(this);

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
			commandType = type->getFirstAttackCommand(targetUnit->getCurrZone());

		//repair allies
		} else {
			commandType = getRepairCommandType(targetUnit);
		}
	} else {
		//check harvest command
		Resource *resource = sc->getResource();
		if (resource != NULL) {
			commandType = type->getFirstHarvestCommand(resource->getType());
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

void Unit::preProcessSkill() {
	//speed
	static const float speedModifier = 1.f / (float)speedDivider / (float)Config::getInstance().getGsWorldUpdateFps();

	progressSpeed = getSpeed() * speedModifier;
	animProgressSpeed = currSkill->getAnimSpeed() * speedModifier;
	//speed modifiers
	if(currSkill->getClass() == SkillClass::MOVE) {

		if (pos.x != nextPos.x && pos.y != nextPos.y) {
			//if moving in diagonal move slower
			progressSpeed *= 0.71f;
		}
		//if moving to a higher cell move slower else move faster
		float heightDiff = map->getCell(lastPos)->getHeight() - map->getCell(pos)->getHeight();
		float heightFactor = clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
		progressSpeed *= heightFactor;
		animProgressSpeed *= heightFactor;
	}
	int frames = 1.0000001f / progressSpeed;
	int end = theWorld.getFrameCount() + frames + 1;
	/*
	if ( !commands.empty() ) {
		UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " updating " 
			+ CommandClassNames[commands.front()->getType()->getClass()] + " command, commencing "
			+ SkillClassNames[currSkill->getClass()] + " skill cycle, will finish @ " + intToStr(end) );
	} else {
		UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " updating no command, commencing "
			+ SkillClassNames[currSkill->getClass()] + " skill cycle, will finish @ " + intToStr(end) );
	}
	*/
	nextCommandUpdate = end;
}

/** @return true when the current skill has completed a cycle */
bool Unit::update() {
	assert(progress <= 1.f);
	PROFILE_START( "Unit Update" );

	//highlight
	if(highlight > 0.f) {
		highlight -= 1.f / (highlightTime * Config::getInstance().getGsWorldUpdateFps());
	}

	//update progresses
	lastAnimProgress = animProgress;
	progress += nextUpdateFrames * progressSpeed;
	animProgress += nextUpdateFrames * animProgressSpeed;
	nextUpdateFrames = 1.f;

	//UNIT_LOG( intToStr(theWorld.getFrameCount()) + "::Unit:" + intToStr(id) + " updating " 
	//	+ SkillClassNames[currSkill->getClass()] + ", progress:" + floatToStr(progress) );

	//update target
	updateTarget();

	//rotation
	if(currSkill->getClass() != SkillClass::STOP) {
		const int rotFactor = 2;
		if(progress < 1.f / rotFactor) {
			if(type->getFirstStOfClass(SkillClass::MOVE)) {
				if(abs(lastRotation - targetRotation) < 180) {
					rotation = lastRotation + (targetRotation - lastRotation) * progress * rotFactor;
				} else {
					float rotationTerm = targetRotation > lastRotation ? -360.f : + 360.f;
					rotation = lastRotation + (targetRotation - lastRotation + rotationTerm) * progress * rotFactor;
				}
			}
		}
	}

	//checks
	if(animProgress > 1.f) {
		animProgress = currSkill->getClass() == SkillClass::DIE ? 1.f : 0.f;
	}

	bool ret = false;
	//checks
	if(progress >= 1.f) {
		lastRotation = targetRotation;

		if(currSkill->getClass() != SkillClass::DIE) {
			progress = 0.f;
			ret = true;
		} else { 
			progress = 1.f;
			deadCount++;
			if (deadCount >= maxDeadCount) {
				toBeUndertaken = true;
			}
		}
	}
	PROFILE_STOP( "Unit Update" );
	return ret;
}

/**
 * Do positive and/or negative Hp and Ep regeneration. This method is
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
			case CommandClass::PATROL:
				break;
			default:
				continue;
		}

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
bool Unit::repair(int amount, float multiplier) {
	if (!isAlive()) {
		return false;
	}

	//if not specified, use default value
	if (!amount) {
		amount = getType()->getMaxHp() / type->getProductionTime() + 1;
	}
	amount = (int)((float)amount * multiplier);

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
		Renderer::getInstance().manageParticleSystem(fps, rsGame);
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

/** Kill all pets belonging to this unit
  * @return the number of pets killed
  */
int Unit::killPets() {
	int count = 0;
	while(pets.size() > 0){
		Unit *pet = pets.back();
		assert(pet->isAlive());
		if(pet->isAlive()) {
			pet->decHp(pet->getHp());
			++count;
		}
		pets.remove(pet);
	}
	return count;
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
	str += " (" + type->getArmorType()->getName() + ")";

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
void Unit::computeTotalUpgrade(){
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

	// redo field
	Field newField;
	if(morphUnitType->getField(Field::LAND)) newField = Field::LAND;
	else if(morphUnitType->getField(Field::AIR)) newField = Field::AIR;
	if ( morphUnitType->getField (Field::AMPHIBIOUS) ) newField = Field::AMPHIBIOUS;
	else if ( morphUnitType->getField (Field::ANY_WATER) ) newField = Field::ANY_WATER;
	else if ( morphUnitType->getField (Field::DEEP_WATER) ) newField = Field::DEEP_WATER;

	if (map->areFreeCellsOrHasUnit(pos, morphUnitType->getSize(), newField, this)) {
		map->clearUnitCells(this, pos);
		faction->deApplyStaticCosts(type);
		type = morphUnitType;
		currField = newField;
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


// ==================== PRIVATE ====================

/** calculate unit height
  * @param pos location ground reference
  * @return the height this unit 'stands' at
  */
inline float Unit::computeHeight(const Vec2i &pos) const {
	float height = map->getCell(pos)->getHeight();

	if (currField == Field::AIR) {
		height += World::airHeight;
	}

	return height;
}

/** updates target information, (targetPos, targetField & tagetVec) and resets targetRotation 
  * @param target the unit we are tracking */
void Unit::updateTarget(const Unit *target) {
	if(!target) {
		target = targetRef.getUnit();
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

	//if this is a pet, make sure we don't have more pets of this type than allowed.
	if (ct->getClass() == CommandClass::PRODUCE) {
		const ProduceCommandType *pct = reinterpret_cast<const ProduceCommandType*>(ct);
		const ProduceSkillType *pst = pct->getProduceSkillType();
		if(pst->isPet()) {
			const UnitType *ut = pct->getProducedUnit();
			int totalPets = getPetCount(ut);
			for(Commands::const_iterator i = commands.begin(); i != commands.end(); ++i) {
				const CommandType *ct2 = (*i)->getType();
				if(*i != &command && ct2->getClass() == CommandClass::PRODUCE) {
					const ProduceCommandType* pct2 = reinterpret_cast<const ProduceCommandType*>(ct2);
					if(pct2->getProducedUnit() == ut) {
						++totalPets;
					}
				}
			}

			if(totalPets >= pst->getMaxPets()) {
				return CommandResult::FAIL_PET_LIMIT;
			}
		}

	//build command specific, check resources and requirements for building
	} else if(ct->getClass() == CommandClass::BUILD) {
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

/**
 * Recalculate the unit's stats (contained in base class
 * EnhancementTypeBase) to take into account changes in the effects and/or
 * totalUpgrade objects.
 */
void Unit::recalculateStats() {
	int oldMaxHp = getMaxHp();
	int oldHp = hp;
	reset();
	setValues(*type);

	// add up all multipliers first and then apply (multiply) once.  e.g.:
	//	baseStat * (multiplier + multiplier)
	// and not:
	//	baseStat * multiplier * multiplier
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

	// if not available in subfaction, it has to decay
	/* maybe not, we need rules in faction.xml to specify this if we're going to do it
	if(!faction->isAvailable(type)) {
		int decAmount = (int)roundf(type->getMaxHp() / 200.f);
		hpRegeneration -= decAmount;

		// if still not degenerating give them one more dose
		if(hpRegeneration >= 0) {
			hpRegeneration -= decAmount;
		}
	}*/
}
/** query the speed at which a skill type is executed 
  * @param st the SkillType
  * @return the speed value this unit would execute st at
  */
int Unit::getSpeed(const SkillType *st) const {
	float speed = (float)st->getSpeed();
	switch(st->getClass()) {
		case SkillClass::MOVE:
		case SkillClass::GET_UP:
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
		case SkillClass::CAST_SPELL:
		case SkillClass::FALL_DOWN:
		case SkillClass::WAIT_FOR_SERVER:
		case SkillClass::COUNT:
			break;
		default:
			throw runtime_error("unhandled SkillClass");
	}
	return (int)(speed > 0.0f ? speed : 0.0f);
}

/** query number of pets of a given type @param unitType type of pets to count 
  * @return number of pets of unitType */
int Unit::getPetCount(const UnitType *unitType) const {
	int count = 0;
	for(Pets::const_iterator i = pets.begin(); i != pets.end(); ++i) {
		if(i->getUnit()->getType() == unitType) {
			++count;
		}
	}
	return count;
}

}}//end namespace

// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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

#include "leak_dumper.h"


using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class UnitPath
// =====================================================

const int UnitPath::maxBlockCount= 10;

// =====================================================
// 	class Unit
// =====================================================

const float Unit::speedDivider= 100.f;
const int Unit::maxDeadCount= 1000;	//time in until the corpse disapears
const float Unit::highlightTime= 0.5f;
const int Unit::invalidId= -1;

// ============================ Constructor & destructor =============================

Unit::Unit(int id, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, Unit* master):
		pos(pos),
		lastPos(pos),
		targetPos(0),
		targetVec(0.0f),
		meetingPos(pos) {
	Random random;

	this->faction=faction;
	this->map= map;
	this->master = master;

	this->id= id;
	hp = 1;
	ep= 0;
	loadCount= 0;
	deadCount= 0;
	progress= 0;
	lastAnimProgress= 0;
	animProgress= 0;
	highlight= 0.f;
	progress2= 0;
	kills= 0;

	if(type->getField(fAir)) currField = fAir;
//	if(getType()->getField(fWater)) currField = fWater;
//	if(getType()->getField(fSubterranean)) currField = fSubterranean;
	if(type->getField(fLand)) currField = fLand;
	targetField = fLand;		// init just to keep it pretty in memory
	level= NULL;

	float rot= 0.f;
	random.init(id);
	rot+= random.randRange(-5, 5);

	lastRotation= rot;
	targetRotation= rot;
	rotation= rot;

	this->type=type;
	loadType = NULL;
	currSkill= getType()->getFirstStOfClass(scStop);	//starting skill
//	lastSkill = currSkill;

	toBeUndertaken= false;
	alive= true;
	autoRepairEnabled= true;

	computeTotalUpgrade();
	hp= type->getMaxHp()/20;

	fire= NULL;
}

Unit::~Unit(){
	//remove commands
	while(!commands.empty()){
		delete commands.back();
		commands.pop_back();
	}
}

void Unit::save(XmlNode *node, bool morphed) const {
	XmlNode *n;

	if(morphed) {
		node->addAttribute("morphed", true);
	}

	node->addChild("id", id);
	node->addChild("hp", hp);
	node->addChild("ep", ep);
	node->addChild("loadCount", loadCount);
	node->addChild("deadCount", deadCount);
	node->addChild("progress", progress);
	node->addChild("lastAnimProgress", lastAnimProgress);
	node->addChild("animProgress", animProgress);
	node->addChild("highlight", highlight);
	node->addChild("progress2", progress2);
	node->addChild("kills", kills);
	targetRef.save(node->addChild("targetRef"));
	node->addChild("currField", currField);
	node->addChild("targetField", targetField);
	node->addChild("pos", pos);
	node->addChild("lastPos", lastPos);
	node->addChild("targetPos", targetPos);
	node->addChild("targetVec", targetVec);
	node->addChild("meetingPos", meetingPos);
	node->addChild("lastRotation", lastRotation);
	node->addChild("targetRotation", targetRotation);
	node->addChild("rotation", rotation);
	node->addChild("type", type->getName());
	node->addChild("loadType", loadType ? loadType->getName() : "null_value");
	node->addChild("currSkill", currSkill ? currSkill->getName() : "null_value");

	node->addChild("toBeUndertaken", toBeUndertaken);
	node->addChild("alive", alive);
	node->addChild("autoRepairEnabled", autoRepairEnabled);

	effects.save(node->addChild("effects"));
	effectsCreated.save(node->addChild("effectsCreated"));

	node->addChild("fire", fire ? true : false);

	n = node->addChild("commands");
	for(Commands::const_iterator i = commands.begin(); i != commands.end(); ++i) {
		(*i)->save(n->addChild("command"));
	}

	n = node->addChild("pets");
	for(Pets::const_iterator i = pets.begin(); i != pets.end(); ++i) {
		i->save(n->addChild("pet"));
	}

	master.save(node->addChild("master"));
}

void Unit::writeMinorUpdate(XmlNode *node) const {
	node->addChild("hp", hp);
	node->addChild("ep", ep);
	node->addChild("loadCount", loadCount);
	node->addChild("kills", kills);
	targetRef.save(node->addChild("targetRef"));
	node->addChild("pos", pos);
	node->addChild("targetPos", targetPos);
	node->addChild("targetVec", targetVec);
	node->addChild("loadType", loadType ? loadType->getName() : "null_value");
	node->addChild("currSkill", currSkill ? currSkill->getName() : "null_value");
	node->addChild("alive", alive);
}

void Unit::updateMinor(const XmlNode *node) {
	string s;
	int oldHp = hp;
	const TechTree *tt = World::getCurrWorld()->getTechTree();

	hp = node->getChildIntValue("hp");
	ep = node->getChildIntValue("ep");
	loadCount = node->getChildIntValue("loadCount");
	kills = node->getChildIntValue("kills");
	targetRef = UnitReference(node->getChild("targetRef")).getUnit();
	pos = node->getChildVec2iValue("pos");
	targetPos = node->getChildVec2iValue("targetPos");
	targetVec = node->getChildVec3fValue("targetVec");
	alive = node->getChildBoolValue("alive");

	s = node->getChildStringValue("loadType");
	loadType = s == "null_value" ? NULL : tt->getResourceType(s);

	s = node->getChildStringValue("currSkill");
	currSkill = s == "null_value" ? NULL : type->getSkillType(s);

	if(hp < oldHp) {
		// pick up fire if we need it
		decHp(0);
	}
}

Unit::Unit(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld) :
		targetRef(node->getChild("targetRef")),
		effects(node->getChild("effects")),
		effectsCreated(node->getChild("effectsCreated")) {
	this->faction = faction;
	this->map = map;

	id = node->getChildIntValue("id");

	update(node, tt, true, putInWorld);
}

void Unit::update(const XmlNode *node, const TechTree *tt, bool creation, bool putInWorld) {
	const XmlNode *n;
	string s;

	XmlAttribute *morphAtt = node->getAttribute("morphed", false);
	bool morphed = morphAtt && morphAtt->getBoolValue();

	if(morphed) {
		map->clearUnitCells(this, pos);
		putInWorld = false;
	}

	if(!creation) {
		targetRef = UnitReference(node->getChild("targetRef")).getUnit();
//		effects(node->getChild("effects")),
//		effectsCreated(node->getChild("effectsCreated")) {

	}
//	hp = node->getChildIntValue("hp");	//set this last because recalculateStats() will overwrite it
	ep = node->getChildIntValue("ep");
	loadCount = node->getChildIntValue("loadCount");
	deadCount = node->getChildIntValue("deadCount");
	progress = node->getChildFloatValue("progress");
	lastAnimProgress = node->getChildFloatValue("lastAnimProgress");
	animProgress = node->getChildFloatValue("animProgress");
	highlight = node->getChildFloatValue("highlight");
	progress2 = node->getChildIntValue("progress2");
	kills = node->getChildIntValue("kills");
	currField = (Field)node->getChildIntValue("currField");
	targetField = (Field)node->getChildIntValue("targetField");

	pos = node->getChildVec2iValue("pos");		//map->putUnitCells() will set this, so we reload it later
	lastPos = node->getChildVec2iValue("lastPos");
	targetPos = node->getChildVec2iValue("targetPos");
	targetVec = node->getChildVec3fValue("targetVec");
//	meetingPos = node->getChildVec2iValue("meetingPos"); //loaded after map->putUnitCells()
	lastRotation = node->getChildFloatValue("lastRotation");
	targetRotation = node->getChildFloatValue("targetRotation");
	rotation = node->getChildFloatValue("rotation");
	type = faction->getType()->getUnitType(node->getChildStringValue("type"));

	s = node->getChildStringValue("loadType");
	loadType = s == "null_value" ? NULL : tt->getResourceType(s);

	s = node->getChildStringValue("currSkill");
	currSkill = s == "null_value" ? NULL : type->getSkillType(s);

// 	if(!currSkill || currSkill->getClass() == scWaitForServer) {
// 		lastSkill = type->getFirstStOfClass(scStop);
// 	} else {
// 		lastSkill = currSkill;
// 	}

	toBeUndertaken = node->getChildBoolValue("toBeUndertaken");
	alive = node->getChildBoolValue("alive");
	autoRepairEnabled = node->getChildBoolValue("autoRepairEnabled");

	n = node->getChild("commands");
	if(!creation) {
		// network clients fudge it
		commands.clear();
	} else {
		clearCommands();
	}
	for(int i = 0; i < n->getChildCount(); ++i) {
		commands.push_back(new Command(n->getChild("command", i), type, faction->getType()));
	}

	unitPath.clear();
	if(creation) {
		observers.clear();
	}

	totalUpgrade.reset();
	computeTotalUpgrade();

	if(creation) {
		fire = NULL;
		faction->addUnit(this);
	}

	if(alive && putInWorld) {
		map->putUnitCells(this, node->getChildVec2iValue("pos"));
	}

	//putUnitCells sets this, so we reset it here
	meetingPos = node->getChildVec2iValue("meetingPos");

	recalculateStats();

	//HP will be at max due to recalculateStats
	hp = node->getChildIntValue("hp");

	if(type->hasSkillClass(scBeBuilt)){
		map->flatternTerrain(this);
	}

	if(node->getChildBoolValue("fire")) {
		decHp(0);
	}

	//care should be taken to ensure that no attempt to access either master or
	//pets fields are made before all of the factions are initialized because
	//these are UnitReferences and the units they refer to may not be loaded yet.
	master = UnitReference(node->getChild("master"));

	n = node->getChild("pets");
	pets.clear();
	for(int i = 0; i < n->getChildCount(); ++i) {
		pets.push_back(UnitReference(n->getChild("pet", i)));
	}
}

// ====================================== get ======================================

Vec2i Unit::getCellPos() const{

	if(type->hasCellMap()){

		//find nearest pos to center that is free
		Vec2i centeredPos= getCenteredPos();
		float nearestDist= -1.f;
		Vec2i nearestPos= pos;

		for(int i=0; i<type->getSize(); ++i){
			for(int j=0; j<type->getSize(); ++j){
				if(type->getCellMapCell(i, j)){
					Vec2i currPos= pos + Vec2i(i, j);
					float dist= currPos.dist(centeredPos);
					if(nearestDist==-1.f || dist<nearestDist){
						nearestDist= dist;
						nearestPos= currPos;
					}
				}
			}
		}
		return nearestPos;
	}
	return pos;
}
/*
float Unit::getVerticalRotation() const{
	/ *if(type->getProperty(UnitType::pRotatedClimb) && currSkill->getClass()==scMove){
		float heightDiff= map->getCell(pos)->getHeight() - map->getCell(targetPos)->getHeight();
		float dist= pos.dist(targetPos);
		return radToDeg(atan2(heightDiff, dist));
	}* /
	return 0.f;
}
*/
int Unit::getProductionPercent() const{
	if(anyCommand()){
		const ProducibleType *produced= commands.front()->getCommandType()->getProduced();
		if(produced!=NULL){
			return clamp(progress2*100/produced->getProductionTime(), 0, 100);
		}
	}
	return -1;
}

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

string Unit::getFullName() const{
	string str;
	if(level!=NULL){
		str+= level->getName() + " ";
	}
	str+= type->getName();
	return str;
}

// ====================================== is ======================================


bool Unit::isInteresting(InterestingUnitType iut) const{
	switch(iut){
	case iutIdleHarvester:
		if(type->hasCommandClass(ccHarvest)){
			if(!commands.empty()){
				const CommandType *ct= commands.front()->getCommandType();
				if(ct!=NULL){
					return ct->getClass()==ccStop;
				}
			}
		}
		return false;

	case iutBuiltBuilding:
		return type->hasSkillClass(scBeBuilt) && isBuilt();
	case iutProducer:
		return type->hasSkillClass(scProduce);
	case iutDamaged:
		return isDamaged();
	case iutStore:
		return type->getStoredResourceCount()>0;
	default:
		return false;
	}
}

bool Unit::isPet(const Unit *u) const {
	for(Pets::const_iterator i = pets.begin(); i != pets.end(); i++) {
		if(*i == u) {
			return true;
		}
	}
	return false;
}

const RepairCommandType * Unit::getRepairCommandType(const Unit *u) const {
	for(int i = 0; i < type->getCommandTypeCount(); i++) {
		const CommandType *ct = type->getCommandType(i);
		if(ct->getClass() == ccRepair) {
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

void Unit::setCurrSkill(const SkillType *currSkill){
	if(currSkill->getClass() == scStop && this->currSkill->getClass() == scStop) {
		return;
	}
	if(currSkill->getClass() != this->currSkill->getClass()){
		animProgress= 0;
		lastAnimProgress= 0;
	}
	progress2= 0;
/*	if(currSkill->getClass() != scWaitForServer) {
		lastSkill = this->currSkill;
	}*/
	this->currSkill= currSkill;
	notifyObservers(UnitObserver::eStateChange);
}

void Unit::setTarget(const Unit *unit){
	//find a free pos in cellmap
	setTargetPos(unit->getCellPos());

	//ser field and vector
	targetField= unit->getCurrField();
	targetVec= unit->getCurrVector();
	targetRef= unit;
}

void Unit::setPos(const Vec2i &pos){
	this->lastPos= this->pos;
	this->pos= pos;
	this->meetingPos= pos - Vec2i(1);
}

void Unit::setTargetPos(const Vec2i &targetPos){

	Vec2i relPos= targetPos - pos;
	Vec2f relPosf= Vec2f(relPos.x, relPos.y);
	targetRotation= radToDeg(atan2(relPosf.x, relPosf.y));
	targetRef= NULL;

	this->targetPos= targetPos;
}

void Unit::setMeetingPos(const Vec2i &meetingPos, bool queue) {
	if(queue && currSkill->getClass() != scStop) {
		giveCommand(new Command(SetMeetingPointCommandType::getInstance(), CommandFlags(cpQueue), meetingPos));
	} else {
		this->meetingPos= meetingPos;
	}
}

// =============================== Render related ==================================

Vec3f Unit::getCurrVectorFlat() const{
	Vec3f v;

	float y1= computeHeight(lastPos);
	float y2= computeHeight(pos);

	if(currSkill->getClass()==scMove){
		v.x= lastPos.x + progress * (pos.x-lastPos.x);
		v.z= lastPos.y + progress * (pos.y-lastPos.y);
		v.y= y1+progress*(y2-y1);
	}
	else{
		v.x= static_cast<float>(pos.x);
		v.z= static_cast<float>(pos.y);
		v.y= y2;
	}
	v.x+= type->getSize()/2.f-0.5f;
	v.z+= type->getSize()/2.f-0.5f;

	return v;
}

// =================== Command list related ===================

const CommandType *Unit::getFirstAvailableCt(CommandClass commandClass) const {
	for(int i = 0; i < type->getCommandTypeCount(); ++i) {
		const CommandType *ct = type->getCommandType(i);
		if(ct && ct->getClass() == commandClass && faction->reqsOk(ct)) {
			return ct;
		}
	}
	return NULL;
}

//give one command (clear, and push back)
CommandResult Unit::giveCommand(Command *command) {

	if(command->getCommandType()->getClass() == ccAttack && pets.size() > 0) {
		// pets don't attack the master's actual target, just to the pos
		Vec2i pos = command->getPos();
		if(pos.x == 0 && pos.y == 0) {
			assert(command->getUnit());
			pos = command->getUnit()->getPos();
		}
		for(Pets::const_iterator i = pets.begin(); i != pets.end(); i++) {
			Unit *pet = i->getUnit();
			const CommandType *ct = pet->getType()->getFirstCtOfClass(ccAttack);
			if(ct) {
				// ignore result
				pet->giveCommand(new Command(ct, CommandFlags(cpQueue, command->isQueue()), pos));
			}
		}
	}

	if(command->getCommandType()->isQueuable() || command->isQueue()) {
		//cancel current command if it is not queuable or marked to be queued
		if(!commands.empty() && !commands.front()->getCommandType()->isQueuable() && !command->isQueue()) {
			cancelCommand();
			unitPath.clear();
		}
	} else {
		//empty command queue
		clearCommands();
		unitPath.clear();

		//for patrol commands, remember where we started from
		if(command->getCommandType()->getClass() == ccPatrol) {
			command->setPos2(pos);
		}
	}

	//check command
	CommandResult result= checkCommand(command);
	if(result==crSuccess){
		applyCommand(command);
	}

	//push back command
	if(result== crSuccess){
		commands.push_back(command);
	} else {
		delete command;
	}

	return result;
}

//pop front (used when order is done)
CommandResult Unit::finishCommand() {

	//is empty?
	if(commands.empty()) {
		return crFailUndefined;
	}

	//pop front
	delete commands.front();
	commands.erase(commands.begin());
	unitPath.clear();

	Command *command = commands.empty() ? NULL : commands.front();

	// we don't let hacky set meeting point commands actually get anywhere
	while(command && command->getCommandType()->getClass() == ccSetMeetingPoint) {
		meetingPos = command->getPos();
		delete command;
		commands.erase(commands.begin());
		command = commands.empty() ? NULL : commands.front();
	}

	//for patrol command, remember where we started from
	if(command && command->getCommandType()->getClass() == ccPatrol) {
		command->setPos2(pos);
	}

	//send an update to the client
	NetworkManager &networkManager = NetworkManager::getInstance();
	if(networkManager.isNetworkGame() && networkManager.isServer()) {
		networkManager.getServerInterface()->unitUpdate(this);
	}

	return crSuccess;
}

/** cancel command on back of queue */
CommandResult Unit::cancelCommand(){

	//is empty?
	if(commands.empty()){
		return crFailUndefined;
	}

	//undo command
	undoCommand(commands.back());

	//delete ans pop command
	delete commands.back();
	commands.pop_back();

	//clear routes
	unitPath.clear();

	return crSuccess;
}

/** cancel current command */
CommandResult Unit::cancelCurrCommand() {
	//is empty?
	if(commands.empty()){
		return crFailUndefined;
	}

	//undo command
	undoCommand(commands.front());

	//pop front
	delete commands.front();
	commands.erase(commands.begin());
	unitPath.clear();

	//clear routes
	unitPath.clear();

	return crSuccess;
}

// =================== route stack ===================

void Unit::create(bool startingUnit){
	faction->addUnit(this);
	map->putUnitCells(this, pos);
	if(startingUnit){
		faction->applyStaticCosts(type);
	}
}

void Unit::born(){
	faction->addStore(type);
	faction->applyStaticProduction(type);
	setCurrSkill(scStop);
	computeTotalUpgrade();
	recalculateStats();
	hp= type->getMaxHp();
	faction->checkAdvanceSubfaction(type, true);
}

void Unit::kill(const Vec2i &lastPos, bool removeFromCells) {

	hp = 0;

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
	setCurrSkill(scDie);

	notifyObservers(UnitObserver::eKill);

	//clear commands
	clearCommands();

	//no longer needs static resources
	if(!isBeingBuilt()) {
		faction->deApplyStaticCosts(type);
	}

	//kill or free pets
	killPets();

	//kiss mom of the cheek
	if(master.getUnit()) {
		master.getUnit()->petDied(this);
	}

	// random decay time
	//deadCount = Random(id).randRange(-256, 256);
	deadCount = 0;
}

// =================== Referencers ===================



// =================== Other ===================

const CommandType *Unit::computeCommandType(const Vec2i &pos, const Unit *targetUnit) const{
	const CommandType *commandType= NULL;
	SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(pos));

	if(targetUnit!=NULL){
		//attack enemies
		if(!isAlly(targetUnit)){
			commandType= type->getFirstAttackCommand(targetUnit->getCurrField());
		}

		//repair allies
		else{
			commandType= getRepairCommandType(targetUnit);
		}
	}
	else{
		//check harvest command
		Resource *resource= sc->getResource();
		if(resource!=NULL){
			commandType= type->getFirstHarvestCommand(resource->getType());
		}
	}

	//default command is move command
	if(commandType==NULL){
		commandType= type->getFirstCtOfClass(ccMove);
	}

	return commandType;
}

bool Unit::update(){
	assert(progress<=1.f);

	//highlight
	if(highlight>0.f){
		highlight-= 1.f/(highlightTime*Config::getInstance().getWorldUpdateFPS());
	}

	//speed
	int speed= getSpeed();

	//speed modifier
	float diagonalFactor= 1.f;
	float heightFactor= 1.f;
	if(currSkill->getClass()==scMove){

		//if moving in diagonal move slower
		Vec2i dest= pos-lastPos;
		if(abs(dest.x)+abs(dest.y) == 2){
			diagonalFactor= 0.71f;
		}

		//if moving to an higher cell move slower else move faster
		float heightDiff= map->getCell(pos)->getHeight() - map->getCell(targetPos)->getHeight();
		heightFactor= clamp(1.f+heightDiff/5.f, 0.2f, 5.f);
	}

	//update progresses
	lastAnimProgress= animProgress;
	progress+= (speed*diagonalFactor*heightFactor)/(speedDivider*Config::getInstance().getWorldUpdateFPS());
	animProgress+= (currSkill->getAnimSpeed()*heightFactor)/(speedDivider*Config::getInstance().getWorldUpdateFPS());

	//update target
	updateTarget();

	//rotation
	if(currSkill->getClass()!=scStop){
		const int rotFactor= 2;
		if(progress<1.f/rotFactor){
			if(type->getFirstStOfClass(scMove)){
				if(abs(lastRotation-targetRotation)<180)
					rotation= lastRotation+(targetRotation-lastRotation)*progress*rotFactor;
				else{
					float rotationTerm= targetRotation>lastRotation? -360.f: +360.f;
					rotation= lastRotation+(targetRotation-lastRotation+rotationTerm)*progress*rotFactor;
				}
			}
		}
	}

	//replace references to dead units with their dying position prior to their
	//deletion
	for(Commands::iterator i = commands.begin(); i != commands.end(); i++) {
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

	//checks
	if(animProgress>1.f){
		animProgress= currSkill->getClass()==scDie? 1.f: 0.f;
	}

	//checks
	if(progress>=1.f){
		lastRotation= targetRotation;
		if(currSkill->getClass()!=scDie){
			progress= 0.f;
			return true;
		}
		else{
			progress= 1.f;
			deadCount++;
			if(deadCount>=maxDeadCount){
				toBeUndertaken= true;
				return false;
			}
		}
	}

	return false;
}

/** Returns true if dead. */
bool Unit::doRegen(int hpRegeneration, int epRegeneration) {
	if(hp < 1) {
		// dead people don't regenerate
		return true;
	}

	//hp regen/degen
	if(hpRegeneration > 0) {
		repair(hpRegeneration);
	} else if(hpRegeneration < 0) {
		if(decHp(-hpRegeneration)) {
			return true;
		}
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

Unit* Unit::tick() {
	Unit *killer = NULL;

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

bool Unit::computeEp(){

	//if not enough ep
	if(currSkill->getEpCost() > 0 && ep - currSkill->getEpCost() < 0){
		return true;
	}

	//decrease ep
	ep-= currSkill->getEpCost();
	if(ep>getMaxEp()){
		ep= getMaxEp();
	}

	return false;
}

bool Unit::repair(int amount, float multiplier) {
	assert(hp == 0 && isDead() || hp > 0 && isAlive());
	if(!alive) {
		return false;
	}

	//if not specified, use default value
	if(!amount) {
		amount = getType()->getMaxHp()/type->getProductionTime() + 1;
	}
	amount = (int)((float)amount * multiplier);

	//increase hp
	hp+= amount;
	if(hp>getMaxHp()){
		hp= getMaxHp();
		if(!isBuilt()) {
			faction->checkAdvanceSubfaction(type, true);
			born();
		}
		return true;
	}

	//stop fire
	if(hp>type->getMaxHp()/2 && fire!=NULL){
		fire->fade();
		fire= NULL;
	}
	return false;
}

//decrements HP and returns true if dead
bool Unit::decHp(int i) {
	assert(i >= 0);
	if (hp == 0) {
		return false;
	}
	// we shouldn't ever go negative
	assert(hp > 0);
	hp -= i;

	//fire
	if (type->getProperty(pBurnable) && hp < type->getMaxHp() / 2 && fire == NULL) {
		FireParticleSystem *fps;
		fps = new FireParticleSystem(200);
		fps->setSpeed(2.5f / Config::getInstance().getWorldUpdateFPS());
		fps->setPos(getCurrVector());
		fps->setRadius(type->getSize() / 3.f);
		fps->setTexture(CoreData::getInstance().getFireTexture());
		fps->setParticleSize(type->getSize() / 3.f);
		fire = fps;
		Renderer::getInstance().manageParticleSystem(fps, rsGame);
	}

	//stop fire on death
	if (hp <= 0) {
		alive = false;
		hp = 0;
		if (fire != NULL) {
			fire->fade();
			fire = NULL;
		}
		return true;
	}
	return false;
}

int Unit::killPets() {
	int count = 0;
	while(pets.size() > 0){
		Unit *pet = pets.back();
		if(pet->isAlive()) {
			pet->decHp(pet->getHp());
			++count;
		} else {
			pets.remove(pet);
		}
	}
	return count;
}


string Unit::getDesc() const{

	Lang &lang= Lang::getInstance();
	int armorBonus = getArmor() - type->getArmor();
	int sightBonus = getSight() - type->getSight();

	//pos
	//str+="Pos: "+v2iToStr(pos)+"\n";

	//hp
	string str= "\n" + lang.get("Hp")+ ": " + intToStr(hp) + "/" + intToStr(getMaxHp());
	if(getHpRegeneration()!=0){
		str+= " (" + lang.get("Regeneration") + ": " + intToStr(getHpRegeneration()) + ")";
	}

	//ep
	if(getMaxEp()!=0){
		str+= "\n" + lang.get("Ep")+ ": " + intToStr(ep) + "/" + intToStr(getMaxEp());
		if(getEpRegeneration()!=0){
			str+= " (" + lang.get("Regeneration") + ": " + intToStr(getEpRegeneration()) + ")";
		}
	}

	//armor
	str+= "\n" + lang.get("Armor")+ ": " + intToStr(type->getArmor());
	if(armorBonus) {
		if(armorBonus > 0) {
			str+="+";
		}
		str+=intToStr(armorBonus);
	}
	str+= " ("+type->getArmorType()->getName()+")";

	//sight
	str+="\n"+ lang.get("Sight")+ ": " + intToStr(type->getSight());
	if(sightBonus){
		if(sightBonus > 0) {
			str+="+";
		}
		str+=intToStr(sightBonus);
	}

	//kills
	const Level *nextLevel= getNextLevel();
	if(kills>0 || nextLevel!=NULL){
		str+= "\n" + lang.get("Kills") +": " + intToStr(kills);
		if(nextLevel!=NULL){
			str+= " (" + nextLevel->getName() + ": " + intToStr(nextLevel->getKills()) + ")";
		}
	}

	//str+= "\nskl: "+scToStr(currSkill->getClass());

	//load
	if(loadCount){
		str+= "\n" + lang.get("Load")+ ": " + intToStr(loadCount) +"  " + loadType->getName();
	}

	//consumable production
	for(int i=0; i<type->getCostCount(); ++i){
		const Resource *r= getType()->getCost(i);
		if(r->getType()->getClass()==rcConsumable){
			str+= "\n";
			str+= r->getAmount()<0? lang.get("Produce")+": ": lang.get("Consume")+": ";
			str+= intToStr(abs(r->getAmount())) + " " + r->getType()->getName();
		}
	}

	//command info
	if(!commands.empty()){
		str+= "\n" + commands.front()->getCommandType()->getName();
		if(commands.size()>1){
			str+="\n"+lang.get("OrdersOnQueue")+": "+intToStr(commands.size());
		}
	}
	else{
		//can store
		if(type->getStoredResourceCount()>0){
			for(int i=0; i<type->getStoredResourceCount(); ++i){
				const Resource *r= type->getStoredResource(i);
				str+= "\n"+lang.get("Store")+": ";
				str+= intToStr(r->getAmount()) + " " + r->getType()->getName();
			}
		}
	}

	//effects
	effects.getDesc(str);

	return str;
}

void Unit::applyUpgrade(const UpgradeType *upgradeType){
	if(upgradeType->isAffected(type)){
		totalUpgrade.sum(upgradeType);
		recalculateStats();
	}
}

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

void Unit::incKills(){
	++kills;

	const Level *nextLevel= getNextLevel();
	if(nextLevel!=NULL && kills>= nextLevel->getKills()){
		level= nextLevel;
		totalUpgrade.sum(level);
		recalculateStats();
	}
}

bool Unit::morph(const MorphCommandType *mct){
	const UnitType *morphUnitType= mct->getMorphUnit();

	if(map->isFreeCellsOrHasUnit(pos, morphUnitType->getSize(), currField, this)){
		map->clearUnitCells(this, pos);
		faction->deApplyStaticCosts(type);
		type= morphUnitType;
		computeTotalUpgrade();
		map->putUnitCells(this, pos);
		faction->applyDiscount(morphUnitType, mct->getDiscount());
		return true;
	}
	else{
		return false;
	}
}

/**
 * Adds an effect to this unit.  Returns true if it had an immediate effect that
 * killed the unit.
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
 * Cancel an effect, usualy because the originator has died.  The caller is
 * expected to clean up the Effect object.
 */
void Unit::remove(Effect *e) {
	effects.remove(e);

	if(effects.isDirty()) {
		recalculateStats();
	}
}

/** Notifies unit that an effect they gave to somebody else expired. */
void Unit::effectExpired(Effect *e){
	e->clearSource();
	effectsCreated.remove(e);
	effects.clearRootRef(e);

	if(effects.isDirty()) {
		recalculateStats();
	}
}


// ==================== PRIVATE ====================

float Unit::computeHeight(const Vec2i &pos) const{
	float height= map->getCell(pos)->getHeight();

	if(currField==fAir){
		height+= World::airHeight;
	}

	return height;
}

void Unit::updateTarget(){
	Unit *target= targetRef.getUnit();
	if(target!=NULL){

		//update target pos
		targetPos= target->getCellPos();
		Vec2i relPos= targetPos - pos;
		Vec2f relPosf= Vec2f((float)relPos.x, (float)relPos.y);
		targetRotation= radToDeg(atan2(relPosf.x, relPosf.y));

		//update target vec
		targetVec= target->getCurrVector();
	}
}

inline void Unit::clearCommands() {
	while(!commands.empty()){
		undoCommand(commands.back());
		delete commands.back();
		commands.pop_back();
	}
}

CommandResult Unit::checkCommand(Command *command) const{

	if(command->getCommandType()->getClass() == ccSetMeetingPoint) {
		return crSuccess;
	}

	//if not operative or has not command type => fail
	if(!isOperative() || command->getUnit() == this || !getType()->hasCommandType(command->getCommandType())){
		return crFailUndefined;
	}

	//if pos is not inside the world (if comand has not a pos, pos is (0, 0) and is inside world
	if(command->getPos().x != -1 && !map->isInside(command->getPos())){
		return crFailUndefined;
	}

	//check produced
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		if(!faction->reqsOk(produced)){
			return crFailReqs;
		}
		if(!faction->checkCosts(produced)){
			return crFailRes;
		}
	}

	//if this is a pet, make sure we don't have more pets of this type than allowed.
	if (command->getCommandType()->getClass() == ccProduce) {
		ProduceCommandType *pct = (ProduceCommandType*)command->getCommandType();
		const ProduceSkillType *pst = pct->getProduceSkillType();
		const UnitType *ut = pct->getProducedUnit();
		if(pst->isPet()) {
			int beingProduced = 0;
			for(Commands::const_iterator i = commands.begin(); i != commands.end(); i++) {
				if((*i)->getCommandType()->getClass() == ccProduce) {
					const ProduceCommandType* pct2 = (const ProduceCommandType*)*i;
					if(pct->getProducedUnit() == ut) {
						++beingProduced;
					}
				}
			}

			if(beingProduced + getPetCount(ut) >= pst->getMaxPets()) {
				return crFailPetLimit;
			}
		}
	}

	//build command specific, check resources and requirements for building
	if(command->getCommandType()->getClass()==ccBuild){
		const UnitType *builtUnit= command->getUnitType();
		if(!faction->reqsOk(builtUnit)){
			return crFailReqs;
		}
		if(command->isReserveResources() && !faction->checkCosts(builtUnit)){
			return crFailRes;
		}
	}

	//upgrade command specific, check that upgrade is not upgraded
	else if(command->getCommandType()->getClass()==ccUpgrade){
		const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());
		if(faction->getUpgradeManager()->isUpgradingOrUpgraded(uct->getProducedUpgrade())){
			return crFailUndefined;
		}
	}

	return crSuccess;
}

void Unit::applyCommand(Command *command){

	//check produced
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		faction->applyCosts(produced);
	}

	//build command specific
	if(command->getCommandType()->getClass() == ccBuild && command->isReserveResources()){
		faction->applyCosts(command->getUnitType());
	}

	//upgrade command specific
	else if(command->getCommandType()->getClass()==ccUpgrade){
		const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());
		faction->startUpgrade(uct->getProducedUpgrade());
	}
}

CommandResult Unit::undoCommand(Command *command){

	//return cost
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		faction->deApplyCosts(produced);
	}

	//return building cost if not already building it or dead
	if(command->getCommandType()->getClass() == ccBuild && command->isReserveResources()){
		if(currSkill->getClass()!=scBuild && currSkill->getClass()!=scDie){
			faction->deApplyCosts(command->getUnitType());
		}
	}

	//upgrade command cancel from list
	if(command->getCommandType()->getClass() == ccUpgrade){
		const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());
		faction->cancelUpgrade(uct->getProducedUpgrade());
	}

	return crSuccess;
}

void Unit::recalculateStats() {
	int oldMaxHp = getMaxHp();
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

	// If this guy is dead, make sure they stay dead
	if(!alive) {
		hp = 0;
	}

	// if not available in subfaction, it has to decay
	if(!faction->isAvailable(type)) {
		int decAmount = (int)roundf(type->getMaxHp() / 200.f);
		hpRegeneration -= decAmount;

		// if still not degenerating give them one more dose
		if(hpRegeneration >= 0) {
			hpRegeneration -= decAmount;
		}
	}
}

int Unit::getSpeed(const SkillType *st) const {
	float speed = (float)st->getSpeed();
	switch(st->getClass()) {
		case scMove:
		case scGetUp:
			speed = speed * moveSpeedMult + moveSpeed;
			break;

		case scAttack:
			speed =  speed * attackSpeedMult + attackSpeed;
			break;

		case scProduce:
		case scUpgrade:
		case scMorph:
			speed =  speed * prodSpeedMult + prodSpeed;
			break;

		case scBuild:
		case scRepair:
			speed =  speed * repairSpeedMult + repairSpeed;
			break;

		case scHarvest:
			speed =  speed * harvestSpeedMult + harvestSpeed;
			break;

		case scBeBuilt:
		case scStop:
		case scDie:
		case scCastSpell:
		case scFallDown:
		case scWaitForServer:
		case scCount:
			break;
	}
	return (int)(speed > 0.0f ? speed : 0.0f);
}

inline int Unit::getPetCount(const UnitType *unitType) const {
	int count = 0;
	for(Pets::const_iterator i = pets.begin(); i != pets.end(); i++) {
		if(i->getUnit()->getType() == unitType) {
			++count;
		}
	}
	return count;
}

}}//end namespace

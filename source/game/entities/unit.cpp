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
#include "route_planner.h"

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

MEMORY_CHECK_IMPLEMENTATION(Unit)

// ============================ Constructor & destructor =============================

/** Construct Unit object */
Unit::Unit(CreateParams params)
		: id(-1)
		, hp(1)
		, ep(0)
		, loadCount(0)
		, deadCount(0)
		, lastAnimReset(0)
		, nextAnimReset(-1)
		, lastCommandUpdate(0)
		, nextCommandUpdate(-1)
		, systemStartFrame(-1)
		, soundStartFrame(-1)
		, progress2(0)
		, kills(0)
		, m_carrier(-1)
		, highlight(0.f)
		, targetRef(-1)
		, targetField(Field::LAND)
		, faceTarget(true)
		, useNearestOccupiedCell(true)
		, level(0)
		, pos(params.pos)
		, lastPos(params.pos)
		, nextPos(params.pos)
		, targetPos(0)
		, targetVec(0.0f)
		, meetingPos(0)
		, lastRotation(0.f)
		, targetRotation(0.f)
		, rotation(0.f)
		, m_facing(params.face)
		, type(params.type)
		, loadType(0)
		, currSkill(0)
		, toBeUndertaken(false)
		, carried(false)
		, m_cloaked(false)
		, m_cloaking(false)
		, m_deCloaking(false)
		, m_cloakAlpha(1.f)
		, fire(0)
		, faction(params.faction)
		, map(params.map)
		, commandCallback(0)
		, hp_below_trigger(0)
		, hp_above_trigger(0)
		, attacked_trigger(false) {
	Random random(id);
	currSkill = getType()->getFirstStOfClass(SkillClass::STOP);	//starting skill
	foreach_enum (AutoCmdFlag, f) {
		m_autoCmdEnable[f] = true;
	}

	ULC_UNIT_LOG( this, " constructed at pos" << pos );

	computeTotalUpgrade();
	hp = type->getMaxHp() / 20;

	setModelFacing(m_facing);
}

Unit::Unit(LoadParams params) //const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld)
		: targetRef(params.node->getOptionalIntValue("targetRef", -1))
		, effects(params.node->getChild("effects"))
		, effectsCreated(params.node->getChild("effectsCreated"))
        , carried(false) {
	const XmlNode *node = params.node;
	this->faction = params.faction;
	this->map = params.map;

	id = node->getChildIntValue("id");

	string s;
	//hp loaded after recalculateStats()
	ep = node->getChildIntValue("ep");
	loadCount = node->getChildIntValue("loadCount");
	deadCount = node->getChildIntValue("deadCount");
	kills = node->getChildIntValue("kills");
	type = faction->getType()->getUnitType(node->getChildStringValue("type"));

	s = node->getChildStringValue("loadType");
	loadType = s == "null_value" ? NULL : g_world.getTechTree()->getResourceType(s);

	lastRotation = node->getChildFloatValue("lastRotation");
	targetRotation = node->getChildFloatValue("targetRotation");
	rotation = node->getChildFloatValue("rotation");
	m_facing = enum_cast<CardinalDir>(node->getChildIntValue("facing"));

	progress2 = node->getChildIntValue("progress2");
	targetField = (Field)node->getChildIntValue("targetField");

	pos = node->getChildVec2iValue("pos");
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

	m_cloaked = node->getChildBoolValue("cloaked");
	m_cloaking = node->getChildBoolValue("cloaking");
	m_deCloaking = node->getChildBoolValue("de-cloaking");
	m_cloakAlpha = node->getChildFloatValue("cloak-alpha");

	if (m_cloaked && !type->getCloakType()) {
		throw runtime_error("Unit marked as cloak has no cloak type!");
	}
	if (m_cloaking && !m_cloaked) {
		throw runtime_error("Unit marked as cloaking is not cloaked!");
	}
	if (m_cloaking && m_deCloaking) {
		throw runtime_error("Unit marked as cloaking and de-cloaking!");
	}

	m_autoCmdEnable[AutoCmdFlag::REPAIR] = node->getChildBoolValue("auto-repair");
	m_autoCmdEnable[AutoCmdFlag::ATTACK] = node->getChildBoolValue("auto-attack");
	m_autoCmdEnable[AutoCmdFlag::FLEE] = node->getChildBoolValue("auto-flee");

	if (type->hasMeetingPoint()) {
		meetingPos = node->getChildVec2iValue("meeting-point");
	}

	XmlNode *n = node->getChild("commands");
	for(int i = 0; i < n->getChildCount(); ++i) {
		commands.push_back(g_world.newCommand(n->getChild("command", i), type, faction->getType()));
	}

	unitPath.read(node->getChild("unitPath"));
	waypointPath.read(node->getChild("waypointPath"));

	totalUpgrade.reset();
	computeTotalUpgrade();

	hp = node->getChildIntValue("hp");
	fire = NULL;

	n = node->getChild("units-carried");
	for(int i = 0; i < n->getChildCount(); ++i) {
		m_carriedUnits.push_back(n->getChildIntValue("unit", i));
	}
	n = node->getChild("units-to-carry");
	for(int i = 0; i < n->getChildCount(); ++i) {
		m_unitsToCarry.push_back(n->getChildIntValue("unit", i));
	}

	n = node->getChild("units-to-unload");
	for(int i = 0; i < n->getChildCount(); ++i) {
		m_unitsToUnload.push_back(n->getChildIntValue("unit", i));
	}
	m_carrier = node->getChildIntValue("unit-carrier");
	if (m_carrier != -1) {
		carried = true;
	}

	faction->add(this);
	if (hp) {
		if (!carried) {
			map->putUnitCells(this, pos);
			meetingPos = node->getChildVec2iValue("meetingPos"); // putUnitCells sets this, so we reset it here
		}
		ULC_UNIT_LOG( this, " constructed at pos" << pos );
	} else {
		ULC_UNIT_LOG( this, " constructed dead." );
	}
	if (type->hasSkillClass(SkillClass::BE_BUILT) && !type->hasSkillClass(SkillClass::MOVE)) {
		map->flatternTerrain(this);
		// was previously in World::initUnits but seems to work fine here
		g_cartographer.updateMapMetrics(getPos(), getSize());
	}
	if (node->getChildBoolValue("fire")) {
		decHp(0); // trigger logic to start fire system
	}
}

/** delete stuff */
Unit::~Unit() {
//	removeCommands();
	if (!g_program.isTerminating() && World::isConstructed()) {
		ULC_UNIT_LOG( this, " deleted." );
	}
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
	node->addChild("cloaked", m_cloaked);
	node->addChild("cloaking", m_cloaking);
	node->addChild("de-cloaking", m_deCloaking);
	node->addChild("cloak-alpha", m_cloakAlpha);

	node->addChild("auto-repair", m_autoCmdEnable[AutoCmdFlag::REPAIR]);
	node->addChild("auto-attack", m_autoCmdEnable[AutoCmdFlag::ATTACK]);
	node->addChild("auto-flee", m_autoCmdEnable[AutoCmdFlag::FLEE]);

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
	n = node->addChild("units-carried");
	foreach_const (UnitIdList, it, m_carriedUnits) {
		n->addChild("unit", *it);
	}
	n = node->addChild("units-to-carry");
	foreach_const (UnitIdList, it, m_unitsToCarry ) {
		n->addChild("unit", *it);
	}
	n = node->addChild("units-to-unload");
	foreach_const (UnitIdList, it, m_unitsToUnload) {
		n->addChild("unit", *it);
	}
	node->addChild("unit-carrier", m_carrier);
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
		CmdClass cmdClass = commands.front()->getType()->getClass();
		if (cmdClass == CmdClass::PRODUCE || cmdClass == CmdClass::MORPH
		|| cmdClass == CmdClass::GENERATE || cmdClass == CmdClass::UPGRADE) {
			const ProducibleType *produced = commands.front()->getProdType();
			if (produced) {
				return clamp(progress2 * 100 / produced->getProductionTime(), 0, 100);
			}
		}
		///@todo CommandRefactoring - hailstone 12Dec2010
		/*
		ProducerBaseCommandType *ct = commands.front()->getType();
		if (ct->getProducedCount()) {
			// prod count can be > 1, need command & progress2 (just pass 'this'?) -silnarm 12-Jun-2011
			ct->getProductionPercent(progress2);
		}
		*/
	}
	return -1;
}

/** query next available level @return next level, or NULL */
const Level *Unit::getNextLevel() const{
	if (!level && type->getLevelCount()) {
		return type->getLevel(0);
	} else {
		for(int i=1; i < type->getLevelCount(); ++i) {
			if (type->getLevel(i - 1) == level) {
				return type->getLevel(i);
			}
		}
	}
	return 0;
}

/** retrieve name description, levelName + unitTypeName */
string Unit::getFullName() const{
	string str;
	if (level) {
		string lvl;
		if (g_lang.lookUp(level->getName(), getFaction()->getType()->getName(), lvl)) {
			str += lvl;
		} else {
			str += formatString(level->getName());
		}
		str.push_back(' ');
	}
	string name;
	if (g_lang.lookUp(type->getName(), getFaction()->getType()->getName(), name)) {
		str += name;
	} else {
		str += formatString(type->getName());
	}
	return str;
}

float Unit::getRenderAlpha() const {
	float alpha = 1.0f;
	int framesUntilDead = GameConstants::maxDeadCount - getDeadCount();

	const SkillType *st = getCurrSkill();
	if (st->getClass() == SkillClass::DIE) {
		const DieSkillType *dst = (const DieSkillType*)st;
		if(dst->getFade()) {
			alpha = 1.0f - getAnimProgress();
		} else if (framesUntilDead <= 300) {
			alpha = (float)framesUntilDead / 300.f;
		}
	} else if (renderCloaked()) {
		alpha = getCloakAlpha();
	}
	return alpha;
}

// ====================================== is ======================================

/** query unit interestingness
  * @param iut the type of interestingness you're interested in
  * @return true if this unit is interesting in the way you're interested in
  */
bool Unit::isInteresting(InterestingUnitType iut) const{
	switch (iut) {
		case InterestingUnitType::IDLE_HARVESTER:
			if (type->hasCommandClass(CmdClass::HARVEST)) {
				if (!commands.empty()) {
					const CommandType *ct = commands.front()->getType();
					if (ct) {
						return ct->getClass() == CmdClass::STOP;
					}
				}
			}
			return false;

		case InterestingUnitType::BUILT_BUILDING:
			return isBuilt() &&
				(type->hasSkillClass(SkillClass::BE_BUILT) || type->hasSkillClass(SkillClass::BUILD_SELF));
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

bool Unit::isTargetUnitVisible(int teamIndex) const {
	return (getCurrSkill()->getClass() == SkillClass::ATTACK
		&& g_map.getTile(Map::toTileCoords(getTargetPos()))->isVisible(teamIndex));
}

bool Unit::isActive() const {
	return (getCurrSkill()->getClass() != SkillClass::DIE && !isCarried());
}

bool Unit::isBuilding() const {
	return ((getType()->hasSkillClass(SkillClass::BE_BUILT) || getType()->hasSkillClass(SkillClass::BUILD_SELF))
		&& isAlive() && !getType()->getProperty(Property::WALL));
}

/** find a repair command type that can repair a unit with
  * @param u the unit in need of repairing
  * @return a RepairCommandType that can repair u, or NULL
  */
const RepairCommandType * Unit::getRepairCommandType(const Unit *u) const {
	for (int i = 0; i < type->getCommandTypeCount<RepairCommandType>(); i++) {
		const RepairCommandType *rct = type->getCommandType<RepairCommandType>(i);
		const RepairSkillType *rst = rct->getRepairSkillType();
		if ((!rst->isSelfOnly() || this == u)
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
	if (isBeingBuilt() && currSkill->isStretchyAnim()) {
		return float(getProgress2()) / float(getMaxHp());
	}
	return float(g_world.getFrameCount() - lastAnimReset)
			/	float(nextAnimReset - lastAnimReset);
}

void Unit::startSkillParticleSystems() {
	Vec2i cPos = getCenteredPos();
	Tile *tile = g_map.getTile(Map::toTileCoords(cPos));
	bool visible = tile->isVisible(g_world.getThisTeamIndex()) && g_renderer.getCuller().isInside(cPos);
	
	for (unsigned i = 0; i < currSkill->getEyeCandySystemCount(); ++i) {
		UnitParticleSystem *ups = currSkill->getEyeCandySystem(i)->createUnitParticleSystem(visible);
		ups->setPos(getCurrVector());
		ups->setRotation(getRotation());
		skillParticleSystems.push_back(ups);
		Colour c = faction->getColour();
		Vec3f colour = Vec3f(c.r / 255.f, c.g / 255.f, c.b / 255.f);
		ups->setTeamColour(colour);
		g_renderer.manageParticleSystem(ups, ResourceScope::GAME);
	}
}

// ====================================== set ======================================

void Unit::setCommandCallback() {
	commandCallback = commands.front()->getId();
}

/** sets the current skill */
void Unit::setCurrSkill(const SkillType *newSkill) {
	assert(newSkill);
	//COMMAND_LOG(g_world.getFrameCount() << "::Unit:" << id << " skill set => " << SkillClassNames[currSkill->getClass()] );
	if (newSkill == currSkill) {
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

	if (!isCarried()) {
		startSkillParticleSystems();
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

Projectile* Unit::launchProjectile(ProjectileType *projType, const Vec3f &endPos) {
	Unit *carrier = isCarried() ? g_world.getUnit(getCarrier()) : 0;
	Vec2i effectivePos = (carrier ? carrier->getCenteredPos() : getCenteredPos());
	Vec3f startPos;
	if (carrier) {
		RUNTIME_CHECK(!carrier->isCarried() && carrier->getPos().x >= 0 && carrier->getPos().y >= 0);
		startPos = carrier->getCurrVectorFlat();
		const LoadCommandType *lct = 
			static_cast<const LoadCommandType *>(carrier->getType()->getFirstCtOfClass(CmdClass::LOAD));
		assert(lct->areProjectilesAllowed());
		Vec2f offsets = lct->getProjectileOffset();
		startPos.y += offsets.y;
		int seed = int(Chrono::getCurMicros());
		Random random(seed);
		float rad = degToRad(float(random.randRange(0, 359)));
		startPos.x += cosf(rad) * offsets.x;
		startPos.z += sinf(rad) * offsets.x;
	} else {
		startPos = getCurrVector();
	}
	//make particle system
	const Tile *sc = map->getTile(Map::toTileCoords(effectivePos));
	const Tile *tsc = map->getTile(Map::toTileCoords(getTargetPos()));
	bool visible = sc->isVisible(g_world.getThisTeamIndex()) || tsc->isVisible(g_world.getThisTeamIndex());
	visible = visible && g_renderer.getCuller().isInside(effectivePos);

	Projectile *projectile = projType->createProjectileParticleSystem(visible);

	switch (projType->getStart()) {
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

	g_simInterface.doUpdateProjectile(this, projectile, startPos, endPos);

	if(projType->isTracking() && targetRef != -1) {
		Unit *target = g_world.getUnit(targetRef);
		projectile->setTarget(target);
	}
	projectile->setTeamColour(faction->getColourV3f());
	g_renderer.manageParticleSystem(projectile, ResourceScope::GAME);
	return projectile;
}

Splash* Unit::createSplash(SplashType *splashType, const Vec3f &pos) {
	const Tile *tile = map->getTile(Map::toTileCoords(getTargetPos()));		
	bool visible = tile->isVisible(g_world.getThisTeamIndex())
				&& g_renderer.getCuller().isInside(getTargetPos());
	Splash *splash = splashType->createSplashParticleSystem(visible);
	splash->setPos(pos);
	splash->setTeamColour(faction->getColourV3f());
	g_renderer.manageParticleSystem(splash, ResourceScope::GAME);
	return splash;
}

void Unit::startSpellSystems(const CastSpellSkillType *sst) {
	RUNTIME_CHECK(getCurrCommand()->getType()->getClass() == CmdClass::CAST_SPELL);
	SpellAffect affect = static_cast<const CastSpellCommandType*>(getCurrCommand()->getType())->getSpellAffects();
	Projectile *projectile = 0;
	Splash *splash = 0;
	Vec3f endPos = getTargetVec();

	// projectile
	if (sst->getProjParticleType()) {
		projectile = launchProjectile(sst->getProjParticleType(), endPos);
		projectile->setCallback(new SpellDeliverer(this, targetRef));
	} else {
		Unit *target = 0;
		if (affect == SpellAffect::SELF) {
			target = this;
		} else if (affect == SpellAffect::TARGET) {
			target = g_world.getUnit(targetRef);
		}
		if (sst->getSplashRadius()) {
			g_world.applyEffects(this, sst->getEffectTypes(), target->getCenteredPos(), 
				target->getType()->getField(), sst->getSplashRadius());
		} else {
			g_world.applyEffects(this, sst->getEffectTypes(), target, 0);
		}
	}

	// splash
	if (sst->getSplashParticleType()) {
		splash = createSplash(sst->getSplashParticleType(), endPos);
		if (projectile) {
			projectile->link(splash);
		}
	}
}

void Unit::startAttackSystems(const AttackSkillType *ast) {
	Projectile *projectile = 0;
	Splash *splash = 0;
	Vec3f endPos = getTargetVec();

	// projectile
	if (ast->getProjParticleType()) {
		projectile = launchProjectile(ast->getProjParticleType(), endPos);
		if (ast->getProjParticleType()->isTracking() && targetRef != -1) {
			projectile->setCallback(new ParticleDamager(this, g_world.getUnit(targetRef)));
		} else {
			projectile->setCallback(new ParticleDamager(this, 0));
		}
	} else {
		g_world.hit(this);
	}

	// splash
	if (ast->getSplashParticleType()) {
		splash = createSplash(ast->getSplashParticleType(), endPos);
		if (projectile) {
			projectile->link(splash);
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

void Unit::clearPath() {
	CmdClass cc = g_simInterface.processingCommandClass();
	if (cc != CmdClass::NULL_COMMAND && cc != CmdClass::INVALID) {
		PF_UNIT_LOG( this, "path cleared." );
		PF_LOG( "Command class = " << CmdClassNames[cc] );
	} else {
		CMD_LOG( "path cleared." );
	}
	unitPath.clear();
	waypointPath.clear();
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
  * @param commandClass CmdClass of interest
  * @return the first executable CommandType matching commandClass, or NULL
  */
const CommandType *Unit::getFirstAvailableCt(CmdClass commandClass) const {
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

void Unit::setAutoCmdEnable(AutoCmdFlag f, bool v) {
	m_autoCmdEnable[f] = v;
	StateChanged(this);
}

/** give one command, queue or clear command queue and push back (depending on flags)
  * @param command the command to execute
  * @return a CmdResult describing success or failure
  */
CmdResult Unit::giveCommand(Command *command) {
	assert(command);
	const CommandType *ct = command->getType();
	CMD_UNIT_LOG( this, "giveCommand() " << *command );
	if (ct->getClass() == CmdClass::SET_MEETING_POINT) {
		if(command->isQueue() && !commands.empty()) {
			commands.push_back(command);
		} else {
			meetingPos = command->getPos();
			g_world.deleteCommand(command);
		}
		CMD_LOG( "Result = SUCCESS" );
		return CmdResult::SUCCESS;
	}

	if (ct->isQueuable() || command->isQueue()) { // user wants this queued...
		// cancel current command if it is not queuable or marked to be queued
		if(!commands.empty() && !commands.front()->getType()->isQueuable() && !command->isQueue()) {
			CMD_LOG( "incoming command wants queue, but current is not queable. Cancel current command" );
			cancelCurrCommand();
			clearPath();
		}
	} else {
		// empty command queue
		CMD_LOG( "incoming command is not marked to queue, Clear command queue" );

		// HACK... The AI likes to re-issue the same commands, which stresses the pathfinder 
		// on big maps. If current and incoming are both attack and have same pos, then do
		// not clear path... (route cache will still be good).
		if (! (!commands.empty() && command->getType()->getClass() == CmdClass::ATTACK
		&& commands.front()->getType()->getClass() == CmdClass::ATTACK
		&& command->getPos() == commands.front()->getPos()) ) {
			clearPath();
		}
		clearCommands();

		// for patrol commands, remember where we started from
		if(ct->getClass() == CmdClass::PATROL) {
			command->setPos2(pos);
		}
	}

	// check command
	CmdResult result = checkCommand(*command);
	bool energyRes = checkEnergy(command->getType());
	if (result == CmdResult::SUCCESS && energyRes) {
		applyCommand(*command);
		
		// start the command type
		ct->start(this, command);

		if (command) {
			commands.push_back(command);
		}
	} else {
		if (!energyRes && getFaction()->isThisFaction()) {
			g_console.addLine(g_lang.get("InsufficientEnergy"));
		}
		g_world.deleteCommand(command);
		command = 0;
	}

	StateChanged(this);

	if (command) {
		CMD_UNIT_LOG( this, "giveCommand() Result = " << CmdResultNames[result] );
	}
	return result;
}

void Unit::loadUnitInit(Command *command) {
	if (std::find(m_unitsToCarry.begin(), m_unitsToCarry.end(), command->getUnitRef()) == m_unitsToCarry.end()) {
		m_unitsToCarry.push_back(command->getUnitRef());
		CMD_LOG( "adding unit to load list " << *command->getUnit() )
		///@bug causes crash at Unit::tick when more than one unit attempts to load at the same time
		/// while doing multiple loads increases the queue count but it decreases afterwards.
		/// Furious clicking to make queued commands causes a crash in AnnotatedMap::annotateLocal.
		/// - hailstone 2Feb2011
		/*if (!commands.empty() && commands.front()->getType()->getClass() == CmdClass::LOAD) {
			CMD_LOG( "deleting load command, already loading.")
			g_world.deleteCommand(command);
			command = 0;
		}*/
	}
}

void Unit::unloadUnitInit(Command *command) {
	if (command->getUnit()) {
		if (std::find(m_unitsToUnload.begin(), m_unitsToUnload.end(), command->getUnitRef()) == m_unitsToUnload.end()) {
			assert(std::find(m_carriedUnits.begin(), m_carriedUnits.end(), command->getUnitRef()) != m_carriedUnits.end());
			m_unitsToUnload.push_back(command->getUnitRef());
			CMD_LOG( "adding unit to unload list " << *command->getUnit() )
			if (!commands.empty() && commands.front()->getType()->getClass() == CmdClass::UNLOAD) {
				CMD_LOG( "deleting unload command, already unloading.")
				g_world.deleteCommand(command);
				command = 0;
			}
		}
	} else {
		m_unitsToUnload.clear();
		m_unitsToUnload = m_carriedUnits;
	}
}

/** removes current command (and any queued Set meeting point commands)
  * @return the command now at the head of the queue (the new current command) */
Command *Unit::popCommand() {
	// pop front
	CMD_LOG( "popping current " << commands.front()->getType()->getName() << " command." );

	g_world.deleteCommand(commands.front());
	commands.erase(commands.begin());
	clearPath();

	Command *command = commands.empty() ? NULL : commands.front();

	// we don't let hacky set meeting point commands actually get anywhere
	while(command && command->getType()->getClass() == CmdClass::SET_MEETING_POINT) {
		setMeetingPos(command->getPos());
		g_world.deleteCommand(command);
		commands.erase(commands.begin());
		command = commands.empty() ? NULL : commands.front();
	}
	if (command) {
		CMD_LOG( "new current is " << command->getType()->getName() << " command." );
	} else {
		CMD_LOG( "now has no commands." );
	}
	StateChanged(this);
	return command;
}
/** pop current command (used when order is done)
  * @return CmdResult::SUCCESS, or CmdResult::FAIL_UNDEFINED on catastrophic failure
  */
CmdResult Unit::finishCommand() {
	//is empty?
	if(commands.empty()) {
		CMD_UNIT_LOG( this, "finishCommand() no command to finish!" );
		return CmdResult::FAIL_UNDEFINED;
	}
	CMD_UNIT_LOG( this, commands.front()->getType()->getName() << " command finished." );

	Command *command = popCommand();

	if (command) {
		command->getType()->finish(this, *command);
	}

	if (commands.empty()) {
		CMD_LOG( "now has no commands." );
	} else {
		CMD_LOG( commands.front()->getType()->getName() << " command next on queue." );
	}

	return CmdResult::SUCCESS;
}

/** cancel command on back of queue */
CmdResult Unit::cancelCommand() {
	// is empty?
	if(commands.empty()){
		CMD_UNIT_LOG( this, "cancelCommand() No commands to cancel!");
		return CmdResult::FAIL_UNDEFINED;
	}

	//undo command
	const CommandType *ct = commands.back()->getType();
	undoCommand(*commands.back());

	//delete ans pop command
	g_world.deleteCommand(commands.back());
	commands.pop_back();

	StateChanged(this);

	//clear routes
	clearPath();
	
	if (commands.empty()) {
		CMD_UNIT_LOG( this, "current " << ct->getName() << " command cancelled.");
	} else {
		CMD_UNIT_LOG( this, "a queued " << ct->getName() << " command cancelled.");
	}
	return CmdResult::SUCCESS;
}

/** cancel current command */
CmdResult Unit::cancelCurrCommand() {
	//is empty?
	if(commands.empty()) {
		CMD_UNIT_LOG( this, "cancelCurrCommand() No commands to cancel!");
		return CmdResult::FAIL_UNDEFINED;
	}

	//undo command
	undoCommand(*commands.front());

	Command *command = popCommand();

	if (!command) {
		CMD_UNIT_LOG( this, "now has no commands." );
	} else {
		CMD_UNIT_LOG( this, command->getType()->getName() << " command next on queue." );
	}
	return CmdResult::SUCCESS;
}

void Unit::removeCommands() {
	if (!g_program.isTerminating() && World::isConstructed()) {
		CMD_UNIT_LOG( this, "clearing all commands." );
	}
	while (!commands.empty()) {
		g_world.deleteCommand(commands.back());
		commands.pop_back();
	}
}

// =================== route stack ===================

/** Creates a unit, places it on the map and applies static costs for starting units
  * @param startingUnit true if this is a starting unit.
  */
void Unit::create(bool startingUnit) {
	ULC_UNIT_LOG( this, "created." );
	faction->add(this);
	lastPos = Vec2i(-1);
	map->putUnitCells(this, pos);
	if (startingUnit) {
		faction->applyStaticCosts(type);
	}
	nextCommandUpdate = -1;
	setCurrSkill(type->getStartSkill());
	startSkillParticleSystems();
}

/** Give a unit life. Called when a unit becomes 'operative'
  */
void Unit::born(bool reborn) {
	if (reborn && (!isAlive() || !isBuilt())) {
		return;
	}
	if (type->getCloakClass() == CloakClass::PERMANENT && faction->reqsOk(type->getCloakType())) {
		cloak();
	}
	if (type->isDetector()) {
		g_world.getCartographer()->detectorActivated(this);
	}
	ULC_UNIT_LOG( this, "born." );
	faction->applyStaticProduction(type);
	computeTotalUpgrade();
	recalculateStats();

	if (!reborn) {
		faction->addStore(type);
		setCurrSkill(SkillClass::STOP);
		hp = type->getMaxHp();
		faction->checkAdvanceSubfaction(type, true);
		g_world.getCartographer()->applyUnitVisibility(this);
		g_simInterface.doUnitBorn(this);
		faction->applyUpgradeBoosts(this);
		if (faction->isThisFaction() && !g_config.getGsAutoRepairEnabled()
		&& type->getFirstCtOfClass(CmdClass::REPAIR)) {
			CmdFlags cmdFlags = CmdFlags(CmdProps::MISC_ENABLE, false);
			Command *cmd = g_world.newCommand(CmdDirective::SET_AUTO_REPAIR, cmdFlags, invalidPos, this);
			g_simInterface.getCommander()->pushCommand(cmd);
		}
		if (!isCarried()) {
			startSkillParticleSystems();
		}
	}
	StateChanged(this);
	faction->onUnitActivated(type);
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
	ULC_UNIT_LOG( this, "killed." );
	hp = 0;
	World &world = g_world;

	if (!m_unitsToCarry.empty()) {
		foreach (UnitIdList, it, m_unitsToCarry) {
			Unit *unit = world.getUnit(*it);
			if (unit->anyCommand() && unit->getCurrCommand()->getType()->getClass() == CmdClass::BE_LOADED) {
				unit->cancelCurrCommand();
			}
		}
		m_unitsToCarry.clear();
	}

	if (!m_carriedUnits.empty()) {
		foreach (UnitIdList, it, m_carriedUnits) {
			Unit *unit = world.getUnit(*it);
			int hp = unit->getHp();
			unit->decHp(hp);
		}
		m_carriedUnits.clear();
	}
	if (isCarried()) {
		Unit *carrier = g_world.getUnit(getCarrier());
		carrier->housedUnitDied(this);
	}

	if (fire) {
		fire->fade();
		fire = 0;
	}

	//REFACTOR Use signal, send this code to Faction::onUnitDied();
	if (isBeingBuilt()) { // no longer needs static resources
		faction->deApplyStaticConsumption(type);
	} else {
		faction->deApplyStaticCosts(type);
		faction->removeStore(type);
		faction->onUnitDeActivated(type);
	}

	Died(this);

	clearCommands();
	setCurrSkill(SkillClass::DIE);
	deadCount = Random(id).randRange(-256, 256); // random decay time

	//REFACTOR use signal, send this to World/Cartographer/SimInterface
	world.getCartographer()->removeUnitVisibility(this);
	if (!isCarried()) { // if not in transport, clear cells
		map->clearUnitCells(this, pos);
	}
	g_simInterface.doUpdateAnimOnDeath(this);
	checkTargets(this); // hack... 'tracking' particle systems might reference this
	if (type->isDetector()) {
		world.getCartographer()->detectorDeactivated(this);
	}
}

void Unit::housedUnitDied(Unit *unit) {
	UnitIdList::iterator it;
	if (Shared::Util::find(m_carriedUnits, unit->getId(), it)) {
		m_carriedUnits.erase(it);
	}
}

void Unit::undertake() {
	faction->remove(this);
	if (!skillParticleSystems.empty()) {
		foreach (UnitParticleSystems, it, skillParticleSystems) {
			(*it)->fade();
		}
		skillParticleSystems.clear();
	}
}

void Unit::resetHighlight() {
	highlight= 1.f;
}

void Unit::cloak() {
	RUNTIME_CHECK(type->getCloakClass() != CloakClass::INVALID);
	if (m_cloaked) {
		return;
	}
	if (type->getCloakClass() == CloakClass::ENERGY) { // apply ep cost on start
		int cost = type->getCloakType()->getEnergyCost();
		if (!decEp(cost)) {
			return;
		}
	}
	m_cloaked = true;
	if (!m_cloaking) {
		// set flags so we know which way to fade the alpha later
		if (m_deCloaking) {
			m_deCloaking = false;
		}
		m_cloaking = true;
		// sound ?
		if (type->getCloakType()->getCloakSound() && g_world.getFrameCount() > 0 
		&& g_renderer.getCuller().isInside(getCenteredPos())) {
			g_soundRenderer.playFx(type->getCloakType()->getCloakSound());
		}
	}
}

void Unit::deCloak() {
	m_cloaked = false;
	if (!m_deCloaking) {
		if (m_cloaking) {
			m_cloaking = false;
		}
		m_deCloaking = true;
		if (type->getCloakType()->getDeCloakSound() && g_renderer.getCuller().isInside(getCenteredPos())) {
			g_soundRenderer.playFx(type->getCloakType()->getDeCloakSound());
		}
	}
}

/** Move a unit to a position on the map using the RoutePlanner
  * @param pos destination position
  * @param moveSkill the MoveSkillType to apply for the move
  * @return true when completed (maxed out BLOCKED, IMPOSSIBLE or ARRIVED)
  */
TravelState Unit::travel(const Vec2i &pos, const MoveSkillType *moveSkill) {
	if (!g_world.getMap()->isInside(pos)) {
		DEBUG_HOOK();
	}
	RUNTIME_CHECK(g_world.getMap()->isInside(pos));
	assert(moveSkill);

	switch (g_routePlanner.findPath(this, pos)) { // head to target pos
		case TravelState::MOVING:
			setCurrSkill(moveSkill);
			face(getNextPos());
			//MOVE_LOG( g_world.getFrameCount() << "::Unit:" << unit->getId() << " updating move " 
			//	<< "Unit is at " << unit->getPos() << " now moving into " << unit->getNextPos() );
			return TravelState::MOVING;

		case TravelState::BLOCKED:
			setCurrSkill(SkillClass::STOP);
			if (getPath()->isBlocked()) { //&& !command->getUnit()) {?? from MoveCommandType and LoadCommandType
				clearPath();
				return TravelState::BLOCKED;
			}
			return TravelState::BLOCKED;

		case TravelState::IMPOSSIBLE:
			setCurrSkill(SkillClass::STOP);
			cancelCurrCommand(); // from AttackCommandType, is this right, maybe dependant flag?? - hailstone 21Dec2010
 			return TravelState::IMPOSSIBLE;

		case TravelState::ARRIVED:
			return TravelState::ARRIVED;

		default:
			throw runtime_error("Unknown TravelState returned by RoutePlanner::findPath().");
	}
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
		} else if (targetUnit->getFactionIndex() == getFactionIndex()) {
			const UnitType *tType = targetUnit->getType();
			if (tType->isOfClass(UnitClass::CARRIER)
			&& tType->getCommandType<LoadCommandType>(0)->canCarry(type)) {
				//move to be loaded
				commandType = type->getFirstCtOfClass(CmdClass::BE_LOADED);
			} else if (getType()->isOfClass(UnitClass::CARRIER)
			&& type->getCommandType<LoadCommandType>(0)->canCarry(tType)) {
			//load
			commandType = type->getFirstCtOfClass(CmdClass::LOAD);
		} else {
				// repair
			commandType = getRepairCommandType(targetUnit);
		}
		} else { // repair allies
			commandType = getRepairCommandType(targetUnit);
		}
	} else {
		//check harvest command
		MapResource *resource = sc->getResource();
		if (resource != NULL) {
			commandType = type->getHarvestCommand(resource->getType());
		}
	}

	//default command is move command
	if (!commandType) {
		commandType = type->getFirstCtOfClass(CmdClass::MOVE);
	}

	if (!commandType && type->hasMeetingPoint()) {
		commandType = type->getFirstCtOfClass(CmdClass::SET_MEETING_POINT);
	}

	return commandType;
}

const Model* Unit::getCurrentModel() const {
	if (type->getField() == Field::AIR){
		return getCurrSkill()->getAnimation();
	}
	if (getCurrSkill()->getClass() == SkillClass::MOVE) {
		SurfaceType from_st = map->getCell(getLastPos())->getType();
		SurfaceType to_st = map->getCell(getNextPos())->getType();
		return getCurrSkill()->getAnimation(from_st, to_st);
	} else {
		SurfaceType st = map->getCell(getPos())->getType();
		return getCurrSkill()->getAnimation(st);
	}
}

/** wrapper for SimulationInterface */
void Unit::updateSkillCycle(const SkillCycleTable *skillCycleTable) {
	if (getCurrSkill()->getClass() == SkillClass::MOVE) {
		if (getPos() == getNextPos()) {
			throw runtime_error("Move Skill set, but pos == nextPos");
		}
		updateMoveSkillCycle();
	} else {
		updateSkillCycle(skillCycleTable->lookUp(this).getSkillFrames());
	}
}

/** wrapper for SimulationInterface */
void Unit::doUpdateAnimOnDeath(const SkillCycleTable *skillCycleTable) {
	assert(getCurrSkill()->getClass() == SkillClass::DIE);
	const CycleInfo &inf = skillCycleTable->lookUp(this);
	updateAnimCycle(inf.getAnimFrames(), inf.getSoundOffset(), inf.getAttackOffset());
}

/** wrapper for SimulationInterface */
void Unit::doUpdateAnim(const SkillCycleTable *skillCycleTable) {
	if (getCurrSkill()->getClass() == SkillClass::DIE) {
		updateAnimDead();
	} else {
		const CycleInfo &inf = skillCycleTable->lookUp(this);
		updateAnimCycle(inf.getAnimFrames(), inf.getSoundOffset(), inf.getAttackOffset());
	}
}

/** wrapper for SimulationInterface */
void Unit::doUnitBorn(const SkillCycleTable *skillCycleTable) {
	const CycleInfo &inf = skillCycleTable->lookUp(this);
	updateSkillCycle(inf.getSkillFrames());
	updateAnimCycle(inf.getAnimFrames());
}

/** wrapper for sim interface */
void Unit::doUpdateCommand() {
	const SkillType *old_st = getCurrSkill();

	// if unit has command process it
	if (anyCommand()) {
		// check if a command being 'watched' has finished
		if (getCommandCallback() != getCurrCommand()->getId()) {
			int last = getCommandCallback();
			ScriptManager::commandCallback(this);
			// if the callback set a new callback we don't want to clear it
			// only clear if the callback id's are the same
			if (last == getCommandCallback()) {
				clearCommandCallback();
			}
		}
		g_simInterface.setProcessingCommandClass(getCurrCommand()->getType()->getClass());
		getCurrCommand()->getType()->update(this);
		g_simInterface.setProcessingCommandClass();
	}
	// if no commands, add stop (or guard for pets) command
	if (!anyCommand() && isOperative()) {
		const UnitType *ut = getType();
		setCurrSkill(SkillClass::STOP);
		if (ut->hasCommandClass(CmdClass::STOP)) {
			giveCommand(g_world.newCommand(ut->getFirstCtOfClass(CmdClass::STOP), CmdFlags()));
		}
	}
	//if unit is out of EP, it stops
	if (computeEp()) {
		if (getCurrCommand()) {
			cancelCurrCommand();
			if (getFaction()->isThisFaction()) {
				g_console.addLine(g_lang.get("InsufficientEnergy"));
			}
		}
		setCurrSkill(SkillClass::STOP);
	}
	g_simInterface.updateSkillCycle(this);

	if (getCurrSkill() != old_st) {	// if starting new skill
		//resetAnim(g_world.getFrameCount() + 1); // reset animation cycle for next frame
		g_simInterface.doUpdateAnim(this);
	}
}

/** called to update animation cycle on a dead unit */
void Unit::updateAnimDead() {
	assert(currSkill->getClass() == SkillClass::DIE);

	// when dead and have already played one complete anim cycle, set startFrame to last frame, endFrame 
	// to this frame to keep the cycle at the 'end' so getAnimProgress() always returns 1.f
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
	if (frameOffset == -1) { // hacky handle move skill
		assert(currSkill->getClass() == SkillClass::MOVE);
		static const float speedModifier = 1.f / GameConstants::speedDivider / float(WORLD_FPS);
		float animSpeed = currSkill->getAnimSpeed() * speedModifier;
		//if moving to a higher cell move slower else move faster
		float heightDiff = map->getCell(lastPos)->getHeight() - map->getCell(pos)->getHeight();
		float heightFactor = clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
		animSpeed *= heightFactor;

		// calculate anim cycle length
		frameOffset = int(1.0000001f / animSpeed);

		if (currSkill->hasSounds()) {
			soundOffset = int(currSkill->getSoundStartTime() / animSpeed);
			if (soundOffset < 1) ++soundOffset;
			assert(soundOffset > 0);
		}
	}
	// modify offsets for attack skills
	if (currSkill->getClass() == SkillClass::ATTACK) {
		fixed ratio = currSkill->getBaseSpeed() / fixed(getSpeed());
		frameOffset = (frameOffset * ratio).round();
		if (soundOffset > 0) {
			soundOffset = (soundOffset * ratio).round();
		}
		if (attackOffset > 0) {
			attackOffset = (attackOffset * ratio).round();
		}
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
		this->systemStartFrame = frame + attackOffset;
	} else {
		this->systemStartFrame = -1;
	}

}

/** called after a command is updated and a skill is selected
  * @param frameOffset the number of frames the next skill cycle will take */
void Unit::updateSkillCycle(int frameOffset) {
	// assert server doesn't use this for move...
	assert(currSkill->getClass() != SkillClass::MOVE || g_simInterface.asClientInterface());

	// modify offset for upgrades/effects/etc
	if (currSkill->getClass() != SkillClass::MOVE) {
		fixed ratio = getBaseSpeed() / fixed(getSpeed());
		frameOffset = (frameOffset * ratio).round();
	}
	// else move skill, server has already modified speed for us
	lastCommandUpdate = g_world.getFrameCount();
	nextCommandUpdate = g_world.getFrameCount() + clamp(frameOffset, 1, 4095);

}

/** called by the server only, updates a skill cycle for the move skill */
void Unit::updateMoveSkillCycle() {
	assert(!g_simInterface.asClientInterface());
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
	int frameOffset = clamp(int(1.0000001f / progressSpeed) + 1, 1, 4095);
	nextCommandUpdate = g_world.getFrameCount() + frameOffset; 
}

/** wrapper for World::updateUnits */
void Unit::doUpdate() {
	if (update()) {
		g_simInterface.doUpdateUnitCommand(this);

		if (getType()->getCloakClass() != CloakClass::INVALID) {
			if (isCloaked()) {
				if (getCurrSkill()->causesDeCloak()) {
					deCloak();
				}
			} else {
				if (getType()->getCloakClass() == CloakClass::PERMANENT
				&& !getCurrSkill()->causesDeCloak() && faction->reqsOk(type->getCloakType())) {
					cloak();
				}
			}
		}

		if (getCurrSkill()->getClass() == SkillClass::MOVE) {
			// move unit in cells
			g_world.moveUnitCells(this);
		}
	}
	// unit death
	if (isDead() && getCurrSkill()->getClass() != SkillClass::DIE) {
		kill();
	}
}

/** wrapper for World, from the point of view of the killer unit*/
void Unit::doKill(Unit *killed) {
	///@todo ?? 
	//if (killed->getCurrSkill()->getClass() == SkillClass::DIE) {
	//	return;
	//}

	ScriptManager::onUnitDied(killed);
	g_simInterface.getStats()->kill(getFactionIndex(), killed->getFactionIndex());
	if (isAlive() && getTeam() != killed->getTeam()) {
		incKills();
	}

	///@todo after stats inc ?? 
	if (killed->getCurrSkill()->getClass() != SkillClass::DIE) {
		killed->kill();
	}

	if (!killed->isMobile()) {
		// obstacle removed
		g_cartographer.updateMapMetrics(killed->getPos(), killed->getSize());
	}
}

/** @return true when the current skill has completed a cycle */
bool Unit::update() { ///@todo should this be renamed to hasFinishedCycle()?
//	_PROFILE_FUNCTION();
	const int &frame = g_world.getFrameCount();

	// start skill sound ?
	if (currSkill->getSound() && frame == getSoundStartFrame()) {
		Unit *carrier = (m_carrier != -1 ? g_world.getUnit(m_carrier) : 0);
		Vec2i cellPos = carrier ? carrier->getCenteredPos() : getCenteredPos();
		Vec3f vec = carrier ? carrier->getCurrVector() : getCurrVector();
		if (map->getTile(Map::toTileCoords(cellPos))->isVisible(g_world.getThisTeamIndex())) {
			g_soundRenderer.playFx(currSkill->getSound(), vec, g_gameState.getGameCamera()->getPos());
		}
	}

	// start attack/spell systems ?
	if (frame == getSystemStartFrame()) {
		if (currSkill->getClass() == SkillClass::ATTACK) {
			startAttackSystems(static_cast<const AttackSkillType*>(currSkill));
		} else if (currSkill->getClass() == SkillClass::CAST_SPELL) {
			startSpellSystems(static_cast<const CastSpellSkillType*>(currSkill));
		} else {
			assert(false);
		}
	}

	// update anim cycle ?
	if (frame >= getNextAnimReset()) {
		// new anim cycle (or reset)
		g_simInterface.doUpdateAnim(this);
	}

	// update emanations every 8 frames
	if (this->getEmanations().size() && !((frame + id) % 8) && isOperative()) {
		updateEmanations();
	}

	// fade highlight
	if (highlight > 0.f) {
		highlight -= 1.f / (GameConstants::highlightTime * WORLD_FPS);
	}

	// update cloak alpha
	if (m_cloaking) {
		m_cloakAlpha -= 0.05f;
		if (m_cloakAlpha <= 0.3) {
			assert(m_cloaked);
			m_cloaking = false;
		}
	} else if (m_deCloaking) {
		m_cloakAlpha += 0.05f;
		if (m_cloakAlpha >= 1.f) {
			assert(!m_cloaked);
			m_deCloaking = false;
		}
	}

	// update target
	updateTarget();
	
	// rotation
	bool moved = currSkill->getClass() == SkillClass::MOVE;
	bool rotated = false;
	if (currSkill->getClass() != SkillClass::STOP) {
		const float rotFactor = 2.f;
		if (getProgress() < 1.f / rotFactor) {
			if (type->getFirstStOfClass(SkillClass::MOVE) || type->getFirstStOfClass(SkillClass::MORPH)) {
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

	if (!carried) {
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

void Unit::checkEffectCloak() {
	if (m_cloaked) {
		foreach (Effects, ei, effects) {
			if ((*ei)->getType()->isCauseCloak() && (*ei)->getType()->isEffectsAlly()) {
				return;
			}
		}
		deCloak();
	}
}

/**
 * Update the unit by one tick.
 * @returns if the unit died during this call, the killer is returned, NULL otherwise.
 */
Unit* Unit::tick() {
	Unit *killer = NULL;
	
	// tick command types
	for (Commands::iterator i = commands.begin(); i != commands.end(); ++i) {
		(*i)->getType()->tick(this, (**i));
	}
	if (isAlive()) {
		if (doRegen(getHpRegeneration(), getEpRegeneration())) {
			if (!(killer = effects.getKiller())) {
				// if no killer, then this had to have been natural degeneration
				killer = this;
			}
		}

		// apply cloak cost
		if (m_cloaked && type->getCloakClass() == CloakClass::ENERGY) {
			int cost = type->getCloakType()->getEnergyCost();
			if (!decEp(cost)) {
				deCloak();
			}
		}
	}

	effects.tick();
	if (effects.isDirty()) {
		recalculateStats();
		checkEffectParticles();
		if (type->getCloakClass() == CloakClass::EFFECT) {
			checkEffectCloak();
		}
	}

	return killer;
}

/** Evaluate current skills energy requirements, subtract from current energy
  *	@return false if the skill can commence, true if energy requirements are not met
  */
bool Unit::computeEp() {

	// if not enough ep
	int cost = currSkill->getEpCost();
	if (cost == 0) {
		return false;
	}
	if (decEp(cost)) {
		if (ep > getMaxEp()) {
			ep = getMaxEp();
		}
		return false;
	}
	return true;
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
	if (!isBuilt() && getCurrSkill()->isStretchyAnim()) {
		if (hp > getProgress2()) {
			setProgress2(hp);
		}
	}
	//stop fire
	if (hp > type->getMaxHp() / 2 && fire != NULL) {
		fire->fade();
		fire = NULL;
	}
	return false;
}

/** Decrements EP by the specified amount
  * @return true if there was sufficient ep, false otherwise */
bool Unit::decEp(int i) {
	if (ep >= i) {
		ep -= i;
		return true;
	} else {
		return false;
	}
}

/** Decrements HP by the specified amount
  * @param i amount of HP to remove
  * @return true if unit is now dead
  */
bool Unit::decHp(int i) {
	assert(i >= 0);
	// we shouldn't ever go negative
	assert(hp > 0 || i == 0);
	hp -= i;
	if (hp_below_trigger && hp < hp_below_trigger) {
		hp_below_trigger = 0;
		ScriptManager::onHPBelowTrigger(this);
	}

	// fire
	if (type->getProperty(Property::BURNABLE) && hp < type->getMaxHp() / 2
	&& fire == NULL && m_carrier == -1) {
		FireParticleSystem *fps;
		Vec2i cPos = getCenteredPos();
		Tile *tile = g_map.getTile(Map::toTileCoords(cPos));
		bool vis = tile->isVisible(g_world.getThisTeamIndex()) && g_renderer.getCuller().isInside(cPos);
		fps = new FireParticleSystem(vis, 200);
		fps->setSpeed(2.5f / g_config.getGsWorldUpdateFps());
		fps->setPos(getCurrVector());
		fps->setRadius(type->getSize() / 3.f);
		fps->setTexture(g_coreData.getFireTexture());
		fps->setSize(type->getSize() / 3.f);
		fire = fps;
		g_renderer.manageParticleSystem(fps, ResourceScope::GAME);
	}

	// stop fire on death
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

string Unit::getShortDesc() const {
	stringstream ss;
	ss << g_lang.get("Hp") << ": " << hp << "/" << getMaxHp();
	if (getHpRegeneration()) {
		ss << " (" << g_lang.get("Regeneration") << ": " << getHpRegeneration() << ")";
	}
	if (getMaxEp()) {
		ss << endl << g_lang.get("Ep") << ": " << ep << "/" << getMaxEp();
		if (getEpRegeneration()) {
			ss << " (" << g_lang.get("Regeneration") << ": " << getEpRegeneration() << ")";
		}
	}
	if (!commands.empty()) { // Show current command being executed
		string factionName = faction->getType()->getName();
		string commandName = commands.front()->getType()->getName();
		string nameString = g_lang.getFactionString(factionName, commandName);
		if (nameString == commandName) {
			nameString = formatString(commandName);
			string classString = formatString(CmdClassNames[commands.front()->getType()->getClass()]);
			if (nameString == classString) {
				nameString = g_lang.get(classString);
			}
		}
		ss << endl << nameString;
	}
	return ss.str();
}

string Unit::getLongDesc() const {
	Lang &lang = g_lang;
	World &world = g_world;
	string shortDesc = getShortDesc();
	stringstream ss;

	const string factionName = type->getFactionType()->getName();
	int armorBonus = getArmor() - type->getArmor();
	int sightBonus = getSight() - type->getSight();

	// armor
	ss << endl << lang.get("Armor") << ": " << type->getArmor();
	if (armorBonus) {
		ss << (armorBonus > 0 ? " +" : " ") << armorBonus;
	}
	string armourName = lang.getTechString(type->getArmourType()->getName());
	if (armourName == type->getArmourType()->getName()) {
		armourName = formatString(armourName);
	}
	ss << " (" << armourName << ")";

	// sight
	ss << endl << lang.get("Sight") << ": " << type->getSight();
	if (sightBonus) {
		ss << (sightBonus > 0 ? " +" : " ") << sightBonus;
	}
	// cloaked ?
	if (isCloaked()) {
		string gRes, res;
		string group = world.getCloakGroupName(type->getCloakType()->getCloakGroup());
		if (!lang.lookUp(group, factionName, gRes)) {
			gRes = formatString(group);
		}
		lang.lookUp("Cloak", factionName, gRes, res);
		ss << endl << res;
	}
	// detector ?
	if (type->isDetector()) {
		string gRes, res;
		const DetectorType *dt = type->getDetectorType();
		if (dt->getGroupCount() == 1) {
			int id = dt->getGroup(0);
			string group = world.getCloakGroupName(id);
			if (!lang.lookUp(group, factionName, gRes)) {
				gRes = formatString(group);
			}
			lang.lookUp("SingleDetector", factionName, gRes, res);
		} else {
			vector<string> list;
			for (int i=0; i < dt->getGroupCount(); ++i) {
				string gName = world.getCloakGroupName(dt->getGroup(i));
				if (!lang.lookUp(gName, factionName, gRes)) {
					gRes = formatString(gName);
				}
				list.push_back(gRes);
			}
			lang.lookUp("MultiDetector", factionName, list, res);
		}		
		ss << endl << res;
	}

	// kills
	const Level *nextLevel = getNextLevel();
	if (kills > 0 || nextLevel) {
		ss << endl << lang.get("Kills") << ": " << kills;
		if (nextLevel) {
			string levelName = lang.getFactionString(getFaction()->getType()->getName(), nextLevel->getName());
			if (levelName == nextLevel->getName()) {
				levelName = formatString(levelName);
			}
			ss << " (" << levelName << ": " << nextLevel->getKills() << ")";
		}
	}

	// resource load
	if (loadCount) {
		string resName = lang.getTechString(loadType->getName());
		if (resName == loadType->getName()) {
			resName = formatString(resName);
		}
		ss << endl << lang.get("Load") << ": " << loadCount << "  " << resName;
	}

	// consumable production
	for (int i = 0; i < type->getCostCount(); ++i) {
		const ResourceAmount r = getType()->getCost(i, getFaction());
		if (r.getType()->getClass() == ResourceClass::CONSUMABLE) {
			string resName = lang.getTechString(r.getType()->getName());
			if (resName == r.getType()->getName()) {
				resName = formatString(resName);
			}
			ss << endl << (r.getAmount() < 0 ? lang.get("Produce") : lang.get("Consume"))
				<< ": " << abs(r.getAmount()) << " " << resName;
		}
	}
	// can store
	if (type->getStoredResourceCount() > 0) {
		for (int i = 0; i < type->getStoredResourceCount(); ++i) {
			ResourceAmount r = type->getStoredResource(i, getFaction());
			string resName = lang.getTechString(r.getType()->getName());
			if (resName == r.getType()->getName()) {
				resName = formatString(resName);
			}
			ss << endl << lang.get("Store") << ": ";
			ss << r.getAmount() << " " << resName;
		}
	}
	// effects
	effects.streamDesc(ss);

	return (shortDesc + ss.str());
}

/** Apply effects of an UpgradeType
  * @param upgradeType the type describing the Upgrade to apply*/
void Unit::applyUpgrade(const UpgradeType *upgradeType) {
	const EnhancementType *et = upgradeType->getEnhancement(type);
	if (et) {
		totalUpgrade.sum(et);
		recalculateStats();
		doRegen(et->getHpBoost(), et->getEpBoost());
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
	int oldSight = getSight();

	EnhancementType::reset();
	UnitStats::setValues(*type);

	// add up all multipliers first and then apply (multiply) once.
	// See EnhancementType::addMultipliers() for the 'adding' strategy
	EnhancementType::addMultipliers(totalUpgrade);
	for (Effects::const_iterator i = effects.begin(); i != effects.end(); ++i) {
		EnhancementType::addMultipliers(*(*i)->getType(), (*i)->getStrength());
	}
	EnhancementType::clampMultipliers();
	UnitStats::applyMultipliers(*this);
	UnitStats::addStatic(totalUpgrade);
	for (Effects::const_iterator i = effects.begin(); i != effects.end(); ++i) {
		UnitStats::addStatic(*(*i)->getType(), (*i)->getStrength());

		// take care of effect damage type
		hpRegeneration += (*i)->getActualHpRegen() - (*i)->getType()->getHpRegeneration();
	}

	effects.clearDirty();

	// correct negatives
	sanitiseUnitStats();

	// adjust hp (if maxHp was modified)
	if (getMaxHp() > oldMaxHp) {
		hp += getMaxHp() - oldMaxHp;
	} else if (hp > getMaxHp()) {
		hp = getMaxHp();
	}

	if (oldSight != getSight() && type->getDetectorType()) {
		g_cartographer.detectorSightModified(this, oldSight);
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
	//if (!isAlive() && !e->getType()->isEffectsNonLiving()) {
	//	g_world.getEffectFactory().deleteInstance(e);
	//	return false;
	//}
	if (!e->getType()->getAffectTag().empty()) {
		if (!type->hasTag(e->getType()->getAffectTag())) {
			g_world.getEffectFactory().deleteInstance(e);
			return false;
		}
	}

	if (e->getType()->isTickImmediately()) {
		if (doRegen(e->getType()->getHpRegeneration(), e->getType()->getEpRegeneration())) {
			g_world.getEffectFactory().deleteInstance(e);
			return true;
		}
		if (e->tick()) {
			// single tick, immediate effect
			g_world.getEffectFactory().deleteInstance(e);
			return false;
		}
	}
	if (type->getCloakClass() == CloakClass::EFFECT && e->getType()->isCauseCloak() 
	&& e->getType()->isEffectsAlly() && faction->reqsOk(type->getCloakType())) {
		cloak();
	}

	const UnitParticleSystemTypes &particleTypes = e->getType()->getParticleTypes();

	bool startParticles = true;
	if (effects.add(e)) {
		if (isCarried()) {
			startParticles = false;
		} else {
			foreach (Effects, it, effects) {
				if (e->getType() == (*it)->getType()) {
					startParticles = false;
					break;
				}
			}
		}
	} else {
		startParticles = false; // extended or rejected, already showing correct systems
	}

	if (startParticles && !particleTypes.empty()) {
		Vec2i cPos = getCenteredPos();
		Tile *tile = g_map.getTile(Map::toTileCoords(cPos));
		bool visible = tile->isVisible(g_world.getThisTeamIndex()) && g_renderer.getCuller().isInside(cPos);

		foreach_const (UnitParticleSystemTypes, it, particleTypes) {
			UnitParticleSystem *ups = (*it)->createUnitParticleSystem(visible);
			ups->setPos(getCurrVector());
			ups->setRotation(getRotation());
			//ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
			effectParticleSystems.push_back(ups);
			Colour c = faction->getColour();
			Vec3f colour = Vec3f(c.r / 255.f, c.g / 255.f, c.b / 255.f);
			ups->setTeamColour(colour);
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
bool Unit::morph(const MorphCommandType *mct, const UnitType *ut, Vec2i offset, bool reprocessCommands) {
	Field newField = ut->getField();
	CloakClass oldCloakClass = type->getCloakClass();
	if (map->areFreeCellsOrHasUnit(pos + offset, ut->getSize(), newField, this)) {
		const UnitType *oldType = type;
		map->clearUnitCells(this, pos);
		faction->deApplyStaticCosts(type);
		type = ut;
		pos += offset;
		computeTotalUpgrade();
		map->putUnitCells(this, pos);
		faction->giveRefund(ut, mct->getRefund());
		faction->applyStaticProduction(ut);

		// make sure the new UnitType has a CloakType before attempting to use it
		if (type->getCloakType()) {
			if (m_cloaked && oldCloakClass != ut->getCloakClass()) {
				deCloak();
			}
			if (ut->getCloakClass() == CloakClass::PERMANENT && faction->reqsOk(type->getCloakType())) {
				cloak();
			}
		} else {
			m_cloaked = false;
		}

		if (reprocessCommands) {
			// reprocess commands
			Commands newCommands;
			Commands::const_iterator i;

			// add current command, which should be the morph command
			assert(commands.size() > 0 
				&& (commands.front()->getType()->getClass() == CmdClass::MORPH
				|| commands.front()->getType()->getClass() == CmdClass::TRANSFORM));
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
		}
		StateChanged(this);
		faction->onUnitMorphed(type, oldType);
		return true;
	} else {
		return false;
	}
}

bool Unit::transform(const TransformCommandType *tct, const UnitType *ut, Vec2i pos, CardinalDir facing) {
	RUNTIME_CHECK(ut->getFirstCtOfClass(CmdClass::BUILD_SELF) != 0);
	Vec2i offset = pos - this->pos;
	m_facing = facing; // needs to be set for putUnitCells() [happens in morph()]
	const UnitType *oldType = type;
	int oldHp = getHp();
	if (morph(tct, ut, offset, false)) {
		rotation = facing * 90.f;
		HpPolicy policy = tct->getHpPolicy();
		if (policy == HpPolicy::SET_TO_ONE) {
			hp = 1;
		} else if (policy == HpPolicy::RESET) {
			hp = type->getMaxHp() / 20;
		} else {
			if (oldHp < getMaxHp()) {
				hp = oldHp;
			} else {
				hp = getMaxHp();
			}
		}
		commands.clear();
		giveCommand(g_world.newCommand(type->getFirstCtOfClass(CmdClass::BUILD_SELF), CmdFlags()));
		setCurrSkill(SkillClass::BUILD_SELF);
		return true;
	}
	return false;
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
		target = g_world.getUnit(targetRef);
	}

	if (target) {
		if (target->isCarried()) {
			target = g_world.getUnit(target->getCarrier());
		}
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
		g_world.deleteCommand(commands.back());
		commands.pop_back();
	}
}												

/** Check if a command can be executed
  * @param command the command to check
  * @return a CmdResult describing success or failure
  */
CmdResult Unit::checkCommand(const Command &command) const {
	const CommandType *ct = command.getType();

	if (command.getArchetype() != CmdDirective::GIVE_COMMAND) {
		return CmdResult::SUCCESS;
	}

	//if not operative or has not command type => fail
	if (!isOperative() || command.getUnit() == this || !getType()->hasCommandType(ct)) {
		return CmdResult::FAIL_UNDEFINED;
	}

	//if pos is not inside the world
	if (command.getPos() != Command::invalidPos && !map->isInside(command.getPos())) {
		return CmdResult::FAIL_UNDEFINED;
	}

	//check produced
	const ProducibleType *produced = command.getProdType();
	if (produced) {
		if (!faction->reqsOk(produced)) {
			return CmdResult::FAIL_REQUIREMENTS;
		}
		if (!command.getFlags().get(CmdProps::DONT_RESERVE_RESOURCES)
		&& !faction->checkCosts(produced)) {
			return CmdResult::FAIL_RESOURCES;
		}
	}

	if (ct->getClass() == CmdClass::CAST_SPELL) {
		const CastSpellCommandType *csct = static_cast<const CastSpellCommandType*>(ct);
		if (csct->getSpellAffects() == SpellAffect::TARGET && !command.getUnit()) {
			return CmdResult::FAIL_UNDEFINED;
		}
	}

	return ct->check(this, command);
}

/** Apply costs for a command.
  * @param command the command to apply costs for
  */
void Unit::applyCommand(const Command &command) {
	command.getType()->apply(faction, command);
	if (command.getType()->getEnergyCost()) {
		ep -= command.getType()->getEnergyCost();
	}
}

/** De-Apply costs for a command
  * @param command the command to cancel
  * @return CmdResult::SUCCESS
  */
CmdResult Unit::undoCommand(const Command &command) {
	command.getType()->undo(this, command);
	return CmdResult::SUCCESS;
}

/** query the speed at which a skill type is executed
  * @param st the SkillType
  * @return the speed value this unit would execute st at
  */
int Unit::getSpeed(const SkillType *st) const {
	fixed speed = st->getSpeed(this);
	return (speed > 1 ? speed.intp() : 1);
}

// =====================================================
//  class UnitFactory
// =====================================================

Unit* UnitFactory::newUnit(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld) {
	Unit::LoadParams params(node, faction, map, tt, putInWorld);
	Unit *unit = EntityFactory<Unit>::newInstance(params);
	if (unit->isAlive()) {
		unit->Died.connect(this, &UnitFactory::onUnitDied);
	} else {
		m_deadList.push_back(unit);
	}
	return unit;
}

Unit* UnitFactory::newUnit(const Vec2i &pos, const UnitType *type, Faction *faction, Map *map, CardinalDir face, Unit* master) {
	Unit::CreateParams params(pos, type, faction, map, face, master);
	Unit *unit = EntityFactory<Unit>::newInstance(params);
	unit->Died.connect(this, &UnitFactory::onUnitDied);
	return unit;
}

void UnitFactory::onUnitDied(Unit *unit) {
	m_deadList.push_back(unit);
}

void UnitFactory::update() {
	Units::iterator it = m_deadList.begin();
	while (it != m_deadList.end()) {
		if ((*it)->getToBeUndertaken()) {
			(*it)->undertake();
			deleteInstance((*it)->getId());
			it = m_deadList.erase(it);
		} else {
			return;
		}
	}
}

void UnitFactory::deleteUnit(Unit *unit) {
	Units::iterator it = std::find(m_deadList.begin(), m_deadList.end(), unit);
	if (it != m_deadList.end()) {
		m_deadList.erase(it);
	}
	deleteInstance(unit->getId());
}

}}//end namespace

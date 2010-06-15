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
#include "world.h"

#include <algorithm>
#include <cassert>

#include "core_data.h"
#include "config.h"
#include "faction.h"
#include "unit.h"
#include "game.h"
#include "logger.h"
#include "sound_renderer.h"
#include "game_settings.h"
#include "network_message.h"
#include "route_planner.h"
#include "cartographer.h"
#include "lang_features.h"
#include "earthquake_type.h"
#include "renderer.h"
#include "sim_interface.h"

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Sim {

// =====================================================
// 	class World
// =====================================================

const float World::airHeight = 5.f;
World *World::singleton = 0;

// ===================== PUBLIC ========================

World::World(SimulationInterface *iSim) 
		: scenario(NULL)
		, iSim(iSim)
		, game(*iSim->getGameState())
		, minimap(iSim->getGameSettings().getFogOfWar())
		, cartographer(NULL)
		, routePlanner(NULL)
		, posIteratorFactory(65)
		, unitTypeFactory(0)
		, upgradeTypeFactory(0)
		, skillTypeFactory(0)
		, commandTypeFactory(0) {
	Config &config = Config::getInstance();

	GameSettings &gs = iSim->getGameSettings();
	fogOfWar = gs.getFogOfWar();
	fogOfWarSmoothing = config.getRenderFogOfWarSmoothing();
	fogOfWarSmoothingFrameSkip = config.getRenderFogOfWarSmoothingFrameSkip();
	shroudOfDarkness = gs.getFogOfWar();

	unfogActive = false;

	frameCount = 0;
	nextUnitId = 0;
	assert(!singleton);
	singleton = this;
	alive = false;
	unitTypeFactory = new UnitTypeFactory();
	upgradeTypeFactory = new UpgradeTypeFactory();
	skillTypeFactory = new SkillTypeFactory();
	commandTypeFactory = new CommandTypeFactory();
}

World::~World() {
	Logger::getInstance().add("~World", !Program::getInstance()->isTerminating());
	alive = false;

	delete scenario;
	delete cartographer;
	delete routePlanner;

	delete unitTypeFactory;
	delete upgradeTypeFactory;
	delete skillTypeFactory;
	delete commandTypeFactory;
	singleton = 0;
}

void World::save(XmlNode *node) const {
	node->addChild("frameCount", frameCount);
	node->addChild("nextUnitId", nextUnitId);
	iSim->getStats()->save(node->addChild("stats"));
	timeFlow.save(node->addChild("timeFlow"));
	XmlNode *factionsNode = node->addChild("factions");
	foreach_const (Factions, i, factions) {
		i->save(factionsNode->addChild("faction"));
	}
	theCartographer.saveMapState(node->addChild("mapState"));
}

// ========================== init ===============================================

void World::init(const XmlNode *worldNode) {
	_PROFILE_FUNCTION();
	initFactions();
	initCells(); //must be done after knowing faction number and dimensions
	initMap();

	initSplattedTextures();

	// must be done after initMap()
	routePlanner = new RoutePlanner(this);
	cartographer = new Cartographer(this);
	
	if (worldNode) {
		initExplorationState();
		loadSaved(worldNode);
		initMinimap(true);
		theCartographer.loadMapState(worldNode->getChild("mapState"));
	} else if (iSim->getGameSettings().getDefaultUnits()) {
		initMinimap();
		initUnits();
		initExplorationState();
	} else {
		initMinimap();
	}
	computeFow();

	alive = true;
}

//load saved game
void World::loadSaved(const XmlNode *worldNode) {
	Logger::getInstance().add("Loading saved game", true);
	GameSettings &gs = iSim->getGameSettings();
	this->thisFactionIndex = gs.getThisFactionIndex();
	this->thisTeamIndex = gs.getTeam(thisFactionIndex);

	frameCount = worldNode->getChildIntValue("frameCount");
	nextUnitId = worldNode->getChildIntValue("nextUnitId");

	iSim->getStats()->load(worldNode->getChild("stats"));
	timeFlow.load(worldNode->getChild("timeFlow"));

	const XmlNode *factionsNode = worldNode->getChild("factions");
	factions.resize(factionsNode->getChildCount());
	for (int i = 0; i < factionsNode->getChildCount(); ++i) {
		const XmlNode *n = factionsNode->getChild("faction", i);
		const FactionType *ft = techTree.getFactionType(gs.getFactionTypeName(i));
		factions[i].load(n, this, ft, gs.getFactionControl(i), &techTree);
	}

	thisTeamIndex = getFaction(thisFactionIndex)->getTeam();
	map.computeNormals();
	map.computeInterpolatedHeights();
	//map.loadExplorationState(worldNode->getChild("explorationState"));
}

// preload tileset and techtree for progressbar
void World::preload() {
	GameSettings &gs = iSim->getGameSettings();
	tileset.count(gs.getTilesetPath());
	set<string> names;
	for (int i = 0; i < gs.getFactionCount(); ++i) {
		if (gs.getFactionTypeName(i).size()) {
			names.insert(gs.getFactionTypeName(i));
		}
	}
	techTree.preload(gs.getTechPath(), names);
}

//load tileset
bool World::loadTileset() {
	tileset.load(iSim->getGameSettings().getTilesetPath());
	timeFlow.init(&tileset);
	return true;
}

//load tech
bool World::loadTech() {
	GameSettings &gs = iSim->getGameSettings();
	set<string> names;
	for (int i = 0; i < gs.getFactionCount(); ++i) {
		if (gs.getFactionTypeName(i).size()) {
			names.insert(gs.getFactionTypeName(i));
		}
	}
	return techTree.load(gs.getTechPath(), names);
}

//load map
bool World::loadMap() {
	const string &path = iSim->getGameSettings().getMapPath();
	map.load(path, &techTree, &tileset, iSim->getObjectFactory());
	return true;
}

bool World::loadScenario(const string &path) {
	assert(!scenario);
	scenario = new Scenario();
	scenario->load(path);
	return true;
}

// ==================== misc ====================
#ifdef EARTHQUAKE_CODE
void World::updateEarthquakes(float seconds) {

	map.update(seconds);

	const Map::Earthquakes &earthquakes = map.getEarthquakes();
	Map::Earthquakes::const_iterator ei;
	for (ei = earthquakes.begin(); ei != earthquakes.end(); ++ei) {
		// 4x/second
		if (!(frameCount % 10)) {
			Earthquake::DamageReport damageReport;
			Earthquake::DamageReport::const_iterator dri;
			float maxDps = (*ei)->getType()->getMaxDps();
			const AttackType *at = (*ei)->getType()->getAttackType();

			(*ei)->getDamageReport(damageReport, 0.25f);
			Unit *attacker = (*ei)->getCause();

			for (dri = damageReport.begin(); dri != damageReport.end(); ++dri) {
				fixed multiplier = techTree.getDamageMultiplier(
						at, dri->first->getType()->getArmourType());

				///@todo make fixed point ... use multiplier

				float intensity = dri->second.intensity;
				float count = (float)dri->second.count;
				float damage = intensity * maxDps;// * multiplier;

				if (!(*ei)->getType()->isAffectsAllies() && attacker->isAlly(dri->first)) {
					continue;
				}

				if (dri->first->decHp((int)roundf(damage)) && attacker) {
					doKill(attacker, dri->first);
					continue;
				}

				const FallDownSkillType *fdst = (const FallDownSkillType *)
						dri->first->getType()->getFirstStOfClass(SkillClass::FALL_DOWN);
				if (fdst && dri->first->getCurrSkill() != fdst
						&& random.randRange(0.f, 1.f) + fdst->getAgility() < intensity / count / 0.25f) {
					dri->first->setCurrSkill(fdst);
				}
			}
		}
	}
}
#endif // Disable Earthquakes

void World::processFrame() {
	_PROFILE_FUNCTION();

	++frameCount;
	iSim->startFrame(frameCount);

	// check ScriptTimers
	ScriptManager::update();

	//time
	timeFlow.update();

	//water effects
	waterEffects.update();

	//update units
	for (Factions::const_iterator f = factions.begin(); f != factions.end(); ++f) {
		const Units &units = f->getUnits();
		for (int i = 0;  i < f->getUnitCount(); ++i) {
			Unit *unit = f->getUnit(i);
			
			if (unit->update()) {

				iSim->doUpdateUnitCommand(unit);

				//move unit in cells
				if (unit->getCurrSkill()->getClass() == SkillClass::MOVE) {

					moveUnitCells(unit);

					//play water sound
					if (map.getCell(unit->getPos())->getHeight() < map.getWaterLevel() 
					&& unit->getCurrField() == Field::LAND
					&& map.getTile(Map::toTileCoords(unit->getPos()))->isVisible(getThisTeamIndex())
					&& theRenderer.getCuller().isInside(unit->getPos())) {
						theSoundRenderer.playFx(CoreData::getInstance().getWaterSound());
					}
				}
			}

			//unit death
			if (unit->isDead() && unit->getCurrSkill()->getClass() != SkillClass::DIE) {
				unit->kill();
			}
			map.assertUnitCells(unit);
		}
	}

//	updateEarthquakes(1.f / 40.f);

	//undertake the dead
	iSim->getUnitFactory().update();

	//consumable resource (e.g., food) costs
	for (int i = 0; i < techTree.getResourceTypeCount(); ++i) {
		const ResourceType *rt = techTree.getResourceType(i);
		if (rt->getClass() == ResourceClass::CONSUMABLE 
		&& frameCount % (rt->getInterval() * WORLD_FPS) == 0) {
			for (int i = 0; i < getFactionCount(); ++i) {
				getFaction(i)->applyCostsOnInterval();
			}
		}
	}

	//fow smoothing
	if (fogOfWarSmoothing && ((frameCount + 1) % (fogOfWarSmoothingFrameSkip + 1)) == 0) {
		float fogFactor = float(frameCount % WORLD_FPS) / WORLD_FPS;
		minimap.updateFowTex(clamp(fogFactor, 0.f, 1.f));
	}

	//tick
	if (frameCount % WORLD_FPS == 0) {
		computeFow();
		tick();
	}
}


// to World
void World::hit(Unit *attacker) {
	hit(attacker, static_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField());
}

void World::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked) {
	_PROFILE_FUNCTION();
	typedef std::map<Unit*, fixed> DistMap;
	//hit attack positions
	if (ast->getSplash() && ast->getSplashRadius()) {
		Vec2i pos;
		fixed distance;
		DistMap hitSet;
		Util::PosCircularIteratorSimple pci(map.getBounds(), targetPos, ast->getSplashRadius());
		while (pci.getNext(pos, distance)) {
			if ((attacked = map.getCell(pos)->getUnit(targetField))
			&& (hitSet.find(attacked) == hitSet.end() || hitSet[attacked] > distance)) {
				hitSet[attacked] = distance;
			}
		}
		foreach (DistMap, it, hitSet) {
			damage(attacker, ast, it->first, it->second);
			if (ast->hasEffects()) {
				applyEffects(attacker, ast->getEffectTypes(), it->first, it->second);
			}
		}
	} else {
		if (!attacked) {
			attacked = map.getCell(targetPos)->getUnit(targetField);
		}
		if (attacked) {
			damage(attacker, ast, attacked, 0);
			if (ast->hasEffects()) {
				applyEffects(attacker, ast->getEffectTypes(), attacked, 0);
			}
			if (attacked->getAttackedTrigger()) {
				ScriptManager::onAttackedTrigger(attacked);
				attacked->setAttackedTrigger(false);
			}
		}
	}
}

// to world
void World::damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, fixed distance) {
	int var = ast->getAttackVar();
	int armor = attacked->getArmor();
	fixed damageMultiplier = getTechTree()->getDamageMultiplier(ast->getAttackType(),
							 attacked->getType()->getArmourType());
	//compute damage
	fixed fDamage = attacker->getAttackStrength(ast);
	fDamage = ((fDamage + random.randRange(-var, var)) / (distance + 1) - armor) * damageMultiplier;
	if (fDamage < 1) {
		fDamage = 1;
	}
	int damage = fDamage.intp();
	if (attacked->decHp(damage)) {
		doKill(attacker, attacked);
	}
	if (attacked->getFaction()->isThisFaction()
	&& !theRenderer.getCuller().isInside(attacked->getPos())) {
		attacked->getFaction()->attackNotice(attacked);
	}
}

void World::doKill(Unit *killer, Unit *killed) {
	ScriptManager::onUnitDied(killed);
	iSim->getStats()->kill(killer->getFactionIndex(), killed->getFactionIndex());
	if (killer->isAlive() && killer->getTeam() != killed->getTeam()) {
		killer->incKills();
	}

	if (killed->getCurrSkill()->getClass() != SkillClass::DIE) {
		killed->kill();
	}
	if (!killed->isMobile()) {
		// obstacle removed
		cartographer->updateMapMetrics(killed->getPos(), killed->getSize());
	}
}


// Apply effects to a specific location, with or without splash
void World::applyEffects(Unit *source, const EffectTypes &effectTypes,
				const Vec2i &targetPos, Field targetField, int splashRadius) {
	typedef std::map<Unit*, fixed> DistMap;
	Unit *target;

	if (splashRadius != 0) {
		DistMap hitList;
		DistMap::iterator i;
		Vec2i pos;
		fixed distance;

		PosCircularIteratorSimple pci(map.getBounds(), targetPos, splashRadius);
		while (pci.getNext(pos, distance)) {
			target = map.getCell(pos)->getUnit(targetField);
			if (target) {
				i = hitList.find(target);
				if (i == hitList.end() || i->second > distance) {
					hitList[target] = distance;
				}
			}
		}
		foreach (DistMap, it, hitList) {
			applyEffects(source, effectTypes, it->first, it->second);
		}
	} else {
		target = map.getCell(targetPos)->getUnit(targetField);
		if (target) {
			applyEffects(source, effectTypes, target, 0);
		}
	}
}

//apply effects to a specific target
void World::applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *target, fixed distance) {
	//apply effects
	for (EffectTypes::const_iterator i = effectTypes.begin();
			i != effectTypes.end(); i++) {
		const EffectType * const &e = *i;

		// lots of tests, roughly in order of speed of evaluation.
		if ((source->isAlly(target) ? e->isEffectsAlly() : e->isEffectsFoe())
		&&	(target->isOfClass(UnitClass::BUILDING) ? e->isEffectsBuildings() : e->isEffectsNormalUnits())
		&&	(e->getChance() != 100 ? random.randPercent() < e->getChance() : true)) {

			fixed strength = e->isScaleSplashStrength() ? fixed(1) / (distance + 1) : 1;
			Effect *primaryEffect = new Effect(e, source, NULL, strength, target, &techTree);

			target->add(primaryEffect);

			foreach_const (EffectTypes, it, e->getRecourse()) {
				source->add(new Effect((*it), NULL, primaryEffect, strength, source, &techTree));
			}
		}
	}
}

//CLEAN: this is never called, and has a silly name considering what it appears to be for...
void World::appyEffect(Unit *u, Effect *e) {
	if (u->add(e)) {
		Unit *attacker = iSim->getUnitFactory().getUnit(e->getSource());
		if (attacker) {
			iSim->getStats()->kill(attacker->getFactionIndex(), u->getFactionIndex());
			attacker->incKills();
		} else if (e->getRoot()) {
			// if killed by a recourse effect, this was suicide
			iSim->getStats()->kill(u->getFactionIndex(), u->getFactionIndex());
		}
	}
}

/** Called every 40 (or whatever WORLD_FPS resolves as) world frames */
void World::tick() {
	if (!fogOfWarSmoothing) {
		minimap.updateFowTex(1.f);
	}

	cartographer->tick();

	//apply regen/degen
	for (int i = 0; i < getFactionCount(); ++i) {
		for (int j = 0; j < getFaction(i)->getUnitCount(); ++j) {
			Unit *unit = getFaction(i)->getUnit(j);
			Unit *killer = unit->tick();

			assert((unit->getHp() == 0 && unit->isDead()) || (unit->getHp() > 0 && unit->isAlive()));
			if (killer) {
				doKill(killer, unit);
			}
		}
	}

	//compute resources balance
	for (int k = 0; k < getFactionCount(); ++k) {
		Faction *faction = getFaction(k);

		//for each resource
		for (int i = 0; i < techTree.getResourceTypeCount(); ++i) {
			const ResourceType *rt = techTree.getResourceType(i);

			//if consumable
			if (rt->getClass() == ResourceClass::CONSUMABLE) {
				int balance = 0;
				for (int j = 0; j < faction->getUnitCount(); ++j) {

					//if unit operative and has this cost
					const Unit *u =  faction->getUnit(j);
					if (u->isOperative()) {
						const Resource *r = u->getType()->getCost(rt);
						if (r != NULL) {
							balance -= u->getType()->getCost(rt)->getAmount();
						}
					}
				}
				faction->setResourceBalance(rt, balance);
			}
		}
	}
}

Unit* World::findUnitById(int id) const {
	return iSim->getUnitFactory().getUnit(id);
}

const UnitType* World::findUnitTypeById(const FactionType* factionType, int id) {
	return unitTypeFactory->getType(id);
}

//looks for a place for a unit around a start lociacion, returns true if succeded
bool World::placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated) {
	bool freeSpace;
	int size = unit->getType()->getSize();
	Field currField = unit->getCurrField();

	for (int r = 1; r < radius; r++) {
		for (int i = -r; i < r; ++i) {
			for (int j = -r; j < r; ++j) {
				Vec2i pos = Vec2i(i, j) + startLoc;
				if (spaciated) {
					const int spacing = 2;
					freeSpace = map.areFreeCells(pos - Vec2i(spacing), size + spacing * 2, currField);
				} else {
					freeSpace = map.areFreeCells(pos, size, currField);
				}

				if (freeSpace) {
					unit->setPos(pos);
					unit->setMeetingPos(pos - Vec2i(1));
					return true;
				}
			}
		}
	}
	return false;
}

//clears a unit old position from map and places new position
void World::moveUnitCells(Unit *unit) {
	Vec2i newPos = unit->getNextPos();
	bool changingTiles = false;
	if (Map::toTileCoords(newPos) != Map::toTileCoords(unit->getPos())) {
		changingTiles = true;
		// remove unit's visibility
		cartographer->removeUnitVisibility(unit);
	}
	assert(routePlanner->isLegalMove(unit, newPos));
	map.clearUnitCells(unit, unit->getPos());
	map.putUnitCells(unit, newPos);
	if (changingTiles) {
		// re-apply unit's visibility
		cartographer->applyUnitVisibility(unit);
	}

	//water splash
	if (tileset.getWaterEffects() && unit->getCurrField() == Field::LAND) {
		if (map.getCell(unit->getLastPos())->isSubmerged()) {
			for (int i = 0; i < 3; ++i) {
				waterEffects.addWaterSplash(
					Vec2f(unit->getLastPos().x + random.randRange(-0.4f, 0.4f), 
						  unit->getLastPos().y + random.randRange(-0.4f, 0.4f))
				);
			}
		}
	}
}

//returns the nearest unit that can store a type of resource given a position and a faction
Unit *World::nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt) {
	float currDist = numeric_limits<float>::infinity();
	Unit *currUnit = NULL;

	for (int i = 0; i < getFaction(factionIndex)->getUnitCount(); ++i) {
		Unit *u = getFaction(factionIndex)->getUnit(i);
		float tmpDist = u->getPos().dist(pos);
		if (tmpDist < currDist && u->getType()->getStore(rt) > 0 && u->isOperative()) {
			currDist = tmpDist;
			currUnit = u;
		}
	}
	return currUnit;
}

///@todo collect all distinct error conditions from the scripting interface, put them in an
/// and enum with a table of string messages... then make error handling in ScriptManager less shit

/** @return unit id, or -1 if factionindex invalid, or -2 if unitName invalid, or -3 if 
  * a position could not be found for the new unit, -4 if pos invalid */
int World::createUnit(const string &unitName, int factionIndex, const Vec2i &pos) {
	if (factionIndex  < 0 && factionIndex >= factions.size()) {
		return -1; // bad faction index
	}
	Faction* faction= &factions[factionIndex];
	const FactionType* ft= faction->getType();
	const UnitType* ut;
	try{
		ut = ft->getUnitType(unitName);
	} catch (runtime_error e) {
		return -2;
	}
	if (!map.isInside(pos)) {
		return -4;
	}
	Unit *unit = iSim->getUnitFactory().newInstance(pos, ut, faction, &map);
	//Unit* unit= new Unit(getNextUnitId(), pos, ut, faction, &map);
	if (placeUnit(pos, generationArea, unit, true)) {
		unit->create(true);
		unit->born();
		if (!unit->isMobile()) {
			cartographer->updateMapMetrics(unit->getPos(), unit->getSize());
		}
		ScriptManager::onUnitCreated(unit);
		return unit->getId();
	} else {
		iSim->getUnitFactory().deleteUnit(unit);
		return -3;
	}
}

int World::giveResource(const string &resourceName, int factionIndex, int amount) {
	if (factionIndex < 0 || factionIndex >= factions.size()) {
		return -1;
	}
	const ResourceType* rt= techTree.getResourceType(resourceName);
	Faction* faction= &factions[factionIndex];
	try {
		rt = techTree.getResourceType(resourceName);
	} catch (runtime_error e) {
		return -2;
	}
	faction->incResourceAmount(rt, amount);
	return 0;
}

void World::unfogMap(const Vec4i &rect, int time) {
	if (time < 1) return;
	unfogActive = true;
	unfogTTL = time;
	unfogArea = rect;
}

///@todo
// these command issueing functions should just return the CommandResult, if they get that far.
// return < 0  == arg error / unknown command / unit had no command of type / etc
// return >= 0 == command given, value returned is CommandResult

int World::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos) {
	Unit* unit= findUnitById(unitId);
	if (unit == NULL) {
		return -1;
	}
	const CommandType *cmdType = NULL;

	if (commandName == "move") {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::MOVE);
	} else if (commandName == "attack") {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::ATTACK);
	} else if (commandName == "harvest") {
		Resource *r = map.getTile(Map::toTileCoords(pos))->getResource();
		bool found = false;
		if (!unit->getType()->getFirstCtOfClass(CommandClass::HARVEST)) {
			return -2; // unit has no appropriate command
		}
		if (!r) {
			cmdType = unit->getType()->getFirstCtOfClass(CommandClass::HARVEST);
		} else {
			for (int i=0; i < unit->getType()->getCommandTypeCount<HarvestCommandType>(); ++i) {
				const HarvestCommandType *hct = unit->getType()->getCommandType<HarvestCommandType>(i);
				if (hct->canHarvest(r->getType())) {
					found = true;
					break;
				}
			}
			if (!found) {
				cmdType = unit->getType()->getFirstCtOfClass(CommandClass::HARVEST);
			}
		}
	} else if (commandName == "patrol") {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::PATROL);
	} else if (commandName == "guard") {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::GUARD);
	} else {
		return -3; // unknown position command

	}
	if (!cmdType) {
		return -2; // unit has no appropriate command
	}
	if (unit->giveCommand(new Command(cmdType, CommandFlags(), pos)) == CommandResult::SUCCESS) {
		return 0; // Ok
	}
	return 1; // command fail
}

/** @return 0 if ok, -1 if unitId or targetId invalid, -2 unit could not attack/repair target,
  * -3 illegal cmd name */
int World::giveTargetCommand (int unitId, const string & cmdName, int targetId) {
	Unit *unit = findUnitById(unitId);
	Unit *target = findUnitById(targetId);
	if (!unit || !target) {
		return -1;
	}
	const CommandType *cmdType = NULL;
	if (cmdName == "attack") {
		const AttackCommandType *act = unit->getType()->getAttackCommand(target->getCurrZone());
		if (act) {
			if (unit->giveCommand(new Command(act, CommandFlags(), target)) == CommandResult::SUCCESS) {
				return 0; // ok
			}
			return 1; // command fail			
		}
		return -2;
	} else if (cmdName == "repair") {
		const RepairCommandType *rct = unit->getType()->getRepairCommand(target->getType());
		if (rct) {
			if (unit->giveCommand(new Command(rct, CommandFlags(), target)) == CommandResult::SUCCESS) {
				return 0; // ok
			}
			return 1; // command fail

		}
		return -2;

	} else if (cmdName == "guard") {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::GUARD);
	} else if (cmdName == "patrol") {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::PATROL);
	} else {
		return -3;
	}
	if (cmdType && unit->giveCommand(new Command(cmdType, CommandFlags(), target))) {
		return 0; // ok
	}
	return 1; // command fail
}
/** return 0 if ok, -1 if bad unit id, -2 if unit has no stop command, -3 illegal cmd name,
  * 1 if command given but failed */
int World::giveStopCommand(int unitId, const string &cmdName) {
	Unit *unit = findUnitById(unitId);
	if (unit == NULL) {
		return -1;
	}
	if (cmdName == "stop") {
		const StopCommandType *sct = (StopCommandType*)unit->getType()->getFirstCtOfClass(CommandClass::STOP);
		if (sct) {
			if (unit->giveCommand(new Command(sct, CommandFlags())) == CommandResult::SUCCESS) {
				return 0; // ok
			}
			return 1; // command fail
		} else {
			return -2;
		}
	} else if (cmdName == "attack-stopped") {
		const AttackStoppedCommandType *asct = 
			(AttackStoppedCommandType *)unit->getType()->getFirstCtOfClass(CommandClass::ATTACK_STOPPED);
		if (asct) {
			if (unit->giveCommand(new Command(asct, CommandFlags())) == CommandResult::SUCCESS) {
				return 0;
			}
			return 1;
		} else {
			return -2;
		}
	} else {
		return -3;
	}
	return 0;
}

/** @return 0 if ok, 1 on command failure, -1 if unitId invalid, -2 if unit cannot produce other unit,
  * -3 if unit's faction has no such unit producedName */
int World::giveProductionCommand(int unitId, const string &producedName) {
	Unit *unit= findUnitById(unitId);
	if (unit == NULL) {
		return -1;
	}
	const UnitType *ut = unit->getType();
	const UnitType *put = NULL;
	try {
		put = unit->getFaction()->getType()->getUnitType(producedName);
	} catch (runtime_error e) {
		return -3;
	}
	const MorphCommandType *mct = NULL;
	//Search for a command that can produce the unit
	for(int i= 0; i<ut->getCommandTypeCount(); ++i) {
		const CommandType* ct= ut->getCommandType(i);
		// if we find a suitable Produce Command, execute and return
		if (ct->getClass()==CommandClass::PRODUCE) {
			const ProduceCommandType *pct= static_cast<const ProduceCommandType*>(ct);
			if (pct->getProducedUnit() == put) {
				CommandResult res = unit->giveCommand(new Command(pct, CommandFlags()));
				if (res == CommandResult::SUCCESS) {
					//theConsole.addLine("produce command success");
					return 0; //Ok
				//} else if (res == CommandResult::FAIL_RESOURCES) {
				//	theConsole.addLine("produce command failed, resources");
				//} else if (res == CommandResult::FAIL_REQUIREMENTS) {
				//	theConsole.addLine("produce command failed, requirements");
				//} else {
				//	theConsole.addLine("produce command failed");
				}
				return 1; // command fail	

			}
		} else if (ct->getClass() == CommandClass::MORPH) { // Morph Command ?
			if (((MorphCommandType*)ct)->getMorphUnit()->getName() == producedName) {
				// just record it for now, and keep looking for a Produce Command
				mct = (MorphCommandType*)ct; 
			}
			
		}
	}
	// didn't find a Produce Command, was there are Morph Command?
	if (mct) {
		CommandResult res = unit->giveCommand(new Command (mct, CommandFlags()));
		if (res == CommandResult::SUCCESS) {
			theConsole.addLine("morph command success");
			return 0; // ok
		} else {
			theConsole.addLine("morph command failed");
			return 1; // fail
		}
	} else {
		return -2;
	}
}

/** #return 0 if ok, -1 if unitId invalid, -2 if unit cannot produce upgrade,
  * -3 if unit's faction has no such upgrade */
int World::giveUpgradeCommand(int unitId, const string &upgradeName) {
	Unit *unit= findUnitById(unitId);
	if (unit==NULL) {
		return -1;
	}
	const UpgradeType *upgrd = NULL;
	try {
		upgrd = unit->getFaction()->getType()->getUpgradeType(upgradeName);
	} catch (runtime_error e) {
		return -3;
	}
	const UnitType *ut= unit->getType();
	//Search for a command that can produce the upgrade
	for(int i= 0; i<ut->getCommandTypeCount(); ++i) {
		const CommandType* ct= ut->getCommandType(i);
		if (ct->getClass()==CommandClass::UPGRADE) {
			const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
			if (uct->getProducedUpgrade() == upgrd) {
				if (unit->giveCommand(new Command(uct, CommandFlags())) == CommandResult::SUCCESS) {
					return 0;
				}
				return 1;
			}
		}
	}
	return -2;
}

int World::getResourceAmount(const string &resourceName, int factionIndex) {
	if (factionIndex<factions.size()) {
		Faction* faction= &factions[factionIndex];
		const ResourceType* rt;
		try {
			rt = techTree.getResourceType(resourceName);
		} catch (runtime_error e) {
			return -2;
		}
		return faction->getResource(rt)->getAmount();
	} else {
		return -1;
	}
}

Vec2i World::getStartLocation(int factionIndex) {
	if (factionIndex<factions.size()) {
		Faction* faction= &factions[factionIndex];
		return map.getStartLocation(faction->getStartLocationIndex());
	} else {
		return Vec2i(-1);
	}
}

Vec2i World::getUnitPosition(int unitId) {
	Unit* unit= findUnitById(unitId);
	if (unit==NULL) {
		return Vec2i(-1);
	}
	return unit->getPos();
}

int World::getUnitFactionIndex(int unitId) {
	Unit* unit= findUnitById(unitId);
	if (unit==NULL) {
		return -1;
	}
	return unit->getFactionIndex();
}

int World::getUnitCount(int factionIndex) {
	if (factionIndex<factions.size()) {
		Faction* faction= &factions[factionIndex];
		int count = 0;
		
		for (int i= 0; i<faction->getUnitCount(); ++i) {
			const Unit* unit= faction->getUnit(i);
			if (unit->isAlive()) {
				++count;
			}
		}
		return count;
	} else {
		return -1;
	}
}

/** @return number of units of type a faction has, -1 if faction index invalid, 
  * -2 if unitType not found */
int World::getUnitCountOfType(int factionIndex, const string &typeName) {
	if (factionIndex<factions.size()) {
		Faction* faction= &factions[factionIndex];
		int count= 0;
		const string &ftName = faction->getType()->getName();
		if (unitTypes[ftName].find(typeName) == unitTypes[ftName].end()) {
			return -2;
		}
		for(int i= 0; i< faction->getUnitCount(); ++i) {
			const Unit* unit= faction->getUnit(i);
			if (unit->isAlive() && unit->getType()->getName()==typeName) {
				++count;
			}
		}
		return count;
	} else {
		return -1;
	}
}

// ==================== PRIVATE ====================

// ==================== private init ====================

//init basic cell state
void World::initCells() {
	GameSettings &gs = iSim->getGameSettings();
	Logger::getInstance().add("State cells", true);
	for (int i = 0; i < map.getTileW(); ++i) {
		for (int j = 0; j < map.getTileH(); ++j) {

			Tile *sc= map.getTile(i, j);

			sc->setFowTexCoord(
				Vec2f(i / (next2Power(map.getTileW()) - 1.f),j / (next2Power(map.getTileH()) - 1.f)));

			for (int k = 0; k < GameConstants::maxPlayers; k++) {
				sc->setExplored(k, !gs.getFogOfWar());
				sc->setVisible(k, !gs.getFogOfWar());
			}
		}
	}
}

//init surface textures
void World::initSplattedTextures() {
	for (int i = 0; i < map.getTileW() - 1; ++i) {
		for (int j = 0; j < map.getTileH() - 1; ++j) {
			Vec2f coord;
			const Texture2D *texture;
			Tile *sc00 = map.getTile(i, j);
			Tile *sc10 = map.getTile(i + 1, j);
			Tile *sc01 = map.getTile(i, j + 1);
			Tile *sc11 = map.getTile(i + 1, j + 1);
			tileset.addSurfTex(
				sc00->getTileType(),
				sc10->getTileType(),
				sc01->getTileType(),
				sc11->getTileType(),
				coord, texture);
			sc00->setTileTexCoord(coord);
			sc00->setTileTexture(texture);
		}
	}
}

//creates each faction looking at each faction name contained in GameSettings
void World::initFactions() {
	Logger::getInstance().add("Faction types", true);
	GameSettings &gs = iSim->getGameSettings();
	if (!gs.getFactionCount()) return;

	if (gs.getFactionCount() > map.getMaxPlayers()) {
		throw runtime_error("This map only supports " + intToStr(map.getMaxPlayers()) + " players");
	}

	//create factions
	this->thisFactionIndex = gs.getThisFactionIndex();
	factions.resize(gs.getFactionCount());
	for (int i = 0; i < factions.size(); ++i) {
		const FactionType *ft= techTree.getFactionType(gs.getFactionTypeName(i));
		factions[i].init(
			ft, gs.getFactionControl(i), &techTree, i, gs.getTeam(i),
			gs.getStartLocationIndex(i), gs.getStartLocationIndex(i),
			i==thisFactionIndex, gs.getDefaultResources()
		);
		if (unitTypes.find(ft->getName()) == unitTypes.end()) {
			unitTypes.insert(pair< string,set<string> >(ft->getName(),set<string>()));
			for (int j = 0; j < ft->getUnitTypeCount(); ++j) {
				unitTypes[ft->getName()].insert(ft->getUnitType(j)->getName());
			}
		}
		//  iSim->getStats()->setTeam(i, gs.getTeam(i));
		//  iSim->getStats()->setFactionTypeName(i, formatString(gs.getFactionTypeName(i)));
		//  iSim->getStats()->setControl(i, gs.getFactionControl(i));
	}

	thisTeamIndex = getFaction(thisFactionIndex)->getTeam();
}

void World::initMinimap(bool resuming) {
	Logger::getInstance().add("Compute minimap surface", true);
	minimap.init(map.getW(), map.getH(), this, resuming);
}

//place units randomly aroud start location
void World::initUnits() {

	Logger::getInstance().add("Generate elements", true);

	//put starting units
	for (int i = 0; i < getFactionCount(); ++i) {
		Faction *f = &factions[i];
		const FactionType *ft = f->getType();
		for (int j = 0; j < ft->getStartingUnitCount(); ++j) {
			const UnitType *ut = ft->getStartingUnit(j);
			int initNumber = ft->getStartingUnitAmount(j);
			for (int l = 0; l < initNumber; l++) {
				Unit *unit = iSim->getUnitFactory().newInstance(Vec2i(0), ut, f, &map);
					//new Unit(getNextUnitId(), Vec2i(0), ut, f, &map);
				int startLocationIndex = f->getStartLocationIndex();

				if (placeUnit(map.getStartLocation(startLocationIndex), generationArea, unit, true)) {
					unit->create(true);
					// sends updates, must be done after all other init
					//unit->born();

				} else {
					throw runtime_error("Unit cant be placed, this error is caused because there "
						"is no enough place to put the units near its start location, make a "
						"better map: " + unit->getType()->getName() + " Faction: "+intToStr(i));
				}
				if (unit->getType()->hasSkillClass(SkillClass::BE_BUILT)) {
					map.flatternTerrain(unit);
					cartographer->updateMapMetrics(unit->getPos(), unit->getSize());
				}
			}
		}
	}
	map.computeNormals();
	map.computeInterpolatedHeights();
}

void World::activateUnits() {
	foreach (Factions, fIt, factions) {
		foreach_const (Units, uIt, fIt->getUnits()) {
			(*uIt)->born();
		}
	}
}

void World::initMap() {
	map.init();
}

void World::initExplorationState() {
	if (!fogOfWar) {
		for (int i = 0; i < map.getTileW(); ++i) {
			for (int j = 0; j < map.getTileH(); ++j) {
				map.getTile(i, j)->setVisible(thisTeamIndex, true);
				map.getTile(i, j)->setExplored(thisTeamIndex, true);
			}
		}
	} else if (!shroudOfDarkness) {
		for (int i = 0; i < map.getTileW(); ++i) {
			for (int j = 0; j < map.getTileH(); ++j) {
				map.getTile(i, j)->setExplored(thisTeamIndex, true);
			}
		}
	}
}


// ==================== exploration ====================

void World::doUnfog() {
	const Vec2i start = Map::toTileCoords(Vec2i(unfogArea.x, unfogArea.y));
	const Vec2i end = Map::toTileCoords(Vec2i(unfogArea.x + unfogArea.z, unfogArea.y + unfogArea.w));
	for (int x = start.x; x < end.x; ++x) {
		for (int y = start.y; y < end.y; ++y) {
			if (map.isInsideTile(x,y)) {
				map.getTile(x,y)->setVisible(thisTeamIndex, true);
			}
		}
	}
	for (int x = start.x; x < end.x; ++x) {
		for (int y = start.y; y < end.y; ++y) {
			if (map.isInsideTile(x,y)) {
				minimap.incFowTextureAlphaSurface(Vec2i(x,y), 1.f);
			}
		}
	}
	--unfogTTL;
	if (!unfogTTL) unfogActive = false;
}

void World::exploreCells(const Vec2i &newPos, int sightRange, int teamIndex) {

	Vec2i newSurfPos = Map::toTileCoords(newPos);
	int surfSightRange = sightRange / GameConstants::cellScale + 1;
	int sweepRange = surfSightRange + indirectSightRange + 1;

	//explore
	for (int x = -sweepRange; x <= sweepRange; ++x) {
		for (int y = -sweepRange; y <= sweepRange; ++y) {
			Vec2i currRelPos(x, y);
			Vec2i currPos = newSurfPos + currRelPos;

			if (map.isInsideTile(currPos)) {
				float dist = currRelPos.length();
				Tile *sc = map.getTile(currPos);

				//explore
				if (dist < sweepRange) {
					sc->setExplored(teamIndex, true);
				}

				//visible
				if (dist < surfSightRange) {
					sc->setVisible(teamIndex, true);
				}
			}
		}
	}
}

//computes the fog of war texture, contained in the minimap
void World::computeFow() {
	GameSettings &gs = iSim->getGameSettings();
	//reset texture
	minimap.resetFowTex();

	//reset visibility in cells
	for (int k = 0; k < GameConstants::maxPlayers; ++k) {
		if (fogOfWar || k != thisTeamIndex) {
			for (int i = 0; i < map.getTileW(); ++i) {
				for (int j = 0; j < map.getTileH(); ++j) {
					map.getTile(i, j)->setVisible(k, !gs.getFogOfWar());
				}
			}
		}
	}

	//compute cells
	for (int i = 0; i < getFactionCount(); ++i) {
		for (int j = 0; j < getFaction(i)->getUnitCount(); ++j) {
			Unit *unit = getFaction(i)->getUnit(j);
	
			//exploration
			if (unit->isOperative()) {
				exploreCells(unit->getCenteredPos(), unit->getSight(), unit->getTeam());
			}
		}
	}

	//fire
	for (int i = 0; i < getFactionCount(); ++i) {
		for (int j = 0; j < getFaction(i)->getUnitCount(); ++j) {
			Unit *unit = getFaction(i)->getUnit(j);
	
			//fire
			ParticleSystem *fire = unit->getFire();
			if (fire) {
				fire->setActive(map.getTile(Map::toTileCoords(unit->getPos()))->isVisible(thisTeamIndex));
			}
		}
	}
	
	//compute texture
	if (unfogActive) { // scripted map reveal
		doUnfog();
	}	
	for (int i = 0; i < getFactionCount(); ++i) {
		Faction *faction = getFaction(i);
		if (faction->getTeam() != thisTeamIndex) {
			continue;
		}
		for (int j = 0; j < faction->getUnitCount(); ++j) {
			const Unit *unit = faction->getUnit(j);
			if (unit->isOperative()) {
				int sightRange = unit->getSight();
				Vec2i pos;
				float distance;

				//iterate through all cells
				PosCircularIteratorSimple pci(map.getBounds(), unit->getPos(), sightRange + indirectSightRange);
				while (pci.getNext(pos, distance)) {
					Vec2i surfPos = Map::toTileCoords(pos);

					//compute max alpha
					float maxAlpha;
					if (surfPos.x > 1 && surfPos.y > 1 && surfPos.x < map.getTileW() - 2 && surfPos.y < map.getTileH() - 2) {
						maxAlpha = 1.f;
					} else if (surfPos.x > 0 && surfPos.y > 0 && surfPos.x < map.getTileW() - 1 && surfPos.y < map.getTileH() - 1) {
						maxAlpha = 0.3f;
					} else {
						maxAlpha = 0.0f;
					}

					//compute alpha
					float alpha;
					if (distance > sightRange) {
						alpha = clamp(1.f - (distance - sightRange) / (indirectSightRange), 0.f, maxAlpha);
					} else {
						alpha = maxAlpha;
					}
					minimap.incFowTextureAlphaSurface(surfPos, alpha);
				}
			}
		}
	}
}


void World::hackyCleanUp(Unit *unit) {
	if (std::find(newlydead.begin(), newlydead.end(), unit) == newlydead.end()) {
		newlydead.push_back(unit);
	}
}

// =====================================================
//	class ParticleDamager
// =====================================================

ParticleDamager::ParticleDamager(Unit *attacker, Unit *target, World *world, const GameCamera *gameCamera) {
	this->gameCamera = gameCamera;
	this->attackerRef = attacker->getId();
	this->targetRef = target ? target->getId() : -1;
	this->ast = static_cast<const AttackSkillType*>(attacker->getCurrSkill());
	this->targetPos = attacker->getTargetPos();
	this->targetField = attacker->getTargetField();
	this->world = world;
}

void ParticleDamager::execute(ParticleSystem *particleSystem) {
	Unit *attacker = theSimInterface->getUnitFactory().getUnit(attackerRef);

	if (attacker) {
		Unit *target = theSimInterface->getUnitFactory().getUnit(targetRef);
		if (target) {
			targetPos = target->getCenteredPos();
			// manually feed the attacked unit here to avoid problems with cell maps and such
			world->hit(attacker, ast, targetPos, targetField, target);
		} else {
			world->hit(attacker, ast, targetPos, targetField, NULL);
		}

		//play sound
		StaticSound *projSound = ast->getProjSound();
		if (particleSystem->getVisible() && projSound) {
			SoundRenderer::getInstance().playFx(
				projSound, Vec3f(float(targetPos.x), 0.f, float(targetPos.y)), gameCamera->getPos());
		}
	}
}

}}//end namespace

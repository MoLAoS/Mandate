// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "leak_dumper.h"
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
#include "command.h"

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Sim {

MasterEntityFactory::MasterEntityFactory()
		: m_unitFactory()
		, m_upgradeFactory(false)
		, m_commandFactory(true)
		, m_effectFactory(true)
		, m_projectileFactory(false)
		, m_mapObjectFactory(true) {
	g_logger.logProgramEvent("MasterEntityFactory");
}

void MasterEntityFactory::deleteCommand(const Glest::Entities::Command *c) {
	deleteCommand(c->getId());
}

// =====================================================
// 	class World
// =====================================================

const float World::airHeight = 5.f;
World *World::singleton = 0;

// ===================== PUBLIC ========================

World::World(SimulationInterface *simInterface) 
		: scenario(NULL)
		, m_simInterface(simInterface)
		, game(*simInterface->getGameState())
		, cartographer(0)
		, routePlanner(0)
		, thisFactionIndex(-1)
		, posIteratorFactory(65)
		, m_cloakGroupIdCounter(0) {
	GameSettings &gs = m_simInterface->getGameSettings();
	string techName = formatString(basename(gs.getTechPath()));
	g_program.setTechTitle(techName);

	fogOfWar = gs.getFogOfWar();
	shroudOfDarkness = gs.getShroudOfDarkness();

	fogOfWarSmoothing = g_config.getRenderFogOfWarSmoothing();
	fogOfWarSmoothingFrameSkip = g_config.getRenderFogOfWarSmoothingFrameSkip();

	unfogActive = false;
	frameCount = 0;
	assert(!singleton);
	singleton = this;
	alive = false;
}

World::~World() {
	g_logger.logProgramEvent("~World", !g_program.isTerminating());
	alive = false;

	delete scenario;
	delete cartographer;
	delete routePlanner;

	singleton = 0;
}

void World::save(XmlNode *node) const {
	node->addChild("frameCount", frameCount);
	node->addChild("nextUnitId", m_unitFactory.getIdCounter());
	node->addChild("nextCmdId", m_commandFactory.getIdCounter());
	m_simInterface->getStats()->save(node->addChild("stats"));
	timeFlow.save(node->addChild("timeFlow"));
	XmlNode *factionsNode = node->addChild("factions");
	foreach_const (Factions, i, factions) {
		i->save(factionsNode->addChild("faction"));
	}
	g_cartographer.saveMapState(node->addChild("mapState"));
}

// ========================== init ===============================================

void World::init(const XmlNode *worldNode) {
	//_PROFILE_FUNCTION();
	initFactions();
	initExplorationState(); // must be done after loadMap()
	map.init();

	// must be done after map.init()
	routePlanner = new RoutePlanner(this);
	cartographer = new Cartographer(this);
	
	if (worldNode) {
		loadSaved(worldNode);
		g_userInterface.initMinimap(fogOfWar, shroudOfDarkness, true);
		g_cartographer.loadMapState(worldNode->getChild("mapState"));
	} else if (m_simInterface->getGameSettings().getDefaultUnits()) {
		g_userInterface.initMinimap(fogOfWar, shroudOfDarkness, false);
		initUnits();
		initExplorationState(true); // reset for human, so we get funky alpha fade in
	} else {
		g_userInterface.initMinimap(fogOfWar, shroudOfDarkness, false);
	}
	computeFow();
	alive = true;
}

int World::getCloakGroupId(const string &name) {
	CloakGroupIdMap::iterator it = m_cloakGroupIds.find(name);
	if (it == m_cloakGroupIds.end()) {
		m_cloakGroupIds[name] = m_cloakGroupIdCounter;
		m_cloakGroupNames[m_cloakGroupIdCounter] = name;
		return m_cloakGroupIdCounter++;
	}
	return it->second;
}

const string noneString = "none";

const string& World::getCloakGroupName(int id) {
	CloakGroupNameMap::iterator it = m_cloakGroupNames.find(id);
	if (it != m_cloakGroupNames.end()) {
		return it->second;
	}
	return noneString;
}

//load saved game
void World::loadSaved(const XmlNode *worldNode) {
	g_logger.logProgramEvent("Loading saved game", true);
	GameSettings &gs = m_simInterface->getGameSettings();
	this->thisFactionIndex = gs.getThisFactionIndex();
	this->thisTeamIndex = gs.getTeam(thisFactionIndex);

	frameCount = worldNode->getChildIntValue("frameCount");
	m_unitFactory.setIdCounter(worldNode->getChildIntValue("nextUnitId"));
	m_commandFactory.setIdCounter(worldNode->getChildIntValue("nextCmdId"));

	m_simInterface->getStats()->load(worldNode->getChild("stats"));
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
	GameSettings &gs = m_simInterface->getGameSettings();
	tileset.count(gs.getTilesetPath());
	set<string> names;
	for (int i = 0; i < gs.getFactionCount(); ++i) {
		if (gs.getFactionTypeName(i).size()) {
			names.insert(gs.getFactionTypeName(i));
		}
	}
	techTree.preload(gs.getTechPath(), names);
	string techName = basename(gs.getTechPath());
	g_lang.loadTechStrings(techName);
	g_lang.loadFactionStrings(techName, names);
}

//load tileset
bool World::loadTileset() {
	tileset.load(m_simInterface->getGameSettings().getTilesetPath(), &techTree);
	timeFlow.init(&tileset);
	return true;
}

//load tech
bool World::loadTech() {
	GameSettings &gs = m_simInterface->getGameSettings();
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
	const string &path = m_simInterface->getGameSettings().getMapPath();
	map.load(path, &techTree, &tileset);
	return true;
}

bool World::loadScenario(const string &path) {
	assert(!scenario);
	scenario = new Scenario();
	scenario->load(path);
	return true;
}

void World::initSurveyor(Faction *f) {
	m_surveyorMap[f->getIndex()] = new Surveyor(f, cartographer);
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
				///@todo move this to unit if it's ever used
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

void World::updateUnits(const Faction *f) {
	const int n = f->getUnitCount();
	for (int i=0; i < n; ++i) {
		Unit *unit = f->getUnit(i);
		unit->doUpdate();
		// assert map cells
		map.assertUnitCells(unit);
	}
}

void World::processFrame() {
	//_PROFILE_FUNCTION);

	++frameCount;
	m_simInterface->startFrame(frameCount);
	g_userInterface.getMinimap()->update(frameCount);

	// check ScriptTimers
	ScriptManager::update();

	//time
	timeFlow.update();

	//water effects
	waterEffects.update();

	//update units
	for (Factions::const_iterator f = factions.begin(); f != factions.end(); ++f) {
		updateUnits(&*f);
	}
	updateUnits(&glestimals);

	//updateEarthquakes(1.f / 40.f);

	//undertake the dead
	m_unitFactory.update();

	//consumable resource (e.g., food) costs
	for (int i = 0; i < techTree.getResourceTypeCount(); ++i) {
		const ResourceType *rt = techTree.getResourceType(i);
		if (rt->getClass() == ResourceClass::CONSUMABLE 
		&& frameCount % (rt->getInterval() * WORLD_FPS) == 0) {
			for (int i = 0; i < getFactionCount(); ++i) {
				getFaction(i)->applyCostsOnInterval(rt);
			}
		}
	}

	//fow smoothing
	if (fogOfWarSmoothing && ((frameCount + 1) % (fogOfWarSmoothingFrameSkip + 1)) == 0) {
		float fogFactor = float(frameCount % WORLD_FPS) / WORLD_FPS;
		
		g_userInterface.getMinimap()->updateFowTex(clamp(fogFactor, 0.f, 1.f));
	}

	//tick
	if (frameCount % WORLD_FPS == 0) {
		computeFow();
		tick();
	}
}

void World::hit(Unit *attacker) {
	hit(attacker, static_cast<const AttackSkillType*>(attacker->getCurrSkill()), attacker->getTargetPos(), attacker->getTargetField());
}

void World::hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked) {
	//_PROFILE_FUNCTION();
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
		if (attacked && attacked->isAlive()) {
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
	&& !g_renderer.getCuller().isInside(attacked->getPos())) {
		attacked->getFaction()->attackNotice(attacked);
	}
}

void World::damage(Unit *unit, int hp) {
	if (unit->decHp(hp)) {
		ScriptManager::onUnitDied(unit);
		unit->kill();
		if (!unit->isMobile()) { // obstacle removed
			cartographer->updateMapMetrics(unit->getPos(), unit->getSize());
		}
	}
}

void World::doKill(Unit *killer, Unit *killed) {
	killer->doKill(killed);
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
			i != effectTypes.end(); ++i) {
		const EffectType * const &e = *i;

		// lots of tests, roughly in order of speed of evaluation.
		if ((source->isAlly(target) ? e->isEffectsAlly() : e->isEffectsFoe())
		//&&	(target->isOfClass(UnitClass::BUILDING) ? e->isEffectsBuildings() : e->isEffectsNormalUnits())
		&&	(e->getChance() != 100 ? random.randPercent() < e->getChance() : true)) {

			fixed strength = e->isScaleSplashStrength() ? fixed(1) / (distance + 1) : 1;
			Effect *primaryEffect = newEffect(e, source, NULL, strength, target, &techTree);

			target->add(primaryEffect);

			foreach_const (EffectTypes, it, e->getRecourse()) {
				source->add(newEffect((*it), NULL, primaryEffect, strength, source, &techTree));
			}
		}
	}
}

//CLEAN: this is never called, and has a silly name considering what it appears to be for...
void World::appyEffect(Unit *u, Effect *e) {
	if (u->add(e)) {
		Unit *attacker = getUnit(e->getSource());
		if (attacker) {
			m_simInterface->getStats()->kill(attacker->getFactionIndex(), u->getFactionIndex());
			attacker->incKills();
		} else if (e->getRoot()) {
			// if killed by a recourse effect, this was suicide
			m_simInterface->getStats()->kill(u->getFactionIndex(), u->getFactionIndex());
		}
	}
}

/** Called every 40 (or whatever WORLD_FPS resolves as) world frames */
void World::tick() {
	if (!fogOfWarSmoothing) {
		g_userInterface.getMinimap()->updateFowTex(1.f);
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

	///@todo foreach(Factions, f, factions) { (*f)->computeResourceBalances(); }

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
						ResourceAmount r = u->getType()->getCost(rt, faction);
						if (r.getType()) {
							balance -= r.getAmount();
						}
					}
				}
				faction->setResourceBalance(rt, balance);
			}
		}
	}
}

const UnitType* World::findUnitTypeById(const FactionType* factionType, int id) {
	return g_prototypeFactory.getUnitType(id);
}

//looks for a place for a unit around a start lociacion, returns true if succeded
bool World::placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated) {
	const int spacing = spaciated ? 2 : 0;
	Field field = unit->getCurrField();
	int effectiveSize = unit->getSize() + spacing * 2;

	Vec2i tl = startLoc - Vec2i(spacing);
	Vec2i br = tl;
	for (int i=0; i <= radius; ++i) {
		PerimeterIterator iter(tl, br);
		while (iter.more()) {
			Vec2i testPos = iter.next();
			if (map.areFreeCells(testPos, effectiveSize, field)) {
				unit->setPos(testPos + Vec2i(spacing));
				unit->setMeetingPos(unit->getPos() - Vec2i(1));
				return true;
			}
		}
		tl -= Vec2i(1);
		br += Vec2i(1);
	}
	return false;
}

//clears a unit old position from map and places new position
void World::moveUnitCells(Unit *unit) {
	Vec2i newPos = unit->getNextPos();
	bool changingTiles = false;
	Vec2i centrePos = unit->getCenteredPos();
	Vec2i dir = unit->getNextPos() - unit->getPos();
	Vec2i newCentrePos = centrePos + dir;

	if (Map::toTileCoords(centrePos) != Map::toTileCoords(newCentrePos)) {
		changingTiles = true;
		// remove unit's visibility
		cartographer->removeUnitVisibility(unit);
	}
	if (unit->getCurrCommand()->getType()->getClass() != CmdClass::TELEPORT) {
		RUNTIME_CHECK(routePlanner->isLegalMove(unit, newPos));
	}
	map.clearUnitCells(unit, unit->getPos());
	map.putUnitCells(unit, newPos);
	if (changingTiles) {
		// re-apply unit's visibility
		cartographer->applyUnitVisibility(unit);
		if (unit->getType()->isDetector()) {
			cartographer->detectorMoved(unit, centrePos);
		}
	}

	if (unit->getCurrCommand()->getType()->getClass() != CmdClass::TELEPORT) {
		// water splash
		if (tileset.getWaterEffects() && unit->getCurrField() == Field::LAND
		&& getThisFaction()->canSee(unit) && map.getCell(unit->getLastPos())->isSubmerged()
		&& g_renderer.getCuller().isInside(newCentrePos)) {
			for (int i = 0; i < 3; ++i) {
				waterEffects.addWaterSplash(
					Vec2f(unit->getLastPos().x + random.randRange(-0.4f, 0.4f), 
						  unit->getLastPos().y + random.randRange(-0.4f, 0.4f))
				);
			}
			g_soundRenderer.playFx(g_coreData.getWaterSound());
		}
	} else {
		unit->setPos(unit->getPos()); // teleport, double setPos() to avoid regular movement between cells
	}
}

//returns the nearest unit that can store a type of resource given a position and a faction
Unit *World::nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt) {
	float currDist = numeric_limits<float>::infinity();
	Unit *currUnit = NULL;

	const Faction *f = getFaction(factionIndex);
	for (int i = 0; i < f->getUnitCount(); ++i) {
		Unit *u = f->getUnit(i);
		float tmpDist = u->getPos().dist(pos);
		if (tmpDist < currDist && u->getType()->getStore(rt, f) > 0 && u->isOperative()) {
			currDist = tmpDist;
			currUnit = u;
		}
	}
	return currUnit;
}

/** @return unit id, or < 0 on error, see LuaCmdResult */
int World::createUnit(const string &unitName, int factionIndex, const Vec2i &pos, bool precise) {
	if (factionIndex  < 0 && factionIndex >= factions.size()) {
		return LuaCmdResult::INVALID_FACTION_INDEX;
	}
	Faction* faction= &factions[factionIndex];
	const FactionType* ft= faction->getType();
	const UnitType* ut;
	try{
		ut = ft->getUnitType(unitName);
	} catch (runtime_error e) {
		return LuaCmdResult::PRODUCIBLE_NOT_FOUND;
	}
	if (!map.isInside(pos)) {
		return LuaCmdResult::INVALID_POSITION;
	}
	Unit *unit = newUnit(pos, ut, faction, &map, CardinalDir::NORTH);
	if ((precise && map.canOccupy(pos, ut->getField(),  ut, CardinalDir::NORTH))
	|| (!precise && placeUnit(pos, generationArea, unit, true))) {
		unit->create(true);
		unit->born();
		if (!unit->isMobile()) {
			cartographer->updateMapMetrics(unit->getPos(), unit->getSize());
		}
		ScriptManager::onUnitCreated(unit);
		return unit->getId();
	} else {
		m_unitFactory.deleteUnit(unit);
		return LuaCmdResult::INSUFFICIENT_SPACE;
	}
}

int World::giveResource(const string &resourceName, int factionIndex, int amount) {
	if (factionIndex < 0 || factionIndex >= factions.size()) {
		return LuaCmdResult::INVALID_FACTION_INDEX;
	}
	const ResourceType* rt= techTree.getResourceType(resourceName);
	Faction* faction= &factions[factionIndex];
	try {
		rt = techTree.getResourceType(resourceName);
	} catch (runtime_error e) {
		return LuaCmdResult::RESOURCE_NOT_FOUND;
	}
	faction->incResourceAmount(rt, amount);
	return LuaCmdResult::OK;
}

void World::unfogMap(const Vec4i &rect, int time) {
	if (time < 1) return;
	unfogActive = true;
	unfogTTL = time;
	unfogArea = rect;
	const Vec2i start = Map::toTileCoords(Vec2i(unfogArea.x, unfogArea.y));
	const Vec2i end = Map::toTileCoords(Vec2i(unfogArea.x + unfogArea.z, unfogArea.y + unfogArea.w));
	RectIterator iter(start, end);
	Vec2i pos;
	while (iter.more()) {
		pos = iter.next();
		if (map.isInsideTile(pos)) {
			map.getTile(pos)->setExplored(thisTeamIndex, true);
		}
	}
}

/** @return < 0 on error (see LuaCmdResult) else see CmdResult */
int World::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos) {
	Unit* unit= findUnitById(unitId);
	if (!unit) {
		return LuaCmdResult::INVALID_UNIT_ID;
	}
	const CommandType *cmdType = 0;

	if (commandName == "move") {
		cmdType = unit->getType()->getFirstCtOfClass(CmdClass::MOVE);
	} else if (commandName == "attack") {
		cmdType = unit->getType()->getFirstCtOfClass(CmdClass::ATTACK);
	} else if (commandName == "harvest") {
		MapResource *r = map.getTile(Map::toTileCoords(pos))->getResource();
		bool found = false;
		if (!unit->getType()->getFirstCtOfClass(CmdClass::HARVEST)) {
			return LuaCmdResult::NO_CAPABLE_COMMAND;
		}
		if (!r) {
			cmdType = unit->getType()->getFirstCtOfClass(CmdClass::HARVEST);
		} else {
			for (int i=0; i < unit->getType()->getCommandTypeCount<HarvestCommandType>(); ++i) {
				const HarvestCommandType *hct = unit->getType()->getCommandType<HarvestCommandType>(i);
				if (hct->canHarvest(r->getType())) {
					found = true;
					cmdType = hct;
					break;
				}
			}
			if (!found) {
				cmdType = unit->getType()->getFirstCtOfClass(CmdClass::HARVEST);
			}
		}
	} else if (commandName == "patrol") {
		cmdType = unit->getType()->getFirstCtOfClass(CmdClass::PATROL);
	} else if (commandName == "guard") {
		cmdType = unit->getType()->getFirstCtOfClass(CmdClass::GUARD);
	} else {
		return LuaCmdResult::INVALID_COMMAND_CLASS;
	}
	if (!cmdType) {
		return LuaCmdResult::NO_CAPABLE_COMMAND;
	}
	return unit->giveCommand(newCommand(cmdType, CmdFlags(), pos));
}

int World::giveBuildCommand(int unitId, const string &commandName, const string &buildType, const Vec2i &pos) {
	Unit* unit= findUnitById(unitId);
	if (!unit) {
		return LuaCmdResult::INVALID_UNIT_ID;
	}
	const UnitType *ut = unit->getType();
	const UnitType *but;
	try {
		but = unit->getFaction()->getType()->getUnitType(buildType);
	} catch (runtime_error &e) {
		return LuaCmdResult::PRODUCIBLE_NOT_FOUND;
	}
	const CommandType *cmdType = 0;

	if (commandName == "build") {
		for (int i=0; i < ut->getCommandTypeCount<BuildCommandType>(); ++i) {
			const BuildCommandType *bct = ut->getCommandType<BuildCommandType>(i);
			if (bct->canBuild(but)) {
				return unit->giveCommand(newCommand(bct, CmdFlags(), pos, but, CardinalDir::NORTH));
			}
		}
		return LuaCmdResult::NO_CAPABLE_COMMAND;
//	} else if (commandName == "placed-morph") {
//
	} else {
		return LuaCmdResult::INVALID_COMMAND_CLASS;
	}
}

/** @return < 0 on error (see LuaCmdResult) else see CmdResult */
int World::giveTargetCommand (int unitId, const string & cmdName, int targetId) {
	Unit *unit = findUnitById(unitId);
	Unit *target = findUnitById(targetId);
	if (!unit || !target) {
		return LuaCmdResult::INVALID_UNIT_ID;
	}
	const CommandType *cmdType = 0;
	if (cmdName == "attack") {
		const AttackCommandType *act = unit->getType()->getAttackCommand(target->getCurrZone());
		if (act) {
			return unit->giveCommand(newCommand(act, CmdFlags(), target));
		} else {
			return LuaCmdResult::NO_CAPABLE_COMMAND;
		}
	} else if (cmdName == "repair") {
		const RepairCommandType *rct = unit->getType()->getRepairCommand(target->getType());
		if (rct) {
			return unit->giveCommand(newCommand(rct, CmdFlags(), target));
		} else {
			return LuaCmdResult::NO_CAPABLE_COMMAND;
		}
	} else if (cmdName == "guard") {
		cmdType = unit->getType()->getFirstCtOfClass(CmdClass::GUARD);
	} else if (cmdName == "patrol") {
		cmdType = unit->getType()->getFirstCtOfClass(CmdClass::PATROL);
	} else {
		return LuaCmdResult::INVALID_COMMAND_CLASS;
	}
	if (cmdType) {
		return unit->giveCommand(newCommand(cmdType, CmdFlags(), target));
	}
	return LuaCmdResult::NO_CAPABLE_COMMAND;
}

/** @return < 0 on error (see LuaCmdResult) else see CmdResult */
int World::giveStopCommand(int unitId, const string &cmdName) {
	Unit *unit = findUnitById(unitId);
	if (!unit) {
		return LuaCmdResult::INVALID_UNIT_ID;
	}
	if (cmdName == "stop") {
		const StopCommandType *sct = (StopCommandType*)unit->getType()->getFirstCtOfClass(CmdClass::STOP);
		if (sct) {
			// return CmdResult
			return unit->giveCommand(newCommand(sct, CmdFlags()));
		} else {
			return LuaCmdResult::NO_CAPABLE_COMMAND;
		}
	} else if (cmdName == "attack-stopped") {
		const AttackStoppedCommandType *asct = 
			(AttackStoppedCommandType *)unit->getType()->getFirstCtOfClass(CmdClass::ATTACK_STOPPED);
		if (asct) {
			return unit->giveCommand(newCommand(asct, CmdFlags()));
		} else {
			return LuaCmdResult::NO_CAPABLE_COMMAND;
		}
	}
	return LuaCmdResult::INVALID_COMMAND_CLASS;
}

/** @return < 0 on error (see LuaCmdResult) else see CmdResult */
int World::giveProductionCommand(int unitId, const string &producedName) {
	Unit *unit= findUnitById(unitId);
	if (!unit) {
		return LuaCmdResult::INVALID_UNIT_ID;
	}
	const UnitType *ut = unit->getType();
	// Search for a command that can produce the producible
	for (int i=0; i < ut->getCommandTypeCount(); ++i) {
		const CommandType* ct= ut->getCommandType(i);
		for (int j=0; j < ct->getProducedCount(); ++j) {
			const ProducibleType *pt = ct->getProduced(j);
			if (pt->getName() == producedName) {
				CmdResult res = unit->giveCommand(
					newCommand(ct, CmdFlags(), Command::invalidPos, pt, CardinalDir::NORTH));
				return res;
			}
		}
	}
	return LuaCmdResult::NO_CAPABLE_COMMAND;
}

/** @return < 0 on error (see LuaCmdResult) else see CmdResult */
int World::giveUpgradeCommand(int unitId, const string &upgradeName) {
	Unit *unit= findUnitById(unitId);
	if (!unit) {
		return LuaCmdResult::INVALID_UNIT_ID;
	}
	const UpgradeType *upgrd = 0;
	try {
		upgrd = unit->getFaction()->getType()->getUpgradeType(upgradeName);
	} catch (runtime_error e) {
		return LuaCmdResult::PRODUCIBLE_NOT_FOUND;
	}
	const UnitType *ut= unit->getType();
	//Search for a command that can produce the upgrade
	for(int i= 0; i < ut->getCommandTypeCount<UpgradeCommandType>(); ++i) {
		const UpgradeCommandType* uct = ut->getCommandType<UpgradeCommandType>(i);
		for (int i=0; i < uct->getProducedCount(); ++i) {
			if (uct->getProducedUpgrade(i) == upgrd) {
				CmdResult cmdRes = unit->giveCommand(newCommand(uct, CmdFlags(), invalidPos, upgrd, CardinalDir::NORTH));
				return cmdRes;
			}
		}
	}
	return LuaCmdResult::NO_CAPABLE_COMMAND;
}

/** @ return < 0 on error (see LuaCmdResult) else query result*/
int World::getResourceAmount(const string &resourceName, int factionIndex) {
	if (factionIndex >= 0 && factionIndex < factions.size()) {
		Faction* faction= &factions[factionIndex];
		const ResourceType* rt;
		try {
			rt = techTree.getResourceType(resourceName);
		} catch (runtime_error e) {
			return LuaCmdResult::RESOURCE_NOT_FOUND;
		}
		return faction->getResource(rt)->getAmount();
	} else {
		return LuaCmdResult::INVALID_FACTION_INDEX;
	}
}

Vec2i World::getStartLocation(int factionIndex) {
	if (factionIndex >= 0 && factionIndex < factions.size()) {
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
		return LuaCmdResult::INVALID_UNIT_ID;
	}
	return unit->getFactionIndex();
}

int World::getUnitCount(int factionIndex) {
	if (factionIndex >= 0 && factionIndex < factions.size()) {
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
		return LuaCmdResult::INVALID_FACTION_INDEX;
	}
}

/** @return number of units of type a faction has, -1 if faction index invalid, 
  * -2 if unitType not found */
int World::getUnitCountOfType(int factionIndex, const string &typeName) {
	if (factionIndex >= 0 && factionIndex < factions.size()) {
		Faction* faction= &factions[factionIndex];
		int count= 0;
		const string &ftName = faction->getType()->getName();
		if (unitTypes[ftName].find(typeName) == unitTypes[ftName].end()) {
			return LuaCmdResult::PRODUCIBLE_NOT_FOUND;
		}
		for(int i= 0; i< faction->getUnitCount(); ++i) {
			const Unit* unit= faction->getUnit(i);
			if (unit->isAlive() && unit->getType()->getName()==typeName) {
				++count;
			}
		}
		return count;
	} else {
		return LuaCmdResult::INVALID_FACTION_INDEX;
	}
}

// ==================== PRIVATE ====================

// ==================== private init ====================

//creates each faction looking at each faction name contained in GameSettings
void World::initFactions() {
	g_logger.logProgramEvent("Faction types", true);
	
	glestimals.init(&tileset.getGlestimalFactionType(), ControlType::INVALID, "Glestimals",
		&techTree, -1, -1, -1, -1, false, false);
	
	GameSettings &gs = m_simInterface->getGameSettings();
	this->thisFactionIndex = gs.getThisFactionIndex();
	if (!gs.getFactionCount()) {
		thisTeamIndex = 0;
		return;
	}

	if (gs.getFactionCount() > map.getMaxPlayers()) {
		throw runtime_error("This map only supports " + intToStr(map.getMaxPlayers()) + " players");
	}

	//create factions
	factions.resize(gs.getFactionCount());
	for (int i = 0; i < factions.size(); ++i) {
		const FactionType *ft= techTree.getFactionType(gs.getFactionTypeName(i));
		factions[i].init(
			ft, gs.getFactionControl(i), gs.getPlayerName(i), &techTree, i, gs.getTeam(i),
			gs.getStartLocationIndex(i), gs.getColourIndex(i),
			i==thisFactionIndex, gs.getDefaultResources()
		);
		if (unitTypes.find(ft->getName()) == unitTypes.end()) {
			unitTypes.insert(pair< string,set<string> >(ft->getName(),set<string>()));
			for (int j = 0; j < ft->getUnitTypeCount(); ++j) {
				unitTypes[ft->getName()].insert(ft->getUnitType(j)->getName());
			}
		}
		//  m_simInterface->getStats()->setTeam(i, gs.getTeam(i));
		//  m_simInterface->getStats()->setFactionTypeName(i, formatString(gs.getFactionTypeName(i)));
		//  m_simInterface->getStats()->setControl(i, gs.getFactionControl(i));
	}
	thisTeamIndex = getFaction(thisFactionIndex)->getTeam();	
}

//place units randomly aroud start location
void World::initUnits() {

	g_logger.logProgramEvent("Generate elements", true);

	//put starting units
	for (int i = 0; i < getFactionCount(); ++i) {
		Faction *f = &factions[i];
		const FactionType *ft = f->getType();
		for (int j = 0; j < ft->getStartingUnitCount(); ++j) {
			const UnitType *ut = ft->getStartingUnit(j);
			int initNumber = ft->getStartingUnitAmount(j);
			for (int l = 0; l < initNumber; l++) {
				Unit *unit = newUnit(Vec2i(0), ut, f, &map, CardinalDir::NORTH);
				int startLocationIndex = f->getStartLocationIndex();

				if (placeUnit(map.getStartLocation(startLocationIndex), generationArea, unit, true)) {
					unit->create(true);
					//unit->born(); ... sends updates, must be done after all other init
				} else {
					throw runtime_error("Unit can't be placed! This error is caused because there "
						"is not enough space to put the units near its start location, make a "
						"better map: " + unit->getType()->getName() + " Faction: " + intToStr(i));
				}
			}
		}
	}
	map.computeNormals();
	map.computeInterpolatedHeights();

	foreach (Factions, fIt, factions) {
		foreach_const (Units, uIt, fIt->getUnits()) {
			const Unit* const &unit = *uIt;
			const UnitType *ut = unit->getType();
			if (ut->hasSkillClass(SkillClass::BE_BUILT) && !ut->hasSkillClass(SkillClass::MOVE)) {
				map.prepareTerrain(unit);
				cartographer->updateMapMetrics(unit->getPos(), unit->getSize());
			}
		}
	}
}

void World::activateUnits(bool resumingGame) {
	foreach (Factions, fIt, factions) {
		foreach_const (Units, uIt, fIt->getUnits()) {
			(*uIt)->born(resumingGame);
		}
	}
//	foreach_const (Units, uIt, glestimals.getUnits()) {
//		(*uIt)->born();
//	}
}

// init tile exploration state
void World::initExplorationState(bool thisFactionOnly) {
	for (int i = 0; i < map.getTileW(); ++i) {
		for (int j = 0; j < map.getTileH(); ++j) {
			Tile *tile = map.getTile(i, j);
			if (thisFactionOnly) {
				tile->setVisible(thisTeamIndex, !fogOfWar);
				tile->setExplored(thisTeamIndex, !shroudOfDarkness);
			} else {
				for (int k = 0; k < GameConstants::maxPlayers; ++k) {
					tile->setVisible(k, !fogOfWar);
					tile->setExplored(k, !shroudOfDarkness);
				}
			}
		}
	}
}


// ==================== exploration ====================

void World::doUnfog() {
	const Vec2i start = Map::toTileCoords(Vec2i(unfogArea.x, unfogArea.y));
	const Vec2i end = Map::toTileCoords(Vec2i(unfogArea.x + unfogArea.z, unfogArea.y + unfogArea.w));
	RectIterator iter(start, end);
	Vec2i pos;
	while (iter.more()) {
		pos = iter.next();
		if (map.isInsideTile(pos)) {
			map.getTile(pos)->setVisible(thisTeamIndex, true);
			g_userInterface.getMinimap()->incFowTextureAlphaSurface(pos, 1.f);
		}
	}
	--unfogTTL;
	if (!unfogTTL) {
		unfogActive = false;
	}
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

				// explore
				if (shroudOfDarkness && dist < sweepRange) {
					sc->setExplored(teamIndex, true);
				}

				// visible
				if (dist < surfSightRange) {
					sc->setVisible(teamIndex, true);
				}
			}
		}
	}
}

//computes the fog of war texture, contained in the minimap
void World::computeFow() {
	//GameSettings &gs = m_simInterface->getGameSettings();
	
	///@todo move to Minimap
	//reset texture
	Minimap *minimap = g_userInterface.getMinimap();
	minimap->resetFowTex();

	// reset visibility in cells		
	for (int i = 0; i < map.getTileW(); ++i) {
		for (int j = 0; j < map.getTileH(); ++j) {
			Tile *tile = map.getTile(i, j);
			for (int k = 0; k < GameConstants::maxPlayers; ++k) {
				bool val =			// if no fog and no shroud, or no fog and shroud with this 
					(!fogOfWar		// tile seen previously, then set visible
					&& (!shroudOfDarkness || (shroudOfDarkness && tile->isExplored(k))));
				tile->setVisible(k, val);
			}
		}
	}
	// explore cells
	for (int i = 0; i < getFactionCount(); ++i) {
		for (int j = 0; j < getFaction(i)->getUnitCount(); ++j) {
			Unit *unit = getFaction(i)->getUnit(j);
			if (unit->isOperative() && !unit->isCarried()) {
				exploreCells(unit->getCenteredPos(), unit->getSight(), unit->getTeam());
			}
		}
	}
	// turn fires on/off (redundant ? all particle-systems now subjected to visibilty checks)
	for (int i = 0; i < getFactionCount(); ++i) {
		for (int j = 0; j < getFaction(i)->getUnitCount(); ++j) {
			Unit *unit = getFaction(i)->getUnit(j);
			// fire
			ParticleSystem *fire = unit->getFire();
			if (fire) {
				fire->setActive(map.getTile(Map::toTileCoords(unit->getPos()))->isVisible(thisTeamIndex));
			}
		}
	}
	
	// compute texture
	if (unfogActive) { // scripted map reveal
		doUnfog();
	}
	Rectangle rect(0,0, map.getTileW(), map.getTileH());
	TypeMap<> tmpMap(rect, numeric_limits<float>::infinity());
	tmpMap.clearMap(numeric_limits<float>::infinity());
	for (int i = 0; i < getFactionCount(); ++i) {
		Faction *faction = getFaction(i);
		if (faction->getTeam() != thisTeamIndex) {
			continue;
		}
		for (int j = 0; j < faction->getUnitCount(); ++j) {
			const Unit *unit = faction->getUnit(j);
			if (unit->isOperative() && !unit->isCarried()) {
				int sightRange = unit->getSight();
				Vec2i pos;
				float distance;

				//iterate through all cells
				PosCircularIteratorSimple pci(map.getBounds(), unit->getPos(), sightRange + indirectSightRange);
				while (pci.getNext(pos, distance)) {
					Vec2i surfPos = Map::toTileCoords(pos);
					float curr = tmpMap.getInfluence(surfPos);
					if (curr == 1.f) {
						continue; // already max
					}

					// compute max alpha
					float maxAlpha;
					if (surfPos.x > 1 && surfPos.y > 1 && surfPos.x < map.getTileW() - 2 && surfPos.y < map.getTileH() - 2) {
						// strictly inside map
						maxAlpha = 1.f;
					} else {
						// map boundary
						maxAlpha = 0.f;
						if (tmpMap.getInfluence(surfPos) == 0.3f) {
							continue; // already max
						}
					}

					// compute alpha
					float alpha;

					if (distance > sightRange) {
						alpha = clamp(1.f - (distance - sightRange) / (indirectSightRange), 0.f, maxAlpha);
					} else {
						alpha = maxAlpha;
					}
					if (alpha != 0.f && (curr == numeric_limits<float>::infinity() || alpha > curr)) {
						tmpMap.setInfluence(surfPos, alpha);
					}
					//minimap->incFowTextureAlphaSurface(surfPos, alpha);
				}
			}
		}
	}
	RectIterator iter(Vec2i(0,0), Vec2i(rect.w - 1, rect.h - 1));
	while (iter.more()) {
		Vec2i tPos = iter.next();
		float val = tmpMap.getInfluence(tPos);
		if (val != numeric_limits<float>::infinity()) {
			minimap->incFowTextureAlphaSurface(tPos, val);
		} else if (!shroudOfDarkness) {
			if (tPos.x == 0 || tPos.y == 0 || tPos.x == map.getTileW() - 1 || tPos.y == map.getTileH() - 1) {
				minimap->incFowTextureAlphaSurface(tPos, 0.f);
			} else {
				minimap->incFowTextureAlphaSurface(tPos, 0.5f);
			}
		}
	}
}

// =====================================================
//	class ParticleDamager
// =====================================================

ParticleDamager::ParticleDamager(Unit *attacker, Unit *target) {
	this->attackerRef = attacker->getId();
	this->targetRef = target ? target->getId() : -1;
	this->ast = static_cast<const AttackSkillType*>(attacker->getCurrSkill());
	this->targetPos = attacker->getTargetPos();
	this->targetField = attacker->getTargetField();
}

void ParticleDamager::projectileArrived(ParticleSystem *particleSystem) {
	World &world = g_world;
	Unit *attacker = world.getUnit(attackerRef);
	if (attacker) {
		Unit *target = world.getUnit(targetRef);
		if (target) {
			targetPos = target->getCenteredPos();
			// manually feed the attacked unit here to avoid problems with cell maps and such
			world.hit(attacker, ast, targetPos, targetField, target);
		} else {
			world.hit(attacker, ast, targetPos, targetField, NULL);
		}

		// sound
		StaticSound *projSound = ast->getProjSound();
		if (particleSystem->getVisible() && projSound) {
			g_soundRenderer.playFx(projSound, Vec3f(float(targetPos.x), 0.f, float(targetPos.y)),
				g_gameState.getGameCamera()->getPos());
		}
	}
}

SpellDeliverer::SpellDeliverer(Unit* caster, UnitId target, Vec2i pos)
		: m_caster(caster->getId())
		, m_targetUnit(target)
		, m_targetPos(pos)
		, m_targetZone(Zone::LAND) {
	m_castSkill = static_cast<const CastSpellSkillType*>(caster->getCurrSkill());
}

void SpellDeliverer::projectileArrived(ParticleSystem *particleSystem) {
	World &world = g_world;
	Unit *caster = world.getUnit(m_caster);
	Unit *target = world.getUnit(m_targetUnit);

	if (m_castSkill->getSplashRadius()) {
		g_world.applyEffects(caster, m_castSkill->getEffectTypes(), target->getCenteredPos(),
			target->getType()->getField(), m_castSkill->getSplashRadius());
	} else {
		g_world.applyEffects(caster, m_castSkill->getEffectTypes(), target, 0);
	}
}


}}//end namespace

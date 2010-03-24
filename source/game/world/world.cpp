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

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
// 	class World
// =====================================================

const float World::airHeight = 5.f;
World *World::singleton = 0;

// ===================== PUBLIC ========================

World::World(Game *game) 
		: scenario(NULL)
		, game(*game)
		, gs(game->getGameSettings())
		, stats(game->getGameSettings())
		, cartographer(NULL)
		, routePlanner(NULL)
		, posIteratorFactory(65)
		, minimap(gs.getFogOfWar()) {
	Config &config = Config::getInstance();

	fogOfWar = gs.getFogOfWar();
	fogOfWarSmoothing = config.getRenderFogOfWarSmoothing();
	fogOfWarSmoothingFrameSkip = config.getRenderFogOfWarSmoothingFrameSkip();
	shroudOfDarkness = gs.getFogOfWar();

	unfogActive = false;

	frameCount = 0;
	nextUnitId = 0;
	scriptManager = NULL;
	assert(!singleton);
	singleton = this;
	alive = false;
}

World::~World() {
	singleton = NULL;
}

void World::end(){
	Logger::getInstance().add("World", !Program::getInstance()->isTerminating());
	alive = false;

	for (int i = 0; i < factions.size(); ++i) {
		factions[i].end();
	}
	delete scenario;
	delete cartographer;
	delete routePlanner;
	//stats will be deleted by BattleEnd
}

// ========================== init ===============================================

void World::init() {
	initFactionTypes();
	initCells(); //must be done after knowing faction number and dimensions
	initMap();

	initSplattedTextures();

	// must be done after initMap()
	routePlanner = new RoutePlanner(this);
	cartographer = new Cartographer(this);
	unitUpdater.init(game);
	
	//minimap must be init after sum computation
	initMinimap();

	if (game.getGameSettings().getDefaultUnits()) {
		initUnits();
	}
	initExplorationState();	
	computeFow();
	alive = true;
}

// preload tileset and techtree for progressbar
void World::preload() {
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
	tileset.load(game.getGameSettings().getTilesetPath());
	timeFlow.init(&tileset);
	return true;
}

//load tech
bool World::loadTech() {
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
	const string &path = gs.getMapPath();
	map.load(path, &techTree, &tileset);
	return true;
}

bool World::loadScenario(const string &path) {
	assert(!scenario);
	scenario = new Scenario();
	scenario->load(path);
	return true;
}

// ==================== misc ====================

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
				float multiplier = techTree.getDamageMultiplier(
						at, dri->first->getType()->getArmorType());
				float intensity = dri->second.intensity;
				float count = (float)dri->second.count;
				float damage = intensity * maxDps * multiplier;

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

void World::update() {
	PROFILE_START( "World Update" );
	++frameCount;

	// check ScriptTimers
	scriptManager->onTimer();

	//time
	timeFlow.update();

	//water effects
	waterEffects.update();

	/* NETWORK:
	//update network clients
	if (isNetworkClient()) {
		updateClient();
	}
	*/

	//update units
	for (Factions::const_iterator f = factions.begin(); f != factions.end(); ++f) {
		const Units &units = f->getUnits();
		for (int i = 0;  i < f->getUnitCount(); ++i) {
			unitUpdater.updateUnit(f->getUnit(i));
		}
	}

	updateEarthquakes(1.f / 40.f);

	//undertake the dead
	for (int i = 0; i < getFactionCount(); ++i) {
		for (int j = 0; j < getFaction(i)->getUnitCount(); ++j) {
			Unit *unit = getFaction(i)->getUnit(j);
			if (unit->getToBeUndertaken()) {
				unit->undertake();
				delete unit;
				j--;
			}
		}
	}

	//consumable resource (e.g., food) costs
	for (int i = 0; i < techTree.getResourceTypeCount(); ++i) {
		const ResourceType *rt = techTree.getResourceType(i);
		if (rt->getClass() == ResourceClass::CONSUMABLE 
		&& frameCount % (rt->getInterval()*Config::getInstance().getGsWorldUpdateFps()) == 0) {
			for (int i = 0; i < getFactionCount(); ++i) {
				getFaction(i)->applyCostsOnInterval();
			}
		}
	}

	//fow smoothing
	if (fogOfWarSmoothing && ((frameCount + 1) % (fogOfWarSmoothingFrameSkip + 1)) == 0) {
		float fogFactor = float(frameCount % Config::getInstance().getGsWorldUpdateFps()) 
									/ Config::getInstance().getGsWorldUpdateFps();
		minimap.updateFowTex(clamp(fogFactor, 0.f, 1.f));
	}

	//tick
	if (frameCount % Config::getInstance().getGsWorldUpdateFps() == 0) {
		computeFow();
		tick();
	}
	assertConsistiency();
	PROFILE_STOP( "World Update" );}

void World::doKill(Unit *killer, Unit *killed) {
	scriptManager->onUnitDied(killed);
	int kills = 1 + killed->getPets().size();
	for (int i = 0; i < kills; i++) {
		stats.kill(killer->getFactionIndex(), killed->getFactionIndex());
		if (killer->isAlive() && killer->getTeam() != killed->getTeam()) {
			killer->incKills();
		}
	}

	if (killed->getCurrSkill()->getClass() != SkillClass::DIE) {
		killed->kill();
	}
	if ( !killed->isMobile() ) {
		// obstacle removed
		cartographer->updateMapMetrics(killed->getPos(), killed->getSize());
	}
}

void World::tick() {
	if (!fogOfWarSmoothing) {
		minimap.updateFowTex(1.f);
	}
	//apply hack cleanup
	doHackyCleanUp();

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
	for (int i = 0; i < getFactionCount(); ++i) {
		const Faction* faction = getFaction(i);

		for (int j = 0; j < faction->getUnitCount(); ++j) {
			Unit* unit = faction->getUnit(j);

			if (unit->getId() == id) {
				return unit;
			}
		}
	}
	return NULL;
}

const UnitType* World::findUnitTypeById(const FactionType* factionType, int id) {
	for (int i = 0; i < factionType->getUnitTypeCount(); ++i) {
		const UnitType* unitType = factionType->getUnitType(i);

		if (unitType->getId() == id) {
			return unitType;
		}
	}
	return NULL;
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
/** @return unit id, or -1 if factionindex invalid, or -2 if unitName invalid, or -3 if 
  * a position could not be found for the new unit, -4 if pos invalid */
int World::createUnit(const string &unitName, int factionIndex, const Vec2i &pos){
	if ( factionIndex  < 0 && factionIndex >= factions.size() ) {
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
	if ( !map.isInside(pos) ) {
		return -4;
	}
	Unit* unit= new Unit(getNextUnitId(), pos, ut, faction, &map);
	if(placeUnit(pos, generationArea, unit, true)){
		unit->create(true);
		unit->born();
		if ( !unit->isMobile() ) {
			cartographer->updateMapMetrics(unit->getPos(), unit->getSize());
		}
		scriptManager->onUnitCreated(unit);
		return unit->getId();
	} else {
		delete unit;
		return -3;
	}
}

int World::giveResource(const string &resourceName, int factionIndex, int amount){
	if ( factionIndex < 0 && factionIndex >= factions.size() ) {
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
	if ( time < 1 ) return;
	unfogActive = true;
	unfogTTL = time;
	unfogArea = rect;
}

int World::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos){
	Unit* unit= findUnitById(unitId);
	if ( unit == NULL ) {
		return -1;
	}
	const CommandType *cmdType = NULL;

	if ( commandName == "move" ) {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::MOVE);
	} else if ( commandName == "attack" ) {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::ATTACK);
	} else if ( commandName == "harvest" ) {
		Resource *r = map.getTile(Map::toTileCoords(pos))->getResource();
		bool found = false;
		if ( ! unit->getType()->getFirstCtOfClass(CommandClass::HARVEST) ) {
			return -2; // unit has no appropriate command
		}
		if ( !r ) {
			cmdType = unit->getType()->getFirstCtOfClass(CommandClass::HARVEST);
		} else {
			for ( int i=0; i < unit->getType()->getCommandTypeCount(); ++i ) {
				cmdType = unit->getType()->getCommandType(i);
				if ( cmdType->getClass() == CommandClass::HARVEST ) {
					HarvestCommandType *hct = (HarvestCommandType*)cmdType;
					if ( hct->canHarvest(r->getType()) ) {
						found = true;
						break;
					}
				}
			}
			if ( !found ) {
				cmdType = unit->getType()->getFirstCtOfClass(CommandClass::HARVEST);
			}
		}
	} else if ( commandName == "patrol" ) {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::PATROL);
	} else if (commandName == "guard") {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::GUARD);
	} else {
		return -3; // unknown position command

	}
	if ( !cmdType ) {
		return -2; // unit has no appropriate command
	}
	if ( unit->giveCommand(new Command(cmdType, CommandFlags(), pos)) == CommandResult::SUCCESS ) {
		return 0; // Ok
	}
	return 1; // command fail
}

/** @return 0 if ok, -1 if unitId or targetId invalid, -2 unit could attack target, -3 illegal cmd name */
int World::giveTargetCommand ( int unitId, const string & cmdName, int targetId ) {
	Unit *unit = findUnitById( unitId );
	Unit *target = findUnitById( targetId );
	if ( !unit || !target ) {
		return -1;
	}
	const CommandType *cmdType = NULL;
	if ( cmdName == "attack" ) {
		for ( int i=0; i < unit->getType()->getCommandTypeCount(); ++i ) {
			if ( unit->getType()->getCommandType ( i )->getClass () == CommandClass::ATTACK ) {
				const AttackCommandType *act = (AttackCommandType *)unit->getType()->getCommandType ( i );
				const AttackSkillTypes *asts = act->getAttackSkillTypes ();
				if ( asts->getZone(target->getCurrZone()) ) {
					if ( unit->giveCommand(new Command(act, CommandFlags(), target)) ) {
						return 0; // ok
					}
					return 1; // command fail			
				}
			}
		}
		return -2;
	} else if ( cmdName == "repair" ) {
		for ( int i=0; i < unit->getType()->getCommandTypeCount(); ++i ) {
			if ( unit->getType()->getCommandType( i )->getClass () == CommandClass::REPAIR ) {
				RepairCommandType *rct = (RepairCommandType*)unit->getType()->getCommandType ( i );
				if ( rct->isRepairableUnitType ( target->getType() ) ) {
					if ( unit->giveCommand(new Command(rct, CommandFlags(), target)) ) {
						return 0; // ok
					}
					return 1; // command fail
				}
			}
		}
		return -2;

	} else if ( cmdName == "guard" ) {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::GUARD);
	} else if ( cmdName == "patrol" ) {
		cmdType = unit->getType()->getFirstCtOfClass(CommandClass::PATROL);
	} else {
		return -3;
	}
	if ( unit->giveCommand(new Command(cmdType, CommandFlags(), target)) ) {
		return 0; // ok
	}
	return 1; // command fail
}
/** return 0 if ok, -1 if bad unit id, -2 if unit has no stop command, -3 illegal cmd name */
int World::giveStopCommand(int unitId, const string &cmdName) {
	Unit *unit = findUnitById(unitId);
	if (unit == NULL) {
		return -1;
	}
	if (cmdName == "stop") {
		const StopCommandType *sct = (StopCommandType *)unit->getType()->getFirstCtOfClass(CommandClass::STOP);
		if (sct) {
			if (unit->giveCommand(new Command(sct, CommandFlags())) == CommandResult::SUCCESS) {
				return 0; // ok
			}
			return 1; // command fail
		} else {
			return -2;
		}
	} else if ( cmdName == "attack-stopped" ) {
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

/** #return 0 if ok, 1 on command failure, -1 if unitId invalid, -2 if unit cannot produce other unit,
  * -3 if unit's faction has no such unit producedName */
int World::giveProductionCommand(int unitId, const string &producedName){
	Unit *unit= findUnitById(unitId);
	if ( unit == NULL) {
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
	for(int i= 0; i<ut->getCommandTypeCount(); ++i){
		const CommandType* ct= ut->getCommandType(i);
		// if we find a suitable Produce Command, execute and return
		if(ct->getClass()==CommandClass::PRODUCE){
			const ProduceCommandType *pct= static_cast<const ProduceCommandType*>(ct);
			if ( pct->getProducedUnit() == put ) {
				CommandResult res = unit->giveCommand(new Command(pct, CommandFlags()));
				if ( res == CommandResult::SUCCESS ) {
					//theConsole.addLine("produce command success");
					return 0; //Ok
				//} else if ( res == CommandResult::FAIL_RESOURCES ) {
				//	theConsole.addLine("produce command failed, resources");
				//} else if ( res == CommandResult::FAIL_REQUIREMENTS ) {
				//	theConsole.addLine("produce command failed, requirements");
				//} else {
				//	theConsole.addLine("produce command failed");
				}
				return 1; // command fail	

			}
		} else if ( ct->getClass() == CommandClass::MORPH ) { // Morph Command ?
			if ( ((MorphCommandType*)ct)->getMorphUnit()->getName() == producedName ) {
				// just record it for now, and keep looking for a Produce Command
				mct = (MorphCommandType*)ct; 
			}
			
		}
	}
	// didn't find a Produce Command, was there are Morph Command?
	if ( mct ) {
		CommandResult res = unit->giveCommand(new Command (mct, CommandFlags()));
		if ( res == CommandResult::SUCCESS ) {
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
int World::giveUpgradeCommand(int unitId, const string &upgradeName){
	Unit *unit= findUnitById(unitId);
	if ( unit==NULL ) {
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
	for(int i= 0; i<ut->getCommandTypeCount(); ++i){
		const CommandType* ct= ut->getCommandType(i);
		if(ct->getClass()==CommandClass::UPGRADE){
			const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
			if ( uct->getProducedUpgrade() == upgrd ) {
				if ( unit->giveCommand(new Command(uct, CommandFlags())) == CommandResult::SUCCESS ) {
					return 0;
				}
				return 1;
			}
		}
	}
	return -2;
}

int World::getResourceAmount(const string &resourceName, int factionIndex){
	if(factionIndex<factions.size()){
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

Vec2i World::getStartLocation(int factionIndex){
	if(factionIndex<factions.size()){
		Faction* faction= &factions[factionIndex];
		return map.getStartLocation(faction->getStartLocationIndex());
	} else {
		return Vec2i(-1);
	}
}

Vec2i World::getUnitPosition(int unitId){
	Unit* unit= findUnitById(unitId);
	if(unit==NULL){
		return Vec2i(-1);
	}
	return unit->getPos();
}

int World::getUnitFactionIndex(int unitId){
	Unit* unit= findUnitById(unitId);
	if(unit==NULL){
		return -1;
	}
	return unit->getFactionIndex();
}

int World::getUnitCount(int factionIndex){
	if(factionIndex<factions.size()){
		Faction* faction= &factions[factionIndex];
		int count = 0;
		
		for (int i= 0; i<faction->getUnitCount(); ++i) {
			const Unit* unit= faction->getUnit(i);
			if(unit->isAlive()){
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
int World::getUnitCountOfType(int factionIndex, const string &typeName){
	if(factionIndex<factions.size()){
		Faction* faction= &factions[factionIndex];
		int count= 0;
		const string &ftName = faction->getType()->getName();
		if ( unitTypes[ftName].find(typeName) == unitTypes[ftName].end() ) {
			return -2;
		}
		for(int i= 0; i< faction->getUnitCount(); ++i){
			const Unit* unit= faction->getUnit(i);
			if(unit->isAlive() && unit->getType()->getName()==typeName){
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
void World::initFactionTypes() {
	Logger::getInstance().add("Faction types", true);
	
	if(!gs.getFactionCount()) return;

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
			gs.getStartLocationIndex(i), i==thisFactionIndex, gs.getDefaultResources ()
		);
		if ( unitTypes.find(ft->getName()) == unitTypes.end() ) {
			unitTypes.insert(pair< string,set<string> >(ft->getName(),set<string>()));
			for ( int j = 0; j < ft->getUnitTypeCount(); ++j ) {
				unitTypes[ft->getName()].insert(ft->getUnitType(j)->getName());
			}
		}
		//  stats.setTeam(i, gs.getTeam(i));
		//  stats.setFactionTypeName(i, formatString(gs.getFactionTypeName(i)));
		//  stats.setControl(i, gs.getFactionControl(i));
	}

	thisTeamIndex = getFaction(thisFactionIndex)->getTeam();
}

void World::initMinimap() {
	Logger::getInstance().add("Compute minimap surface", true);
	minimap.init(map.getW(), map.getH(), this);
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
				Unit *unit = new Unit(getNextUnitId(), Vec2i(0), ut, f, &map);
				int startLocationIndex = f->getStartLocationIndex();

				if (placeUnit(map.getStartLocation(startLocationIndex), generationArea, unit, true)) {
					unit->create(true);
					unit->born();

				} else {
					throw runtime_error("Unit cant be placed, this error is caused because there "
						"is no enough place to put the units near its start location, make a "
						"better map: " + unit->getType()->getName() + " Faction: "+intToStr(i));
				}
				//if ( !unit->isMobile() )
				//   unitUpdater.routePlanner->updateMapMetrics ( unit->getPos(),
				//                unit->getSize(), true, unit->getCurrField () );
				if(unit->getType()->hasSkillClass(SkillClass::BE_BUILT)) {
					map.flatternTerrain(unit);
					cartographer->updateMapMetrics(unit->getPos(), unit->getSize());
				}
			}
		}
	}
	map.computeNormals();
	map.computeInterpolatedHeights();
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
	} else if ( !shroudOfDarkness ) {
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
	for ( int x = start.x; x < end.x; ++x ) {
		for ( int y = start.y; y < end.y; ++y ) {
			if ( map.isInsideTile(x,y) ) {
				map.getTile(x,y)->setVisible(thisTeamIndex, true);
			}
		}
	}
	for ( int x = start.x; x < end.x; ++x ) {
		for ( int y = start.y; y < end.y; ++y ) {
			if ( map.isInsideTile(x,y) ) {
				minimap.incFowTextureAlphaSurface(Vec2i(x,y), 1.f);
			}
		}
	}
	--unfogTTL;
	if ( !unfogTTL ) unfogActive = false;
}

void World::exploreCells(const Vec2i &newPos, int sightRange, int teamIndex) {

	Vec2i newSurfPos = Map::toTileCoords(newPos);
	int surfSightRange = sightRange / Map::cellScale + 1;
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
	if ( unfogActive ) { // scripted map reveal
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
				PosCircularIteratorSimple pci(map, unit->getPos(), sightRange + indirectSightRange);
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
	for (Units::const_iterator i = newlydead.begin(); i != newlydead.end(); ++i) {
		if (unit == *i) {
			return;
		}
	}
	newlydead.push_back(unit);
}

}}//end namespace

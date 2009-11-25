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
#include "path_finder.h"
#include "lang_features.h"

//DEBUG remove me some time...
#include "renderer.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
//  class World
// =====================================================

const float World::airHeight = 5.f;
World *World::singleton = NULL;

// ===================== PUBLIC ========================

World::World(Game *game)
		: game(*game)
		, gs(game->getGameSettings())
		, unitUpdater(*game)
		, stats(game->getGameSettings())
		, posIteratorFactory(65) {
	Config &config = Config::getInstance();

	fogOfWar = config.getGsFogOfWarEnabled();
	fogOfWarSmoothing = config.getRenderFogOfWarSmoothing();
	fogOfWarSmoothingFrameSkip = config.getRenderFogOfWarSmoothingFrameSkip();
	shroudOfDarkness = config.getGsFogOfWarEnabled();

	unfogActive = false;

	frameCount = 0;
	nextUnitId = 0;
	scriptManager = NULL;
	assert(!singleton);
	singleton = this;
	alive = false;
}

void World::end() {
	Logger::getInstance().add("World", !Program::getInstance()->isTerminating());
	alive = false;

	for (int i = 0; i < factions.size(); ++i) {
		factions[i].end();
	}
	//stats will be deleted by BattleEnd
}

void World::save(XmlNode *node) const {
	node->addChild("frameCount", frameCount);
	node->addChild("nextUnitId", nextUnitId);

	stats.save(node->addChild("stats"));
	timeFlow.save(node->addChild("timeFlow"));
	XmlNode *factionsNode = node->addChild("factions");
	for (Factions::const_iterator i = factions.begin(); i != factions.end(); ++i) {
		i->save(factionsNode->addChild("faction"));
	}

	NetworkDataBuffer mmbuf(0x1000); // 4kb
	NetworkDataBuffer buf(0x10000); // 64kb
	minimap.write(mmbuf);
	uint32 mmDataSize = mmbuf.size();
	buf.write(mmDataSize);
	buf.write(mmbuf.data(), mmDataSize);
	map.write(buf);
	buf.compressUuencodIntoXml(node->addChild("map"), 80);
}

// ========================== init ===============================================

void World::init(const XmlNode *worldNode) {

#  ifdef _GAE_DEBUG_EDITION_
	loadPFDebugTextures();
#  endif
	initFactionTypes();
	initCells(); //must be done after knowing faction number and dimensions
	initMap();

	initSplattedTextures();

	unitUpdater.init(game); // must be done after initMap()

	//minimap must be init after sum computation
	initMinimap();

	if (worldNode) {
		loadSaved(worldNode);
	} else if (game.getGameSettings().getDefaultUnits()) {
		initUnits();
	}

	initExplorationState();

	if (worldNode) {
		NetworkDataBuffer buf;
		NetworkManager &networkManager = NetworkManager::getInstance();
		uint32 mmDataSize;

		buf.uudecodeUncompressFromXml(worldNode->getChild("map"));
		buf.read(mmDataSize);

		// if client resuming saved game we omit minimap alpha data and sythesize it instead.
		if (isNetworkClient()) {
			buf.pop(mmDataSize);
			map.read(buf);
			minimap.synthesize(&map, thisTeamIndex);
		} else {
			minimap.read(buf);
			map.read(buf);
		}

		// make sure I read every last byte
		assert(!buf.size());
	}
	computeFow();

	if (isNetworkServer()) {
		initNetworkServer();
	}

	alive = true;
}

void World::initNetworkServer() {
	// For network games, we want to randomize this so we don't send all updates at
	// once.
	int64 now = Chrono::getCurMicros();
	int64 interval = Config::getInstance().getNetMinFullUpdateInterval() * 1000LL;

	for (Factions::iterator f = factions.begin(); f != factions.end(); ++f) {
		const Units &units = f->getUnits();
		for (Units::const_iterator u = units.begin(); u != units.end(); ++u) {
			(*u)->setLastUpdated(now + random.randRange(0, interval));
		}
	}
}

//load tileset
bool World::loadTileset(Checksum &checksum) {
	tileset.load(game.getGameSettings().getTilesetPath(), checksum);
	timeFlow.init(&tileset);
	return true;
}

//load tech
bool World::loadTech(Checksum &checksum) {
	set<string> names;
	for (int i = 0; i < gs.getFactionCount(); ++i)
		if (gs.getFactionTypeName(i).size()) {
			names.insert(gs.getFactionTypeName(i));
		}
	return techTree.load(gs.getTechPath(), names, checksum);
}

//load map
bool World::loadMap(Checksum &checksum) {
	const string &path = gs.getMapPath();
	checksum.addFile(path, false);
	map.load(path, &techTree, &tileset);
	return true;
}

bool World::loadScenario(const string &path, Checksum *checksum) {
	checksum->addFile(path, true);
	scenario.load(path);
	return true;
}

//load saved game
void World::loadSaved(const XmlNode *worldNode) {
	Logger::getInstance().add("Loading saved game", true);
	this->thisFactionIndex = gs.getThisFactionIndex();
	this->thisTeamIndex = gs.getTeam(thisFactionIndex);

	frameCount = worldNode->getChildIntValue("frameCount");
	nextUnitId = worldNode->getChildIntValue("nextUnitId");

	stats.load(worldNode->getChild("stats"));
	timeFlow.load(worldNode->getChild("timeFlow"));

	const XmlNode *factionsNode = worldNode->getChild("factions");
	factions.resize(factionsNode->getChildCount());
	for (int i = 0; i < factionsNode->getChildCount(); ++i) {
		const XmlNode *n = factionsNode->getChild("faction", i);
		const FactionType *ft = techTree.getFactionType(gs.getFactionTypeName(i));
		factions[i].load(n, this, ft, gs.getFactionControl(i), &techTree);
	}

	map.computeNormals();
	map.computeInterpolatedHeights();

	thisTeamIndex = getFaction(thisFactionIndex)->getTeam();
}

/**
 * Moves the unit to the specified location, evicting any current inhabitants and adding them to the
 * evicted list.  If oldPos is specified, the unit is first removed from those cells.  For new
 * units, units who have been previously evicted or otherwise do not reside in the map, oldPos
 * should be NULL.
 */
inline void World::moveAndEvict(Unit *unit, vector<Unit*> &evicted, Vec2i *oldPos) {
	Vec2i pos = unit->getPos();

	// if they haven't moved, we don't bother
	if (!oldPos || *oldPos != pos) {
		bool isEvicted = false;
		for (vector<Unit*>::iterator i = evicted.begin(); i != evicted.end(); ++i) {
			if (*i == unit) {
				isEvicted = true;
				evicted.erase(i);
				break;
			}
		}
		if (oldPos && !isEvicted) {
			map.clearUnitCells(unit, *oldPos);
		}
		map.evict(unit, pos, evicted);
		map.putUnitCells(unit, pos);
	}
}

inline static string pos2str(Vec2i pos) {
	return intToStr(pos.x) + ", " + intToStr(pos.y);
}

static void logUnit(Unit *unit, string action, Vec2i *oldPos, int *oldHp) {
	Logger::getClientLog().add(action + " " + unit->getType()->getName()
			+ " (" + intToStr(unit->getId()) + ")\tpos "
			+ (oldPos ? pos2str(*oldPos) : string("none"))
			+ " => " + pos2str(unit->getPos())
			+ "\thp " + (oldHp ? intToStr(*oldHp) : string("none"))
			+ " => " + intToStr(unit->getHp())
			, false);
}


void World::doClientUnitUpdate(XmlNode *n, bool minor, vector<Unit*> &evicted, float nextAdvanceFrames) {
	UnitReference unitRef(n);
	Unit *unit = unitRef.getUnit();

	if (unlikely(!unit)) {
		//who the fuck is that?
		if (minor) {
			NetworkManager::getInstance().getClientInterface()->requestFullUpdate(unitRef);
			if (Config::getInstance().getMiscDebugMode()) {
				Logger::getClientLog().add("Received minor update for unknown unit, sending full "
						"update request to server. id = " + intToStr(unitRef.getUnitId())
						+ ", faction = " + intToStr(unitRef.getFaction()->getId()));
			}
			return;
		}
		Faction *faction = getFaction(n->getAttribute("faction")->getIntValue());
		unit = new Unit(n, faction, &map, &techTree, false);
		if (nextUnitId <= unit->getId()) {
			nextUnitId = unit->getId() + 1;
		}

		if (Config::getInstance().getMiscDebugMode()) {
			logUnit(unit, "received full update for unknown unit, so created a new one", NULL, NULL);
		}
		return;
	}
	Vec2i lastPos = unit->getPos();
	int lastHp = unit->getHp();
	Field lastField = unit->getCurrField();
	const SkillType *lastSkill = unit->getCurrSkill();
	bool wasEvicted = false;
	bool morphed = false;

	// if they were previously evicted, remove them from the eviction list because they will get
	// replaced here.
	for (vector<Unit*>::iterator i = evicted.begin(); i != evicted.end(); ++i) {
		if (*i == unit) {
			wasEvicted = true;
			evicted.erase(i);
			break;
		}
	}

	if (!wasEvicted) {
		map.assertUnitCells(unit);
	}

#if defined(DEBUG_NETWORK_LEVEL) && DEBUG_NETWORK_LEVEL > 0
	XmlNode pre("unit");
	unit->save(&pre);
#endif
	if (minor) {
		unit->updateMinor(n);
	} else {
		XmlAttribute *morphAtt = n->getAttribute("morphed", false);
		morphed = morphAtt && morphAtt->getBoolValue();
		unit->update(n, &techTree, false, false, true, nextAdvanceFrames);
		// possibly update selection
		unit->notifyObservers(UnitObserver::eStateChange);
	}

#if defined(DEBUG_NETWORK_LEVEL) && DEBUG_NETWORK_LEVEL > 0
	{
		XmlNode post("unit");
		bool diffFound = false;
		unit->save(&post);
		assert(pre.getChildCount() == post.getChildCount());
		for (int i = 0; i < post.getChildCount(); ++i) {
			char *a = pre.getChild(i)->toString(true);
			char *b = post.getChild(i)->toString(true);
			if (strcmp(a, b)) {
				if (!diffFound) {
					diffFound = true;
					cerr << unit->getType()->getName() << " id " << unit->getId() << endl;
				}
				cerr << "----" << endl << a << endl << "++++" << endl << b << endl;
			}
			delete [] a;
			delete [] b;
		}
		if (diffFound) {
			cerr << "========================" << endl;
		}
	}
#endif


	if (Config::getInstance().getMiscDebugMode()) {
		logUnit(unit, morphed ? "morphed" : "update", &lastPos, &lastHp);
	}

	// were they alive or not killed before?
	if (lastHp || lastSkill->getClass() != scDie) {
		// are they alive now?
		if (unit->getHp()) {
			moveAndEvict(unit, evicted, (wasEvicted || morphed) ? NULL : &lastPos);
		} else {
			if (!unit->getHp()) {
				assert(lastSkill->getClass() != scDie);
				// we kill them here so their cells are freed and we don't have to relocate them if
				// there's another unit (in this update) where they used to be.
				unit->kill(lastPos, !(wasEvicted || morphed));
			}
		}
	} else {
		// make sure they are still dead
		if (unit->getHp()) {
			// doh! we gotta bring em back
			moveAndEvict(unit, evicted, NULL);

			// remove them from hacky cleanup list
			for (Units::iterator i = newlydead.begin(); i != newlydead.end(); ++i) {
				if (*i == unit) {
					newlydead.erase(i);
					break;
				}
			}
		}
	}
	map.assertUnitCells(unit);
}

//placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated)
void World::updateClient() {
	// world enters an inconsistent state while executing this function, but should be consistent
	// when it exits.

	ClientInterface *clientInterface = NetworkManager::getInstance().getClientInterface();
	// here we try to make up for the lag time between when the server sent this data and what the
	// state on the server probably is now.
	float nextAdvanceFrames = 1.f + clientInterface->getAvgLatency() / 2000000.f * Config::getInstance().getGsWorldUpdateFps();
	NetworkMessageUpdate *msg;
	vector<Unit*> evicted;

	while ((msg = clientInterface->getNextUpdate())) {
		/*
		if(Config::getInstance().getDebugMode()) {
		 Logger::getInstance().add(string("\n======================\n")
		   + msg->getData() + "\n======================\n");
		}*/

		msg->parse();
		XmlNode *root = msg->getRootNode();
		XmlNode *n = NULL;
		XmlNode *unitNode = NULL;

		if ((n = root->getChild("new-units", 0, false))) {
			for (int i = 0; i < n->getChildCount(); ++i) {
				unitNode = n->getChild("unit", i);
				Faction *faction = getFaction(unitNode->getAttribute("faction")->getIntValue());
				Unit *unit = new Unit(unitNode, faction, &map, &techTree, false);
				if (nextUnitId <= unit->getId()) {
					nextUnitId = unit->getId() + 1;
				}
				unit->setNextUpdateFrames(nextAdvanceFrames);

				moveAndEvict(unit, evicted, NULL);
				if (Config::getInstance().getMiscDebugMode()) {
					logUnit(unit, "new unit", NULL, NULL);
				}
				if (unit->getType()->hasSkillClass(scBeBuilt)) {
					map.prepareTerrain(unit);
				}
				map.assertUnitCells(unit);
			}
		}

		if ((n = root->getChild("unit-updates", 0, false))) {
			for (int i = 0; i < n->getChildCount(); ++i) {
				doClientUnitUpdate(n->getChild("unit", i), false, evicted, nextAdvanceFrames);
			}
		}

		if ((n = root->getChild("minor-unit-updates", 0, false))) {
			for (int i = 0; i < n->getChildCount(); ++i) {
				doClientUnitUpdate(n->getChild("unit", i), true, evicted, nextAdvanceFrames);
			}
		}

		if ((n = root->getChild("factions", 0, false))) {
			for (int i = 0; i < n->getChildCount(); ++i) {
				if (i > factions.size()) {
					throw runtime_error("too many factions in client update");
				}
				factions[i].update(n->getChild("faction", i));
			}
		}

		delete msg;
	}

	//relocated the evicted
	for (vector<Unit *>::iterator i = evicted.begin(); i != evicted.end(); ++i) {
		Unit *unit = *i;
		Vec2i oldPos = unit->getPos();
		if (!placeUnit(unit->getPos(), 32, unit, false)) {
			// if unable, let the server update him and tell us where he's
			// supposed to be
			unit->kill(oldPos, false);
			logUnit(unit, "couldn't replace unit", &oldPos, NULL);
		} else {
			map.putUnitCells(unit, unit->getPos());
			logUnit(unit, "relocated unit", &oldPos, NULL);
		}
		clientInterface->requestUpdate(unit);
		map.assertUnitCells(unit);
	}
	clientInterface->sendUpdateRequests();
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
				float count = dri->second.count;
				float damage = intensity * maxDps * multiplier;

				if (!(*ei)->getType()->isAffectsAllies() && attacker->isAlly(dri->first)) {
					continue;
				}

				if (dri->first->decHp((int)roundf(damage)) && attacker) {
					doKill(attacker, dri->first);
					continue;
				}

				const FallDownSkillType *fdst = (const FallDownSkillType *)
												dri->first->getType()->getFirstStOfClass(scFallDown);

				if (fdst && dri->first->getCurrSkill() != fdst
						&& random.randRange(0.f, 1.f) + fdst->getAgility() < intensity / count / 0.25f) {
					dri->first->setCurrSkill(fdst);
				}
			}
		}
	}
}

void World::update() {

	++frameCount;

	// check ScriptTimers
	scriptManager->onTimer();

	//time
	timeFlow.update();

	//water effects
	waterEffects.update();

	//update network clients
	if (isNetworkClient()) {
		updateClient();
	}

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
		if (rt->getClass() == rcConsumable && frameCount % (rt->getInterval()*Config::getInstance().getGsWorldUpdateFps()) == 0) {
			for (int i = 0; i < getFactionCount(); ++i) {
				getFaction(i)->applyCostsOnInterval();
			}
		}
	}

	//fow smoothing
	if (fogOfWarSmoothing && ((frameCount + 1) % (fogOfWarSmoothingFrameSkip + 1)) == 0) {
		float fogFactor = static_cast<float>(frameCount % Config::getInstance().getGsWorldUpdateFps()) / Config::getInstance().getGsWorldUpdateFps();
		minimap.updateFowTex(clamp(fogFactor, 0.f, 1.f));
	}

	//tick
	if (frameCount % Config::getInstance().getGsWorldUpdateFps() == 0) {
		computeFow();
		tick();
	}
	assertConsistiency();
	//if we're the server, send any updates needed to the client
	if (isNetworkServer()) {
		ServerInterface &si = *(NetworkManager::getInstance().getServerInterface());
		int64 oldest = Chrono::getCurMicros() - Config::getInstance().getNetMinFullUpdateInterval() * 1000;

		for (Factions::iterator f = factions.begin(); f != factions.end(); ++f) {
			const Units &units = f->getUnits();
			for (Units::const_iterator u = units.begin(); u != units.end(); ++u) {
				if ((*u)->getLastUpdated() < oldest) {
					si.unitUpdate(*u);
				}
			}
		}

		si.sendUpdates();
	}
}

void World::doKill(Unit *killer, Unit *killed) {
	scriptManager->onUnitDied(killed);
	int kills = 1 + killed->getPets().size();
	for (int i = 0; i < kills; i++) {
		stats.kill(killer->getFactionIndex(), killed->getFactionIndex());
		if (killer->isAlive() && killer->getTeam() != killed->getTeam()) {
			killer->incKills();
		}
	}

	if (killed->getCurrSkill()->getClass() != scDie) {
		killed->kill();
	}
	if ( !killed->isMobile() ) {
		unitUpdater.pathFinder->updateMapMetrics ( killed->getPos (), killed->getSize () );
	}
}

void World::tick() {
	if (!fogOfWarSmoothing) {
		minimap.updateFowTex(1.f);
	}

	//apply hack cleanup
	doHackyCleanUp();

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
			if (rt->getClass() == rcConsumable) {
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

Unit* World::findUnitById(int id) {
	for (int i = 0; i < getFactionCount(); ++i) {
		Faction* faction = getFaction(i);

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

	/*if (newPos == unit->getPos()) {
		return;
	}*/

	// FIXME: workaround for client problems trying to move units into occupied cells
	/*
	if (isNetworkClient()) {
		// make sure route is still clear

		if (!map.canMove(unit, unit->getPos(), newPos)) {
			fprintf(stderr, "Unit %d needs a new path and I'm stopping.\n", unit->getId());
			unit->getPath()->clear();
			unit->setCurrSkill(scStop);
			return;
		}
	}*/

	assert(unitUpdater.pathFinder->isLegalMove(unit, newPos));
	map.clearUnitCells(unit, unit->getPos());
	map.putUnitCells(unit, newPos);

	//water splash
	if (tileset.getWaterEffects() && unit->getCurrField() == FieldWalkable) {
		if (map.getCell(unit->getLastPos())->isSubmerged()) {
			for (int i = 0; i < 3; ++i) {
				waterEffects.addWaterSplash(
					Vec2f(unit->getLastPos().x + random.randRange(-0.4f, 0.4f), unit->getLastPos().y + random.randRange(-0.4f, 0.4f)));
			}
		}
	}
}

//returns the nearest unit that can store a type of resource given a position and a faction
Unit *World::nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt) {
	float currDist = infinity;
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
			Search::PathFinder *pf = Search::PathFinder::getInstance();
			pf->updateMapMetrics(unit->getPos(), unit->getSize());
		}
		scriptManager->onUnitCreated(unit);
		return unit->getId();
	} else{
		delete unit;
		return -3;
	}
}

int World::giveResource(const string &resourceName, int factionIndex, int amount){
	if ( factionIndex < 0 && factionIndex >= factions.size() ) {
		return -1;
	}
	Faction* faction= &factions[factionIndex];
	const ResourceType* rt;
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
		cmdType = unit->getType()->getFirstCtOfClass(ccMove);
	} else if ( commandName == "attack" ) {
		cmdType = unit->getType()->getFirstCtOfClass(ccAttack);
	} else if ( commandName == "harvest" ) {
		Resource *r = map.getTile(Map::toTileCoords(pos))->getResource();
		bool found = false;
		if ( ! unit->getType()->getFirstCtOfClass(ccHarvest) ) {
			return -2; // unit has no appropriate command
		}
		if ( !r ) {
			cmdType = unit->getType()->getFirstCtOfClass(ccHarvest);
		} else {
			for ( int i=0; i < unit->getType()->getCommandTypeCount(); ++i ) {
				cmdType = unit->getType()->getCommandType(i);
				if ( cmdType->getClass() == ccHarvest ) {
					HarvestCommandType *hct = (HarvestCommandType*)cmdType;
					if ( hct->canHarvest(r->getType()) ) {
						found = true;
						break;
					}
				}
			}
			if ( !found ) {
				cmdType = unit->getType()->getFirstCtOfClass(ccHarvest);
			}
		}
	} else if ( commandName == "patrol" ) {
		//TODO insert code
		return 0;
	} else {
		return -3; // unknown position command
	}
	if ( !cmdType ) {
		return -2; // unit has no appropriate command
	}
	if ( unit->giveCommand(new Command(cmdType, CommandFlags(), pos)) == crSuccess ) {
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
	if ( cmdName == "attack" ) {
		for ( int i=0; i < unit->getType()->getCommandTypeCount(); ++i ) {
			if ( unit->getType()->getCommandType ( i )->getClass () == ccAttack ) {
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
			if ( unit->getType()->getCommandType( i )->getClass () == ccRepair ) {
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
		//TODO add code
		return 0;
	} else {
		return -3;
	}

}
/** return 0 if ok, -1 if bad unit id, -2 if unit has no stop command, -3 illegal cmd name */
int World::giveStopCommand(int unitId, const string &cmdName) {
	Unit *unit = findUnitById(unitId);
	if ( unit == NULL ) {
		return -1;
	}
	if ( cmdName == "stop" ) {
		const StopCommandType *sct = (StopCommandType *)unit->getType()->getFirstCtOfClass(ccStop);
		if ( sct ) {
			if ( unit->giveCommand(new Command(sct, CommandFlags())) == crSuccess ) {
				return 0; // ok
			}
			return 1; // command fail
		} else {
			return -2;
		}
	} else if ( cmdName == "attack-stopped" ) {
		const AttackStoppedCommandType *asct = 
			(AttackStoppedCommandType *)unit->getType()->getFirstCtOfClass(ccAttackStopped);
		if ( asct ) {
			if ( unit->giveCommand(new Command(asct, CommandFlags())) == crSuccess ) {
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

/** #return 0 if ok, -1 if unitId invalid, -2 if unit cannot produce other unit,
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
		if(ct->getClass()==ccProduce){
			const ProduceCommandType *pct= static_cast<const ProduceCommandType*>(ct);
			if ( pct->getProducedUnit() == put ) {
				CommandResult res = unit->giveCommand(new Command(pct, CommandFlags()));
				if ( res == crSuccess ) {
					//theConsole.addLine("produce command success");
					return 0; //Ok
				//} else if ( res == crFailRes ) {
				//	theConsole.addLine("produce command failed, resources");
				//} else if ( res == crFailReqs ) {
				//	theConsole.addLine("produce command failed, requirements");
				//} else {
				//	theConsole.addLine("produce command failed");
				}
				return 1; // command fail	
			}
		} else if ( ct->getClass() == ccMorph ) { // Morph Command ?
			if ( ((MorphCommandType*)ct)->getMorphUnit()->getName() == producedName ) {
				// just record it for now, and keep looking for a Produce Command
				mct = (MorphCommandType*)ct; 
			}
			
		}
	}
	// didn't find a Produce Command, was there are Morph Command?
	if ( mct ) {
		CommandResult res = unit->giveCommand(new Command (mct, CommandFlags()));
		if ( res == crSuccess ) {
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
		if(ct->getClass()==ccUpgrade){
			const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
			if ( uct->getProducedUpgrade() == upgrd ) {
				if ( unit->giveCommand(new Command(uct, CommandFlags())) == crSuccess ) {
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
		int count= 0;
		for(int i= 0; i<faction->getUnitCount(); ++i){
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

			Tile *sc = map.getTile(i, j);

			sc->setFowTexCoord(Vec2f(
								   i / (next2Power(map.getTileW()) - 1.f),
								   j / (next2Power(map.getTileH()) - 1.f)));

			for (int k = 0; k < GameConstants::maxPlayers; k++) {
				sc->setExplored(k, false);
				sc->setVisible(k, 0);
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
					coord,
					texture);
			sc00->setTileTexCoord(coord);
			sc00->setTileTexture(texture);
		}
	}
}

//creates each faction looking at each faction name contained in GameSettings
void World::initFactionTypes() {
	Logger::getInstance().add("Faction types", true);

	if (gs.getFactionCount() > map.getMaxPlayers()) {
		throw runtime_error("This map only supports " + intToStr(map.getMaxPlayers()) + " players");
	}

	//create factions
	this->thisFactionIndex = gs.getThisFactionIndex();
	factions.resize(gs.getFactionCount());
	for (int i = 0; i < factions.size(); ++i) {
		const FactionType *ft= techTree.getFactionType(gs.getFactionTypeName(i));
		factions[i].init( ft, gs.getFactionControl(i), &techTree, i, gs.getTeam(i),
				gs.getStartLocationIndex(i), i==thisFactionIndex, gs.getDefaultResources ());
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
										"better map: " + unit->getType()->getName() + " Faction: " + intToStr(i));
				}
				if (unit->getType()->hasSkillClass(scBeBuilt)) {
					map.flatternTerrain(unit);
					unitUpdater.pathFinder->updateMapMetrics(unit->getPos(), unit->getSize());
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
					map.getTile(i, j)->setVisible(k, false);
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


#ifdef _GAE_DEBUG_EDITION_
#define _load_tex(i,f) \
	PFDebugTextures[i]=Renderer::getInstance().newTexture2D(rsGame);\
	PFDebugTextures[i]->setMipmap(false);\
	PFDebugTextures[i]->getPixmap()->load(f);

void World::loadPFDebugTextures() {
	char buff[128];
	for (int i = 0; i < 4; ++i) {
		sprintf(buff, "data/core/misc_textures/g%02d.bmp", i);
		_load_tex(i, buff);
	}
	for (int i = 13; i < 13 + 4; ++i) {
		sprintf(buff, "data/core/misc_textures/l%02d.bmp", i - 13);
		_load_tex(i, buff);
	}

	//_load_tex ( 4, "data/core/misc_textures/local0.bmp" );

	_load_tex(5, "data/core/misc_textures/path_start.bmp");
	_load_tex(6, "data/core/misc_textures/path_dest.bmp");
	_load_tex(7, "data/core/misc_textures/path_both.bmp");
	_load_tex(8, "data/core/misc_textures/path_return.bmp");
	_load_tex(9, "data/core/misc_textures/path.bmp");

	_load_tex(10, "data/core/misc_textures/path_node.bmp");
	_load_tex(11, "data/core/misc_textures/open_node.bmp");
	_load_tex(12, "data/core/misc_textures/closed_node.bmp");
}

#undef _load_tex
#endif


}}//end namespace

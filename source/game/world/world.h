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

#ifndef _GLEST_GAME_WORLD_H_
#define _GLEST_GAME_WORLD_H_

#include "vec.h"
#include "math_util.h"
#include "resource.h"
#include "tech_tree.h"
#include "tileset.h"
#include "console.h"
#include "map.h"
#include "scenario.h"
#include "minimap.h"
#include "logger.h"
#include "stats.h"
#include "time_flow.h"
#include "upgrade.h"
#include "water_effects.h"
#include "faction.h"
#include "unit_updater.h"
#include "random.h"
#include "game_constants.h"
#include "pos_iterator.h"

namespace Glest{ namespace Game{

using Shared::Math::Quad2i;
using Shared::Math::Rect2i;
using Shared::Util::Random;
using Glest::Game::Util::PosCircularIteratorFactory;

class Faction;
class Unit;
class Config;
class Game;
class GameSettings;
class ScriptManager;
namespace Search { class Cartographer; class RoutePlanner; }
using namespace Search;

// =====================================================
// 	class World
//
///	The game world: Map + Tileset + TechTree
// =====================================================

class World{
private:
	typedef vector<Faction> Factions;
	typedef std::map< string,set<string> > UnitTypes;

public:
	/** max radius to look when placing units */
	static const int generationArea= 100;
	/** height air units are drawn at. @todo this is not game data, probably belongs somewhere else */
	static const float airHeight;
	/** ??? anyone ? */
	static const int indirectSightRange= 5;

private:
	Map map;
	Tileset tileset;
	TechTree techTree;
	TimeFlow timeFlow;
	Scenario *scenario;
	Game &game;
	const GameSettings &gs;

	UnitUpdater unitUpdater;
	WaterEffects waterEffects;
	Minimap minimap;
	Stats stats;

	Factions factions;

	Random random;

	ScriptManager *scriptManager;
	Cartographer *cartographer;
	RoutePlanner *routePlanner;

	int thisFactionIndex;
	int thisTeamIndex;
	int frameCount;
	int nextUnitId;

	//config
	bool fogOfWar, shroudOfDarkness;
	int fogOfWarSmoothingFrameSkip;
	bool fogOfWarSmoothing;

	bool unfogActive;
	int unfogTTL;
	Vec4i unfogArea;

	static World *singleton;
	bool alive;

	UnitTypes unitTypes;

	Units newlydead;
	PosCircularIteratorFactory posIteratorFactory;

public:
	World(Game *game);
	~World();
	void end(); //to die before selection does

	static World& getInstance() { return *singleton; }
	static bool isConstructed() { return singleton != 0; }

	//get
	int getMaxPlayers() const						{return map.getMaxPlayers();}
	int getThisFactionIndex() const					{return thisFactionIndex;}
	int getThisTeamIndex() const					{return thisTeamIndex;}
	const Faction *getThisFaction() const			{return &factions[thisFactionIndex];}
	int getFactionCount() const						{return factions.size();}
	const Map *getMap() const 						{return &map;}
	const Tileset *getTileset() const 				{return &tileset;}
	const TechTree *getTechTree() const 			{return &techTree;}
	const Scenario* getScenario () const			{return scenario;}
	const TimeFlow *getTimeFlow() const				{return &timeFlow;}
	Tileset *getTileset() 							{return &tileset;}
	Map *getMap() 									{return &map;}
	Cartographer* getCartographer()					{return cartographer;}
	RoutePlanner* getRoutePlanner()					{return routePlanner;}
	const Faction *getFaction(int i) const			{return &factions[i];}
	Faction *getFaction(int i) 						{return &factions[i];}
	const Minimap *getMinimap() const				{return &minimap;}
	Stats &getStats() 								{return stats;}
	const WaterEffects *getWaterEffects() const		{return &waterEffects;}
	int getNextUnitId()								{return nextUnitId++;}
	int getFrameCount() const						{return frameCount;}
	static World *getCurrWorld()					{return singleton;}
	bool isAlive() const							{return alive;}
	const PosCircularIteratorFactory &getPosIteratorFactory() {return posIteratorFactory;}

	//init & load
	void init();
	void preload();
	bool loadTileset();
	bool loadTech();
	bool loadMap();
	bool loadScenario(const string &path);

	//misc
	void update();
	void moveUnitCells(Unit *unit);
	Unit* findUnitById(int id) const;
	const UnitType* findUnitTypeById(const FactionType* factionType, int id);
	bool placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated= false);
	Unit *nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt);
	void doKill(Unit *killer, Unit *killed);
	void assertConsistiency();
	void hackyCleanUp(Unit *unit);
	
	bool toRenderUnit(const Unit *unit) const {
		return map.getTile(Map::toTileCoords(unit->getCenteredPos()))->isVisible(thisTeamIndex)
			|| (unit->getCurrSkill()->getClass() == SkillClass::ATTACK
			&& map.getTile(Map::toTileCoords(unit->getTargetPos()))->isVisible(thisTeamIndex));
	}

	//scripting interface
	int createUnit(const string &unitName, int factionIndex, const Vec2i &pos);
	int givePositionCommand(int unitId, const string &commandName, const Vec2i &pos);
	int giveTargetCommand( int unitId, const string &commandName, int targetId);
	int giveStopCommand( int unitId, const string &commandName);
	int giveProductionCommand(int unitId, const string &producedName);
	int giveUpgradeCommand(int unitId, const string &upgradeName);
	int giveResource(const string &resourceName, int factionIndex, int amount);
	int getResourceAmount(const string &resourceName, int factionIndex);
	Vec2i getStartLocation(int factionIndex);
	Vec2i getUnitPosition(int unitId);
	int getUnitFactionIndex(int unitId);
	int getUnitCount(int factionIndex);
	int getUnitCountOfType(int factionIndex, const string &typeName);

	void unfogMap(const Vec4i &rect, int time);

#ifdef _GAE_DEBUG_EDITION_
	// these should be in DebugRenderer
	void loadPFDebugTextures();
	Texture2D *PFDebugTextures[18];
#endif

private:
	void initCells();
	void initSplattedTextures();
	void initFactionTypes();
	void initMinimap();
	void initUnits();
	void initMap();
	void initExplorationState();
	void initNetworkServer();

	//misc
	void updateClient();
	void updateEarthquakes(float seconds);
	void tick();
	void computeFow();
	void doUnfog();
	void exploreCells(const Vec2i &newPos, int sightRange, int teamIndex);
	void loadSaved(const XmlNode *worldNode);
	void moveAndEvict(Unit *unit, vector<Unit*> &evicted, Vec2i *oldPos);
	void doClientUnitUpdate(XmlNode *n, bool minor, vector<Unit*> &evicted, float nextAdvanceFrames);
	//NETWORK:bool isNetworkServer() {return NetworkManager::getInstance().isNetworkServer();}
	//NETWORK:bool isNetworkClient() {return NetworkManager::getInstance().isNetworkClient();}
	void doHackyCleanUp();
};

}}//end namespace

#endif

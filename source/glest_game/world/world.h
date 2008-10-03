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

namespace Glest{ namespace Game{

using Shared::Graphics::Quad2i;
using Shared::Graphics::Rect2i;
using Shared::Util::Random;

class Faction;
class Unit;
class Config;
class Game;
class GameSettings;

// =====================================================
// 	class World
//
///	The game world: Map + Tileset + TechTree
// =====================================================

class World{
private:
	typedef vector<Faction> Factions;

public:
	static const int generationArea= 100;
	static const float airHeight;
	static const int indirectSightRange= 5;

private:

	Map map;
	Tileset tileset;
	TechTree techTree;
	TimeFlow timeFlow;
	Game &game;
	const GameSettings &gs;

	UnitUpdater unitUpdater;
    WaterEffects waterEffects;
	Minimap minimap;
    Stats stats;	//BattleEnd will delete this object

	Factions factions;

	Random random;

	int thisFactionIndex;
	int thisTeamIndex;
	int frameCount;
	int nextUnitId;

	//config
	bool fogOfWar;
	int fogOfWarSmoothingFrameSkip;
	bool fogOfWarSmoothing;

	static World *singleton;
	bool alive;

public:
	World(Game *game);
	~World()										{singleton = NULL;}
	void end(); //to die before selection does

	//get
	int getMaxPlayers() const						{return map.getMaxPlayers();}
	int getThisFactionIndex() const					{return thisFactionIndex;}
	int getThisTeamIndex() const					{return thisTeamIndex;}
	const Faction *getThisFaction() const			{return &factions[thisFactionIndex];}
	int getFactionCount() const						{return factions.size();}
	const Map *getMap() const 						{return &map;}
	const Tileset *getTileset() const 				{return &tileset;}
	const TechTree *getTechTree() const 			{return &techTree;}
	const TimeFlow *getTimeFlow() const				{return &timeFlow;}
	Tileset *getTileset() 							{return &tileset;}
	Map *getMap() 									{return &map;}
	const Faction *getFaction(int i) const			{return &factions[i];}
	Faction *getFaction(int i) 						{return &factions[i];}
	const Minimap *getMinimap() const				{return &minimap;}
	const Stats *getStats() const					{return &stats;};
	Stats *getStats()								{return &stats;};
	const WaterEffects *getWaterEffects() const		{return &waterEffects;}
	int getNextUnitId()								{return nextUnitId++;}
	int getFrameCount() const						{return frameCount;}
	static World *getCurrWorld()					{return singleton;}
	bool isAlive() const							{return alive;}

	//init & load
	void init(const XmlNode *worldNode = NULL);
	void loadTileset(Checksum &checksum);
	void loadTech(Checksum &checksum);
	void loadMap(Checksum &checksum);
	void save(XmlNode *node) const;

	//misc
	void update();
	void updateClient();
	Unit* findUnitById(int id);
	const UnitType* findUnitTypeById(const FactionType* factionType, int id);
	bool placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated= false);
	void moveUnitCells(Unit *unit);
	bool toRenderUnit(const Unit *unit, const Quad2i &visibleQuad) const;
	bool toRenderUnit(const Unit *unit) const;
	Unit *nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt);
	void doKill(Unit *killer, Unit *killed);
	void assertConsistiency();

private:

	void initCells();
	void initSplattedTextures();
	void initFactionTypes();
	void initMinimap();
	void initUnits();
	void initMap();
	void initExplorationState();

	//misc
	void updateEarthquakes();
	void tick();
	void computeFow();
	void exploreCells(const Vec2i &newPos, int sightRange, int teamIndex);
	void loadSaved(const XmlNode *worldNode);
	void moveAndEvict(Unit *unit, vector<Unit*> &evicted, Vec2i *oldPos);
	void doClientUnitUpdate(XmlNode *n, bool minor, vector<Unit*> &evicted);
	bool isNetworkServer() {return NetworkManager::getInstance().isNetworkServer();}
	bool isNetworkClient() {return NetworkManager::getInstance().isNetworkClient();}
};

}}//end namespace

#endif

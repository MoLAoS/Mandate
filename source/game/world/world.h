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
#include "time_flow.h"
#include "upgrade.h"
#include "water_effects.h"
#include "faction.h"
#include "random.h"
#include "game_constants.h"
#include "pos_iterator.h"

#include "forward_decs.h"

using Shared::Math::Quad2i;
using Shared::Math::Rect2i;
using Shared::Util::Random;
using Glest::Util::PosCircularIteratorFactory;
using namespace Glest::Entities;
using namespace Glest::Script;
using namespace Glest::Gui;
using namespace Glest::Search;

namespace Glest { namespace Sim {

// =====================================================
// 	class World
//
///	The game world: Map + Tileset + TechTree
// =====================================================

class World {
private:
	typedef vector<Faction> Factions;
	typedef std::map< string,set<string> > UnitTypes;

public:
	/** max radius to look when placing units */
	static const int generationArea= 100;
	/** height air units are drawn at. @todo this is not game data, probably belongs somewhere else */
	static const float airHeight;
	/** additional sight range that allows to see/reveal tileset objects (but not units) */
	static const int indirectSightRange= 5;

private:
	Map map;
	Tileset tileset;
	TechTree techTree; // < to SimulationInterface ?
	TimeFlow timeFlow;
	Scenario *scenario; // < to SimulationInterface ?
	GameState &game;

	SimulationInterface *iSim;

	// WaterEffects == Eye candy, not game data, send to UserInterface
	WaterEffects waterEffects;

	Factions factions; // < to SimulationInterface ?
	Faction glestimals;

	Random random;

	Cartographer *cartographer;
	RoutePlanner *routePlanner;

	// to UserInterface, code using these should ultimately be reimplemented
	// by Connecting signals from 'this' factions/teams units to the UserInterface
	int thisFactionIndex;	
	int thisTeamIndex;

	int frameCount;
	
	//config
	bool fogOfWar, shroudOfDarkness;
	int fogOfWarSmoothingFrameSkip;  // GameGuiState
	bool fogOfWarSmoothing;			// GameGuiState

	// re-implement:
	//	struct MapReveal { ... };
	//	vector<MapReveal> mapReveals;
	bool unfogActive;
	int unfogTTL;
	Vec4i unfogArea;

	static World *singleton;
	bool alive;

	UnitTypes unitTypes; // ?? unit-types by faction-type ?

	PosCircularIteratorFactory posIteratorFactory;

	SkillTypeFactory	m_skillTypeFactory;
	CommandTypeFactory	m_commandTypeFactory;
	UpgradeTypeFactory	m_upgradeTypeFactory;
	UnitTypeFactory		m_unitTypeFactory;

	ProducibleTypeFactory m_producibleTypeFactory;

	ModelFactory		m_modelFactory;

public:
	World(SimulationInterface *iSim);
	~World();

	void save(XmlNode *node) const;
	
	static World& getInstance() { return *singleton; }
	static bool isConstructed() { return singleton != 0; }

	//get
	UnitTypeFactory& getUnitTypeFactory()			{return m_unitTypeFactory;}
	UpgradeTypeFactory& getUpgradeTypeFactory()		{return m_upgradeTypeFactory;}
	SkillTypeFactory& getSkillTypeFactory()			{return m_skillTypeFactory;}
	CommandTypeFactory& getCommandTypeFactory()		{return m_commandTypeFactory;}
	ProducibleTypeFactory& getProducibleFactory()	{return m_producibleTypeFactory;}
	ModelFactory& getModelFactory()					{return m_modelFactory;}

	int getMaxPlayers() const						{return map.getMaxPlayers();}
	int getThisFactionIndex() const					{return thisFactionIndex;}
	int getThisTeamIndex() const					{return thisTeamIndex;}
	const Faction *getThisFaction() const			{return factions.empty() ? 0 : &factions[thisFactionIndex];}
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
	const Faction *getGlestimals() const			{return &glestimals;}
	Faction *getGlestimals()						{return &glestimals;}
	const WaterEffects *getWaterEffects() const		{return &waterEffects;}
	int getFrameCount() const						{return frameCount;}
	static World *getCurrWorld()					{return singleton;}
	bool isAlive() const							{return alive;}
	const PosCircularIteratorFactory &getPosIteratorFactory() {return posIteratorFactory;}

	//init & load
	void init(const XmlNode *worldNode = NULL);
	void preload();
	bool loadTileset();
	bool loadTech();
	bool loadMap();
	bool loadScenario(const string &path);
	void activateUnits();

	// update
	void processFrame();

	//misc
	void moveUnitCells(Unit *unit);
	Unit* findUnitById(int id) const;
	const UnitType* findUnitTypeById(const FactionType* factionType, int id);
	bool placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated= false);
	Unit *nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt);
	void doKill(Unit *killer, Unit *killed);
	
	bool toRenderUnit(const Unit *unit) const {
		return unit->isVisible() && 
			(map.getTile(Map::toTileCoords(unit->getCenteredPos()))->isVisible(thisTeamIndex)
			|| (unit->getCurrSkill()->getClass() == SkillClass::ATTACK
			&& map.getTile(Map::toTileCoords(unit->getTargetPos()))->isVisible(thisTeamIndex)));
	}

	// attack
	void hit(Unit *attacker);
	void hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked = NULL);
	void damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, fixed distance);
	void damage(Unit *unit, int hp);

	// effects
	void applyEffects(Unit *source, const EffectTypes &effectTypes, const Vec2i &targetPos, Field targetField, int splashRadius);
	void applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *dest, fixed distance);
	void appyEffect(Unit *unit, Effect *effect);

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
	void initFactions();
	void initUnits();
	void initMap();
	void initExplorationState();

	//misc
	//void updateEarthquakes(float seconds);
	void tick();
	void computeFow();
	void doUnfog();
	void exploreCells(const Vec2i &newPos, int sightRange, int teamIndex);
	void loadSaved(const XmlNode *worldNode);
	void moveAndEvict(Unit *unit, vector<Unit*> &evicted, Vec2i *oldPos);
};

// =====================================================
//	class ParticleDamager
// =====================================================

class ParticleDamager {
public:
	UnitId attackerRef;
	const AttackSkillType* ast;
	World *world;
	const GameCamera *gameCamera;
	Vec2i targetPos;
	Field targetField;
	UnitId targetRef;

public:
	ParticleDamager(Unit *attacker, Unit *target, World *world, const GameCamera *gameCamera);
	void execute(ParticleSystem *particleSystem);
};


}}//end namespace

#endif

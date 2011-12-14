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
#include "type_factories.h"
#include "command.h"
#include "upgrade.h"

#include "forward_decs.h"

namespace Glest { namespace Sim {

// Shared
using Shared::Math::Quad2i;
using Shared::Math::Rect2i;
using Shared::Util::Random;

// Glest
using Util::PosCircularIteratorFactory;
using namespace Entities;
using namespace Script;
using namespace Gui;
using namespace Search;

// ===============================
// 	class MasterEntityFactory
// ===============================

typedef EntityFactory<Upgrade>     UpgradeFactory;
typedef EntityFactory<Command>     CommandFactory;
typedef EntityFactory<Effect>      EffectFactory;
typedef EntityFactory<Projectile>  ProjectileFactory;
typedef EntityFactory<MapObject>   MapObjectFactory;

class MasterEntityFactory {
protected:
	UnitFactory        m_unitFactory;
	UpgradeFactory     m_upgradeFactory;
	CommandFactory     m_commandFactory;
	EffectFactory      m_effectFactory;
	ProjectileFactory  m_projectileFactory;
	MapObjectFactory   m_mapObjectFactory;

public:
	MasterEntityFactory();

	Effect* newEffect(const EffectType *type, Unit *source, Effect *root, fixed strength,
			const Unit *recipient, const TechTree *tt) {
		Effect::CreateParams params(type, source, root, strength, recipient, tt);
		return m_effectFactory.newInstance(params);
	}

	Effect* newEffect(const XmlNode *node) { return m_effectFactory.newInstance(node); }

	Unit*	newUnit(const Vec2i &pos, const UnitType *type, Faction *faction, Map *map,
			CardinalDir face = CardinalDir::NORTH, Unit* master = NULL) {
		return m_unitFactory.newUnit(pos, type, faction, map, face, master);
	}

	Unit*	newUnit(const XmlNode *node, Faction *faction, Map *map, const TechTree *tt, bool putInWorld = true) {
		return m_unitFactory.newUnit(node, faction, map, tt, putInWorld);
	}

	Upgrade* newUpgrade(const UpgradeType *type, int factionNdx) {
		Upgrade::CreateParams params(type, factionNdx);
		return m_upgradeFactory.newInstance(params);
	}

	Upgrade* newUpgrade(const XmlNode *node, Faction *f) {
		Upgrade::LoadParams params(node, f);
		return m_upgradeFactory.newInstance(params);
	}

	Projectile* newProjectile(bool visible, const ParticleSystemBase &model, int particleCount= 1000) {
		Projectile::CreateParams params(visible, model, particleCount);
		return m_projectileFactory.newInstance(params);
	}

	Command* newCommand(CmdDirective archetype, CmdFlags flags, const Vec2i &pos = invalidPos, Unit *commandedUnit = NULL) {
		Command::CreateParamsArch params(archetype, flags, pos, commandedUnit);
		return m_commandFactory.newInstance(params);
	}
	Command* newCommand(const CommandType *type, CmdFlags flags, const Vec2i &pos = invalidPos, Unit *commandedUnit = NULL) {
		Command::CreateParamsPos params(type, flags, pos, commandedUnit);
		return m_commandFactory.newInstance(params);
	}
	Command* newCommand(const CommandType *type, CmdFlags flags, Unit *unit, Unit *commandedUnit = NULL) {
		Command::CreateParamsUnit params(type, flags, unit, commandedUnit);
		return m_commandFactory.newInstance(params);
	}
	Command* newCommand(const CommandType *type, CmdFlags flags, const Vec2i &pos, const ProducibleType *prodType, CardinalDir facing, Unit *commandedUnit = NULL) {
		Command::CreateParamsProd params(type, flags, pos, prodType, facing, commandedUnit);
		return m_commandFactory.newInstance(params);
	}
	Command* newCommand(const XmlNode *node, const UnitType *ut, const FactionType *ft) {
		Command::CreateParamsLoad params(node, ut, ft);
		return m_commandFactory.newInstance(params);
	}

	MapObject* newMapObject(MapObjectType *objType, const Vec2i &tilePos, const Vec3f &worldPos) {
		MapObject::CreateParams params(objType, tilePos, worldPos);
		return m_mapObjectFactory.newInstance(params);
	}

	Unit*	getUnit(int id) { return m_unitFactory.getInstance(id); }
	MapObject* getMapObj(int id) { return m_mapObjectFactory.getInstance(id); }

	void	deleteUnit(Unit *unit) { m_unitFactory.deleteUnit(unit); }
	void	deleteCommand(int id) { m_commandFactory.deleteInstance(id); }
	void	deleteCommand(const Command *c);
	void	deleteEffect(const Effect *e) { m_effectFactory.deleteInstance(e); }

	UnitFactory&        getUnitFactory() { return m_unitFactory; }
	UpgradeFactory&     getUpgradeFactory() { return m_upgradeFactory; }
	CommandFactory&     getCommandFactory() { return m_commandFactory; }
	EffectFactory&      getEffectFactory() { return m_effectFactory; }
	ProjectileFactory&  getProjectileFactory() { return m_projectileFactory; }
	MapObjectFactory&   getMapObjectFactory() { return m_mapObjectFactory; }
};

// =====================================================
// 	class World
//
///	The game world: Map + Tileset + TechTree
// =====================================================

class World : public MasterEntityFactory {
private:
	typedef vector<Faction> Factions;
	typedef std::map< string,set<string> > UnitTypes;

	typedef std::map<string, int>   CloakGroupIdMap;
	typedef std::map<int, string>   CloakGroupNameMap;

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

	SimulationInterface *m_simInterface;

	// WaterEffects == Eye candy, not game data, send to UserInterface
	WaterEffects waterEffects;

	Factions factions; // < to SimulationInterface ?
	Faction glestimals;

	Random random;

	Cartographer *cartographer;
	RoutePlanner *routePlanner;
	std::map<int, Surveyor*>	m_surveyorMap;

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

	ModelFactory			 m_modelFactory;

	// cloaking groups
	CloakGroupIdMap      m_cloakGroupIds;
	CloakGroupNameMap    m_cloakGroupNames;
	int m_cloakGroupIdCounter;

public:
	World(SimulationInterface *iSim);
	~World();

	void save(XmlNode *node) const;
	
	static World& getInstance() { return *singleton; }
	static bool isConstructed() { return singleton != 0; }

	ModelFactory& getModelFactory()						{return m_modelFactory;}

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
	Surveyor* getSurveyor(int ndx)					{return m_surveyorMap[ndx];}
	Surveyor* getSurveyor(Faction *f)				{return m_surveyorMap[f->getIndex()];}
	const Faction *getFaction(int i) const			{return &factions[i];}
	Faction *getFaction(int i) 						{return &factions[i];}
	const Faction *getGlestimals() const			{return &glestimals;}
	Faction *getGlestimals()						{return &glestimals;}
	const WaterEffects *getWaterEffects() const		{return &waterEffects;}
	int getFrameCount() const						{return frameCount;}
	static World *getCurrWorld()					{return singleton;}
	bool isAlive() const							{return alive;}
	const PosCircularIteratorFactory &getPosIteratorFactory() const {return posIteratorFactory;}

	//init & load
	void init(const XmlNode *worldNode = NULL);
	void preload();
	bool loadTileset();
	bool loadTech();
	bool loadMap();
	bool loadScenario(const string &path);
	void activateUnits(bool resumingGame);

	void initSurveyor(Faction *f);

	int getCloakGroupId(const string &name);
	const string& getCloakGroupName(int id);

	int getCloakGroupCount() const { return m_cloakGroupIdCounter; }

	// update
	void processFrame();

	//misc
	void moveUnitCells(Unit *unit);
	Unit* findUnitById(int id) { return getUnit(id); }
	const UnitType* findUnitTypeById(const FactionType* factionType, int id);
	bool placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated= false);
	Unit *nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt);
	void doKill(Unit *killer, Unit *killed);
	
	// attack
	void hit(Unit *attacker);
	void hit(Unit *attacker, const AttackSkillType* ast, const Vec2i &targetPos, Field targetField, Unit *attacked = NULL);
	void damage(Unit *attacker, const AttackSkillType* ast, Unit *attacked, fixed distance);
	void damage(Unit *unit, int hp);

	// effects
	void applyEffects(Unit *source, const EffectTypes &effectTypes, const Vec2i &targetPos, Field targetField, int splashRadius);
	void applyEffects(Unit *source, const EffectTypes &effectTypes, Unit *dest, fixed distance);
	void appyEffect(Unit *unit, Effect *effect);

	// scripting interface
	int createUnit(const string &unitName, int factionIndex, const Vec2i &pos, bool precise);
	int givePositionCommand(int unitId, const string &commandName, const Vec2i &pos);
	int giveBuildCommand(int unitId, const string &commandName, const string &buildType, const Vec2i &pos);
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
	void initSplattedTextures();
	void initFactions();
	void initUnits();
	void initExplorationState(bool thisFctionOnly = false);

	//misc
	//void updateEarthquakes(float seconds);
	void tick();
	void computeFow();
	void doUnfog();
	void exploreCells(const Vec2i &newPos, int sightRange, int teamIndex);
	void loadSaved(const XmlNode *worldNode);
	void moveAndEvict(Unit *unit, vector<Unit*> &evicted, Vec2i *oldPos);
	void updateUnits(const Faction *f);
};

// =====================================================
//	class ParticleDamager
// =====================================================

class ParticleDamager : public ProjectileCallback {
public:
	UnitId attackerRef;
	const AttackSkillType* ast;
	Vec2i targetPos;
	Field targetField;
	UnitId targetRef;

public:
	ParticleDamager(Unit *attacker, Unit *target);
	virtual void projectileArrived(ParticleSystem *particleSystem) override;
};

class SpellDeliverer : public ProjectileCallback {
public:
	UnitId  m_caster;
	UnitId  m_targetUnit;
	Vec2i   m_targetPos;
	Zone    m_targetZone;
	const CastSpellSkillType *m_castSkill;

public:
	SpellDeliverer(Unit *caster, UnitId targetId, Vec2i pos = invalidPos);
	virtual void projectileArrived(ParticleSystem *particleSystem) override;
};

}}//end namespace

#endif

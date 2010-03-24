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

#ifndef _GLEST_GAME_GAMECONSTANTS_H_
#define _GLEST_GAME_GAMECONSTANTS_H_

// The 'Globals'
#define theGame				(*Game::getInstance())
#define theWorld			(World::getInstance())
#define theMap				(*World::getInstance().getMap())
#define theCamera			(*Game::getInstance()->getGameCamera())
#define theGameSettings		(Game::getInstance()->getGameSettings())
#define theGui				(*Gui::getCurrentGui())
#define theConsole			(*Game::getInstance()->getConsole())
#define theConfig			(Config::getInstance())
#define theRoutePlanner		(*World::getInstance().getRoutePlanner())
#define theCartographer		(*World::getInstance().getCartographer())
#define theRenderer			(Renderer::getInstance())
#define theNetworkManager	(NetworkManager::getInstance())
#define theSoundRenderer	(SoundRenderer::getInstance())
#define theLogger			(Logger::getInstance())
#define theLang				(Lang::getInstance())

#if _GAE_DEBUG_EDITION_
#	define IF_DEBUG_EDITION(x) x
#else
#	define IF_DEBUG_EDITION(x)
#endif

#ifndef NDEBUG
#	define LOG(x) Logger::getInstance().add(x)
#else
#	define LOG(x) {}
#endif

#include "util.h"
using Shared::Util::EnumNames;

// =====================================================
//	Enumerations
// =====================================================
	 /* doxygen enum template
	  * <ul><li><b>VALUE</b> description</li>
	  *		<li><b>VALUE</b> description</li>
	  *		<li><b>VALUE</b> description</li></ul>
	  */

namespace Glest { namespace Game {

namespace Search {
	/** result set for path finding 
	  * <ul><li><b>ARRIVED</b> Arrived at destination (or as close as unit can get to target)</li>
	  *		<li><b>MOVING</b> On the way to destination</li>
	  *		<li><b>BLOCKED</b> path is blocked</li></ul>
	  */
	REGULAR_ENUM( TravelState, 
						ARRIVED, MOVING, BLOCKED, IMPOSSIBLE
				);

	/** result set for aStar() 
	  * <ul><li><b>FAILED</b> No path exists
	  *		<li><b>COMPLETE</b> complete path found</li>
	  *		<li><b>NODE_LIMIT</b> node limit reached, partial path available</li>
	  *		<li><b>TIME_LIMIT</b> search ongoing (time limit reached)</li></ul>
	  */
	REGULAR_ENUM( AStarResult, 
					FAILURE, COMPLETE, NODE_LIMIT, TIME_LIMIT
				);

	REGULAR_ENUM( HAAStarResult,
					FAILURE, COMPLETE, START_TRAP, GOAL_TRAP
				);

	/** Specifies a 'space' to search 
	  * <ul><li><b>CELLMAP</b> search on cell map</li>
	  *		<li><b>TILEMAP</b> search on tile map</li></ul>
	  */
	REGULAR_ENUM( SearchSpace,
						CELLMAP, TILEMAP, CLUSTERMAP
				);

	/** The cardinal and ordinal directions enumerated for convenience */
	REGULAR_ENUM( OrdinalDir,
						NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST
				);

	REGULAR_ENUM( CardinalDir, 
						NORTH, EAST, SOUTH, WEST 
				);

} // end namespace Search


REGULAR_ENUM( NetworkMessageType,
				NO_MSG,
				INTRO,
				PING,
				AI_SYNC,
				READY,
				LAUNCH,
				COMMAND_LIST,
				TEXT,
				LOG_UNIT,
				QUIT
			)

/** The control type of a 'faction' (aka, player)
  * <ul><li><b>CLOSED</b> Slot closed, no faction</li>
  *		<li><b>CPU_EASY</b> CPU easy player</li>
  *		<li><b>CPU</b> CPU player</li>
  *		<li><b>CPU_ULTRA</b> Cheating CPU player</li>
  *		<li><b>CPU_MEGA</b> Extreemly cheating CPU player</li>
  *		<li><b>NETWORK</b> Network player</li>
  *		<li><b>HUMAN</b> Local Player</li></ul>
  */
STRINGY_ENUM( ControlType, 
					CLOSED, CPU_EASY, CPU, CPU_ULTRA, CPU_MEGA, NETWORK, HUMAN 
			);

/** fields of movement
  * <ul><li><b>LAND</b> land traveller</li>
  *		<li><b>AIR</b> flying units</li>
  *		<li><b>ANY_WATER</b> travel on water only</li>
  *		<li><b>DEEP_WATER</b> travel in deep water only</li>
  *		<li><b>AMPHIBIOUS</b> land or water</li></ul>
  */
STRINGY_ENUM( Field,
				   LAND,
				   AIR,
				   ANY_WATER,
				   DEEP_WATER,
				   AMPHIBIOUS
			);

/** surface type for cells
  * <ul><li><b>LAND</b> land (above sea level)</li>
  *		<li><b>FORDABLE</b> shallow (fordable) water</li>
  *		<li><b>DEEP_WATER</b> deep (non-fordable) water</li></ul>
  */
REGULAR_ENUM( SurfaceType,
				   LAND, 
				   FORDABLE, 
				   DEEP_WATER
			);

/** zones of unit occupance
  * <ul><li><b>SURFACE_PROP</b> A surface prop, not used yet.</li>
  *		<li><b>SURFACE</b> the surface zone</li>
  *		<li><b>AIR</b> the air zone</li></ul>
  */
STRINGY_ENUM( Zone,
				   SURFACE_PROP,
				   LAND,
				   AIR
			);

/** unit properties
  * <ul><li><b>BURNABLE</b> can catch fire.</li>
  *		<li><b>ROTATED_CLIMB</b> currently deprecated</li>
  *		<li><b>WALL</b> is a wall</li></ul>
  */
STRINGY_ENUM( Property,
					BURNABLE,
					ROTATED_CLIMB,
					WALL
			);

/** Whether the effect is detrimental, neutral or benificial */
STRINGY_ENUM( EffectBias,
	DETRIMENTAL,
	NEUTRAL,
	BENIFICIAL
)

/**
 * How an attempt to apply multiple instances of an effect should be
 * handled
 */
STRINGY_ENUM( EffectStacking,
	STACK,
	EXTEND,
	OVERWRITE,
	REJECT
)

/** effects flags
  * effect properties:
  * <ul><li><b>ALLY</b> effects allies.</li>
  *		<li><b>FOE</b> effects foes.</li>
  *		<li><b>NO_NORMAL_UNITS</b> doesn't effects normal units.</li>
  *		<li><b>BUILDINGS</b> effects buildings.</li>
  *		<li><b>PETS_ONLY</b> only effects pets of the originator.</li>
  *		<li><b>NON_LIVING</b> .</li>
  *		<li><b>SCALE_SPLASH_STRENGTH</b> decrease strength when applied from splash.</li>
  *		<li><b>ENDS_WITH_SOURCE</b> ends when the unit causing the effect dies.</li>
  *		<li><b>RECOURsE_ENDS_WITH_ROOT</b> ends when root effect ends (recourse effects only).</li>
  *		<li><b>PERMANENT</b> the effect has an infinite duration.</li>
  *		<li><b>ALLOW_NEGATIVE_SPEED</b> .</li>
  *		<li><b>TICK_IMMEDIATELY</b> .</li></ul>
  * AI hints:
  *	<ul><li><b>AI_DAMAGED</b> use on damaged units (benificials only).</li>
  *		<li><b>AI_RANGED</b> use on ranged attack units.</li>
  *		<li><b>AI_MELEE</b> use on melee units.</li>
  *		<li><b>AI_WORKER</b> use on worker units.</li>
  *		<li><b>AI_BUILDING</b> use on buildings.</li>
  *		<li><b>AI_HEAVY</b> perfer to use on heavy units.</li>
  *		<li><b>AI_SCOUT</b> useful for scouting units.</li>
  *		<li><b>AI_COMBAT</b> don't use outside of combat (benificials only).</li>
  *		<li><b>AI_SPARINGLY</b> use sparingly.</li>
  *		<li><b>AI_LIBERALLY</b> use liberally.</li></ul>
  */
STRINGY_ENUM( EffectTypeFlag,
					ALLY,
					FOE,
					NO_NORMAL_UNITS,
					BUILDINGS,
					PETS_ONLY,
					NON_LIVING,
					SCALE_SPLASH_STRENGTH,
					ENDS_WITH_SOURCE,
					RECOURSE_ENDS_WITH_ROOT,
					PERMANENT,
					ALLOW_NEGATIVE_SPEED,
					TICK_IMMEDIATELY,
					AI_DAMAGED,
					AI_RANGED,
					AI_MELEE,
					AI_WORKER,
					AI_BUILDING,
					AI_HEAVY,
					AI_SCOUT,
					AI_COMBAT,
					AI_USE_SPARINGLY,
					AI_USE_LIBERALLY
			);

/** attack skill preferences
  */
STRINGY_ENUM( AttackSkillPreference,
					WHENEVER_POSSIBLE,
					AT_MAX_RANGE,
					ON_LARGE,
					ON_BUILDING,
					WHEN_DAMAGED
			);

/** unit classes
  */
REGULAR_ENUM( UnitClass,
					WARRIOR,
					WORKER,
					BUILDING
			);

/** command result set
  * <ul><li><b>SUCCESS</b> command succeeded.</li>
  *		<li><b>FAIL_RESOURCES</b> failed, resource requirements not met.</li>
  *		<li><b>FAIL_REQUIREMENTS</b> failed, unit/upgrade requirements not met.</li>
  *		<li><b>FAIL_PET_LIMIT</b> failed, would exceed pet limit.</li>
  *		<li><b>FAIL_UNDEFINED</b> failed.</li>
  *		<li><b>SOME_FAILED</b> partially failed.</li></ul>
  */
REGULAR_ENUM( CommandResult,
					SUCCESS,
					FAIL_RESOURCES,
					FAIL_REQUIREMENTS,
					FAIL_PET_LIMIT,
					FAIL_UNDEFINED,
					SOME_FAILED
			);

/** interesting unit types
  */
REGULAR_ENUM( InterestingUnitType,
					IDLE_BUILDER,
					IDLE_HARVESTER,
					IDLE_WORKER,
					IDLE_REPAIRER,
					IDLE_RESTORER,
					BUILT_BUILDING,
					PRODUCER,
					IDLE_PRODUCER,
					DAMAGED,
					STORE
			);

/** upgrade states
  */
REGULAR_ENUM( UpgradeState,
					UPGRADING,
					UPGRADED
			);

/** command classes
  */
STRINGY_ENUM( CommandClass,
					STOP,
					MOVE,
					ATTACK,
					ATTACK_STOPPED,
					BUILD,
					HARVEST,
					REPAIR,
					PRODUCE,
					UPGRADE,
					MORPH,
					CAST_SPELL,
					GUARD,
					PATROL,
					SET_MEETING_POINT,
					NULL_COMMAND
			);

/** click count
  */
REGULAR_ENUM( Clicks,
					ONE,
					TWO
			);
			
/** resource classes
  * <ul><li><b>TECH</b> resource is defined in tech tree.</li>
  *		<li><b>TILESET</b> resource is defined in tileset.</li>
  *		<li><b>STATIC</b> resource is static.</li>
  *		<li><b>CONSUMABLE</b> resource is consumable.</li></ul>
  */
STRINGY_ENUM( ResourceClass,
					TECHTREE,
					TILESET,
					STATIC,
					CONSUMABLE
			);

/** skill classes
  */
STRINGY_ENUM( SkillClass,
					STOP,
					MOVE,
					ATTACK,
					BUILD,
					HARVEST,
					REPAIR,
					BE_BUILT,
					PRODUCE,
					UPGRADE,
					MORPH,
					DIE,
					CAST_SPELL,
					FALL_DOWN,
					GET_UP,
					WAIT_FOR_SERVER
			);

/** weather set
  * <ul><li><b>SUNNY</b> Sunny weather, no weather particle system.</li>
  *		<li><b>RAINY</b> Rainy weather..</li>
  *		<li><b>SNOWY</b> Snowy.</li></ul>
  */
REGULAR_ENUM( Weather,
					SUNNY,
					RAINY,
					SNOWY
			);

/** command properties
  */
REGULAR_ENUM( CommandProperties,
					QUEUE,
					AUTO,
					DONT_RESERVE_RESOURCES,
					AUTO_REPAIR_ENABLED
			);

/** Command Archetypes
  */
REGULAR_ENUM( CommandArchetype,
					GIVE_COMMAND,
					CANCEL_COMMAND,
					SET_MEETING_POINT
				//	SET_AUTO_REPAIR
			);

// =====================================================
//	class GameConstants
// =====================================================

class GameConstants{
public:
	static const int maxPlayers= 4;
	static const int serverPort= 61357;
//	static const int updateFps= 40;
	static const int cameraFps= 100;
	static const int networkFramePeriod= 10;
	static const int networkExtraLatency= 200;
};

}}//end namespace

#endif

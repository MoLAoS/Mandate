// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V2, see source/licence.txt
// ==============================================================

#ifndef _GAME_SIMULATION_CONSTANTS_
#define _GAME_SIMULATION_CONSTANTS_

#include "util.h"
using Shared::Util::EnumNames;

namespace Glest { namespace Sim {

/** The Game Role of 'this' player
  * <ul><li><b>LOCAL</b> Local game, no network.</li>
  *		<li><b>SERVEER</b> Network server.</li>
  *		<li><b>CLIENT</b> Network client.</li>
  * </ul>
  */
STRINGY_ENUM( GameRole, 
	LOCAL,
	SERVER,
	CLIENT
)

STRINGY_ENUM( GameSpeed,
	PAUSED,
	SLOWEST,
	SLOW,
	NORMAL,
	FAST,
	FASTEST
)

/** The control type of a 'faction' (aka, player)
  * <ul><li><b>CLOSED</b> Slot closed, no faction</li>
  *		<li><b>CPU_EASY</b> CPU easy player</li>
  *		<li><b>CPU</b> CPU player</li>
  *		<li><b>CPU_ULTRA</b> Cheating CPU player</li>
  *		<li><b>CPU_MEGA</b> Extremely cheating CPU player</li>
  *		<li><b>NETWORK</b> Network player</li>
  *		<li><b>HUMAN</b> Local Player</li></ul>
  */
STRINGY_ENUM( ControlType, 
	CLOSED,
	HUMAN,
	NETWORK,
	CPU_EASY,
	CPU,
	CPU_ULTRA,
	CPU_MEGA
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
STRINGY_ENUM( SurfaceType,
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
//	SURFACE_PROP,
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

/** unit classes [could be WRAPPED_ENUM in Unit ?]
  */
STRINGY_ENUM( UnitClass,
	WARRIOR,
	WORKER,
	BUILDING,
	CARRIER
);

/** command result set [could be WRAPPED_ENUM in Command ?? or will we want this in debug ed?]
  * <ul><li><b>SUCCESS</b> command succeeded.</li>
  *		<li><b>FAIL_BLOCKED</b> failed, an obstacle prevents the command occuring.</li>
  *		<li><b>FAIL_RESOURCES</b> failed, resource requirements not met.</li>
  *		<li><b>FAIL_REQUIREMENTS</b> failed, unit/upgrade requirements not met.</li>
  *		<li><b>FAIL_PET_LIMIT</b> failed, would exceed pet limit.</li>
  *		<li><b>FAIL_UNDEFINED</b> failed.</li>
  *		<li><b>SOME_FAILED</b> partially failed.</li></ul>
  */
STRINGY_ENUM( CmdResult,
	SUCCESS,
	FAIL_BLOCKED,
	FAIL_RESOURCES,
	FAIL_REQUIREMENTS,
	FAIL_PET_LIMIT,
	FAIL_LOAD_LIMIT,
	FAIL_INVALID_LOAD,
	FAIL_UNDEFINED,
	SOME_FAILED
);

/** interesting unit types [not WRAPPED, will want stringy version in debug edition]
  */
STRINGY_ENUM( InterestingUnitType,
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

}}

#endif
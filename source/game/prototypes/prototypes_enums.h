// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V2, see source/licence.txt
// ==============================================================

#ifndef _GAME_PROTOTYPES_CONSTANTS_
#define _GAME_PROTOTYPES_CONSTANTS_

#include "util.h"
using Shared::Util::EnumNames;

namespace Glest { namespace ProtoTypes {

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
	LOAD,
	UNLOAD,
	DIE		// == 10, == 11 skill classes
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
	LOAD,
	UNLOAD,
	NULL_COMMAND
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
  *		<li><b>RECOURSE_ENDS_WITH_ROOT</b> ends when root effect ends (recourse effects only).</li>
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

}}

#endif

// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_MANDATE_AI_ENUMS_H_
#define _GLEST_MANDATE_AI_ENUMS_H_

#include "util.h"
using Shared::Util::EnumNames;

namespace Glest { namespace Plan {

STRINGY_ENUM( Goal,
	EMPTY,
	LIVE,
	BUILD,
	COLLECT,
	TRANSPORT,
	TRADE,
	PROCURE,
	EXPLORE,
	SHOP,
	DEMOLISH,
	RAID,
	HUNT,
	PATROL,
	ATTACK,
	DEFEND,
	BUFF,
	HEAL,
	SPELL,
	REST
)

}}

#endif

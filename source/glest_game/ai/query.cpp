// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "query.h"

#include "command.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
// 	class QueryCriteria::InvalidQueryException
// =====================================================
inline string QueryCriteria::InvalidQueryException::desc(
		const char* function, bool set, QueryConstraint field) {
	string str = function;
	str += " failed because ";
	str += QueryCriteria::getName(field);
	str += " is ";
	str += (set ? "already" : "not");
	str += " set";
	return str;
}

// don't inline ctor because we want asserts to be small
QueryCriteria::InvalidQueryException::InvalidQueryException(const char* function, bool set, QueryConstraint field) :
		std::runtime_error(desc(function, set, field)) {
}

// =====================================================
// 	class QueryCriteria
// =====================================================

const char *QueryCriteria::names[qcCount] = {
	"qcWithAbility",
	"qcAbilityIsReady",
	"qcAbilityHasField",
	"qcAbilityIsAe",
	"qcAbilityMinAeRadius",
	"qcAbilityMaxAeRadius",
	"qcWithCurrentCommand",
	"qcBestDamageMultiplier",
	"qcWithMinRange",
	"qcWithMaxRange",
	"qcNearest",
	"qcFurthest",
	"qcIdleOnly",
	"qcIgnoreAutoCmd",
	"qcMinHp",
	"qcMinHpPercentage",
	"qcMaxHp",
	"qcMaxHpPercentage",
	"qcMinEp",
	"qcMinEpPercentage",
	"qcMaxEp",
	"qcMaxEpPercentage",
	"qcHasDetrimentalEffect",
	"qcHasBenificialEffect"
};

bool QueryCriteria::meetsCriteria(Unit *unit) {
	if(get(qcWithAbility)) {
		const UnitType *ut = unit->getType();
		if(!ut->hasCommandClass(ability)) {
			return false;
		}

		if(get(qcAbilityIsReady) || get(qcWithMinRange) || get(qcWithMaxRange)) {
			const UnitType::CommandTypes &commandTypes = ut->getCommandTypes();
			bool good = false;

			for(UnitType::CommandTypes::const_iterator i = commandTypes.begin();
					i != commandTypes.end(); ++i) {
				if((*i)->getClass() != ability) {
					continue;
				}

				if(get(qcAbilityIsReady) && !unit->isReady(*i)) {
					continue;
				}
				good = true;
				break;
			}

			if(!good) {
				return false;
			}
		}
	}

	if(get(qcHasDetrimentalEffect)
			&& !unit->getEffects().hasEffectOfBias(EffectType::ebDetrimental)) {
		return false;
	}

	if(get(qcHasBenificialEffect)
			&& !unit->getEffects().hasEffectOfBias(EffectType::ebBenificial)) {
		return false;
	}

	if(get(qcWithCurrentCommand)) {
		Command *cmd = unit->getCurrCommand();
		if(!cmd || cmd->getCommandType()->getClass() != currentCommand
				|| get(qcIgnoreAutoCmd) && cmd->isAuto()) {
			return false;
		}
	}

	if(get(qcIdleOnly)) {
		Command *cmd = unit->getCurrCommand();
		if(!cmd) {
			const CommandType *ct = cmd->getCommandType();
			if(ct->getClass() != ccStop && get(qcIgnoreAutoCmd) && !cmd->isAuto()) {
				return false;
			}
		}
	}

	if(get(qcMinHp) && unit->getHp() <= _minHp) {
		return false;
	}

	if(get(qcMinHpPercentage) && unit->getHpRatio() <= _minHpPercentage) {
		return false;
	}

	if(get(qcMaxHp) && unit->getHp() > _maxHp) {
		return false;
	}

	if(get(qcMaxHpPercentage) && unit->getHpRatio() > _minHpPercentage) {
		return false;
	}


	if(get(qcMinEp) && unit->getEp() <= _minEp) {
		return false;
	}

	if(get(qcMinEpPercentage) && unit->getEpRatio() <= _minEpPercentage) {
		return false;
	}

	if(get(qcMaxEp) && unit->getEp() > _maxEp) {
		return false;
	}

	if(get(qcMaxEpPercentage) && unit->getEpRatio() > _minEpPercentage) {
		return false;
	}

/* TODO: unmanaged criteria
	qcBestDamageMultiplier,	// has the best damage multiplier when attacking ref unit
	qcWithMinRange,			// specifies a min range of ability, valid only if withAbility
	qcWithMaxRange,			// specifies a max range of ability, valid only if withAbility
	qcNearest,				// nearest to specified pos
	qcFurthest,				// furthest from specified pos
	qcHasAEAttack,			// has an attack that effects an area
	qcAeAttackMinRadius,	// a minimum effect radius for AE attack is specified
	qcAeAttackMaxRadius,	// a maximum effect radius for AE attack is specified
*/
	return true;
}

}}//end namespace

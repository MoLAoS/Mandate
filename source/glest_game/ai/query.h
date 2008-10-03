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

#ifndef _GLEST_GAME_QUERY_H_
#define _GLEST_GAME_QUERY_H_

#include <cassert>
#include <stdexcept>

#include "unit.h"
#include "flags.h"

namespace Glest { namespace Game {

enum QueryConstraint {
	qcWithAbility,			// has the specified ability
	qcAbilityIsReady,		// the ability specified in withAbility is ready for use
	qcAbilityHasField,
	qcAbilityIsAe,			// the ability effects more than one target (area effect)
	qcAbilityMinAeRadius,	// a minimum effect radius for AE ability is specified
	qcAbilityMaxAeRadius,	// a maximum effect radius for AE ability is specified

	qcWithCurrentCommand,	// is executing the specified command
	qcBestDamageMultiplier,	// has the best damage multiplier when attacking ref unit
	qcWithMinRange,			// specifies a min range of ability, valid only if withAbility
	qcWithMaxRange,			// specifies a max range of ability, valid only if withAbility
	qcNearest,				// nearest to specified pos
	qcFurthest,				// furthest from specified pos
	qcIdleOnly,				// only idle units
	qcIgnoreAutoCmd,		// ignore auto commanded units (count as idle)
	qcMinHp,
	qcMinHpPercentage,
	qcMaxHp,
	qcMaxHpPercentage,
	qcMinEp,
	qcMinEpPercentage,
	qcMaxEp,
	qcMaxEpPercentage,
	qcHasDetrimentalEffect,	// has an effect flaged as detrimental
	qcHasBenificialEffect,	// has an effect flaged as benificial

	qcCount
};

/** Class for performing advanced queries on a collection of units */
class QueryCriteria : protected Flags<QueryConstraint, qcCount, unsigned int>  {
public:
	class InvalidQueryException : public std::runtime_error {
	public:
		InvalidQueryException(const string &msg) : std::runtime_error(msg) {}
		InvalidQueryException(const char* function, bool set, QueryConstraint field);

	private:
		static string desc(const char* function, bool set, QueryConstraint field);
	};

private:
	static const char *names[qcCount];

	CommandClass ability;
	int minRange;
	int maxRange;
	Field abilityField;
	int _abilityMinAeRadius;
	int _abilityMaxAeRadius;

	CommandClass currentCommand;
	const Unit* refUnit;
	Vec2f pos;
	int _minHp;
	float _minHpPercentage;
	int _maxHp;
	float _maxHpPercentage;
	int _minEp;
	float _minEpPercentage;
	int _maxEp;
	float _maxEpPercentage;

public:
	QueryCriteria();

	bool meetsCriteria(Unit *unit);

	#define qcAssertSet(a) QueryCriteria::assertSet(__PRETTY_FUNCTION__, a)
	#define qcAssertClear(a) QueryCriteria::assertClear(__PRETTY_FUNCTION__, a)

	// Ability criteria (available command types)

	/** has a command of the specified class */
	QueryCriteria &withAbility(CommandClass ability) {
		qcAssertClear(qcWithAbility);
		set(qcWithAbility, true);
		this->ability = ability;
		return *this;
	}

	/** the ability previously specified in withAbility is ready for use */
	QueryCriteria &abilityIsReady() {
		qcAssertSet(qcWithAbility);
		set(qcWithAbility, true);
		return *this;
	}

	/** the ability (attack, move, etc.) has the specfied field */
	QueryCriteria &abilityHasField(Field field) {
		qcAssertSet(qcWithAbility);
		qcAssertClear(qcAbilityHasField);
		set(qcAbilityHasField, true);
		this->abilityField = field;
		return *this;
	}

	/** has an area of effect attack, e.g., splash damage */
	QueryCriteria &abilityIsAreaEffect() {
		qcAssertSet(qcWithAbility);
		set(qcAbilityIsAe, true);
		return *this;
	}

	/** the area effect ability previously specified has the specified minimum radius */
	QueryCriteria &abilityMinAeRadius(int radius) {
		qcAssertSet(qcWithAbility);
		qcAssertSet(qcAbilityIsAe);
		qcAssertClear(qcAbilityMinAeRadius);
		set(qcAbilityMinAeRadius, true);
		this->_abilityMinAeRadius = radius;
		return *this;
	}

	/** the area effect ability previously specified has the specified maximum radius */
	QueryCriteria &abilityMaxAeRadius(int radius) {
		qcAssertSet(qcWithAbility);
		qcAssertSet(qcAbilityIsAe);
		qcAssertClear(qcAbilityMaxAeRadius);
		set(qcAbilityMaxAeRadius, true);
		this->_abilityMaxAeRadius = radius;
		return *this;
	}


	// other criteria

	/** is executing a command of the specified class */
	QueryCriteria &withCurrentCommand(CommandClass currentCommand) {
		qcAssertClear(qcWithCurrentCommand);
		set(qcWithCurrentCommand, true);
		this->currentCommand = currentCommand;
		return *this;
	}

	/** of the results, return the one with the highest damage multiplier for the given target unit */
	QueryCriteria &bestDamageMultiplier(const Unit *refUnit) {
		qcAssertClear(qcBestDamageMultiplier);
		set(qcBestDamageMultiplier, true);
		this->refUnit = refUnit;
		return *this;
	}

	/** the pre-specified ability has a range of at least minRange */
	QueryCriteria &withMinRange(int minRange) {
		qcAssertSet(qcWithAbility);
		qcAssertClear(qcWithMinRange);
		set(qcWithMinRange, true);
		this->minRange = minRange;
		return *this;
	}

	/** the pre-specified ability has a range of at most maxRange */
	QueryCriteria &withMaxRange(int maxRange) {
		qcAssertSet(qcWithAbility);
		qcAssertClear(qcWithMaxRange);
		set(qcWithMaxRange, true);
		this->maxRange = maxRange;
		return *this;
	}

	/** if more than one unit found, select the unit that is nearest to pos */
	QueryCriteria &nearest(const Vec2f &pos) {
		qcAssertClear(qcNearest);
		qcAssertClear(qcFurthest);
		set(qcNearest, true);
		this->pos = pos;
		return *this;
	}

	/** if more than one unit found, select the unit that is furthest from pos */
	QueryCriteria &furthest(const Vec2f &pos) {
		qcAssertClear(qcNearest);
		qcAssertClear(qcFurthest);
		set(qcFurthest, true);
		this->pos = pos;
		return *this;
	}

	/** select only idle units */
	QueryCriteria &idleOnly() {
		qcAssertClear(qcIdleOnly);
		set(qcIdleOnly, true);
		return *this;
	}

	/**
	 * If selecting only idle units, consider units with an auto-command as idle.  If looking for
	 * units with a specified currentCommand, ignore units who have an auto-command.
	 */
	QueryCriteria &ignoreAutoCmd() {
		qcAssertClear(qcIgnoreAutoCmd);
		set(qcIgnoreAutoCmd, true);
		return *this;
	}

	/** has at least the specified number of hit points */
	QueryCriteria &minHp(int hp) {
		qcAssertClear(qcMinHp);
		set(qcMinHp, true);
		this->_minHp = hp;
		return *this;
	}

	/** has at least the specified percentage of their max hit points */
	QueryCriteria &minHpPercentage(float pct) {
		qcAssertClear(qcMinHpPercentage);
		set(qcMinHpPercentage, true);
		this->_minHpPercentage = pct;
		return *this;
	}

	/** has at most the specified number of hit points */
	QueryCriteria &maxHp(int hp) {
		qcAssertClear(qcMaxHp);
		set(qcMaxHp, true);
		this->_maxHp = hp;
		return *this;
	}

	/** has at most the specified percentage of their max hit points */
	QueryCriteria &maxHpPercentage(float pct) {
		qcAssertClear(qcMaxHpPercentage);
		set(qcMaxHpPercentage, true);
		this->_maxHpPercentage = pct;
		return *this;
	}

	/** has at least the specified number of energy points */
	QueryCriteria &minEp(int ep) {
		qcAssertClear(qcMinEp);
		set(qcMinEp, true);
		this->_minEp = ep;
		return *this;
	}

	/** has at least the specified percentage of their max energy points */
	QueryCriteria &minEpPercentage(float pct) {
		qcAssertClear(qcMinEpPercentage);
		set(qcMinEpPercentage, true);
		this->_minEpPercentage = pct;
		return *this;
	}

	/** has at most the specified number of energy points */
	QueryCriteria &maxEp(int ep) {
		qcAssertClear(qcMaxEp);
		set(qcMaxEp, true);
		this->_maxEp = ep;
		return *this;
	}

	/** has at most the specified percentage of their max energy points */
	QueryCriteria &maxEpPercentage(float pct) {
		qcAssertClear(qcMaxEpPercentage);
		set(qcMaxEpPercentage, true);
		this->_maxEpPercentage = pct;
		return *this;
	}

	/** has at least one detrimental effect */
	QueryCriteria &hasDetrimentalEffect() {
		set(qcHasDetrimentalEffect, true);
		return *this;
	}

	/** has at least one benificial effect */
	QueryCriteria &hasBenificialEffect() {
		set(qcHasBenificialEffect, true);
		return *this;
	}

	#undef qcAssertSet
	#undef qcAssertClear


	static const char* getName(QueryConstraint qc) {
		assert((int)qc >= 0 && (int)qc < qcCount);
		return names[qc];
	}

private:
	void assertSet(const char* function, QueryConstraint field) {
		bool value = get(field);

		assert(value);
		if(!value) {
			throw InvalidQueryException(function, true, field);
		}
	}

	void assertClear(const char* function, QueryConstraint field) {
		bool value = get(field);

		assert(!value);
		if(value) {
			throw InvalidQueryException(function, false, field);
		}
	}

};

}}//end namespace

#endif

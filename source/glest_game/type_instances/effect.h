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

#ifndef GLEST_GAMEEFFECT_H
#define GLEST_GAMEEFFECT_H

#include "effect_type.h"
#include "vec.h"
#include "unit_reference.h"

namespace Glest{ namespace Game{

class Unit;
class UnitReference;

// ===============================
// 	class Effect
// ===============================

/**
 * An effect, usually temporary, that modifies the stats,
 * regeneration/degeneration or other attributes of a unit.
 * TODO: Implement lighting, particles & sound
 * TODO: Better implementation of effect strength
 */
class Effect{
private:
	/** Unit that caused this effect, or NULL if this is a recourse effect. */
	UnitReference source;

	/**
	 * If this is a recourse effect, the primary effect that this is a recourse
	 * of, NULL otherwise.
	 */
	Effect *root;

	/** The effect type. */
	const EffectType *type;

	/**
	 * A modifier that adjusts how powerful this effect, based upon the type,
	 * should be. Each values of this effect will be multiplied by strength
	 * before being applied to the Unit or any other modifiers.
	 */
	float strength;

	/** The effect's duration in game ticks */
	int duration;

	bool recourse;

	int actualHpRegen;

public:
	Effect(const EffectType* type, Unit *source, Effect *root, float strength,
			const Unit *recipient, const TechTree *tt);
	Effect(const XmlNode *node, World *world, const TechTree *tt);

	virtual ~Effect();

	Unit *getSource()				{return source.getUnit();}
	Effect *getRoot()				{return root;}
	const EffectType *getType()		{return type;}
	float getStrength() const		{return strength;}
	int getDuration() const			{return duration;}
	bool isRecourse() const			{return recourse;}
	int getActualHpRegen() const	{return actualHpRegen;}

	void clearSource()				{source = NULL;}
	void clearRoot()				{root = NULL;}
	void setDuration(int duration)	{this->duration = duration;}
	void setStrength(float strength){this->strength = strength;}

	/**
	 * Causes the effect to age one tick and returns true when the effect has
	 * expired (and should be removed from the Unit).
	 */
	bool tick() {
		return type->isPermanent() ? false : --duration <= 0;
	}

	void save(XmlNode *node) const;
};

// ===============================
// 	class Effects
//
// ===============================

/**
 * All effects currently effecting a unit. The Effects class serves as both
 * container for the effects as well as delegate to operations related to
 * the effects on the unit it belongs to, so it is more than a simple
 * collection. Effects manages the following functions:
 * <ul>
 * <li>Determine how a new effect should be handled based upon stacking
 * rules</li>
 * <li>Manage the lifecycle of all events, appropriately removing them when
 * they expire. When an Effect is deleted, it will automatically inform the
 * Unit who created it that the effect is expiring, unless that Unit has
 * previously died and informed us of that.</li>
 * <li>Manages notifications from the units that caused effects when those
 * units die (so we don't try to talk to a deleted unit object when it
 * expires).</li>
 * <li>Provides a mechanism for the Unit owning these effects to know who
 * killed them, when they die during a tick (i.e., from an effect).</li>
 */

class Effects : public list<Effect*> {
private:
	bool dirty;

public:
	Effects();
	Effects(const XmlNode *node, World *world, const TechTree *tt);

	/**
	 * Destructor. Clears all pointers to other objects (source and root
	 * variables) of each effect in this collection and then deletes them.
	 */
	virtual ~Effects();

	/**
	 * Returns true if the effects contained have changed in a way that will
	 * effect stat calculations.
	 */
	bool isDirty() const	{return dirty;}

	/** Clears the dirty flag. */
	void clearDirty()		{dirty = false;}

	/** Add an effect, applying stacking rules based upon current effects. */
	void add(Effect *e);

	/** Removes an effect. The effect is expected to be deleted elsewhere. */
	void remove(Effect *e);

	/**
	 * Finds the effect, if it still exists, and clears it's root reference,
	 * thus notifying us that a root-cause effect has expired and we shouldn't
	 * attempt to access that object after this call returns.
	 */
	void clearRootRef(Effect *e);

	/**
	 * Causes all of the effects to age one game tick. If any effects in the
	 * collection expire during this tick, they are removed from the internal
	 * collection and deleted. The Effect destructor takes care of notifying the
	 * unit that caused the effect so that they can have any related recourse
	 * effects expire (if appropriate).
	 */
	void tick();

	/**
	 * Returns the unit, who caused the effect that killed the unit who owns
	 * this collection.
	 */
	Unit *getKiller() const;

	/**
	 * Appends a string description/summary of all of the effects in this
	 * collection and returns the supplied string object.
	 */
	string &getDescr(string &str) const;
	void save(XmlNode *node) const;
};

}}//end namespace

#endif

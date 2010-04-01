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

#ifndef _GLEST_GAME_UNIT_STAT_BASE_H_
#define _GLEST_GAME_UNIT_STAT_BASE_H_

#include <cassert>
#include <stdexcept>
#include "vec.h"
#include "damage_multiplier.h"
#include "xml_parser.h"
#include "conversion.h"
#include "util.h"
#include "flags.h"
#include "game_constants.h"

using std::runtime_error;

namespace Glest{ namespace Game{
using namespace Shared::Graphics;
using namespace Shared::Xml;
using namespace Shared::Util;

class TechTree;
class FactionType;
class EnhancementType;

/** Fields of travel, and indirectly zone of occupance */
class Fields : public XmlBasedFlags<Field, Field::COUNT> {
private:
	//static const char *names[Field::COUNT];

public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<Field, Field::COUNT>::load(node, dir, tt, ft, "field", FieldNames);
	}
	//static const char* getName ( Field f ) { return names[f]; }
};

/** Zones of attack (air, surface, etc.) */
class Zones : public XmlBasedFlags<Zone, Zone::COUNT> {
private:
	//static const char *names[Zone::COUNT];

public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<Zone, Zone::COUNT>::load(node, dir, tt, ft, "field", ZoneNames);
	}
};


// ==============================================================
// 	enum Property & class UnitProperties
// ==============================================================



/** Properties that can be applied to a unit. */
class UnitProperties : public XmlBasedFlags<Property, Property::COUNT>{
private:
	//static const char *names[pCount];

public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<Property, Property::COUNT>::load(node, dir, tt, ft, "property", PropertyNames);
	}
};
// ===============================
// 	class UnitStats
// ===============================

/**
 * Base stats for a unit type, upgrade, effect or other enhancement.
 */
class UnitStats {
protected:
	int maxHp;
	int hpRegeneration;
    int maxEp;
	int epRegeneration;

    int sight;
	int armor;
	const ArmorType *armorType; // ??? UnitStats?

    /** size in cells square (i.e., size x size) */
    int size;
    int height;

	bool light;			// ??? UnitStats?
    Vec3f lightColor;	// ??? UnitStats?

	// these are skill stats
	int attackStrength;		// ??? UnitStats?
	fixed effectStrength;	// ??? UnitStats?
	fixed attackPctStolen;	// ??? UnitStats?
	int attackRange;		// ??? UnitStats?
	int moveSpeed;			// ??? UnitStats?
	int attackSpeed;		// ??? UnitStats?
	int prodSpeed;			// ??? UnitStats?
	int repairSpeed;		// ??? UnitStats?
	int harvestSpeed;		// ??? UnitStats?


	// move these 'down' (to UnitType)
	UnitProperties properties;
	Fields fields;
	/** Resistance / Damage Multipliers */
	static size_t damageMultiplierCount;
	vector<float> damageMultipliers;

public:
	UnitStats() {damageMultipliers.resize(damageMultiplierCount);}
	virtual ~UnitStats() {}

	virtual void doChecksum(Checksum &checksum) const;

	// ==================== get ====================

	int getMaxHp() const					{return maxHp;}
	int getHpRegeneration() const			{return hpRegeneration;}
	int getMaxEp() const					{return maxEp;}
	int getEpRegeneration() const			{return epRegeneration;}
	int getSight() const					{return sight;}
	int getArmor() const					{return armor;}
	const ArmorType *getArmorType() const	{return armorType;}
	bool getLight() const					{return light;}
	Vec3f getLightColor() const				{return lightColor;}
	int getSize() const						{return size;}
	int getHeight() const					{return height;}

	int getAttackStrength() const			{return attackStrength;}
	fixed getEffectStrength() const			{return effectStrength;}
	fixed getAttackPctStolen() const		{return attackPctStolen;}
	int getAttackRange() const				{return attackRange;}
	int getMoveSpeed() const				{return moveSpeed;}
	int getAttackSpeed() const				{return attackSpeed;}
	int getProdSpeed() const				{return prodSpeed;}
	int getRepairSpeed() const				{return repairSpeed;}
	int getHarvestSpeed() const				{return harvestSpeed;}
	const Fields &getFields() const			{return fields;}
	bool getField(Field field) const		{return fields.get(field);}
	const UnitProperties &getProperties() const	{return properties;}
	bool getProperty(Property property) const	{return properties.get(property);}

	static size_t getDamageMultiplierCount()	{return damageMultiplierCount;}
	float getDamageMultiplier(size_t i) const	{assert(i < damageMultiplierCount); return damageMultipliers[i];}

	// ==================== set ====================

	// this is called from TechTree::load()
	static void setDamageMultiplierCount(size_t count)	{damageMultiplierCount = count;}

	// ==================== misc ====================

	/** Resets the values of all fields to zero or other base value. */
	virtual void reset();

	/**
	 * Initialize the object from an XmlNode object. It is important to note
	 * that all xxxSpeed and attackRange variables are not initialized by this
	 * function. This is essentially the portions of the load method of the
	 * legacy UnitType class that appeared under the <properties> node and that
	 * is exactly what XmlNode object the UnitType load() method supplies to
	 * this method.
	 */
	bool load(const XmlNode *parametersNode, const string &dir, const TechTree *tt, const FactionType *ft);

	virtual void save(XmlNode *node);

	/** Equivilant to an assignment operator; initializes values based on supplied object. */
	void setValues(const UnitStats &us);

	/**
	 * Apply all the multipliers to in the supplied EnhancementType to the
	 * applicable static value (i.e., addition/subtraction values) in this
	 * object.
	 */
	void applyMultipliers(const EnhancementType &e);

	/**
	 * Add all static values (i.e., addition/subtraction values) in to this
	 * object, using the supplied multiplier strength before adding. I.e., stat =
	 * e.stat * strength. This includes adding and removing effected fields and
	 * properties as well as overwriting light and/or armor if either of those
	 * are specified in the object e.
	 */
	void addStatic(const EnhancementType &e, fixed strength = 1);


};

// ===============================
// 	class EnhancementType
// ===============================

/** An extension of UnitStats, which contains values suitable for an
 * addition/subtraction alteration to a Unit's stats, that also has a multiplier
 * for each of those stats.  This is the base class for both UpgradeType and
 * EffectType.
 */
class EnhancementType : public UnitStats {
protected:
	fixed maxHpMult;
	fixed hpRegenerationMult;
	fixed maxEpMult;
	fixed epRegenerationMult;
	fixed sightMult;
	fixed armorMult;
	fixed attackStrengthMult;
	fixed effectStrengthMult;
	fixed attackPctStolenMult;
	fixed attackRangeMult;
	fixed moveSpeedMult;
	fixed attackSpeedMult;
	fixed prodSpeedMult;
	fixed repairSpeedMult;
	fixed harvestSpeedMult;
	//Note: the member variables fields, properties, armorType, bodyType, light,
	//lightColor, size and height don't get multipliers.

	/** Fields which are removed by this enhancement/degridation object. */
	// replace with generic immobilize ?
	Fields antiFields;

	/** Properties which are removed by this enhancement/degridation object. */
	UnitProperties antiProperties;

public:
	EnhancementType() {
		reset();
	}

	fixed getMaxHpMult() const			{return maxHpMult;}
	fixed getHpRegenerationMult() const	{return hpRegenerationMult;}
	fixed getMaxEpMult() const			{return maxEpMult;}
	fixed getEpRegenerationMult() const	{return epRegenerationMult;}
	fixed getSightMult() const			{return sightMult;}
	fixed getArmorMult() const			{return armorMult;}
	fixed getAttackStrengthMult() const	{return attackStrengthMult;}
	fixed getEffectStrengthMult() const	{return effectStrengthMult;}
	fixed getAttackPctStolenMult() const{return attackPctStolenMult;}
	fixed getAttackRangeMult() const	{return attackRangeMult;}
	fixed getMoveSpeedMult() const		{return moveSpeedMult;}
	fixed getAttackSpeedMult() const	{return attackSpeedMult;}
	fixed getProdSpeedMult() const		{return prodSpeedMult;}
	fixed getRepairSpeedMult() const	{return repairSpeedMult;}
	fixed getHarvestSpeedMult() const	{return harvestSpeedMult;}

	const Fields &getRemovedFields() const				{return antiFields;}
	bool getRemovedField(Field field) const				{return antiFields.get(field);}
	const UnitProperties &getRemovedProperties() const	{return antiProperties;}
	bool getRemovedProperty(Property property) const	{return antiProperties.get(property);}

	/**
	 * Adds multipliers, normalizing and adjusting for strength. The formula
	 * used to calculate the new value for each multiplier field is "field +=
	 * (e.field - 1.0f) * strength". This effectively causes the original
	 * multiplier for each field to be adjusted by the deviation from the value
	 * 1.0f of each multiplier in the supplied object e.
	 */
	void addMultipliers(const EnhancementType &e, fixed strength = 1);

	/**
	 * Resets all multipliers to 1.0f and all base class members to their
	 * appropriate default values (0, NULL, etc.).
	 */
	virtual void reset();

	/**
	 * Returns a string description of this object, only supplying information
	 * on fields which cause a modification.
	 */
	virtual void getDesc(string &str, const char *prefix) const;

	/**
	 * Initializes this object from the specified XmlNode object.
	 * TODO: explain better.
	 */
	virtual bool load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft);

	virtual void doChecksum(Checksum &checksum) const;

	virtual void save(XmlNode *node) const;

	/**
	 * Appends a uniform description of the supplied value, if non-zero.
	 * Essentially, this will contain either a + or - sign followed by the
	 * value, unless the value is zero and is primarily used as an inline
	 * function to keep redundant clutter out of other functions which provide
	 * descriptions.
	 */
	static void describeModifier(string &str, int value) {
		if(value != 0) {
			if(value > 0) {
				str += "+";
			}
			str += intToStr(value);
		}
	}

	void sum(const EnhancementType *enh) {
		addStatic(*enh);
		addMultipliers(*enh);
	}

private:
	/** Initialize value from <static-modifiers> node */
	void initStaticModifier(const XmlNode *node, const string &dir);

	/** Initialize value from <multipliers> node */
	void initMultiplier(const XmlNode *node, const string &dir);

	static void formatModifier(string &str, const char *pre, const char* label, int value, fixed multiplier);
};

}}//end namespace

#endif

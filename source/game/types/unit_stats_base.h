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

using std::runtime_error;

namespace Glest{ namespace Game{
using namespace Shared::Graphics;
using namespace Shared::Xml;
using namespace Shared::Util;

class TechTree;
class FactionType;
class EnhancementTypeBase;

// ===============================
// 	enum Field & Zone, and SurfaceType
// ===============================

// Adding a new field ???
// see instructions at the top of annotated_map.h

enum Zone // Zone of unit _occupance_
{  // used for attack fields, actual occupance is indirectly 
   // specified by the unit's current Field
   ZoneSurfaceProp,
   ZoneSurface,
   ZoneAir,
   //ZoneSubSurface,
   ZoneCount
};

enum SurfaceType // for each cell's surface zone,
{  // is computed in Map::setCellTypes ()
   // FIXME: may change... need hook in Map::prepareTerrain()
   SurfaceTypeLand, 
   SurfaceTypeFordable, 
   SurfaceTypeDeepWater, 
   SurfaceTypeCount
};

enum Field // Zones of Movement
{
   FieldWalkable, // ZoneSurface + ( SurfaceTypeLand || SurfaceTypeFordable )
   FieldAir,      // ZoneAir
   FieldAnyWater, // ZoneSurface + ( SurfaceTypeFordable || SurfaceTypeDeepWater )
   FieldDeepWater, // ZoneSurface + SurfaceTypeDeepWater
   FieldAmphibious, // ZoneSurface + SuraceType*
   FieldCount
};

/** Fields of travel, and indirectly zone of occupance */
class Fields : public XmlBasedFlags<Field, FieldCount> {
private:
	static const char *names[FieldCount];

public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<Field, FieldCount>::load(node, dir, tt, ft, "field", names);
	}
	static const char* getName ( Field f ) { return names[f]; }
};

/** Zones of attack (air, surface, etc.) */
class Zones : public XmlBasedFlags<Zone, ZoneCount> {
private:
	static const char *names[ZoneCount];

public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<Zone, ZoneCount>::load(node, dir, tt, ft, "field", names);
	}
};


// ==============================================================
// 	enum Property & class UnitProperties
// ==============================================================

enum Property{
	pBurnable,
	pRotatedClimb,
	pWall,

	pCount
};

/** Properties that can be applied to a unit. */
class UnitProperties : public XmlBasedFlags<Property, pCount>{
private:
	static const char *names[pCount];

public:
	void load(const XmlNode *node, const string &dir, const TechTree *tt, const FactionType *ft) {
		XmlBasedFlags<Property, pCount>::load(node, dir, tt, ft, "property", names);
	}
};

// ===============================
// 	class UnitStatsBase
// ===============================

/**
 * Base stats for a unit type, upgrade, effect or other enhancement.
 */
class UnitStatsBase {
protected:
	int maxHp;
	int hpRegeneration;
    int maxEp;
	int epRegeneration;

	/**
	 * Fields of travel. For enhancment-type objects, there are fields that are
	 * added.
	 */
	Fields fields;

	/**
	 * Properties (burnable, etc.). For enhancment-type objects, there are
	 * properties that are added.
	 */
	UnitProperties properties;
    int sight;
	int armor;
	const ArmorType *armorType;
	bool light;
    Vec3f lightColor;

    /** size in cells squared (i.e., size x size) */
    int size;
    int height;

	int attackStrength;
	float effectStrength;
	float attackPctStolen;
	int attackRange;
	int moveSpeed;
	int attackSpeed;
	int prodSpeed;
	int repairSpeed;
	int harvestSpeed;

	/**
	 * Resistance / Damage Multipliers
	 */
	static size_t damageMultiplierCount;
	vector<float> damageMultipliers;

public:
	UnitStatsBase() {damageMultipliers.resize(damageMultiplierCount);}

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
	float getEffectStrength() const			{return effectStrength;}
	float getAttackPctStolen() const		{return attackPctStolen;}
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
	void setValues(const UnitStatsBase &us);

	/**
	 * Apply all the multipliers to in the supplied EnhancementTypeBase to the
	 * applicable static value (i.e., addition/subtraction values) in this
	 * object.
	 */
	void applyMultipliers(const EnhancementTypeBase &e);

	/**
	 * Add all static values (i.e., addition/subtraction values) in to this
	 * object, using the supplied multiplier strength before adding. I.e., stat =
	 * e.stat * strength. This includes adding and removing effected fields and
	 * properties as well as overwriting light and/or armor if either of those
	 * are specified in the object e.
	 */
	void addStatic(const EnhancementTypeBase &e, float strength = 1.0f);


};

// ===============================
// 	class EnhancementTypeBase
// ===============================

/** An extension of UnitStatsBase, which contains values suitable for an
 * addition/subtraction alteration to a Unit's stats, that also has a multiplier
 * for each of those stats.  This is the base class for both UpgradeType and
 * EffectType.
 */
class EnhancementTypeBase : public UnitStatsBase{
protected:
	float maxHpMult;
	float hpRegenerationMult;
	float maxEpMult;
	float epRegenerationMult;
	float sightMult;
	float armorMult;
	float attackStrengthMult;
	float effectStrengthMult;
	float attackPctStolenMult;
	float attackRangeMult;
	float moveSpeedMult;
	float attackSpeedMult;
	float prodSpeedMult;
	float repairSpeedMult;
	float harvestSpeedMult;
	//Note: the member variables fields, properties, armorType, bodyType, light,
	//lightColor, size and height don't get multipliers.

	/** Fields which are removed by this enhancement/degridation object. */
	Fields antiFields;

	/** Properties which are removed by this enhancement/degridation object. */
	UnitProperties antiProperties;

public:
	EnhancementTypeBase() {
		reset();
	}

	float getMaxHpMult() const			{return maxHpMult;}
	float getHpRegenerationMult() const	{return hpRegenerationMult;}
	float getMaxEpMult() const			{return maxEpMult;}
	float getEpRegenerationMult() const	{return epRegenerationMult;}
	float getSightMult() const			{return sightMult;}
	float getArmorMult() const			{return armorMult;}
	float getAttackStrengthMult() const	{return attackStrengthMult;}
	float getEffectStrengthMult() const	{return effectStrengthMult;}
	float getAttackPctStolenMult() const{return attackPctStolenMult;}
	float getAttackRangeMult() const	{return attackRangeMult;}
	float getMoveSpeedMult() const		{return moveSpeedMult;}
	float getAttackSpeedMult() const	{return attackSpeedMult;}
	float getProdSpeedMult() const		{return prodSpeedMult;}
	float getRepairSpeedMult() const	{return repairSpeedMult;}
	float getHarvestSpeedMult() const	{return harvestSpeedMult;}

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
	void addMultipliers(const EnhancementTypeBase &e, float strength = 1.0f);

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

	void sum(const EnhancementTypeBase *enh) {
		addStatic(*enh);
		addMultipliers(*enh);
	}

private:
	/** Initialize value from <static-modifiers> node */
	void initStaticModifier(const XmlNode *node, const string &dir);

	/** Initialize value from <multipliers> node */
	void initMultiplier(const XmlNode *node, const string &dir);

	static void formatModifier(string &str, const char *pre, const char* label, int value, float multiplier);
};

}}//end namespace

#endif

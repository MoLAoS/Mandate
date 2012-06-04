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

#ifndef _GLEST_GAME_UNITTYPE_H_
#define _GLEST_GAME_UNITTYPE_H_

#include "cloak_type.h"
#include "command_type.h"
#include "damage_multiplier.h"
#include "sound_container.h"
#include "checksum.h"
#include "particle_type.h"
#include <set>
using std::set;

using Shared::Sound::StaticSound;
using Shared::Util::Checksum;
using Shared::Util::MultiFactory;

namespace Glest { namespace ProtoTypes {
using namespace Search;

// ===============================
// 	class Level
// ===============================

class Level: public EnhancementType, public NameIdPair {
private:
	int kills;
	int exp;

public:
	Level() : EnhancementType() {
		const fixed onePointFive = fixed(3) / 2;
		maxHpMult = onePointFive;
		maxSpMult = onePointFive;
		maxEpMult = onePointFive;
        maxCpMult = onePointFive;
		sightMult = fixed(6) / 5;
		armorMult = onePointFive;
		effectStrength = fixed(1) / 10;
	}

	virtual bool load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft);
	virtual void doChecksum(Checksum &checksum) const {
		NameIdPair::doChecksum(checksum);
		EnhancementType::doChecksum(checksum);
	}
	int getKills() const			{return kills;}
	int getExp() const			    {return exp;}
};

Vec2i rotateCellOffset(const Vec2i &offsetconst, const int unitSize, const CardinalDir facing);

// ===============================
// 	class Load Bonus
// ===============================
/** resource amount modifier */
typedef map<const ResourceType*, Modifier> ResModifierMap;
/** A unit type enhancement, an EnhancementType + resource cost modifiers + resource storage modifiers + resource creation modifiers */
struct GarrisonEffect {
	EnhancementType  m_enhancement;
	ResModifierMap   m_costModifiers;
	ResModifierMap   m_storeModifiers;
	ResModifierMap   m_createModifiers;
	const EnhancementType* getEnhancement() const { return &m_enhancement; }
};

class LoadBonus {
public:
    string source;
    typedef GarrisonEffect Enhancement;
    Enhancement m_enhancement;
	LoadBonus();
	virtual bool load(const XmlNode *loadBonusNode, const string &dir, const TechTree *tt, const FactionType *ft);
    bool loadNewStyle(const XmlNode *node, const string &dir, const TechTree *techTree, const FactionType *factionType);
	void loadResourceModifier(const XmlNode *node, ResModifierMap &map, const TechTree *techTree);
	const Enhancement getEnhancement() const {return m_enhancement;}
	string getSource() const {return source;}
};

// ===============================
// 	class Timer
// ===============================

class Timer {
public:
    int timerValue;
    int currentStep;

    Timer() : timerValue(0), currentStep(0) {}
	Timer(const Timer &that) : timerValue(that.timerValue), currentStep(that.currentStep) {}

	void init(const XmlNode *n, const TechTree *tt);
	void init(int timerValue, int currentStep);

    int getTimerValue() {return timerValue;}
    int getCurrentStep() {return currentStep;}

    virtual void setTimerValue(int v) { currentStep = v; }
    virtual void setCurrentStep(int v) { currentStep = v; }

	void save(XmlNode *node) const;
};

// =====================================================
// 	class CreatedUnit
//
/// Amount of a given ResourceType
// =====================================================

class CreatedUnit {
protected:
	const UnitType       *m_type;
	int	                 m_amount;
	int	                 m_amount_plus;
	float	             m_amount_multiply;

public:
	CreatedUnit() : m_type(0), m_amount(0), m_amount_plus(0), m_amount_multiply(0) {}
	CreatedUnit(const CreatedUnit &that) : m_type(that.m_type), m_amount(that.m_amount),
	m_amount_plus(that.m_amount_plus), m_amount_multiply(that.m_amount_multiply) {}

	void init(const XmlNode *n, const Faction *f);
	void init(const UnitType *ut, const int amount, const int amount_plus, const float amount_multiply);

	virtual void setAmount(int v) { m_amount = v; }
	int  getAmount() const { return m_amount; }
	int  getAmountPlus() const { return m_amount_plus; }
	float  getAmountMultiply() const { return m_amount_multiply; }
	const UnitType *getType() const { return m_type; }

	void save(XmlNode *node) const;
};

// ===============================
// 	class UnitType
//
///	A unit or building type
// ===============================

class UnitType : public ProducibleType, public UnitStats {
private:
	typedef vector<SkillType*>          SkillTypes;
	typedef vector<CommandType*>        CommandTypes;
	typedef vector<ResourceAmount>      StoredResources;
	typedef vector<ResourceAmount>      CreatedResources;
	typedef vector<CreatedUnit>         CreatedUnits;
	typedef vector<Timer>               CreatedResourceTimers;
	typedef vector<Timer>               CreatedUnitTimers;
	typedef vector<Level>               Levels;
	typedef vector<LoadBonus>          LoadBonuses;
	typedef vector<ParticleSystemType*> ParticleSystemTypes;

	//typedef vector<PetRule*> PetRules;
	//typedef map<int, const CommandType*> CommandTypeMap;

private:
	//basic
	string name;

	bool multiBuild;
	bool multiSelect;

	const ArmourType *armourType;

    /** size in cells square (i.e., size x size) */
    int size;
    int height;

	bool light;
    Vec3f lightColour;

	CloakType	  *m_cloakType;
	DetectorType  *m_detectorType;

	set<string> m_tags;

	//sounds
	SoundContainer selectionSounds;
	SoundContainer commandSounds;

	//info
	StoredResources storedResources;
public:
	CreatedResources createdResources;
	mutable CreatedResourceTimers createdResourceTimers;
	CreatedUnits createdUnits;
	mutable CreatedUnitTimers createdUnitTimers;
private:
	Levels levels;
public:
	LoadBonuses loadBonuses;
private:
	Emanations emanations;

	//meeting point
	bool meetingPoint;
	Texture2D *meetingPointImage;

	CommandTypes commandTypes;
	CommandTypes commandTypesByClass[CmdClass::COUNT]; // command types mapped by CmdClass

	SkillTypes skillTypes;
	SkillTypes skillTypesByClass[SkillClass::COUNT];

	SkillType *startSkill;

	fixed halfSize;
	fixed halfHeight;

	PatchMap<1> *m_cellMap;
	PatchMap<1> *m_colourMap; // 'footprint' on minimap

	UnitProperties properties;
	//Fields fields;
	Field field;
	Zone zone;
	bool m_hasProjectileAttack;

	const FactionType *m_factionType;

public:
	static ProducibleClass typeClass() { return ProducibleClass::UNIT; }

public:
	//creation and loading
	UnitType();
	virtual ~UnitType();
	void preLoad(const string &dir);
	bool load(const string &dir, const TechTree *techTree, const FactionType *factionType, bool glestimal = false);
	void addBeLoadedCommand();
	virtual void doChecksum(Checksum &checksum) const;
	const FactionType* getFactionType() const { return m_factionType; }

	ProducibleClass getClass() const override { return typeClass(); }

	CloakClass getCloakClass() const {
		return m_cloakType ? m_cloakType->getClass() : CloakClass(CloakClass::INVALID);
	}
	const Texture2D* getCloakImage() const {
		return m_cloakType ? m_cloakType->getImage() : 0;
	}
	const CloakType* getCloakType() const        { return m_cloakType; }
	const DetectorType* getDetectorType() const  { return m_detectorType; }
	bool isDetector() const                      { return m_detectorType ? true : false; }

	//get
	string getUnitName() const              {return name;}
	const Model *getIdleAnimation() const	{return getFirstStOfClass(SkillClass::STOP)->getAnimation();}
	bool getMultiSelect() const				{return multiSelect;}
	const ArmourType *getArmourType() const	{return armourType;}
	bool getLight() const					{return light;}
	Vec3f getLightColour() const			{return lightColour;}
	int getSize() const						{return size;}
	int getHeight() const					{return height;}
	Field getField() const					{return field;}
	Zone getZone() const					{return zone;}
	bool hasTag(const string &tag) const	{return m_tags.find(tag) != m_tags.end();}

	const UnitProperties &getProperties() const	{return properties;}
	bool getProperty(Property property) const	{return properties.get(property);}

	const SkillType *getStartSkill() const				{return startSkill;}

	int getSkillTypeCount() const						{return skillTypes.size();}
	const SkillType *getSkillType(int i) const			{return skillTypes[i];}
	const MoveSkillType *getFirstMoveSkill() const			{
		return skillTypesByClass[SkillClass::MOVE].empty() ? 0
			: static_cast<const MoveSkillType *>(skillTypesByClass[SkillClass::MOVE].front());
	}

	int getCommandTypeCount() const						{return commandTypes.size();}
	const CommandType *getCommandType(int i) const		{return commandTypes[i];}

	const CommandType *getCommandType(const string &name) const;

	template <typename ConcreteType>
	int getCommandTypeCount() const {
		return commandTypesByClass[ConcreteType::typeClass()].size();
	}
	template <typename ConcreteType>
	const ConcreteType* getCommandType(int i) const {
		return static_cast<const ConcreteType*>(commandTypesByClass[ConcreteType::typeClass()][i]);
	}
	const CommandTypes& getCommandTypes(CmdClass cc) const {
		return commandTypesByClass[cc];
	}
	const CommandType *getFirstCtOfClass(CmdClass cc) const {
		return commandTypesByClass[cc].empty() ? 0 : commandTypesByClass[cc].front();
	}
    const HarvestCommandType *getHarvestCommand(const ResourceType *resourceType) const;
	const AttackCommandType *getAttackCommand(Zone zone) const;
	const RepairCommandType *getRepairCommand(const UnitType *repaired) const;

	const Level *getLevel(int i) const					{return &levels[i];}
	int getLevelCount() const							{return levels.size();}

//	const PetRules &getPetRules() const					{return petRules;}
	const Emanations &getEmanations() const				{return emanations;}
	bool isMultiBuild() const							{return multiBuild;}
	fixed getHalfSize() const							{return halfSize;}
	fixed getHalfHeight() const							{return halfHeight;}
	bool isMobile () const {
		const SkillType *st = getFirstStOfClass(SkillClass::MOVE);
		return st && st->getBaseSpeed() > 0 ? true: false;
	}

	//cellmap
	bool getCellMapCell(Vec2i pos, CardinalDir facing) const;
	bool getCellMapCell(int x, int y, CardinalDir facing) const	{ return getCellMapCell(Vec2i(x,y), facing); }

	const PatchMap<1>& getMinimapFootprint() const { return *m_colourMap; }

	// resources stored
	int getStoredResourceCount() const					{return storedResources.size();}
	ResourceAmount getStoredResource(int i, const Faction *f) const;
	int getStore(const ResourceType *rt, const Faction *f) const;

    // resources created
	int getCreatedResourceCount() const					{return createdResources.size();}
	ResourceAmount getCreatedResource(int i, const Faction *f) const;
	int getCreate(const ResourceType *rt, const Faction *f) const;

	// resources created timers
	int getCreatedResourceTimerCount()                 {return createdResourceTimers.size();}
	Timer getCreatedResourceTimer(int i, const Faction *f) const;
	int getCreateTimer(const ResourceType *rt, const Faction *f) const;

    // units created
	int getCreatedUnitCount() const					{return createdUnits.size();}
	CreatedUnit getCreatedUnit(int i, const Faction *f) const;
	int getCreateUnit(const UnitType *ut, const Faction *f) const;

	// units created timers
	int getUnitTimerCount()                 {return createdUnitTimers.size();}
	Timer getCreatedUnitTimer(int i, const Faction *f) const;
	int getCreateUnitTimer(const UnitType *ut, const Faction *f) const;

	// meeting point
	bool hasMeetingPoint() const						{return meetingPoint;}
	Texture2D *getMeetingPointImage() const				{return meetingPointImage;}

	// sounds
	StaticSound *getSelectionSound() const				{return selectionSounds.getRandSound();}
	StaticSound *getCommandSound() const				{return commandSounds.getRandSound();}

	const SkillType *getSkillType(const string &skillName, SkillClass skillClass = SkillClass::COUNT) const;
	const SkillType *getFirstStOfClass(SkillClass sc) const {
		return skillTypesByClass[sc].empty() ? 0 : skillTypesByClass[sc].front();
	}

	// has
	bool hasCommandType(const CommandType *ct) const;
	bool hasCommandClass(CmdClass cc) const		{return !commandTypesByClass[cc].empty();}
	bool hasSkillType(const SkillType *skillType) const;
	bool hasSkillClass(SkillClass skillClass) const;
	bool hasCellMap() const							{return m_cellMap != NULL;}
	bool hasProjectileAttack() const				{return m_hasProjectileAttack;}

	// is
	bool isOfClass(UnitClass uc) const;

private:
	void setDeCloakSkills(const vector<string> &names, const vector<SkillClass> &classes);
    void sortSkillTypes();
    void sortCommandTypes();
};

}}//end namespace


#endif

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

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <time.h>

#include "forward_decs.h"
#include "creatable_type.h"
#include "vec.h"
#include "prototypes_enums.h"
#include "simulation_enums.h"
#include "abilities.h"
#include "cloak_type.h"
#include "sound_container.h"
#include "checksum.h"
#include "particle_type.h"
#include "hero.h"
#include <set>
using std::set;

using Shared::Sound::StaticSound;
using Shared::Util::Checksum;
using Shared::Util::MultiFactory;

namespace Glest { namespace ProtoTypes {
using namespace Search;
Vec2i rotateCellOffset(const Vec2i &offsetconst, const int unitSize, const CardinalDir facing);
// ===============================
// 	class BonusPowerType
//
///	A bonus from another
/// map object or building
// ===============================
class BonusPower : public Statistics {
private:
    ResourceProductionSystem resourceGeneration;
    ItemProductionSystem itemGeneration;
    UnitProductionSystem unitGeneration;
    ProcessProductionSystem processingSystem;
public:
    const ResourceProductionSystem *getResourceProductionSystem() const {return &resourceGeneration;}
    const ItemProductionSystem *getItemProductionSystem() const {return &itemGeneration;}
    const UnitProductionSystem *getUnitProductionSystem() const {return &unitGeneration;}
    const ProcessProductionSystem *getProcessProductionSystem() const {return &processingSystem;}
    bool load(const XmlNode *bonusPowerNode, const string &dir, const TechTree *techTree, const FactionType *factionType);
};

// ===============================
// 	class UnitType
//
///	A unit or building type
// ===============================
class UnitType : public CreatableType {
private:
	typedef vector<Level>               Levels;
	typedef vector<LoadBonus>           LoadBonuses;
	typedef vector<ParticleSystemType*> ParticleSystemTypes;
	typedef vector<ResourceStore>       ResourceStores;
	typedef vector<BonusPower>          BonusPowers;
private:
	bool multiBuild;
	bool multiSelect;
public:
    Hero hero;
    Mage mage;
    Leader leader;
    bool inhuman;
    string personality;
    string foundation;
    int live;
    Statistics dayPower;
    Statistics nightPower;
    const Statistics *getDayPower() const   {return &dayPower;}
    const Statistics *getNightPower() const {return &nightPower;}
    string bonusObjectName;
    string getBonusObjectName() const {return bonusObjectName;}
    BonusPowers bonusPowers;
    int getBonusPowerCount() const {return bonusPowers.size();}
    const BonusPower *getBonusPower(int i) const {return &bonusPowers[i];}
private:
	bool light;
    Vec3f lightColour;
	CloakType	  *m_cloakType;
	DetectorType  *m_detectorType;
	set<string> m_tags;
	SoundContainer selectionSounds;
	SoundContainer commandSounds;
	LoadBonuses loadBonuses;
    bool isMage;
    bool isLeader;
    bool isHero;
	typedef vector<string> StarterItems;
	typedef vector<ItemStore> ItemStores;
	typedef vector<Equipment> Equipments;
	StarterItems starterItems;
	ItemStores itemStores;
	Equipments equipment;
	int itemLimit;
public:
    int getItemLimit() const {return itemLimit;}
    StarterItems getStarterItems() const {return starterItems;}
    ItemStores getItemStores() const {return itemStores;}
    Equipments getEquipment() const {return equipment;}
    bool getIsLeader() const {return isLeader;}
    bool getIsMage() const {return isMage;}
    bool getIsHero() const {return isHero;}
    LoadBonuses getLoadBonuses() const {return loadBonuses;}
private:
    ResourceStores resourceStores;

	Levels levels;
	bool meetingPoint;
	Texture2D *meetingPointImage;
	PatchMap<1> *m_cellMap;
	PatchMap<1> *m_colourMap;
	UnitProperties properties;
	Field field;
	Zone zone;
	fixed halfSize;
	fixed halfHeight;
	const SkillType *startSkill;
	bool m_hasProjectileAttack;
public:
    ResourceStores getResourceStores() const {return resourceStores;}

	static ProducibleClass typeClass() { return ProducibleClass::UNIT; }
	UnitType();
	virtual ~UnitType();
	void preLoad(const string &dir);
	bool load(const string &dir, const TechTree *techTree, const FactionType *factionType);
	void addBeLoadedCommand();
	virtual void doChecksum(Checksum &checksum) const;
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
	const Model *getIdleAnimation() const	{return getActions()->getFirstStOfClass(SkillClass::STOP)->getSoundsAndAnimations()->getAnimation();}
	bool getMultiSelect() const				{return multiSelect;}
	bool getLight() const					{return light;}
	Vec3f getLightColour() const			{return lightColour;}
	Field getField() const					{return field;}
	Zone getZone() const					{return zone;}
	bool hasTag(const string &tag) const	{return m_tags.find(tag) != m_tags.end();}
	const UnitProperties &getProperties() const	{return properties;}
	bool getProperty(Property property) const	{return properties.get(property);}
	const MoveSkillType *getFirstMoveSkill() const			{
		return getActions()->getFirstStOfClass(SkillClass::MOVE) ? 0
			: static_cast<const MoveSkillType *>(getActions()->getFirstStOfClass(SkillClass::MOVE));
	}
    const SkillType *getStartSkill() const				{return startSkill;}
	fixed getHalfSize() const							{return halfSize;}
	fixed getHalfHeight() const							{return halfHeight;}
	bool hasProjectileAttack() const				    {return m_hasProjectileAttack;}
	const Level *getLevel(int i) const					{return &levels[i];}
	int getLevelCount() const							{return levels.size();}
	bool isMultiBuild() const							{return multiBuild;}
	bool isMobile () const {
		const SkillType *st = getActions()->getFirstStOfClass(SkillClass::MOVE);
		return st && st->getBaseSpeed() > 0 ? true: false;
	}
	bool getCellMapCell(Vec2i pos, CardinalDir facing) const;
	bool getCellMapCell(int x, int y, CardinalDir facing) const	{ return getCellMapCell(Vec2i(x,y), facing); }
	const PatchMap<1>& getMinimapFootprint() const { return *m_colourMap; }
	bool hasMeetingPoint() const						{return meetingPoint;}
	Texture2D *getMeetingPointImage() const				{return meetingPointImage;}
	StaticSound *getSelectionSound() const				{return selectionSounds.getRandSound();}
	StaticSound *getCommandSound() const				{return commandSounds.getRandSound();}
	bool hasCellMap() const							{return m_cellMap != NULL;}
	bool isOfClass(UnitClass uc) const;
	bool display;
    bool isDisplay() const			{return display;}
};

}}//end namespace


#endif

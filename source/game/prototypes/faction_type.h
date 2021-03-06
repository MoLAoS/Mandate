// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_FACTIONTYPE_H_
#define _GLEST_GAME_FACTIONTYPE_H_

#include "unit_type.h"
#include "upgrade_type.h"
#include "item_type.h"
#include "modifications.h"
#include "mandate_ai_personalities.h"
#include "sound.h"

using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;

using namespace Glest::Plan;

namespace Glest{ namespace ProtoTypes {
class TechTree;
class EventType;

// =====================================================
// 	class FactionType
//
///	Each of the possible factions the user can select
// =====================================================

class FactionType : public NameIdPair {
private:
	typedef pair<const UnitType*, int> PairPUnitTypeInt;
	typedef vector<UnitType*> UnitTypes;
	typedef set<UnitType*> UnitTypeSet;
	typedef vector<UpgradeType*> UpgradeTypes;
	typedef vector<PairPUnitTypeInt> StartingUnits;
	typedef vector<StoredResource> SResources;
    typedef vector<CreatedResource> CResources;
	typedef vector<Modification> Modifications;
    typedef vector<EventType*> EventTypes;
	typedef vector<ItemType*> ItemTypes;
	typedef vector<Texture2D*> ItemImages;
	typedef vector<string> GuiFileNames;

    typedef vector<Personality> Personalities;
	typedef vector<ResourceAmount> ResourceTrades;

private:
    typedef vector<string> FactionTypeNames;
    FactionTypeNames factionTypeNames;
    ResourceTrades resourceTrades;
    string guiDirectory;
    GuiFileNames guiFileNames;
	UnitTypes unitTypes;
	UpgradeTypes upgradeTypes;
	ItemImages itemImages;
	Modifications modifications;
    EventTypes eventTypes;
	ItemTypes itemTypes;
	StartingUnits startingUnits;
	SResources startingResources;
	StrSound *music;
	SoundContainer *attackNotice;
	SoundContainer *enemyNotice;
	int attackNoticeDelay;
	int enemyNoticeDelay;
	UnitTypeSet loadableUnitTypes;
	Pixmap2D *m_logoTeamColour, *m_logoRgba;
	bool onHitExp;
	bool usesSovereign;

	string personalityDirectory;
    Personalities personalities;

    CitizenNeeds citizenNeeds;

	typedef vector<Specialization> ListSpecs;
	typedef vector<Actions> ListActions;

    ListTraits traitsList;
    ListSpecs specializations;
    ListActions listActions;
    Actions actions;

public:
	int getTraitCount() const								{return traitsList.size();}
	int getSpecializationCount() const						{return specializations.size();}
	const Trait *getTrait(int i) const	        		    {return &traitsList[i];}
	const Specialization *getSpecialization(int i) const	{return &specializations[i];}
	Trait *getTrait(int i)          	        		    {return &traitsList[i];}
	Specialization *getSpecialization(int i)               	{return &specializations[i];}
    Trait *getTraitById(int id);
    Actions *getActions()                                   {return &actions;}
    const Actions *getActions() const                       {return &actions;}

	Specialization *getSpecialization(const string &name){
		for (int i = 0; i < getSpecializationCount(); ++i) {
		    if (specializations[i].getSpecName() == name) {
                return &specializations[i];
            }
		}
		throw runtime_error("Specialization not found:" + name);
	}



    const CitizenNeeds *getCitizenNeeds() const {return &citizenNeeds;}

    ResourceTrades getResourceTrades() const {return resourceTrades;}
    int getResourceTradeCount() const {return resourceTrades.size();}
    const ResourceAmount *getResourceTrade(int i) const {return &resourceTrades[i];}
    Personalities getPersonalities() const {return personalities;}
    Modifications getModifications() const {return modifications;}
    FactionTypeNames getFactionTypeNames() const {return factionTypeNames;}

    bool getOnHitExp() const {return onHitExp;}
    bool useSovereign() const {return usesSovereign;}
	//init
	FactionType();

	bool preLoad(const string &dir, const TechTree *techTree);
	bool load(int ndx, const string &dir, const TechTree *techTree);
	bool loadSpecTraitSkills(int ndx, const string &dir, const TechTree *techTree);

	bool guiPreLoad(const string &dir, const TechTree *techTree);

	bool preLoadGlestimals(const string &dir, const TechTree *techTree);
	bool loadGlestimals(const string &dir, const TechTree *techTree);

	void doChecksum(Checksum &checksum) const;
	void setLoadableUnitType(UnitType *ut)  { loadableUnitTypes.insert(ut); }
	~FactionType();

	//get
	int getGuiFileNamesCount() const                    {return guiFileNames.size();}
	string getGuiFileName(int i) const                  {return guiFileNames[i];}
	int getUnitTypeCount() const						{return unitTypes.size();}
	int getUpgradeTypeCount() const						{return upgradeTypes.size();}
	int getItemImagesCount() const                      {return itemImages.size();}
	Texture2D *getItemImage(int i) const                {return itemImages[i];}
    int getItemTypeCount() const						{return itemTypes.size();}
	const UnitType *getUnitType(int i) const			{return unitTypes[i];}
	const ItemType *getItemType(int i) const			{return itemTypes[i];}
	const UpgradeType *getUpgradeType(int i) const		{return upgradeTypes[i];}

	int getEventTypeCount() const                       {return eventTypes.size();}
	const EventType *getEventType(int i) const          {return eventTypes[i];}
	EventType *getEventType(int i)                      {return eventTypes[i];}

	StrSound *getMusic() const							{return music;}
	int getStartingUnitCount() const					{return startingUnits.size();}
	const UnitType *getStartingUnit(int i) const		{return startingUnits[i].first;}
	int getStartingUnitAmount(int i) const				{return startingUnits[i].second;}
	const SoundContainer *getAttackNotice() const		{return attackNotice;}
	const SoundContainer *getEnemyNotice() const		{return enemyNotice;}
	int getAttackNoticeDelay() const					{return attackNoticeDelay;}
	const Pixmap2D* getLogoTeamColour() const			{return m_logoTeamColour;}
	const Pixmap2D* getLogoRgba() const					{return m_logoRgba;}

	int getEnemyNoticeDelay() const						{return enemyNoticeDelay;}

	const UnitType *getUnitType(const string &name) const;
	const ItemType *getItemType(const string &name) const;
	const UpgradeType *getUpgradeType(const string &name) const;
	int getStartingResourceAmount(const ResourceType *resourceType) const;
};

}}//end namespace

#endif

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

#ifndef _GLEST_GAME_FACTIONTYPE_H_
#define _GLEST_GAME_FACTIONTYPE_H_

#include "unit_type.h"
#include "upgrade_type.h"
#include "sound.h"

using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;

namespace Glest{ namespace ProtoTypes {

class TechTree;
class MusicPlaylistType;

// =====================================================
// 	class SubfactionType
//
// =====================================================

class SubfactionType{
public:
    SubfactionType(string strName): name(strName), musicPlaylist(0) {}
    ~SubfactionType();

    string getName() const { return name; }

    void setMusic(MusicPlaylistType *newMusic);
    const MusicPlaylistType *getMusic() const { return musicPlaylist; }

private:
	string name;
    MusicPlaylistType *musicPlaylist;

	//FIXME: We should probably play a sound, display a banner-type notice or
	//something.
	//SoundContainer *advancementNotice;
};

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
	typedef vector<StoredResource> Resources;
	typedef vector<SubfactionType*> Subfactions;

private:
	UnitTypes unitTypes;
	UpgradeTypes upgradeTypes;
	StartingUnits startingUnits;
	Resources startingResources;
	Subfactions subfactions;
	MusicPlaylistType *music;   // legacy music xml, converted under the hood to new musicplaylist runtime classes
	SoundContainer *attackNotice;
	SoundContainer *enemyNotice;
	int attackNoticeDelay;
	int enemyNoticeDelay;
	UnitTypeSet loadableUnitTypes;
	Pixmap2D *m_logoTeamColour, *m_logoRgba;
    MusicPlaylistType *m_playlist;

public:
	//init
	FactionType();

	bool preLoad(const string &dir, const TechTree *techTree);
	bool load(int ndx, const string &dir, const TechTree *techTree);

	bool preLoadGlestimals(const string &dir, const TechTree *techTree);
	bool loadGlestimals(const string &dir, const TechTree *techTree);

	void doChecksum(Checksum &checksum) const;
	void setLoadableUnitType(UnitType *ut)  { loadableUnitTypes.insert(ut); }
	~FactionType();

	//get
	int getUnitTypeCount() const						{return unitTypes.size();}
	int getUpgradeTypeCount() const						{return upgradeTypes.size();}
	int getSubfactionCount() const						{return subfactions.size();}
	const UnitType *getUnitType(int i) const			{return unitTypes[i];}
	const UpgradeType *getUpgradeType(int i) const		{return upgradeTypes[i];}
	const SubfactionType *getSubfaction(int i) const	{return subfactions[i];}
	int getSubfactionIndex(const string &name) const;
	MusicPlaylistType *getMusic() const				    {return music;}
	int getStartingUnitCount() const					{return startingUnits.size();}
	const UnitType *getStartingUnit(int i) const		{return startingUnits[i].first;}
	int getStartingUnitAmount(int i) const				{return startingUnits[i].second;}
	const SoundContainer *getAttackNotice() const		{return attackNotice;}
	const SoundContainer *getEnemyNotice() const		{return enemyNotice;}
	int getAttackNoticeDelay() const					{return attackNoticeDelay;}
	const Pixmap2D* getLogoTeamColour() const			{return m_logoTeamColour;}
	const Pixmap2D* getLogoRgba() const					{return m_logoRgba;}
    MusicPlaylistType *getMusicPlaylist() const         {return m_playlist;}

	int getEnemyNoticeDelay() const						{return enemyNoticeDelay;}

	const UnitType *getUnitType(const string &name) const;
	const UpgradeType *getUpgradeType(const string &name) const;
	int getStartingResourceAmount(const ResourceType *resourceType) const;
};

}}//end namespace

#endif

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

#ifndef _GLEST_GAME_UPGRADE_H_
#define _GLEST_GAME_UPGRADE_H_

#include <vector>
#include "xml_parser.h"
#include "game_constants.h"

using std::vector;
using Shared::Xml::XmlNode;

namespace Glest{ namespace Game{

class Unit;
class UpgradeType;
class Game;

class UpgradeManager;
class EnhancementTypeBase;
class FactionType;

// =====================================================
// 	class Upgrade
//
/// A bonus to an UnitType
// =====================================================

class Upgrade{
private:
	UpgradeState state;
	int factionIndex;
	const UpgradeType *type;

	friend class UpgradeManager;

public:
	Upgrade(const XmlNode *node, const FactionType *ft);
	Upgrade(const UpgradeType *upgradeType, int factionIndex);

	//get
	const UpgradeType * getType() const;
	UpgradeState getState() const;

	void save(XmlNode *node) const;

private:
	//get
	int getFactionIndex() const;

	//set
	void setState(UpgradeState state);
};


// ===============================
// 	class UpgradeManager
// ===============================

class UpgradeManager{
private:
	typedef vector<Upgrade*> Upgrades;
	Upgrades upgrades;

public:
	~UpgradeManager();

	int getUpgradeCount() const		{return upgrades.size();}
	const Upgrade &getUpgrade(int i) const	{return *upgrades[i];}

	void startUpgrade(const UpgradeType *upgradeType, int factionIndex);
	void cancelUpgrade(const UpgradeType *upgradeType);
	void finishUpgrade(const UpgradeType *upgradeType);

	bool isUpgraded(const UpgradeType *upgradeType) const;
	bool isUpgrading(const UpgradeType *upgradeType) const;
	bool isUpgradingOrUpgraded(const UpgradeType *upgradeType) const;
	void computeTotalUpgrade(const Unit *unit, EnhancementTypeBase *totalUpgrade) const;

	void load(const XmlNode *node, const FactionType *ft);
	void save(XmlNode *node) const;
};

}}//end namespace

#endif

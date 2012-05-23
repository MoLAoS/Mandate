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
using std::vector;

#include "xml_parser.h"
using Shared::Xml::XmlNode;

#include "game_constants.h"
#include "forward_decs.h"

namespace Glest { namespace Entities {

using namespace Glest::ProtoTypes;
using Shared::Xml::XmlNode;
using Sim::EntityFactory;

// =====================================================
// 	class Upgrade
//
/// A bonus to an UnitType
// =====================================================

class Upgrade {
	friend class EntityFactory<Upgrade>;
	friend class UpgradeManager;

private:
	int m_id;
	UpgradeState state;
	int factionIndex;
	const UpgradeType *type;

public:
	struct CreateParams {
		const UpgradeType *upgradeType;
		int factionIndex;
		CreateParams(const UpgradeType *type, int fNdx) :
		upgradeType(type), factionIndex(fNdx) {}
	};
	struct LoadParams {
		const XmlNode *node;
		Faction *faction;
		LoadParams(const XmlNode *n, Faction *f) : node(n), faction(f) {}
	};

public:
	Upgrade(LoadParams params);
	Upgrade(CreateParams params);

	void setId(int v) { m_id = v; }

public:
	//get
	int getId() const { return m_id; }
	const UpgradeType * getType() const;
	UpgradeState getState() const;

	void save(XmlNode *node) const;

	MEMORY_CHECK_DECLARATIONS(Upgrade)

private:
	//get
	int getFactionIndex() const;

	//set
	void setState(UpgradeState state);
};


// ===============================
// 	class UpgradeManager
// ===============================

class UpgradeManager {
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
	void updateUpgrade(const Upgrade *upgrade);

	bool isUpgraded(const UpgradeType *upgradeType) const;
	bool isUpgrading(const UpgradeType *upgradeType) const;
	bool isUpgradingOrUpgraded(const UpgradeType *upgradeType) const;
	void computeTotalUpgrade(const Unit *unit, EnhancementType *totalUpgrade) const;
	void addPointBoosts(Unit *unit) const;

	void load(const XmlNode *node, Faction *f);
	void save(XmlNode *node) const;
};

}}//end namespace

#endif

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

#ifndef _GLEST_GAME_SELECTION_
#define _GLEST_GAME_SELECTION_

#include <vector>
#include <map>

#include "unit.h"
#include "sigslot.h"

using std::vector;
using std::map;

namespace Glest { namespace Gui {

class UserInterface;

WRAPPED_ENUM( AutoCmdFlag,
	REPAIR,
	ATTACK,
	FLEE
)

// AutoCmdState : describes an auto command category state of 
// the selection (auto-repair, auto-attack & auto-flee)
WRAPPED_ENUM( AutoCmdState,
	NONE,
	ALL_ON,
	ALL_OFF,
	MIXED
)

// =====================================================
// 	class Selection
//
///	List of selected units and groups
// =====================================================

class Selection: public sigslot::has_slots {
public:
	typedef UnitVector::const_iterator UnitIterator;
	typedef map<Unit*, int> UnitRefMap;

public:
	static const int maxGroups = 10;
	static const int maxUnits = 24;

private:
	bool empty;
	bool enemy;
	bool uniform;
	bool commandable;
	bool cancelable;
	bool meetable;
	bool canRepair;
	AutoCmdState m_autoCmdStates[AutoCmdFlag::COUNT];
	int factionIndex;
	UnitVector selectedUnits;
	UnitVector groups[maxGroups];
	UserInterface *gui;

public:
	Selection();
	virtual ~Selection();
	void init(UserInterface *gui, int factionIndex);

	void select(Unit *unit);
	void select(const UnitVector &units) {
		//add units to gui
		for(UnitIterator it= units.begin(); it!=units.end(); ++it) {
			select(*it);
		}
	}
	void unSelect(const UnitVector &units);
	void unSelect(const Unit *unit);
	void unSelectAllOfType(const UnitType *type);
	void unSelectAllNotOfType(const UnitType *type);
	void clear();

	bool isEmpty() const				{return empty;}
	bool isUniform() const				{return uniform;}
	bool isEnemy() const				{return enemy;}
	bool isComandable() const			{return commandable;}
	bool isCancelable() const			{return cancelable;}
	bool isMeetable() const				{return meetable;}
	bool isCanRepair() const			{return canRepair;}
	bool hasUnit(const Unit *unit) const {
		return (std::find(selectedUnits.begin(), selectedUnits.end(), unit) != selectedUnits.end());
			}

	int getCount() const					{return selectedUnits.size();}
	const Unit *getUnit(int i) const		{return selectedUnits[i];}
	const Unit *getFrontUnit() const		{return selectedUnits.front();}
	const UnitVector &getUnits() const	{return selectedUnits;}
	Vec3f getRefPos() const;
	AutoCmdState getAutoRepairState() const	{return m_autoCmdStates[AutoCmdFlag::REPAIR];}

	void assignGroup(int groupIndex);
	void recallGroup(int groupIndex);

	void update();
	void removeCarried();

	void load(const XmlNode *node);
	void save(XmlNode *node) const;

	void onUnitStateChanged(Unit *unit);

	void clearDeadUnits();

protected:
	void unSelect(UnitVector::iterator it);
};

}}//end namespace

#endif

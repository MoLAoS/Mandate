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

#include "unit.h"
#include <vector>

using std::vector;

namespace Glest{ namespace Game{

class Gui;

enum AutoRepairState {
	arsOn,
	arsOff,
	arsMixed
};

// =====================================================
// 	class Selection
//
///	List of selected units and groups
// =====================================================

class Selection: public UnitObserver{
public:
	typedef vector<Unit*> UnitContainer;
	typedef UnitContainer::const_iterator UnitIterator;

public:
	static const int maxGroups= 10;
	static const int maxUnits= 16;

private:
	bool empty;
	bool enemy;
	bool uniform;
	bool commandable;
	bool cancelable;
	bool meetable;
	bool canRepair;
	AutoRepairState autoRepairState;
	int factionIndex;
	UnitContainer selectedUnits;
	UnitContainer groups[maxGroups];
	Gui *gui;

public:
	Selection();
	virtual ~Selection();
	void init(Gui *gui, int factionIndex);

	void select(Unit *unit);
	void select(const UnitContainer &units) {
		//add units to gui
		for(UnitIterator it= units.begin(); it!=units.end(); ++it){
			select(*it);
		}
	}
	void unSelect(const UnitContainer &units);
	void clear();

	bool isEmpty() const				{return empty;}
	bool isUniform() const				{return uniform;}
	bool isEnemy() const				{return enemy;}
	bool isComandable() const			{return commandable;}
	bool isCancelable() const			{return cancelable;}
	bool isMeetable() const				{return meetable;}
	bool isCanRepair() const			{return canRepair;}

	int getCount() const				{return selectedUnits.size();}
	const Unit *getUnit(int i) const	{return selectedUnits[i];}
	const Unit *getFrontUnit() const	{return selectedUnits.front();}
	Vec3f getRefPos() const;
	AutoRepairState getAutoRepairState() const	{return autoRepairState;}

	void assignGroup(int groupIndex);
	void recallGroup(int groupIndex);

	virtual void unitEvent(UnitObserver::Event event, const Unit *unit);
	void update();

protected:
	void unSelect(int unitIndex);
};

}}//end namespace

#endif

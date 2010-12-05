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

// =====================================================
// 	class Selection
//
///	List of selected units and m_groups
// =====================================================

class Selection: public sigslot::has_slots {
public:
	typedef UnitVector::const_iterator UnitIterator;
	typedef map<Unit*, int> UnitRefMap;

public:
	static const int maxGroups = 10;
	static const int maxUnits = 24;

private:
	bool			m_empty;
	bool			m_enemy;
	bool			m_uniform;
	bool			m_commandable;
	bool			m_cancelable;
	bool			m_meetable;
	bool			m_canRepair;
	bool			m_canAttack;
	bool			m_canMove;
	bool			m_canCloak;
	AutoCmdState	m_autoCmdStates[AutoCmdFlag::COUNT];
	int				m_factionIndex;
	UnitVector		m_selectedUnits;
	UnitVector		m_groups[maxGroups];
	UserInterface*	m_gui;

public:
	Selection();
	virtual ~Selection();
	void init(UserInterface *m_gui, int m_factionIndex);

	void select(Unit *unit);
	void select(const UnitVector &units) {
		//add units to m_gui
		for(UnitIterator it= units.begin(); it!=units.end(); ++it) {
			select(*it);
		}
	}
	void unSelect(const UnitVector &units);
	void unSelect(const Unit *unit);
	void unSelectAllOfType(const UnitType *type);
	void unSelectAllNotOfType(const UnitType *type);
	void clear();

	bool isEmpty() const				{return m_empty;}
	bool isUniform() const				{return m_uniform;}
	bool isEnemy() const				{return m_enemy;}
	bool isComandable() const			{return m_commandable;}
	bool isCancelable() const			{return m_cancelable;}
	bool isMeetable() const				{return m_meetable;}
	bool canRepair() const				{return m_canRepair;}
	bool canAttack() const				{return m_canAttack;}
	bool canMove() const				{return m_canMove;}
	bool canCloak() const				{return m_canCloak;}
	bool anyCloaked() const {
		foreach_const (UnitVector, it, m_selectedUnits) {
			if ((*it)->isCloaked()) {
				return true;
			}
		}
		return false;
	}
	bool hasUnit(const Unit *unit) const {
		return (std::find(m_selectedUnits.begin(), m_selectedUnits.end(), unit) != m_selectedUnits.end());
	}

	const Texture2D* getCommandImage(CommandClass cc) const;
	const Texture2D* getCloakImage() const;

	int getCount() const					{return m_selectedUnits.size();}
	const Unit *getUnit(int i) const		{return m_selectedUnits[i];}
	const Unit *getFrontUnit() const		{return m_selectedUnits.front();}
	const UnitVector &getUnits() const	{return m_selectedUnits;}
	Vec3f getRefPos() const;
	AutoCmdState getAutoCmdState(AutoCmdFlag f) const	{return m_autoCmdStates[f];}
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

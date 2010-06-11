// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_COMPOUND_WIDGETS_INCLUDED_
#define _GLEST_COMPOUND_WIDGETS_INCLUDED_

#include "widgets.h"
#include "game_constants.h"

namespace Glest { namespace Widgets {
using Sim::ControlType;

class PlayerSlotWidget : public Panel, public sigslot::has_slots {
public:
	typedef PlayerSlotWidget* Ptr;
private:
	StaticText::Ptr stLabel;
	DropList::Ptr	dlControl;
	DropList::Ptr	dlFaction;
	DropList::Ptr	dlTeam;

public:
	PlayerSlotWidget(Container::Ptr parent, Vec2i pos, Vec2i size);

	void setNameText(const string &name) { stLabel->setText(name); }
	
	void setFactionItems(const vector<string> &items) {
		dlFaction->clearItems();
		dlFaction->addItems(items);
	}

	void setSelectedControl(ControlType ct) {
		dlControl->setSelected(ct);
	}
	void setSelectedFaction(int ndx) {
		dlFaction->setSelected(ndx);
	}

	void setSelectedTeam(int team) {
		assert (team >= -1 && team < GameConstants::maxPlayers);
		dlTeam->setSelected(team);
	}

	ControlType getControlType() const { return ControlType(dlControl->getSelectedIndex()); }
	int getSelectedFactionIndex() const { return dlFaction->getSelectedIndex(); }
	int getSelectedTeamIndex() const { return dlTeam->getSelectedIndex(); }

	sigslot::signal<Ptr> ControlChanged;
	sigslot::signal<Ptr> FactionChanged;
	sigslot::signal<Ptr> TeamChanged;

private: // re-route signals from DropLists
	void onControlChanged(ListBase::Ptr) { ControlChanged(this); }
	void onFactionChanged(ListBase::Ptr) { FactionChanged(this); }
	void onTeamChanged(ListBase::Ptr)	 { TeamChanged(this);	 }
};

}} // namespace Glest::Widgets

#endif

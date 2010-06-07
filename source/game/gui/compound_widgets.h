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

namespace Glest { namespace Widgets {
using Sim::ControlType;

class PlayerSlotWidget : public Panel {
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
		assert (team >= 1 && team < GameConstants::maxPlayers);
		dlTeam->setSelected(team - 1);
	}

	sigslot::signal<Ptr> ControlChanged;
	sigslot::signal<Ptr> FactionChanged;
	sigslot::signal<Ptr> TeamChanged;
};

}} // namespace Glest::Widgets

#endif

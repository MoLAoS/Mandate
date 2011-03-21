// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST__WIDGETS__PLAYER_SLOT_WIDGET__INCLUDED_
#define _GLEST__WIDGETS__PLAYER_SLOT_WIDGET__INCLUDED_

#include "complex_widgets.h"
#include "game_constants.h"
#include "lang.h"

namespace Glest { namespace Widgets {
using Sim::ControlType;
using Global::Lang;

class ColourButton : public Button {
private:
	Colour	m_colourBase, m_colourOutline;

public:
	ColourButton(Container *parent);
	ColourButton(Container *parent, Vec2i pos, Vec2i size);
	void clearColour();
	void setColour(Colour base, Colour outline);
	Colour getColour() const { return m_colourBase; }
	void render() {
		Button::render();
	}
	virtual void setStyle() override;
	virtual string descType() const override { return "ColourButton"; }
};

class FloatingPanel : public Panel {
public:
	FloatingPanel(WidgetWindow *window) : Panel(window) {
		setStyle(g_widgetConfig.getWidgetStyle(WidgetType::LIST_BOX));
		m_backgroundStyle.setNone();
	}

	virtual string descType() const override { return "FloatingPanel"; }
};

class ColourPicker : public Panel, public sigslot::has_slots {
private:
	ColourButton*	m_showingItem;
	Button*			m_button;

	FloatingPanel*	m_dropDownPanel;
	ColourButton*	m_colourButtons[GameConstants::maxColours];

	int				m_selectedIndex;

private:
	void onExpand(Button*);
	void onSelect(Button*);

	void init();

public:
	ColourPicker(Container *parent);
	ColourPicker(Container *parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz);

	void setColour(int index);
	int getColourIndex()	{ return m_selectedIndex; }

	sigslot::signal<ColourPicker*> ColourChanged;

	virtual string descType() const override { return "ColourPicker"; }
};

class PlayerSlotLabels : public CellStrip {
public:
	PlayerSlotLabels(Container* parent);
};

class PlayerSlotWidget : public CellStrip, public sigslot::has_slots {
private:
	StaticText*		m_label;
	DropList*		m_controlList;
	DropList*		m_factionList;
	DropList*		m_teamList;
	ColourPicker*	m_colourPicker;

	bool	m_freeSlot; // closed but open-able

public:
	PlayerSlotWidget(Container* parent);

	void setNameText(const string &name) { m_label->setText(name); }
	
	void setFactionItems(const vector<string> &items) {
		m_factionList->clearItems();
		m_factionList->addItems(items);
		m_factionList->addItem(g_lang.get("Random"));
	}

	void setSelectedControl(ControlType ct) {
		if (ControlType(m_controlList->getSelectedIndex()) == ControlType::CLOSED
		&& ct != ControlType::CLOSED) {
			enableSlot();
		}
		m_controlList->setSelected(ct);
		if (ct == ControlType::CLOSED) {
			disableSlot();
		}
	}

	void setSelectedFaction(int ndx) {
		m_factionList->setSelected(ndx);
	}

	void setSelectedTeam(int team) {
		assert (team >= -1 && team < GameConstants::maxPlayers);
		m_teamList->setSelected(team);
	}

	void setSelectedColour(int ndx) {
		assert (ndx >= -1 && ndx < GameConstants::maxColours);
		m_colourPicker->setColour(ndx);
	}

	void setFree(bool v);

	ControlType getControlType() const { return ControlType(m_controlList->getSelectedIndex()); }
	int getSelectedFactionIndex() const { return m_factionList->getSelectedIndex(); }
	int getSelectedTeamIndex() const { return m_teamList->getSelectedIndex(); }
	int getSelectedColourIndex() const { return m_colourPicker->getColourIndex(); }

	sigslot::signal<PlayerSlotWidget*> ControlChanged;
	sigslot::signal<PlayerSlotWidget*> FactionChanged;
	sigslot::signal<PlayerSlotWidget*> TeamChanged;
	sigslot::signal<PlayerSlotWidget*> ColourChanged;

private:
	void disableSlot();
	void enableSlot();

private: // re-route signals from DropLists
	void onControlChanged(ListBase*) {
		ControlChanged(this);
		if (m_controlList->getSelectedIndex() == ControlType::CLOSED) {
			m_factionList->setEnabled(false);
			m_teamList->setEnabled(false);
			m_colourPicker->setEnabled(false);
		} else {
			m_factionList->setEnabled(true);
			m_teamList->setEnabled(true);
			m_colourPicker->setEnabled(true);
		}
	}
	void onFactionChanged(ListBase*) { FactionChanged(this); }
	void onTeamChanged(ListBase*)	 { TeamChanged(this);	 }
	void onColourChanged(ColourPicker*)	 { ColourChanged(this);	 }

	virtual string descType() const override { return "PlayerSlotWidget"; }
};

}} // namespace Glest::Widgets

#endif

// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST__WIDGETS__PLAYER_SLOT_WIDGET__INCLUDED_
#define _GLEST__WIDGETS__PLAYER_SLOT_WIDGET__INCLUDED_

#include "widgets.h"
#include "game_constants.h"
#include "lang.h"

namespace Glest { namespace Widgets {
using Sim::ControlType;
using Global::Lang;

class ColourButton : public Button {
private:
	Colour	m_colourBase, m_colourOutline;

public:
	ColourButton(Container *parent)
			: Button(parent, Vec2i(0), Vec2i(32), false, false)
			, m_colourBase(g_widgetConfig.getColour(WidgetColour::DARK_BACKGROUND))
			, m_colourOutline(g_widgetConfig.getColour(WidgetColour::DARK_BACKGROUND)) {
		m_borderStyle.setSizes(4);
		m_borderStyle.setSolid(g_widgetConfig.getColourIndex(m_colourOutline));
		m_backgroundStyle.setColour(g_widgetConfig.getColourIndex(m_colourBase));
	}

	ColourButton(Container *parent, Vec2i pos, Vec2i size)
			: Button(parent, pos, size, false, false)
			, m_colourBase(g_widgetConfig.getColour(WidgetColour::DARK_BACKGROUND))
			, m_colourOutline(g_widgetConfig.getColour(WidgetColour::DARK_BACKGROUND)) {
		m_borderStyle.setSizes(4);
		m_borderStyle.setSolid(g_widgetConfig.getColourIndex(m_colourOutline));
		m_backgroundStyle.setColour(g_widgetConfig.getColourIndex(m_colourBase));
	}

	void clearColour() {
		m_colourBase =  m_colourOutline = g_widgetConfig.getColour(WidgetColour::DARK_BACKGROUND);
		m_backgroundStyle.setColour(WidgetColour::DARK_BACKGROUND);
		m_borderStyle.setSolid(WidgetColour::DARK_BACKGROUND);
	}

	void setColour(Colour base, Colour outline) {
		m_colourBase = base;
		m_colourOutline = outline;
		m_backgroundStyle.setColour(g_widgetConfig.getColourIndex(m_colourBase));
		m_borderStyle.setSolid(g_widgetConfig.getColourIndex(m_colourOutline));
	}

	Colour getColour() const { return m_colourBase; }

	void render() {
		Button::render();
	}

};

class FloatingPanel : public Panel {
public:
	FloatingPanel(WidgetWindow *window) : Panel(window) {
		m_borderStyle.setSizes(2);
		m_borderStyle.setEmbed(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
	}
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
};

class PlayerSlotPanel : public Panel {
private:
	Widget* m_columns[5];
	int m_childCount;

public:
	PlayerSlotPanel(Container* parent, Vec2i pos, Vec2i size);

	virtual void addChild(Widget* child);
	virtual void layoutChildren();
};

class PlayerSlotLabels : public PlayerSlotPanel {
public:
	PlayerSlotLabels(Container* parent, Vec2i pos, Vec2i size);
};

class PlayerSlotWidget : public PlayerSlotPanel, public sigslot::has_slots {
private:
	StaticText*		m_label;
	DropList*		m_controlList;
	DropList*		m_factionList;
	DropList*		m_teamList;
	ColourPicker*		m_colourPicker;

	bool	m_freeSlot; // closed but open-able

public:
	PlayerSlotWidget(Container* parent, Vec2i pos, Vec2i size);

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
};

}} // namespace Glest::Widgets

#endif

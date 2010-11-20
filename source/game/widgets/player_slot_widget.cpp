// ==============================================================
//	This file is part of The Glest Advanced Engi
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "player_slot_widget.h"
#include "widget_window.h"
#include "core_data.h"
#include "lang.h"
#include "faction.h"
#include "program.h"

#include "leak_dumper.h"

#include <list>

namespace Glest { namespace Widgets {
using Shared::Util::intToStr;
using Global::CoreData;
using Global::Lang;
using Sim::ControlTypeNames;

ColourPicker::ColourPicker(Container *parent)
		: Panel(parent)
		, m_dropDownPanel(0)
		, m_selectedIndex(-1) {
	init();
}

ColourPicker::ColourPicker(Container *parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size)
		, m_dropDownPanel(0)
		, m_selectedIndex(-1) {
	init();
	setSize(size);
}

void ColourPicker::init() {
	setAutoLayout(false);
	setPadding(0);
	m_borderStyle.setNone();
	m_showingItem = new ColourButton(this);
	m_button = new Button(this);
	m_button->Clicked.connect(this, &ColourPicker::onExpand);
	m_showingItem->Clicked.connect(this, &ColourPicker::onExpand);
}

void ColourPicker::setSize(const Vec2i &sz) {
	Panel::setSize(sz);
	Vec2i space = sz - getBordersAll();
	//int h = space.y;
	
	Vec2i itemPos(getBorderLeft(), getBorderBottom());
	Vec2i itemSize(space.x - space.y, space.y);
	Vec2i btnPos(getBorderLeft() + space.x - space.y, getBorderBottom());
	Vec2i btnSize(space.y, space.y);

	m_showingItem->setPos(itemPos);
	m_showingItem->setSize(itemSize);
	m_button->setPos(btnPos);
	m_button->setSize(btnSize);
}

void ColourPicker::onExpand(Button*) {
	Vec2i size(4 * 32 + 4, 4 * 32 + 4);
	Vec2i pos(getScreenPos());
	pos.y -= 4 * 32 + 4;

	m_dropDownPanel = new FloatingPanel(WidgetWindow::getInstance());
	m_dropDownPanel->setPos(pos);
	m_dropDownPanel->setSize(size);

	int y = 3 * 32 + 2;
	int x = 2;
	for (int i=0; i < 4; ++i) {
		for (int j=0; j < 4; ++j) {
			m_colourButtons[i*4+j] = new ColourButton(m_dropDownPanel, Vec2i(x,y), Vec2i(32,32));
			m_colourButtons[i*4+j]->setColour(factionColours[i*4+j], factionColoursOutline[i*4+j]);
			m_colourButtons[i*4+j]->Clicked.connect(this, &ColourPicker::onSelect);
			x += 32;
		}
		x = 2;
		y -= 32;
	}
	WidgetWindow::getInstance()->setFloatingWidget(m_dropDownPanel);
}

void ColourPicker::setColour(int index) {
	if (index < 0) {
		m_showingItem->clearColour();
		m_selectedIndex = -1;
	} else {
		assert(index < GameConstants::maxColours);
		m_showingItem->setColour(factionColours[index], factionColoursOutline[index]);
		m_selectedIndex = index;
	}
}

void ColourPicker::onSelect(Button *b) {
	WidgetWindow::getInstance()->removeFloatingWidget(m_dropDownPanel);
	ColourButton *btn = static_cast<ColourButton*>(b);
	int i=0;
	for ( ; i < GameConstants::maxColours; ++i) {
		if (m_colourButtons[i] == btn) {
			break;
		}
	}
	assert(i < GameConstants::maxColours);
	m_selectedIndex = i;
	setColour(i);
	ColourChanged(this);
}


// =====================================================
//  class PlayerSlotPanel
// =====================================================

PlayerSlotPanel::PlayerSlotPanel(Container* parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size), m_childCount(0) {
	Panel::setAutoLayout(false);
}

void PlayerSlotPanel::addChild(Widget* child) {
	Panel::addChild(child);
	assert(m_childCount >=0 && m_childCount < 5);
	m_columns[m_childCount++] = child;
}

void PlayerSlotPanel::layoutChildren() {
	assert(m_childCount == 5);
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::DROP_LIST);
	Widget::setPadding(0);
	assert(getWidth() > 200);
	float size_x = float(getWidth() - 30);
	float fwidths[] = {
		size_x * 18.f / 100.f,
		size_x * 33.f / 100.f,
		size_x * 33.f / 100.f,
		size_x * 8.f / 100.f,
		size_x * 8.f / 100.f
	};
	int widths[5];
	for (int i=0; i < 5; ++i) {
		widths[i] = int(fwidths[i]);
	}
	Vec2i cpos(5, 2);

	m_columns[0]->setPos(cpos);
	m_columns[0]->setSize(Vec2i(widths[0], 30));

	cpos.x += widths[0] + 5;
	m_columns[1]->setPos(cpos);
	m_columns[1]->setSize(Vec2i(widths[1], 30));

	cpos.x += widths[1] + 5;
	m_columns[2]->setPos(cpos);
	m_columns[2]->setSize(Vec2i(widths[2], 30));
	
	cpos.x += widths[2] + 5;
	m_columns[3]->setPos(cpos);
	m_columns[3]->setSize(Vec2i(widths[3], 30));
	
	cpos.x += widths[3] + 5;
	m_columns[4]->setPos(cpos);
	m_columns[4]->setSize(Vec2i(widths[4], 30));
}

// =====================================================
//  class PlayerSlotLabels
// =====================================================

PlayerSlotLabels::PlayerSlotLabels(Container* parent, Vec2i pos, Vec2i size)
		: PlayerSlotPanel(parent, pos, size) {
	CoreData &coreData = CoreData::getInstance();
	
	StaticText *label = new StaticText(this);
	label->setTextParams(g_lang.get("Player"), Vec4f(1.f), coreData.getFTMenuFontNormal(), true);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label = new StaticText(this);
	label->setTextParams(g_lang.get("Control"), Vec4f(1.f), coreData.getFTMenuFontNormal(), true);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label = new StaticText(this);
	label->setTextParams(g_lang.get("Faction"), Vec4f(1.f), coreData.getFTMenuFontNormal(), true);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label = new StaticText(this);
	label->setTextParams(g_lang.get("Team"), Vec4f(1.f), coreData.getFTMenuFontNormal(), false);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label = new StaticText(this);
	label->setTextParams(g_lang.get("Colour"), Vec4f(1.f), coreData.getFTMenuFontNormal(), false);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	layoutChildren();
}

// =====================================================
//  class PlayerSlotWidget
// =====================================================

PlayerSlotWidget::PlayerSlotWidget(Container* parent, Vec2i pos, Vec2i size)
		: PlayerSlotPanel(parent, pos, size), m_freeSlot(false) {
	CoreData &coreData = CoreData::getInstance();
	m_label = new StaticText(this);
	m_label->setTextParams("Player #", Vec4f(1.f), coreData.getFTMenuFontNormal(), true);

	m_controlList = new DropList(this);
	foreach_enum (ControlType, ct) {
		m_controlList->addItem(g_lang.get(ControlTypeNames[ct]));
	}

	m_factionList = new DropList(this);
	m_factionList->setDropBoxHeight(200);
	m_teamList = new DropList(this);
	for (int i=1; i <= GameConstants::maxPlayers; ++i) {
		m_teamList->addItem(intToStr(i));
	}
	m_colourPicker = new Widgets::ColourPicker(this);
	m_controlList->SelectionChanged.connect(this, &PlayerSlotWidget::onControlChanged);
	m_factionList->SelectionChanged.connect(this, &PlayerSlotWidget::onFactionChanged);
	m_teamList->SelectionChanged.connect(this, &PlayerSlotWidget::onTeamChanged);
	m_colourPicker->ColourChanged.connect(this, &PlayerSlotWidget::onColourChanged);

	layoutChildren();
}

void PlayerSlotWidget::disableSlot() {
	setSelectedFaction(-1);
	setSelectedTeam(-1);
	setSelectedColour(-1);
	m_factionList->setEnabled(false);
	m_teamList->setEnabled(false);
	m_colourPicker->setEnabled(false);
}

void PlayerSlotWidget::enableSlot() {
	m_factionList->setEnabled(true);
	m_teamList->setEnabled(true);
	m_colourPicker->setEnabled(true);
}

void PlayerSlotWidget::setFree(bool v) {
	m_freeSlot = v;
	if (v) {
		m_controlList->setEnabled(true);
		m_factionList->setEnabled(false);
		m_teamList->setEnabled(false);
		m_colourPicker->setEnabled(false);
	} else {
		m_controlList->setEnabled(true);
		m_factionList->setEnabled(true);
		m_teamList->setEnabled(true);
		m_colourPicker->setEnabled(true);
	}
}

}}

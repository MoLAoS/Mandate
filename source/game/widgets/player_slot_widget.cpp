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

ColourButton::ColourButton(Container *parent)
		: Button(parent, Vec2i(0), Vec2i(32)) {
	setWidgetStyle(WidgetType::COLOUR_BUTTON);
	m_colourBase = getRootWindow()->getConfig()->getColour(m_backgroundStyle.m_colourIndices[0]);
	m_colourOutline = getRootWindow()->getConfig()->getColour(m_borderStyle.m_colourIndices[0]);
}

ColourButton::ColourButton(Container *parent, Vec2i pos, Vec2i size)
		: Button(parent, pos, size) {
	setWidgetStyle(WidgetType::COLOUR_BUTTON);
}

void ColourButton::clearColour() {
	setWidgetStyle(WidgetType::COLOUR_BUTTON);
	m_colourBase = getRootWindow()->getConfig()->getColour(m_backgroundStyle.m_colourIndices[0]);
	m_colourOutline = getRootWindow()->getConfig()->getColour(m_borderStyle.m_colourIndices[0]);
}

void ColourButton::setColour(Colour base, Colour outline) {
	m_colourBase = base;
	m_colourOutline = outline;
	m_backgroundStyle.setColour(g_widgetConfig.getColourIndex(m_colourBase));
	m_borderStyle.setSolid(g_widgetConfig.getColourIndex(m_colourOutline));
}

void ColourButton::setStyle() {
	setWidgetStyle(WidgetType::COLOUR_BUTTON);

	m_backgroundStyle.setColour(g_widgetConfig.getColourIndex(m_colourBase));
	m_borderStyle.setSolid(g_widgetConfig.getColourIndex(m_colourOutline));
}

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
	pos += getSize() / 2;
	pos -= size / 2;

	m_dropDownPanel = new FloatingPanel(getRootWindow());
	m_dropDownPanel->setPos(pos);
	m_dropDownPanel->setSize(size);

	int y = 2;
	int x = 2;
	for (int i=0; i < 4; ++i) {
		for (int j=0; j < 4; ++j) {
			m_colourButtons[i*4+j] = new ColourButton(m_dropDownPanel, Vec2i(x,y), Vec2i(32,32));
			m_colourButtons[i*4+j]->setColour(factionColours[i*4+j], factionColoursOutline[i*4+j]);
			m_colourButtons[i*4+j]->Clicked.connect(this, &ColourPicker::onSelect);
			x += 32;
		}
		x = 2;
		y += 32;
	}
	getRootWindow()->setFloatingWidget(m_dropDownPanel);
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
//  class PlayerSlotLabels
// =====================================================

PlayerSlotLabels::PlayerSlotLabels(Container* parent)
		: CellStrip(parent, Orientation::HORIZONTAL, Origin::CENTRE, 5) {
	CoreData &coreData = CoreData::getInstance();
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	int hints[] = { 18, 33, 33, 8, 8 };
	setPercentageHints(hints);
	
	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	StaticText *label = new StaticText(this);
	label->setCell(0);
	label->setTextParams(g_lang.get("Player"), Vec4f(1.f), font, true);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);

	label = new StaticText(this);
	label->setCell(1);
	label->setTextParams(g_lang.get("Control"), Vec4f(1.f), font, true);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);

	label = new StaticText(this);
	label->setCell(2);
	label->setTextParams(g_lang.get("Faction"), Vec4f(1.f), font, true);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);

	label = new StaticText(this);
	label->setCell(3);
	label->setTextParams(g_lang.get("Team"), Vec4f(1.f), font, false);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);

	label = new StaticText(this);
	label->setCell(4);
	label->setTextParams(g_lang.get("Colour"), Vec4f(1.f), font, false);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);
}

// =====================================================
//  class PlayerSlotWidget
// =====================================================

PlayerSlotWidget::PlayerSlotWidget(Container* parent)
		: CellStrip(parent, Orientation::HORIZONTAL, Origin::CENTRE, 5)
		, m_freeSlot(false) {
	Anchors anchors(Anchor(AnchorType::RIGID, 3), Anchor(AnchorType::RIGID, 0));
	//Anchors anchors(Anchor(AnchorType::RIGID, 0));
	int hints[] = { 18, 33, 33, 8, 8 };
	setPercentageHints(hints);

	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	m_label = new StaticText(this);
	m_label->setCell(0);
	m_label->setTextParams("Player #", Vec4f(1.f), font, true);
	m_label->setAnchors(anchors);

	m_controlList = new DropList(this);
	m_controlList->setCell(1);
	m_controlList->setAnchors(anchors);

	foreach_enum (ControlType, ct) {
		m_controlList->addItem(g_lang.get(ControlTypeNames[ct]));
	}

	m_factionList = new DropList(this);
	m_factionList->setCell(2);
	m_factionList->setDropBoxHeight(200);
	m_factionList->setAnchors(anchors);
	
	m_teamList = new DropList(this);
	m_teamList->setCell(3);
	for (int i=1; i <= GameConstants::maxPlayers; ++i) {
		m_teamList->addItem(intToStr(i));
	}
	m_teamList->setAnchors(anchors);

	m_colourPicker = new Widgets::ColourPicker(this);
	m_colourPicker->setCell(4);
	m_colourPicker->setAnchors(anchors);

	m_controlList->SelectionChanged.connect(this, &PlayerSlotWidget::onControlChanged);
	m_factionList->SelectionChanged.connect(this, &PlayerSlotWidget::onFactionChanged);
	m_teamList->SelectionChanged.connect(this, &PlayerSlotWidget::onTeamChanged);
	m_colourPicker->ColourChanged.connect(this, &PlayerSlotWidget::onColourChanged);
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

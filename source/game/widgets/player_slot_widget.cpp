// ==============================================================
//	This file is part of The Glest Advanced Engi
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
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

// =====================================================
// class ColourButton
// =====================================================

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

// =====================================================
// class ColourPicker
// =====================================================

ColourPicker::ColourPicker(Container *parent)
		: CellStrip(parent, Orientation::HORIZONTAL, 2)
		, m_dropDownPanel(0)
		, m_selectedIndex(-1) {
	init();
}

//ColourPicker::ColourPicker(Container *parent, Vec2i pos, Vec2i size)
//		: Panel(parent, pos, size)
//		, m_dropDownPanel(0)
//		, m_selectedIndex(-1) {
//	init();
//	setSize(size);
//}

void ColourPicker::init() {
	Anchors a = Anchors ::getFillAnchors();
	setWidgetStyle(WidgetType::COLOUR_PICKER);
	m_showingItem = new ColourButton(this);
	m_showingItem->setCell(0);
	m_showingItem->setAnchors(a);
	m_button = new Button(this);
	m_button->setCell(1);
	m_button->setAnchors(a);
	m_button->Clicked.connect(this, &ColourPicker::onExpand);
	m_showingItem->Clicked.connect(this, &ColourPicker::onExpand);
}

void ColourPicker::setSize(const Vec2i &sz) {
	setSizeHint(1, SizeHint(-1, sz.h));
	CellStrip::setSize(sz);
}

void ColourPicker::onExpand(Button*) {
	Anchors anchors = Anchors::getFillAnchors();
	Vec2i bdims = g_widgetConfig.getBorderStyle(WidgetType::COLOUR_PICKER).getBorderDims();
	Vec2i size(4 * 32 + bdims.w, 4 * 32 + bdims.h);
	Vec2i pos(getScreenPos());
	pos += getSize() / 2;
	pos -= size / 2;

	m_dropDownPanel = new FloatingStrip(m_rootWindow, Orientation::VERTICAL, Origin::FROM_TOP, 4);
	m_dropDownPanel->setPos(pos);
	m_dropDownPanel->setSize(size);

	for (int i=0; i < 4; ++i) {
		CellStrip *strip = new CellStrip(m_dropDownPanel, Orientation::HORIZONTAL, 4);
		strip->setCell(i);
		strip->setAnchors(anchors);
		for (int j=0; j < 4; ++j) {
			const int ndx = i * 4 + j;
			m_colourButtons[ndx] = new ColourButton(strip);
			m_colourButtons[ndx]->setCell(j);
			m_colourButtons[ndx]->setAnchors(anchors);
			m_colourButtons[ndx]->setColour(factionColours[ndx], factionColoursOutline[ndx]);
			m_colourButtons[ndx]->Clicked.connect(this, &ColourPicker::onSelect);
		}
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
	setWidgetStyle(WidgetType::STATIC_WIDGET);
	CoreData &coreData = CoreData::getInstance();
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	int hints[] = { 18, 33, 33, 8, 8 };
	setPercentageHints(hints);
	
	StaticText *label = new StaticText(this);
	label->setCell(0);
	label->setText(g_lang.get("Player"));
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);

	label = new StaticText(this);
	label->setCell(1);
	label->setText(g_lang.get("Control"));
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);

	label = new StaticText(this);
	label->setCell(2);
	label->setText(g_lang.get("Faction"));
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);

	label = new StaticText(this);
	label->setCell(3);
	label->setText(g_lang.get("Team"));
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);

	label = new StaticText(this);
	label->setCell(4);
	label->setText(g_lang.get("Colour"));
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setAnchors(anchors);
}

// =====================================================
//  class PlayerSlotWidget
// =====================================================

PlayerSlotWidget::PlayerSlotWidget(Container* parent)
		: CellStrip(parent, Orientation::HORIZONTAL, Origin::CENTRE, 5)
		, m_freeSlot(false) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
	Anchors anchors(Anchor(AnchorType::RIGID, 3), Anchor(AnchorType::RIGID, 0));
	//Anchors anchors(Anchor(AnchorType::RIGID, 0));
	int hints[] = { 18, 33, 33, 8, 8 };
	setPercentageHints(hints);

	m_label = new StaticText(this);
	m_label->setCell(0);
	m_label->setText("Player #");
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

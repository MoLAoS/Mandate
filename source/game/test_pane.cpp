// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "battle_end.h"

#include "test_pane.h"
#include "main_menu.h"
#include "program.h"
#include "core_data.h"
#include "lang.h"
#include "util.h"
#include "renderer.h"
#include "main_menu.h"
#include "sound_renderer.h"
#include "metrics.h"
#include "test_pane.h"

#include "leak_dumper.h"

#include "sim_interface.h"
#include "player_slot_widget.h"

using namespace Shared::Util;

namespace Glest { namespace Main {
using namespace Sim;
using namespace Menu;

class ScrollPane : public CellStrip {
private:
	ScrollBar  *m_vertBar;
	ScrollBar  *m_horizBar;
	WidgetCell *m_contentCell;

	void init();

public:
	ScrollPane(Container *parent);
	ScrollPane(Container *parent, Vec2i pos, Vec2i sz);

	WidgetCell* getContentCell() { return m_contentCell; }

	virtual void render() override {
		if (m_dirty) {
			DEBUG_HOOK();
		}
		CellStrip::render();
	}
};

ScrollPane::ScrollPane(Container *parent)
		: CellStrip(parent, Orientation::HORIZONTAL, Origin::CENTRE, 2) {
	init();
	setPos(Vec2i(0));
}

ScrollPane::ScrollPane(Container *parent, Vec2i pos, Vec2i sz)
		: CellStrip(parent, pos, sz, Orientation::HORIZONTAL, Origin::CENTRE, 2) {
	init();
	setPos(pos);
}

void ScrollPane::init() {
	Anchors anchors;
	anchors.set(Edge::COUNT, 0, false); // fill

	CellStrip *bigStrip = new CellStrip(getCell(0), Orientation::VERTICAL, Origin::CENTRE, 2);
	bigStrip->setAnchors(anchors);
	CellStrip *littleStrip = new CellStrip(getCell(1), Orientation::VERTICAL, Origin::CENTRE, 2);
	littleStrip->setAnchors(anchors);
	m_contentCell = bigStrip->getCell(0);
	WidgetCell *horizBarCell = bigStrip->getCell(1);
	WidgetCell *vertBarCell = littleStrip->getCell(0);
	WidgetCell *spacerCell = littleStrip->getCell(1);

	int barSize = m_rootWindow->getConfig()->getDefaultElementHeight();

	getCell(0)->setSizeHint(SizeHint(100));
	getCell(1)->setSizeHint(SizeHint(-1, barSize));

	m_contentCell->setSizeHint(SizeHint(100));
	horizBarCell->setSizeHint(SizeHint(-1, barSize));
	vertBarCell->setSizeHint(SizeHint(100));
	spacerCell->setSizeHint(SizeHint(-1, barSize));

	m_vertBar = new ScrollBar(vertBarCell, true, 10);
	m_vertBar->setAnchors(anchors);
	m_horizBar = new ScrollBar(horizBarCell, false, 10);
	m_horizBar->setAnchors(anchors);
}

// =====================================================
//  class TestPane
// =====================================================

TestPane::TestPane(Program &program)
		: ProgramState(program) {
	Container *window = static_cast<Container*>(&program);
	Anchors anchors;
	anchors.set(Edge::COUNT, 15, false); // fill

	CellStrip *strip = new CellStrip(window, Orientation::VERTICAL, Origin::FROM_TOP, 3);
	strip->setPos(Vec2i(0));
	strip->setSize(g_metrics.getScreenDims());
	
	//ColourButton *btn = new ColourButton(strip->getCell(0));
	//btn->setAnchors(anchors);
	//btn->setColour(Colour(0xFFu, 0x00u, 0x00u, 0xFFu), Colour(0xFFu, 0xFFu, 0xFFu, 0xFFu));

	CellStrip *middleStrip = new CellStrip(strip->getCell(1), Orientation::HORIZONTAL, Origin::CENTRE, 3);
	middleStrip->setAnchors(anchors);
	// or ??
	//CellStrip *middleStrip = new CellStrip(strip->getCell(1), Orientation::HORIZONTAL);
	//middleStrip->addCells(3);

	ScrollPane *scrollPane = new ScrollPane(middleStrip->getCell(0));
	scrollPane->setAnchors(anchors);

	ListBox *listBox = new ListBox(middleStrip->getCell(1), Vec2i(50,150), Vec2i(250, 150));
	listBox->setAnchors(anchors);
	listBox->addItem("Apple");
	listBox->addItem("Pear");
	listBox->addItem("Peach");
	listBox->addItem("Banana");
	listBox->addItem("Apricot");
	listBox->addItem("Orange");
	listBox->addItem("Grape");
	listBox->addItem("Plum");
	listBox->addItem("Kiwi");
	listBox->addItem("Pine Apple");
	listBox->addItem("Water Melon");
	listBox->addItem("Berry");
	listBox->addItem("Pomegranate");
	listBox->addItem("Date");
	listBox->addItem("Lime");

	ScrollText *scrollText = new ScrollText(middleStrip->getCell(2));
	scrollText->setAnchors(anchors);

	strip->layoutCells();
	middleStrip->layoutCells();

	string txt = "La de da.\n\nTest text, testing text, this is some text to test the ScrollText widget.\n";
	txt += "   and this is some more! ...\nmore\nmore\nmore\nmore\nThis is a last bit.";
	scrollText->setText(txt);
}

TestPane::~TestPane() {
	g_soundRenderer.playMusic(g_coreData.getMenuMusic());
}

void TestPane::update() {
}

void TestPane::renderBg() {
	g_renderer.clearBuffers();
	g_renderer.renderBackground(g_coreData.getBackgroundTexture());
}

void TestPane::renderFg() {
	g_renderer.swapBuffers();
}

void TestPane::keyDown(const Key &key) {
	if (!key.isModifier()) {
		program.clear();
		program.setState(new MainMenu(program));
	}
}

void TestPane::mouseDownLeft(int x, int y) {
	program.clear();
	program.setState(new MainMenu(program));
}

}}//end namespace

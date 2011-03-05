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
#include "slider.h"

#include "leak_dumper.h"

#include "sim_interface.h"
#include "player_slot_widget.h"

using namespace Shared::Util;

namespace Glest { namespace Main {
using namespace Sim;
using namespace Menu;

class ScrollCell : public WidgetCell/*Container*/ {
private:
	typedef map<Widget*, Vec2i>   OffsetMap;

private:
	OffsetMap  m_childOffsets;

public:
	ScrollCell(Container *parent) : WidgetCell(parent) {}
	ScrollCell(Container *parent, Vec2i pos, Vec2i sz) : WidgetCell(parent, pos, sz) {}

	void anchorWidgets() { WidgetCell::anchorWidgets(); }

	virtual void addChild(Widget* child) override {
		Container::addChild(child);
		m_childOffsets[child] = child->getPos();
	}

	virtual void remChild(Widget* child) override {
		Container::remChild(child);
		m_childOffsets.erase(child);
	}

	void setOffset(Vec2i offset) {
		foreach (WidgetList, it, m_children) {
			Widget *child = *it;
			child->setPos(m_childOffsets[child] + offset);
		}
	}

	virtual void setSize(const Vec2i &sz) override {
		WidgetCell::setSize(sz);
		Resized(sz);
	}
	
	sigslot::signal<Vec2i> Resized;

	virtual string descType() const override { return "ScrollCell"; }
};

class ScrollPane : public CellStrip, public sigslot::has_slots {
private:
	ScrollBar  *m_vertBar;
	ScrollBar  *m_horizBar;
	ScrollCell *m_scrollCell;
	Vec2i       m_offset;
	Vec2i       m_totalRange;

	void init();
	void setOffset(Vec2i offset = Vec2i(0));
	void onVerticalScroll(int diff);
	void onHorizontalScroll(int diff);

public:
	ScrollPane(Container *parent);
	ScrollPane(Container *parent, Vec2i pos, Vec2i sz);

	ScrollCell* getScrollCell() { return m_scrollCell; }

	virtual void render() override {
		if (m_dirty) {
			DEBUG_HOOK();
		}
		CellStrip::render();
	}

	void onScrollCellResized(Vec2i avail) {
		WIDGET_LOG( descShort() << " ScrollPane::layoutCells() setting available scroll range to " << avail );
		m_vertBar->setRanges(m_totalRange.h, avail.h);
		m_horizBar->setRanges(m_totalRange.w, avail.w);
		setOffset(m_offset);
	}

	void setTotalRange(Vec2i total) {
		m_totalRange = total;
		Vec2i avail = m_scrollCell->getSize();
		WIDGET_LOG( descShort() << " ScrollPane::setTotalRange() setting available scroll range to " << avail );
		m_vertBar->setRanges(m_totalRange.h, avail.h);
		m_horizBar->setRanges(m_totalRange.w, avail.w);
		setOffset(m_offset);
	}
	virtual string descType() const override { return "ScrollPane"; }
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
	
	m_scrollCell = new ScrollCell(bigStrip->getCell(0));
	m_scrollCell->setAnchors(anchors);
	m_scrollCell->Resized.connect(this, &ScrollPane::onScrollCellResized);

	WidgetCell *horizBarCell = bigStrip->getCell(1);
	WidgetCell *vertBarCell = littleStrip->getCell(0);
	WidgetCell *spacerCell = littleStrip->getCell(1);

	int barSize = m_rootWindow->getConfig()->getDefaultItemHeight();

	getCell(0)->setSizeHint(SizeHint(100));
	getCell(1)->setSizeHint(SizeHint(-1, barSize));

	bigStrip->getCell(0)->setSizeHint(SizeHint(100));
	horizBarCell->setSizeHint(SizeHint(-1, barSize));
	vertBarCell->setSizeHint(SizeHint(100));
	spacerCell->setSizeHint(SizeHint(-1, barSize));

	m_vertBar = new ScrollBar(vertBarCell, true, 10);
	m_vertBar->setAnchors(anchors);
	m_vertBar->ThumbMoved.connect(this, &ScrollPane::onVerticalScroll);
	m_horizBar = new ScrollBar(horizBarCell, false, 10);
	m_horizBar->setAnchors(anchors);
	m_horizBar->ThumbMoved.connect(this, &ScrollPane::onHorizontalScroll);

	m_offset = Vec2i(0);
}

void ScrollPane::setOffset(Vec2i offset) {
	m_scrollCell->setOffset(offset);
}

void ScrollPane::onVerticalScroll(int diff) {
	m_offset.y = -diff;
	m_scrollCell->setOffset(m_offset);
}

void ScrollPane::onHorizontalScroll(int diff) {
	m_offset.x = -diff;
	m_scrollCell->setOffset(m_offset);
}


// =====================================================
//  class TestPane
// =====================================================

TestPane::TestPane(Program &program)
		: ProgramState(program) {
	Container *window = static_cast<Container*>(&program);
	WidgetConfig &cfg = g_widgetConfig;

	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding

	Anchors centreAnchors;
	centreAnchors.setCentre(true);

	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));

	Anchors btnAnchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 5),
		Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 5));

	CellStrip *strip = new CellStrip(window, Orientation::VERTICAL, Origin::FROM_TOP, 4);
	strip->setPos(Vec2i(0));
	strip->setSize(g_metrics.getScreenDims());
	
	//ColourButton *btn = new ColourButton(strip->getCell(0));
	//btn->setAnchors(anchors);
	//btn->setColour(Colour(0xFFu, 0x00u, 0x00u, 0xFFu), Colour(0xFFu, 0xFFu, 0xFFu, 0xFFu));

	std::vector<string> fruit;
	fruit.push_back("Apple");
	fruit.push_back("Pear");
	fruit.push_back("Peach");
	fruit.push_back("Banana");
	fruit.push_back("Apricot");
	fruit.push_back("Orange");
	fruit.push_back("Grape");
	fruit.push_back("Plum");
	fruit.push_back("Kiwi");
	fruit.push_back("Pine Apple");
	fruit.push_back("Water Melon");
	fruit.push_back("Berry");
	fruit.push_back("Pomegranate");
	fruit.push_back("Date");
	fruit.push_back("Lime");

	strip->getCell(0)->setSizeHint(SizeHint(10, -1));
	strip->getCell(2)->setSizeHint(SizeHint(10, -1));

	CellStrip *topStrip = new CellStrip(strip->getCell(0), Orientation::HORIZONTAL);
	topStrip->setAnchors(padAnchors);
	topStrip->addCells(1);
	topStrip->getCell(0)->setSizeHint(SizeHint(-1, 200));

	CellStrip *middleStrip = new CellStrip(strip->getCell(1), Orientation::HORIZONTAL);
	middleStrip->setAnchors(padAnchors);
	middleStrip->addCells(3);

	CellStrip *middleStrip2 = new CellStrip(strip->getCell(2), Orientation::HORIZONTAL);
	middleStrip2->setAnchors(padAnchors);
	middleStrip2->addCells(3);
	middleStrip2->getCell(1)->setSizeHint(SizeHint(-1, 40));

	//Vec2i sz(200, cfg.getDefaultItemHeight());
	//sz += cfg.getBorderStyle(WidgetType::DROP_LIST).getBorderDims();

	DropList *dropList = new DropList(topStrip->getCell(0));
	dropList->addItems(fruit);
	dropList->setAnchors(fillAnchors);
	dropList->setDropBoxHeight(200);

	CheckBox *checkBox = new CheckBox(middleStrip2->getCell(1));
	checkBox->setAnchors(fillAnchors);
	checkBox->setChecked(true);

	Slider2 *slider = new Slider2(middleStrip2->getCell(0), false);
	slider->setAnchors(btnAnchors);
	slider->setRange(100);
	slider->setValue(50);

	Texture2D *tex = g_coreData.getGaeSplashTexture();

	ScrollPane *scrollPane = new ScrollPane(middleStrip->getCell(0));
	scrollPane->setTotalRange(tex->getPixmap()->getSize());
	scrollPane->setAnchors(padAnchors);

	StaticImage *image = new StaticImage(scrollPane->getScrollCell(), Vec2i(0), tex->getPixmap()->getSize());
	image->setImage(tex);

	ListBox *listBox = new ListBox(middleStrip->getCell(1), Vec2i(50,150), Vec2i(250, 150));
	listBox->setAnchors(padAnchors);
	listBox->addItems(fruit);

	ScrollText *scrollText = new ScrollText(middleStrip->getCell(2));
	scrollText->setAnchors(padAnchors);
	string txt = "La de da.\n\nTest text, testing text, this is some text to test the ScrollText widget.\n";
	txt += "   and this is some more! ...\nmore\nmore\nmore\nmore\nThis is a last bit.";
	scrollText->setText(txt);

	CellStrip *buttonStrip = new CellStrip(middleStrip2->getCell(2), Orientation::HORIZONTAL, 2);
	buttonStrip->setAnchors(fillAnchors);

	Button *btn = new Button(buttonStrip->getCell(0));
	btn->setAnchors(btnAnchors);
	btn->setText("Button 1");

	btn = new Button(buttonStrip->getCell(1));
	btn->setAnchors(btnAnchors);
	btn->setText("Button 2");
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

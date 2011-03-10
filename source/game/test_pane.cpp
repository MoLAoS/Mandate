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

namespace Glest {
	
using namespace Sim;
using namespace Menu;

namespace Widgets {

class TextInBox : public StaticText {
public:
	TextInBox(Container *parent) : StaticText(parent) {
		setWidgetStyle(WidgetType::TICKER_TAPE);
	}

	virtual void setStyle() override { setWidgetStyle(WidgetType::TICKER_TAPE); }
	virtual string descType() const override { return "TickerTape"; }
};

class TickerTape : public StaticText {
private:
	struct Action {
		int       m_index;
		int       m_actionNumber;
		int       m_counter;
		Vec2f     m_inPos;
		Vec2f     m_midPos;
		Vec2f     m_outPos;
	};

	typedef std::deque<Action> Actions;

private:
	Origin        m_origin;
	bool          m_alternateOrigin;
	bool          m_overlapTransitions;
	int           m_displayInterval;
	int           m_transitionInterval;
	int           m_actionCounter;
	Actions       m_actions;

private:
	void setPositions(Action &action);
	void startAction(int ndx);
	void startTicker();

public:
	TickerTape(Container *parent, Origin origin);
	~TickerTape() {
		m_rootWindow->unregisterUpdate(this);
	}

	void addItems(const vector<string> &strings);
	void addItems(const char **strings);
	void addItems(const char **strings, unsigned n);

	void setDisplayInterval(int v)      { m_displayInterval = v;    }
	void setTransitionInterval(int v)   { m_transitionInterval = v; }
	void setAlternateOrigin(bool v)     { m_alternateOrigin = v;    }
	void setOverlapTransitions(bool v)  { m_overlapTransitions = v; }

	// Widget overrides
	virtual Vec2i getMinSize() const override;
	virtual Vec2i getPrefSize() const override { return getMinSize(); }
	virtual void setSize(const Vec2i &sz) override;
	virtual void render() override;
	virtual void update() override;
	virtual void setStyle() override { setWidgetStyle(WidgetType::TICKER_TAPE); }
	virtual string descType() const override { return "TickerTape"; }
};

TickerTape::TickerTape(Container *parent, Origin origin)
		: StaticText(parent)
		, m_origin(origin)
		, m_alternateOrigin(false)
		, m_overlapTransitions(false)
		, m_displayInterval(240)
		, m_transitionInterval(120)
		, m_actionCounter(0)
		/*, m_currentIndex(-1)*/ {
	setWidgetStyle(WidgetType::TICKER_TAPE);
}

void TickerTape::setPositions(Action &action) {
	TextWidget::centreText(action.m_index);
	Vec2i p = TextWidget::getTextPos(action.m_index);
	action.m_midPos = Vec2f(p);

	Origin origin = m_origin;
	if (m_alternateOrigin && action.m_actionNumber % 2 == 1) {
		switch (m_origin) {
			case Origin::FROM_TOP: origin = Origin::FROM_BOTTOM; break;
			case Origin::FROM_BOTTOM: origin = Origin::FROM_TOP; break;
			case Origin::FROM_LEFT: origin = Origin::FROM_RIGHT; break;
			case Origin::FROM_RIGHT: origin = Origin::FROM_LEFT; break;
			default: assert(false);
		}
	}
	switch (origin) {
		case Origin::FROM_TOP:
			action.m_inPos = action.m_midPos + Vec2f(0.f, float(-getHeight()));
			action.m_outPos = action.m_midPos + Vec2f(0.f, float(getHeight()));
			break;
		case Origin::FROM_BOTTOM:
			action.m_inPos = action.m_midPos + Vec2f(0.f, float(getHeight()));
			action.m_outPos = action.m_midPos + Vec2f(0.f, float(-getHeight()));
			break;
		case Origin::CENTRE:
			action.m_inPos = action.m_outPos = action.m_midPos;
			break;
		case Origin::FROM_LEFT:
			action.m_inPos = action.m_midPos + Vec2f(float(-getWidth()), 0.f);
			action.m_outPos = action.m_midPos + Vec2f(float(getWidth()), 0.f);
			break;
		case Origin::FROM_RIGHT:
			action.m_inPos = action.m_midPos + Vec2f(float(getWidth()), 0.f);
			action.m_outPos = action.m_midPos + Vec2f(float(-getWidth()), 0.f);
			break;
	}
}

void TickerTape::startAction(int ndx) {
	m_actions.push_back(Action());
	Action &action = m_actions.back();
	action.m_counter = 0;
	action.m_index = ndx;
	action.m_actionNumber = m_actionCounter++;
	setPositions(action);
}

void TickerTape::startTicker() {
	m_rootWindow->registerUpdate(this);
	startAction(0);
}

void TickerTape::setSize(const Vec2i &sz) {
	StaticText::setSize(sz);
	foreach (Actions, a, m_actions) {
		setPositions(*a);
	}
}

void TickerTape::addItems(const vector<string> &strings) {
	foreach_const (vector<string>, it, strings) {
		TextWidget::addText(*it);
	}
	if (!strings.empty() && TextWidget::numSnippets() == strings.size()) {
		startTicker();
	}
}

void TickerTape::addItems(const char **strings) {
	int i = 0;
	for ( ; strings[i]; ++i) {
		TextWidget::addText(strings[i]);
	}
	if (i && TextWidget::numSnippets() == i) {
		startTicker();
	}
}

void TickerTape::addItems(const char **strings, unsigned n) {
	for (int i=0; i < n; ++i) {
		TextWidget::addText(strings[i]);
	}
	if (n && TextWidget::numSnippets() == n) {
		startTicker();
	}
}

Vec2i TickerTape::getMinSize() const {
	Vec2i txtDim = getTextDimensions(0);
	Vec2i xtra = getBordersAll();
	for (int i=1; i < TextWidget::numSnippets(); ++i) {
		Vec2i dim = getTextDimensions(1);
		if (dim.w > txtDim.w) {
			txtDim.w = dim.w;
		}
		if (dim.h > txtDim.h) {
			txtDim.h = dim.h;
		}
	}
	return txtDim + xtra;
}

void TickerTape::render() {
	Widget::render();
	m_rootWindow->pushClipRect(getScreenPos() + Vec2i(getBorderLeft(), getBorderTop()), getSize() - getBordersAll());
	foreach (Actions, a, m_actions) {
		Action &action = *a;
		float t;
		Vec2f p;
		if (action.m_counter < m_transitionInterval) { // transition in (t 0.f -> 1.f)
			t = action.m_counter / float(m_transitionInterval);
			p = action.m_inPos.lerp(t, action.m_midPos);
		} else if (action.m_counter < m_transitionInterval + m_displayInterval) { // display (t 1.f)
			t = 1.f;
			p = action.m_midPos;
		} else { // transition out (t 1.f -> 0.f)
			t = 1.f - ((action.m_counter - m_transitionInterval - m_displayInterval) / float(m_transitionInterval));
			p = action.m_outPos.lerp(t, action.m_midPos); // out -> mid because t 1 -> 0
		}
		Vec2i pos = getScreenPos() + Vec2i(p);
		Vec4f col(1.f, 1.f, 1.f, t);
		TextWidget::renderText(getText(action.m_index), pos.x, pos.y, col, m_textStyle.m_fontIndex);
	}
	m_rootWindow->popClipRect();
}

void TickerTape::update() {
	const int totalInterval = 2 * m_transitionInterval + m_displayInterval;
	int startNdx = -1;
	Actions::iterator a = m_actions.begin();
	while (a != m_actions.end()) {
		Action &action = *a;
		++action.m_counter;
		if (action.m_counter == m_transitionInterval + m_displayInterval) {
			if (m_overlapTransitions) {
				startNdx = (action.m_index + 1) % TextWidget::numSnippets();
			}
		}
		if (action.m_counter == totalInterval) {
			if (!m_overlapTransitions) {
				startNdx = (action.m_index + 1) % TextWidget::numSnippets();
			}
			a = m_actions.erase(a);
		} else {
			++a;
		}
	}
	if (startNdx != -1) {
		startAction(startNdx);
	}
}

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

}

namespace Main {

void populateFruitVector(std::vector<string> &fruit) {
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
}

void populateMiscStrip1(WidgetCell *cell, std::vector<string> &fruit) {
	cell->setSizeHint(SizeHint(10, -1));
	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	CellStrip *topStrip = new CellStrip(cell, Orientation::HORIZONTAL);
	topStrip->setAnchors(padAnchors);
	topStrip->addCells(3);
//	topStrip->getCell(0)->setSizeHint(SizeHint(-1, 200));

	TickerTape *tickerTape = new TickerTape(topStrip->getCell(0), Origin::CENTRE);
	tickerTape->addItems(fruit);
	tickerTape->setAnchors(fillAnchors);
	tickerTape->setTransitionInterval(120);
	tickerTape->setDisplayInterval(120);
	tickerTape->setOverlapTransitions(true);
	tickerTape->setAlternateOrigin(false);

	DropList *dropList = new DropList(topStrip->getCell(1));
	dropList->addItems(fruit);
	dropList->setAnchors(fillAnchors);
	dropList->setDropBoxHeight(200);

	TextInBox *txtInBox = new TextInBox(topStrip->getCell(2));
	txtInBox->setAnchors(fillAnchors);
	txtInBox->setText("Apple");
}

void populateBigTestStrip(WidgetCell *cell, std::vector<string> &fruit) {
	cell->setSizeHint(SizeHint());
	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding
	CellStrip *middleStrip = new CellStrip(cell, Orientation::HORIZONTAL);
	middleStrip->setAnchors(padAnchors);
	middleStrip->addCells(3);

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
}

void populateMiscStrip2(WidgetCell *cell) {
	cell->setSizeHint(SizeHint(10, -1));
	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors btnAnchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 5),
		Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 5));

	CellStrip *middleStrip2 = new CellStrip(cell, Orientation::HORIZONTAL);
	middleStrip2->setAnchors(padAnchors);
	middleStrip2->addCells(3);
	middleStrip2->getCell(1)->setSizeHint(SizeHint(-1, 40));

	CheckBox *checkBox = new CheckBox(middleStrip2->getCell(1));
	checkBox->setAnchors(fillAnchors);
	checkBox->setChecked(true);

	Slider2 *slider = new Slider2(middleStrip2->getCell(0), false);
	slider->setAnchors(btnAnchors);
	slider->setRange(100);
	slider->setValue(50);

	CellStrip *buttonStrip = new CellStrip(middleStrip2->getCell(2), Orientation::HORIZONTAL, 2);
	buttonStrip->setAnchors(fillAnchors);

	Button *btn = new Button(buttonStrip->getCell(0));
	btn->setAnchors(btnAnchors);
	btn->setText("Button 1");

	btn = new Button(buttonStrip->getCell(1));
	btn->setAnchors(btnAnchors);
	btn->setText("Button 2");
}

void populatePlayerSlotStrip(WidgetCell *cell) {
	cell->setSizeHint(SizeHint(10, -1));
	Anchors psw_anchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 15),
		Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 15));
	PlayerSlotWidget *psw = new PlayerSlotWidget(cell);
	psw->setAnchors(psw_anchors);
}

// =====================================================
//  class TestPane
// =====================================================

TestPane::TestPane(Program &program)
		: ProgramState(program) {
	Container *window = static_cast<Container*>(&program);
	WidgetConfig &cfg = g_widgetConfig;

	CellStrip *strip = new CellStrip(window, Orientation::VERTICAL, Origin::FROM_TOP, 4);
	strip->setPos(Vec2i(0));
	strip->setSize(g_metrics.getScreenDims());

	std::vector<string> fruit;
	populateFruitVector(fruit);

	populateMiscStrip1(      strip->getCell(1), fruit );
	populateBigTestStrip(    strip->getCell(2), fruit );
	populateMiscStrip2(      strip->getCell(3)        );
	populatePlayerSlotStrip( strip->getCell(0)        );
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

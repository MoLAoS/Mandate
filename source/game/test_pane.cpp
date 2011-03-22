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
#include "ticker_tape.h"

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

void populateMiscStrip1(CellStrip *strip, int cell, std::vector<string> &fruit) {
	strip->setSizeHint(cell, SizeHint(10, -1));
	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors tickerAnchors(Anchor(AnchorType::RIGID, 5), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, 5), Anchor(AnchorType::RIGID, 0));
	CellStrip *topStrip = new CellStrip(strip, Orientation::HORIZONTAL);
	topStrip->setCell(cell);
	topStrip->setAnchors(padAnchors);
	topStrip->addCells(3);
//	topStrip->getCell(0)->setSizeHint(SizeHint(-1, 200));

	TickerTape *tickerTape = new TickerTape(topStrip, Origin::FROM_LEFT, Alignment::FLUSH_RIGHT);
	tickerTape->setCell(0);
	tickerTape->addItems(fruit);
	tickerTape->setAnchors(tickerAnchors);
	tickerTape->setTransitionInterval(120);
	tickerTape->setDisplayInterval(120);
	tickerTape->setTransitionFunc(TransitionFunc::LOGARITHMIC);
	tickerTape->setOverlapTransitions(true);
	tickerTape->setAlternateOrigin(false);
	tickerTape->startTicker();

	DropList *dropList = new DropList(topStrip);
	dropList->setCell(1);
	dropList->addItems(fruit);
	dropList->setAnchors(fillAnchors);
	dropList->setDropBoxHeight(200);

	TextInBox *txtInBox = new TextInBox(topStrip);
	txtInBox->setCell(2);
	txtInBox->setAnchors(fillAnchors);
	txtInBox->setText("Apple");
}

void populateBigTestStrip(CellStrip *strip, int cell, std::vector<string> &fruit) {
	strip->setSizeHint(cell, SizeHint());
	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding
	CellStrip *middleStrip = new CellStrip(strip, Orientation::HORIZONTAL);
	middleStrip->setCell(cell);
	middleStrip->setAnchors(padAnchors);
	middleStrip->addCells(3);

	CellStrip *leftStrip = new CellStrip(middleStrip, Orientation::VERTICAL);
	leftStrip->setCell(0);
	leftStrip->addCells(2);
	leftStrip->setAnchors(Anchors(Anchor(AnchorType::RIGID, 2)));

	Texture2D *tex = g_coreData.getGaeSplashTexture();

	ScrollPane *scrollPane = new ScrollPane(leftStrip);
	scrollPane->setCell(0);
	scrollPane->setTotalRange(tex->getPixmap()->getSize());
	scrollPane->setAnchors(padAnchors);
	StaticImage *image = new StaticImage(scrollPane->getScrollCell(), Vec2i(0), tex->getPixmap()->getSize());
	image->setImage(tex);

	ScrollText *scrollText = new ScrollText(leftStrip);
	scrollText->setCell(1);
	scrollText->setAnchors(padAnchors);
	string txt = "La de da.\n\nTest text, testing text, this is some text to test the ScrollText widget.\n";
	txt += "   and this is some more! ...\nmore\nmore\nmore\nmore\nThis is a last bit.";
	scrollText->setText(txt);

	ListBox *listBox = new ListBox(middleStrip, Vec2i(50,150), Vec2i(250, 150));
	listBox->setCell(1);
	listBox->setAnchors(padAnchors);
	listBox->addItems(fruit);
}

void populateMiscStrip2(CellStrip *strip, int cell) {
	strip->setSizeHint(cell, SizeHint(10, -1));
	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors btnAnchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 5),
		Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 5));

	CellStrip *middleStrip2 = new CellStrip(strip, Orientation::HORIZONTAL);
	middleStrip2->setCell(cell);
	middleStrip2->setAnchors(padAnchors);
	middleStrip2->addCells(3);
	middleStrip2->setSizeHint(1, SizeHint(-1, 40));

	CheckBox *checkBox = new CheckBox(middleStrip2);
	checkBox->setCell(1);
	checkBox->setAnchors(fillAnchors);
	checkBox->setChecked(true);

	Slider2 *slider = new Slider2(middleStrip2, false);
	slider->setCell(0);
	slider->setAnchors(btnAnchors);
	slider->setRange(100);
	slider->setValue(50);

	CellStrip *buttonStrip = new CellStrip(middleStrip2, Orientation::HORIZONTAL, 2);
	buttonStrip->setCell(2);
	buttonStrip->setAnchors(fillAnchors);

	Button *btn = new Button(buttonStrip);
	btn->setCell(0);
	btn->setAnchors(btnAnchors);
	btn->setText("Button 1");

	btn = new Button(buttonStrip);
	btn->setCell(1);
	btn->setAnchors(btnAnchors);
	btn->setText("Button 2");
}

void populatePlayerSlotStrip(CellStrip *strip, int cell) {
	strip->setSizeHint(cell, SizeHint(10, -1));
	Anchors psw_anchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 15),
		Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 15));
	PlayerSlotWidget *psw = new PlayerSlotWidget(strip);
	psw->setCell(0);
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

	populateMiscStrip1(strip, 1, fruit);
	populateBigTestStrip(strip, 2, fruit);
	populateMiscStrip2(strip, 3);
	populatePlayerSlotStrip(strip, 0);
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

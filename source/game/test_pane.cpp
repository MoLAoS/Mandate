// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "test_pane.h"
#include "game_util.h"
#include "core_data.h"
#include "sound_renderer.h"
#include "renderer.h"
#include "main_menu.h"

#include "leak_dumper.h"

namespace Glest { namespace Main {

using namespace Shared::Util;
using namespace Widgets;
using namespace Util;
using namespace Global;

void test1(TestPane *pane) {
	Vec2i size(560, 320);
	Vec2i pos = (g_metrics.getScreenDims() - size) / 2;
	CellStrip *strip = new CellStrip((Container*)&g_program, Orientation::VERTICAL, 1);
	strip->setSize(size);
	strip->setPos(pos);
	strip->borderStyle().setSolid(g_widgetConfig.getColourIndex(Vec3f(0.f, 0.5f, 0.f)));
	strip->borderStyle().setSizes(4);
	strip->backgroundStyle().setColour(g_widgetConfig.getColourIndex(Vec3f(0.9f, 1.f, 1.f)));

	StaticText *text = new StaticText(strip);
	text->setCell(0);

	// stick to left, top & right, but not bottom. This means height must be set manually.
	Anchors anchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::NONE, 0));
	
	text->setAnchors(anchors);
	text->setText(gaeVersionString);
	text->borderStyle().m_type = BorderType::INVISIBLE;
	text->borderStyle().setSizes(3);
	text->backgroundStyle().setColour(g_widgetConfig.getColourIndex(Vec3f(0.7f, 1.f, 1.f)));
	text->setTextColour(Vec4f(1.f, 0.2f, 0.2f, 1.f));
	text->setTextFont(g_widgetConfig.getTitleFontNdx());
	int textH = int(g_widgetConfig.getTitleFont()->getMetrics()->getHeight()) + 6;
	text->setSize(Vec2i(0, textH));
	text->setAlignment(Alignment::FLUSH_RIGHT);

	int y = textH;
	for (int i=0; i < getAboutStringCount(); ++i) {
		anchors.set(Edge::TOP, Anchor(AnchorType::RIGID, y));
		text = new StaticText(strip);
		text->setAnchors(anchors);
		text->setText(getAboutString(i));
		text->borderStyle().m_type = BorderType::INVISIBLE;
		text->borderStyle().setSizes(3);
		text->setTextColour(Vec4f(0.7f, 0.f, 0.f, 1.f));
		text->setTextFont(g_widgetConfig.getVersionFontNdx());
		int textH2 = int(g_widgetConfig.getVersionFont()->getMetrics()->getHeight()) + 6;
		text->setSize(Vec2i(0, textH2));
		text->setAlignment(Alignment::FLUSH_LEFT);
		y += textH2;
	}
	// stick to right & bottom (6px in from borders), but not top or left.
	// because this wont stretch the widget, size must be set (Width & height)
	anchors = Anchors(Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0),
		Anchor(AnchorType::RIGID, 6), Anchor(AnchorType::RIGID, 6));

	Button *btn = new Button(strip);
	btn->setCell(0);
	btn->setAnchors(anchors);
	btn->setText("Goto Game");
	btn->setTextFont(g_widgetConfig.getVersionFontNdx());
	int btnH = int(g_widgetConfig.getVersionFont()->getMetrics()->getHeight()) + 6;
	btn->setSize(Vec2i(6 * btnH, btnH));

	btn->Clicked.connect(pane, &TestPane::onContinue);
}

void test2(TestPane *pane) {
	CellStrip *vertStrip = new CellStrip((Container*)&g_program, Orientation::VERTICAL, 3);
	vertStrip->setSize(g_metrics.getScreenDims());
	vertStrip->setSizeHint(0, SizeHint(30)); // left cell, 30%
	vertStrip->setSizeHint(2, SizeHint(30)); // right cell, 30%
	// leave middle cell on default (which means, 'the remaineder') (will be 40%)

	Anchors a = Anchors::getFillAnchors();
	CellStrip *horizStrip = new CellStrip(vertStrip, Orientation::HORIZONTAL, 3);
	horizStrip->setCell(1);     // put in middle cell of parent
	horizStrip->setAnchors(a);  // fill cell
	horizStrip->setSizeHint(0, SizeHint(25)); // top cell, 25%
	horizStrip->setSizeHint(2, SizeHint(25)); // bottom cell, 25%
	// leave medium cell on default (will be 50%)

	CellStrip *inner = new CellStrip(horizStrip, Orientation::VERTICAL, 3);
	inner->setCell(1);     // put in middle cell of parent
	inner->setAnchors(a);  // fill cell
	inner->borderStyle().setSolid(g_widgetConfig.getColourIndex(Vec3f(0.f, 0.5f, 0.f)));
	inner->borderStyle().setSizes(4);
	inner->backgroundStyle().setColour(g_widgetConfig.getColourIndex(Vec3f(0.9f, 1.f, 1.f)));

	Anchors anchors = Anchors::getFillAnchors(); // fill cell

	StaticText *text = new StaticText(inner);
	text->setCell(0);
	text->setAnchors(anchors);
	text->setText(gaeVersionString);
	text->borderStyle().m_type = BorderType::INVISIBLE;
	text->borderStyle().setSizes(3);
	text->backgroundStyle().setColour(g_widgetConfig.getColourIndex(Vec3f(0.7f, 1.f, 1.f)));
	text->setTextColour(Vec4f(1.f, 0.2f, 0.2f, 1.f));
	text->setTextFont(g_widgetConfig.getTitleFontNdx());
	int textH = int(g_widgetConfig.getTitleFont()->getMetrics()->getHeight()) + 6;
	text->setSize(Vec2i(0, textH));
	text->setAlignment(Alignment::FLUSH_RIGHT);

	inner->setSizeHint(0, SizeHint(-1, textH)); // absolute size

	string txt;
	for (int i=0; i < getAboutStringCount(); ++i) {
		txt += (i ? "\n" : "") + getAboutString(i);
	}
	text = new StaticText(inner);
	text->setCell(1);
	text->setAnchors(anchors);
	text->setText(txt);

	text->borderStyle().m_type = BorderType::INVISIBLE;
	text->borderStyle().setSizes(3);
	text->setTextColour(Vec4f(0.7f, 0.f, 0.f, 1.f));
	text->setTextFont(g_widgetConfig.getVersionFontNdx());
	text->setAlignment(Alignment::FLUSH_LEFT);

	inner->setSizeHint(1, SizeHint(-1, -1)); // default

	// stick to right & bottom (6px in from borders), but not top or left.
	// because this wont stretch the widget, size must be set (Width & height)
	anchors = Anchors(Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0),
		Anchor(AnchorType::RIGID, 6), Anchor(AnchorType::RIGID, 6));

	Button *btn = new Button(inner);
	btn->setCell(2);
	btn->setAnchors(anchors);
	btn->setText("Goto Game");
	btn->setTextFont(g_widgetConfig.getVersionFontNdx());
	int btnH = int(g_widgetConfig.getVersionFont()->getMetrics()->getHeight()) + 6;
	btn->setSize(Vec2i(6 * btnH, btnH));

	inner->setSizeHint(2, SizeHint(-1, btnH + 6)); // absolute size

	btn->Clicked.connect(pane, &TestPane::onContinue);
}

// =====================================================
//  class TestPane
// =====================================================

TestPane::TestPane(Program &program)
		: ProgramState(program), m_done(false), m_removingDialog(false), m_messageDialog(0), m_action(0) {
	Container *window = static_cast<Container*>(&program);
	WidgetConfig &cfg = g_widgetConfig;

	KeymapWidget *kmw = new KeymapWidget(window);

	Vec2i sd = g_metrics.getScreenDims();
	Vec2i sz = Vec2i(sd.w * 8 / 10, sd.h * 7 / 10);
	Vec2i pos = (sd - sz) / 2;
	kmw->setPos(pos);
	kmw->setSize(sz);
}

TestPane::~TestPane() {
	g_soundRenderer.playMusic(g_coreData.getMenuMusic());
}

void TestPane::update() {
	if (m_action) {
		if (m_action->update()) {
			delete m_action;
			m_action = 0;
			if (m_removingDialog) {
				m_removingDialog = false;
				g_widgetWindow.removeFloatingWidget(m_messageDialog);
				m_messageDialog = 0;
			}
		}
	}
	if (m_done) {
		program.clear();
		program.setState(new MainMenu(program));
	}
}

void TestPane::onContinue(Widget*) {
	if (m_messageDialog) {
		g_widgetWindow.removeFloatingWidget(m_messageDialog);
		m_messageDialog = 0;
	}
	delete m_action;
	m_action = 0;
	m_done = true;
}

InputDialog *testDialog = 0;

void TestPane::onDoDialog(Widget*) {
	Vec2i size(500, 400);
	Vec2i pos = (g_metrics.getScreenDims() - size) / 2;
	m_messageDialog = MessageDialog::showDialog(pos, size, "Test MessageBox",
		"Message text goes here.\n\nSelect Continue to proceed to game.", "Continue", "Cancel");
	m_messageDialog->Button1Clicked.connect(this, &TestPane::onContinue);
	m_messageDialog->Button2Clicked.connect(this, &TestPane::onDismissDialog);
	m_messageDialog->Close.connect(this, &TestPane::onDismissDialog);
	m_messageDialog->Escaped.connect(this, &TestPane::onDismissDialog);

	m_action = new MoveWidgetAction(60, m_messageDialog);
	Vec2f start(pos);
	start.x -= g_metrics.getScreenW();
	m_action->setPosTransition(start, Vec2f(pos), TransitionFunc::LOGARITHMIC);
	m_action->setAlphaTransition(0.f, 1.f, TransitionFunc::LINEAR);
	m_action->update();
}

void TestPane::onDismissDialog(Widget *d) {
	if (m_messageDialog) {
		if (m_action) {
			m_action->reset();
		} else {
			m_action = new MoveWidgetAction(40, m_messageDialog);
		}
		Vec2f startPos(m_messageDialog->getPos());
		Vec2f endPos(startPos + Vec2f(0.f, 150.f));
		m_action->setPosTransition(startPos, endPos, TransitionFunc::EXPONENTIAL);
		m_action->setAlphaTransition(m_messageDialog->getFade(), 0.f, TransitionFunc::LINEAR);
		m_removingDialog = true;
	} else {
		g_program.removeFloatingWidget(testDialog);
		testDialog = 0;
	}
}

void TestPane::renderBg() {
	g_renderer.clearBuffers();
	g_renderer.renderBackground(g_coreData.getBackgroundTexture());
}

void TestPane::renderFg() {
	g_renderer.swapBuffers();
}

void TestPane::keyDown(const Key &key) {
}

void TestPane::mouseDownLeft(int x, int y) {
}

}}//end namespace

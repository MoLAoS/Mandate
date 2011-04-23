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

class TooltipTest : public CellStrip {
private:
	class HeaderWidget : public StaticText {
	public:
		HeaderWidget(Container *parent) : StaticText(parent) {
			setWidgetStyle(WidgetType::TEST_WIDGET_HEADER);
		}
	};

	class TipWidget : public StaticText {
	private:
		string  m_origString;

	public:
		TipWidget(Container *parent) : StaticText(parent) {
			setWidgetStyle(WidgetType::TEST_WIDGET_MAINBIT);
		}

		void setText(const string &text) {
			m_origString = text;
			string txt = text;
			if (getWidth() != 0) {
				getFont()->getMetrics()->wrapText(txt, getWidth() - getBordersHoriz());
			}
			TextWidget::setText(txt);
		}

		virtual void setSize(const Vec2i &sz) override {
			Widget::setSize(sz);
			string txt = m_origString;
			getFont()->getMetrics()->wrapText(txt, sz.w - getBordersHoriz());
			TextWidget::setText(txt);
		}
	};

	class CodeWidget : public StaticText {
	public:
		CodeWidget(Container *parent) : StaticText(parent) {
			setWidgetStyle(WidgetType::TEST_WIDGET_CODEBIT);
		}
	};

private:
	HeaderWidget *m_header;
	TipWidget    *m_tip;
	CodeWidget   *m_code;

public:
	TooltipTest(Container *parent) : CellStrip(parent, Orientation::VERTICAL, 3) {
		setWidgetStyle(WidgetType::TEST_WIDGET);
		Anchors anchors(Anchor(AnchorType::RIGID, 2));
		m_header = new HeaderWidget(this);
		m_header->setCell(0);
		m_header->setAnchors(anchors);
		m_header->setText("");
		m_header->setAlignment(Alignment::FLUSH_LEFT);
		m_tip = new TipWidget(this);
		m_tip->setCell(1);
		m_tip->setAnchors(anchors);
		m_tip->setText("");
		m_tip->setAlignment(Alignment::NONE);
		m_code = new CodeWidget(this);
		m_code->setCell(2);
		m_code->setAnchors(anchors);
		m_code->setText("");
		m_code->setAlignment(Alignment::NONE);
		int h = int(getFont(m_header->textStyle().m_fontIndex)->getMetrics()->getHeight() + 12);
		setSizeHint(0, SizeHint(0, h));
	}

	void setHeader(const string &hdr) { m_header->setText(hdr); }
	void setTip(const string &tip)    { m_tip->setText(tip); }
	void setCode(const string &code)  { m_code->setText(code); }

	virtual void setStyle() override { setWidgetStyle(WidgetType::TEST_WIDGET); }
	virtual string descType() const override { return "ToolTipTest"; }
};

class TestTextWidget : public StaticText {
public:
	TestTextWidget(Container *parent) : StaticText(parent) {
		m_backgroundStyle.setColour(m_rootWindow->getConfig()->getColourIndex(Vec3f(0.6f, 0.6f, 0.6f)));
		m_textStyle.m_colourIndex = m_rootWindow->getConfig()->getColourIndex(Vec3f(0.f, 0.f, 0.f));
	}

	virtual void render() override {
		Widget::renderBackground();
		int h = int(getFont(m_textStyle.m_fontIndex)->getMetrics()->getHeight());
		const int x1 = getScreenPos().x;
		const int x2 = x1 + getWidth();
		const int y_off = getScreenPos().y;
		int yLine = h;
		glDisable(GL_BLEND);
		glLineWidth(1.f);
		int i = 1;
		while (yLine < getHeight()) {
			Vec2i p1(x1, y_off + yLine);
			Vec2i p2(x2, y_off + yLine);
			glBegin(GL_LINES);
				glColor3f(0.1f, 0.1f, 0.1f);
				glVertex2iv(p1.ptr());
				glVertex2iv(p2.ptr());
			glEnd();
			yLine += h;
		}
		glEnable(GL_BLEND);
		if (TextWidget::hasText()) {
			TextWidget::renderText();
		}
	}
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

void populateMiscStrip1(CellStrip *parent, int cell, std::vector<string> &fruit) {
	parent->setSizeHint(cell, SizeHint(10, -1));
	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors tickerAnchors(Anchor(AnchorType::RIGID, 5), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, 5), Anchor(AnchorType::RIGID, 0));
	CellStrip *topStrip = new CellStrip(parent, Orientation::HORIZONTAL);
	topStrip->setCell(cell);
	topStrip->setAnchors(padAnchors);
	topStrip->addCells(3);
//	topStrip->getCell(0)->setSizeHint(SizeHint(-1, 200));

	TickerTape *tickerTape = new TickerTape(topStrip, SizeHint(80), Alignment::FLUSH_RIGHT);
	tickerTape->setCell(0);
	tickerTape->addItems(fruit);
	tickerTape->setAnchors(tickerAnchors);
	tickerTape->setOffsets(Vec2i(-200, 0), Vec2i(200, 0));
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

void populateBigTestStrip(CellStrip *parent, int cell, std::vector<string> &fruit) {
	parent->setSizeHint(cell, SizeHint());
	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding
	CellStrip *middleStrip = new CellStrip(parent, Orientation::HORIZONTAL);
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

	CellStrip *rightStrip = new CellStrip(middleStrip, Orientation::VERTICAL, 1);
	rightStrip->setCell(2);
	rightStrip->setAnchors(Anchors(Anchor(AnchorType::RIGID, 2)));

	txt = "Battlemage";
	TooltipTest *testText = new TooltipTest(rightStrip);
	testText->setCell(0);
	testText->setAnchors(padAnchors);
	testText->setHeader(txt);
	txt = "Versatile magic warrior. Has a powerful ranged attack, good speed and decent armor. Weak against melee units. Can be upgraded to Archmage.";
	testText->setTip(txt);

	txt = "int n = getTextureIndex(path);\n\
if (n != -1) {\n\
    return n;\n\
}\n\
Texture *tex = newTex(ResourceScope::GLOBAL);\n\
try {\n\
    tex->setMipmap(false);\n\
    tex->load(path);\n\
    tex->init();\n\
    addGlestTexture(name, tex);\n\
    return m_textures.size() - 1;\n\
} catch (const runtime_error &e) {\n\
    return -1;\n\
}";

	testText->setCode(txt);
	//testText->setText(txt);
	//testText->setCentre(false);
	//testText->setTextPos(Vec2i(0));
}

void populateMiscStrip2(CellStrip *parent, int cell, TestPane *tp) {
	parent->setSizeHint(cell, SizeHint(10, -1));
	Anchors padAnchors(Anchor(AnchorType::RIGID, 15)); // fill with 15 px padding
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors btnAnchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 5),
		Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 5));

	CellStrip *middleStrip2 = new CellStrip(parent, Orientation::HORIZONTAL);
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
	btn->setText("Dialog");
	btn->Clicked.connect(tp, &TestPane::onDoDialog);

	btn = new Button(buttonStrip);
	btn->setCell(1);
	btn->setAnchors(btnAnchors);
	btn->setText("Continue");
	btn->Clicked.connect(tp, &TestPane::onContinue);
}

void populatePlayerSlotStrip(CellStrip *parent, int cell) {
	parent->setSizeHint(cell, SizeHint(10, -1));
	Anchors psw_anchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 15),
		Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 15));
	PlayerSlotWidget *psw = new PlayerSlotWidget(parent);
	psw->setCell(0);
	psw->setAnchors(psw_anchors);
}

// =====================================================
//  class TestPane
// =====================================================

TestPane::TestPane(Program &program)
		: ProgramState(program), m_done(false), m_removingDialog(false), m_messageDialog(0), m_action(0) {
	Container *window = static_cast<Container*>(&program);
	WidgetConfig &cfg = g_widgetConfig;

	CellStrip *strip = new CellStrip(window, Orientation::VERTICAL, Origin::FROM_TOP, 4);
	strip->setPos(Vec2i(0));
	strip->setSize(g_metrics.getScreenDims());

	std::vector<string> fruit;
	populateFruitVector(fruit);

	populateMiscStrip1(strip, 1, fruit);
	populateBigTestStrip(strip, 2, fruit);
	populateMiscStrip2(strip, 3, this);
	populatePlayerSlotStrip(strip, 0);
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

	const string allowMask = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
	Vec2i size(320, 200), pos = g_metrics.getScreenDims() / 2 - size / 2;
	InputDialog* dialog = InputDialog::showDialog(pos, size, g_lang.get("SaveGame"),
		g_lang.get("SelectSaveGame"), g_lang.get("Save"), g_lang.get("Cancel"));
	dialog->setInputMask(allowMask);
	//dialog->Button1Clicked.connect(this, &GameState::onSaveSelected);
	dialog->Button2Clicked.connect(this, &TestPane::onDismissDialog);
	dialog->Escaped.connect(this, &TestPane::onDismissDialog);
	dialog->Close.connect(this, &TestPane::onDismissDialog);
	testDialog = dialog;

	//Vec2i size(500, 400);
	//Vec2i pos = (g_metrics.getScreenDims() - size) / 2;
	//m_messageDialog = MessageDialog::showDialog(pos, size, "Test MessageBox",
	//	"Message text goes here.\n\nSelect Continue to proceed to game.", "Continue", "Cancel");
	//m_messageDialog->Button1Clicked.connect(this, &TestPane::onContinue);
	//m_messageDialog->Button2Clicked.connect(this, &TestPane::onDismissDialog);
	//m_messageDialog->Close.connect(this, &TestPane::onDismissDialog);
	//m_messageDialog->Escaped.connect(this, &TestPane::onDismissDialog);

	//m_action = new MoveWidgetAction(60, m_messageDialog);
	//Vec2f start(pos);
	//start.x -= g_metrics.getScreenW();
	//m_action->setPosTransition(start, Vec2f(pos), TransitionFunc::LOGARITHMIC);
	//m_action->setAlphaTransition(0.f, 1.f, TransitionFunc::LINEAR);
	//m_action->update();
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

// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "compound_widgets.h"
#include "widget_window.h"
#include "core_data.h"
#include "lang.h"
#include "leak_dumper.h"
#include "config.h"
#include "renderer.h"

#include <list>

namespace Glest { namespace Widgets {
using Shared::Util::intToStr;
using namespace Global;
using namespace Shared::Graphics::Gl;


// =====================================================
//  class OptionContainer
// =====================================================

OptionContainer::OptionContainer(Container* parent, Vec2i pos, Vec2i size, const string &text)
		: Container(parent, pos, size) {
	m_abosulteLabelSize = false;
	m_labelSize = 30;
	int w = int(size.x * float(30) / 100.f);
	m_label = new StaticText(this, Vec2i(0), Vec2i(w, size.y));
	m_label->setTextParams(text, Vec4f(1.f), g_widgetConfig.getFont(m_textStyle.m_fontIndex), true);
	m_widget = 0;
}

void OptionContainer::setWidget(Widget* widget) {
	m_widget = widget;
	Vec2i size = getSize();
	int w;
	if (m_abosulteLabelSize) {
		w = m_labelSize;
	} else {
		w = int(size.x * float(m_labelSize) / 100.f);
	}
	m_label->setSize(Vec2i(w, size.y));
	if (m_widget) {
		m_widget->setPos(Vec2i(w,0));
		m_widget->setSize(Vec2i(size.x - w, size.y));
	}
}

void OptionContainer::setLabelWidth(int value, bool absolute) {
	m_abosulteLabelSize = absolute;
	m_labelSize = value;
	Vec2i size = getSize();
	int w;
	if (m_abosulteLabelSize) {
		w = m_labelSize;
	} else {
		w = int(size.x * float(m_labelSize) / 100.f);
	}
	m_label->setSize(Vec2i(w, size.y));
	if (m_widget) {
		m_widget->setPos(Vec2i(w,0));
		m_widget->setSize(Vec2i(size.x - w, size.y));
	}
}

Vec2i OptionContainer::getPrefSize() const {
	return Vec2i(700, 40);
}

Vec2i OptionContainer::getMinSize() const {
	return Vec2i(400, 30);
}

// =====================================================
//  class ScrollText
// =====================================================

ScrollText::ScrollText(Container* parent)
		: CellStrip(parent, Orientation::HORIZONTAL, 1)
		, TextWidget(this) {
	init();
}

ScrollText::ScrollText(Container* parent, Vec2i pos, Vec2i size)
		: CellStrip(parent, pos, size, Orientation::HORIZONTAL, Origin::CENTRE, 1)
		, TextWidget(this) {
	init();
	recalc();
}

void ScrollText::init() {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::TEXT_BOX));

	int itemSize = m_rootWindow->getConfig()->getDefaultItemHeight();

	// Anchors for scroll-bar, stick to the top, right & bottom sides.
	// Not anchored to left border, so we must set size (width will be respected, height wont)
	Anchors anchors(Anchor(AnchorType::NONE, 0), Anchor(AnchorType::RIGID, 0), // left, top
		Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0));           // right, bottom
	
	m_scrollBar = new ScrollBar(getCell(0), true, 10);
	m_scrollBar->setAnchors(anchors);
	m_scrollBar->setSize(Vec2i(itemSize));
	m_scrollBar->ThumbMoved.connect(this, &ScrollText::onScroll);

	// Anchors for text widget, stick to left, top & bottom, and 'itemSize' in from right
	anchors = Anchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, itemSize), Anchor(AnchorType::RIGID, 0));

	m_staticText = new StaticText(getCell(0));
	m_staticText->setAnchors(anchors);
}

void ScrollText::recalc() {
	int th = m_staticText->getTextDimensions().y;
	int ch = m_staticText->getHeight() - m_staticText->getBordersVert();
	m_scrollBar->setRanges(th, ch);
}

void ScrollText::onScroll(int offset) {
	int ox = m_staticText->getPos().x;
	m_staticText->setPos(Vec2i(ox, - offset));
}

void ScrollText::setSize(const Vec2i &sz) {
	CellStrip::setSize(sz);
	layoutCells();
	recalc();
}

void ScrollText::setText(const string &txt, bool scrollToBottom) {
	const FontMetrics *fm = m_staticText->getTextFont()->getMetrics();
	int width = getSize().w - m_scrollBar->getSize().w - getBordersHoriz() - m_staticText->getBordersHoriz();
	string text = txt;
	fm->wrapText(text, width);
	
	m_staticText->setText(text);
	recalc();
	
	if (scrollToBottom) {
		m_scrollBar->setOffsetPercent(100);
	}
}

void ScrollText::render() {
	CellStrip::render();
	assertGl();
}

// =====================================================
//  class TitleBar
// =====================================================

TitleBar::TitleBar(Container* parent)
		: Container(parent)
		, TextWidget(this)
		, m_title("")
		, m_closeButton(0) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::TITLE_BAR));
	setTextParams(m_title, Vec4f(1.f), g_widgetConfig.getFont(m_textStyle.m_fontIndex), false);
	setTextPos(Vec2i(5, 2));
}

TitleBar::TitleBar(Container* parent, Vec2i pos, Vec2i size, string title, bool closeBtn)
		: Container(parent, pos, size)
		//, MouseWidget(this)
		, TextWidget(this)
		, m_title(title)
		, m_closeButton(0) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::TITLE_BAR));
	setTextParams(m_title, Vec4f(1.f), g_widgetConfig.getFont(m_textStyle.m_fontIndex), false);
	setTextPos(Vec2i(5, 2));
	if (closeBtn) {
		int btn_sz = size.y - 4;
		Vec2i pos(size.x - btn_sz - 2, 2);
		m_closeButton = new Button(this, pos, Vec2i(btn_sz));
//		m_closeButton->setWidgetStyle(WidgetType::TITLE_BAR_CLOSE);
	}
}

void TitleBar::render() {
	Widget::renderBackground();
	TextWidget::renderText();
	Container::render();
}

Vec2i TitleBar::getPrefSize() const { return Vec2i(-1); }
Vec2i TitleBar::getMinSize() const { return Vec2i(-1); }

// =====================================================
//  class Frame
// =====================================================

Frame::Frame(WidgetWindow *ww)
		: Container(ww)
		, MouseWidget(this)
		, m_pressed(false) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::MESSAGE_BOX));
	m_titleBar = new TitleBar(this);
}

Frame::Frame(Container *parent)
		: Container(parent)
		, MouseWidget(this)
		, m_pressed(false) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::MESSAGE_BOX));
	m_titleBar = new TitleBar(this);
}

Frame::Frame(Container *parent, Vec2i pos, Vec2i sz)
		: Container(parent, pos, sz)
		, MouseWidget(this)
		, m_pressed(false) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::MESSAGE_BOX));
	m_titleBar = new TitleBar(this);
}

void Frame::init(Vec2i pos, Vec2i size, const string &title) {
	setPos(pos);
	setSize(size);
	setTitleText(title);
}

void Frame::setSize(Vec2i size) {
	Container::setSize(size);
	Vec2i p, s;
	Font *font = g_widgetConfig.getFont(m_textStyle.m_fontIndex);
	const FontMetrics *fm = font->getMetrics();

	int a = int(fm->getHeight() + 1.f) + 4;
	p = Vec2i(getBorderLeft(), getHeight() - a - getBorderTop());
	s = Vec2i(getWidth() - getBordersHoriz(), a);

	m_titleBar->setPos(p);
	m_titleBar->setSize(s);
}

void Frame::setTitleText(const string &text) {
	m_titleBar->setText(text);
}

bool Frame::mouseDown(MouseButton btn, Vec2i pos) {
	if (m_titleBar->isInside(pos) && btn == MouseButton::LEFT) {
		m_pressed = true;
		m_lastPos = pos;
		return true;
	}
	return false;
}

bool Frame::mouseMove(Vec2i pos) {
	if (m_pressed) {
		Vec2i offset = pos - m_lastPos;
		Vec2i newPos = getPos() + offset;
		setPos(newPos);
		m_lastPos = pos;
		return true;
	}
	return false;
}

bool Frame::mouseUp(MouseButton btn, Vec2i pos) {
	if (m_pressed && btn == MouseButton::LEFT) {
		m_pressed = false;
		return true;
	}
	return false;
}

void Frame::render() {
	renderBackground();
	Container::render();
}

// =====================================================
//  class BasicDialog
// =====================================================

BasicDialog::BasicDialog(WidgetWindow* window)
		: Frame(window), m_content(0)
		, m_button1(0), m_button2(0), m_buttonCount(0) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::MESSAGE_BOX));
	m_titleBar = new TitleBar(this);
}

BasicDialog::BasicDialog(Container* parent, Vec2i pos, Vec2i sz)
		: Frame(parent, pos, sz), m_content(0)
		, m_button1(0) , m_button2(0), m_buttonCount(0) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::MESSAGE_BOX));
	m_titleBar = new TitleBar(this);
}

void BasicDialog::init(Vec2i pos, Vec2i size, const string &title, 
					   const string &btn1Text, const string &btn2Text) {
	Frame::init(pos, size, title);
	setButtonText(btn1Text, btn2Text);
}

void BasicDialog::setContent(Widget* content) {
	m_content = content;
	Vec2i p, s;
	int a = m_titleBar->getHeight();
	p = Vec2i(getBorderLeft(), getBorderBottom() + (m_button1 ? 50 : 0));
	s = Vec2i(getWidth() - getBordersHoriz(), getHeight() - a - getBordersVert() - (m_button1 ? 50 : 0));
	m_content->setPos(p);
	m_content->setSize(s);
}

void BasicDialog::setButtonText(const string &btn1Text, const string &btn2Text) {
	int ndx = m_rootWindow->getConfig()->getWidgetStyle(WidgetType::BUTTON).textStyle().m_fontIndex;
	Font *font = g_widgetConfig.getFont(ndx);
	delete m_button1;
	m_button1 = 0;
	delete m_button2;
	m_button2 = 0;
	if (btn2Text.empty()) {
		if (btn1Text.empty()) {
			m_buttonCount = 0;
		} else {
			m_buttonCount = 1;
		}
	} else {
		m_buttonCount = 2;
	}
	if (!m_buttonCount) {
		return;
	}
	int gap = (getWidth() - 150 * m_buttonCount) / (m_buttonCount + 1);
	Vec2i p(gap, 10 + getBorderBottom());
	Vec2i s(150, 30);
	m_button1 = new Button(this, p, s);
	m_button1->setTextParams(btn1Text, Vec4f(1.f), font);
	m_button1->Clicked.connect(this, &MessageDialog::onButtonClicked);

	if (m_buttonCount == 2) {
		p.x += 150 + gap;
		m_button2 = new Button(this, p, s);
		m_button2->setTextParams(btn2Text, Vec4f(1.f), font);
		m_button2->Clicked.connect(this, &MessageDialog::onButtonClicked);
	}
}

void BasicDialog::onButtonClicked(Button* btn) {
	if (btn == m_button1) {
		Button1Clicked(this);
	} else {
		Button2Clicked(this);
	}
}

void BasicDialog::render() {
	renderBackground();
	Container::render();
}

// =====================================================
//  class MessageDialog
// =====================================================

MessageDialog::MessageDialog(WidgetWindow* window)
		: BasicDialog(window) {
	m_scrollText = new ScrollText(this);
}

MessageDialog* MessageDialog::showDialog(Vec2i pos, Vec2i size, const string &title, 
								const string &msg,  const string &btn1Text, const string &btn2Text) {
	MessageDialog* msgBox = new MessageDialog(&g_widgetWindow);
	g_widgetWindow.setFloatingWidget(msgBox, true);
	msgBox->init(pos, size, title, btn1Text, btn2Text);
	msgBox->setMessageText(msg);
	return msgBox;
}

MessageDialog::~MessageDialog(){
	delete m_scrollText;
}

void MessageDialog::setMessageText(const string &text) {
	setContent(m_scrollText);
	m_scrollText->setText(text);
	//m_scrollText->init();
}

// =====================================================
// class InputBox
// =====================================================

InputBox::InputBox(Container *parent)
		: TextBox(parent) {
}

InputBox::InputBox(Container *parent, Vec2i pos, Vec2i size)
		: TextBox(parent, pos, size){
}

bool InputBox::keyDown(Key key) {
	KeyCode code = key.getCode();
	switch (code) {
		case KeyCode::ESCAPE:
			Escaped(this);
			return true;
	}
	return TextBox::keyDown(key);
}

// =====================================================
//  class InputDialog
// =====================================================

InputDialog::InputDialog(WidgetWindow* window)
		: BasicDialog(window) {
	m_panel = new Panel(this);
	m_panel->setLayoutParams(true, Orientation::VERTICAL);
	m_panel->setPaddingParams(10, 10);
	m_label = new StaticText(m_panel);
	m_label->setTextParams("", Vec4f(1.f), g_widgetConfig.getFont(m_textStyle.m_fontIndex));
	m_inputBox = new InputBox(m_panel);
	m_inputBox->setTextParams("", Vec4f(1.f), g_widgetConfig.getFont(m_textStyle.m_fontIndex));
	m_inputBox->InputEntered.connect(this, &InputDialog::onInputEntered);
	m_inputBox->Escaped.connect(this, &InputDialog::onEscaped);
}

InputDialog* InputDialog::showDialog(Vec2i pos, Vec2i size, const string &title, const string &msg, 
							   const string &btn1Text, const string &btn2Text) {
	InputDialog* dlg = new InputDialog(&g_widgetWindow);
	g_widgetWindow.setFloatingWidget(dlg, true);
	dlg->init(pos, size, title, btn1Text, btn2Text);
	dlg->setContent(dlg->m_panel);
	dlg->setMessageText(msg);
	Vec2i sz = dlg->m_label->getPrefSize() + Vec2i(4);
	dlg->m_label->setSize(sz);
	sz.x = dlg->m_panel->getSize().x - 20;
	dlg->m_inputBox->setSize(sz);
	dlg->m_panel->layoutChildren();
	dlg->m_inputBox->gainFocus();
	dlg->m_inputBox->Escaped.connect(dlg, &InputDialog::onEscaped);
	return dlg;
}

void InputDialog::onInputEntered(TextBox*) {
	if (!m_inputBox->getText().empty()) {
		Button1Clicked(this);
	}
}

void InputDialog::setMessageText(const string &text) {
	m_label->setText(text);
}

}}

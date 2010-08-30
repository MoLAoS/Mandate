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
#include "faction.h"

#include <list>

namespace Glest { namespace Widgets {
using Shared::Util::intToStr;
using Global::CoreData;
using Global::Lang;
using Sim::ControlTypeNames;

// =====================================================
//  class PlayerSlotWidget
// =====================================================

PlayerSlotWidget::PlayerSlotWidget(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size) {
	Panel::setAutoLayout(false);
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::DROP_LIST);
	Widget::setPadding(0);
	assert(size.x > 200);
	float size_x = float(size.x - 30);
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

	CoreData &coreData = CoreData::getInstance();

	m_label = new StaticText(this, cpos, Vec2i(widths[0], 30));
	m_label->setTextParams("Player #", Vec4f(1.f), coreData.getFTMenuFontNormal(), true);

	cpos.x += widths[0] + 5;
	m_controlList = new DropList(this, cpos, Vec2i(widths[1], 30));
	foreach_enum (ControlType, ct) {
		m_controlList->addItem(g_lang.get(ControlTypeNames[ct]));
	}

	cpos.x += widths[1] + 5;
	m_factionList = new DropList(this, cpos, Vec2i(widths[2], 30));

	cpos.x += widths[2] + 5;
	m_teamList = new DropList(this, cpos, Vec2i(widths[3], 30));
	for (int i=1; i <= GameConstants::maxPlayers; ++i) {
		m_teamList->addItem(intToStr(i));
	}

	cpos.x += widths[3] + 5;
	m_colourList = new Widgets::DropList(this, cpos, Vec2i(widths[4], 30));
	for (int i=0; i < GameConstants::maxPlayers; ++ i) {
		m_colourList->addItem(Entities::Faction::factionColours[i]);
	}
	m_colourList->setSelected(0);

	m_controlList->SelectionChanged.connect(this, &PlayerSlotWidget::onControlChanged);
	m_factionList->SelectionChanged.connect(this, &PlayerSlotWidget::onFactionChanged);
	m_teamList->SelectionChanged.connect(this, &PlayerSlotWidget::onTeamChanged);
	m_colourList->SelectionChanged.connect(this, &PlayerSlotWidget::onColourChanged);
}

void PlayerSlotWidget::disableSlot() {
	setSelectedFaction(-1);
	setSelectedTeam(-1);
	setSelectedColour(-1);
	m_factionList->setEnabled(false);
	m_teamList->setEnabled(false);
	m_colourList->setEnabled(false);
}

void PlayerSlotWidget::enableSlot() {
	m_factionList->setEnabled(true);
	m_teamList->setEnabled(true);
	m_colourList->setEnabled(true);
}

// =====================================================
//  class OptionContainer
// =====================================================

OptionContainer::OptionContainer(Container::Ptr parent, Vec2i pos, Vec2i size, const string &text)
		: Container(parent, pos, size) {
	CoreData &coreData = CoreData::getInstance();
	m_abosulteLabelSize = false;
	m_labelSize = 30;
	int w = int(size.x * float(30) / 100.f);
	m_label = new StaticText(this, Vec2i(0), Vec2i(w, size.y));
	m_label->setTextParams(text, Vec4f(1.f), coreData.getFTMenuFontNormal(), true);
	m_widget = 0;
}

void OptionContainer::setWidget(Widget::Ptr widget) {
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

ScrollText::ScrollText(Container::Ptr parent)
		: Panel(parent)
		, MouseWidget(this)
		, TextWidget(this) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::TEXT_BOX);
//	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::TEXT_BOX);
	setAutoLayout(false);
	setPaddingParams(2, 0);
	setTextParams("", Vec4f(1.f), g_coreData.getFTMenuFontSmall(), false);
	m_scrollBar = new VerticalScrollBar(this);
}

ScrollText::ScrollText(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size)
		, MouseWidget(this)
		, TextWidget(this) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::TEXT_BOX);
//	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::TEXT_BOX);
	setAutoLayout(false);
	setPaddingParams(2, 0);
	setTextParams("", Vec4f(1.f), g_coreData.getFTMenuFontNormal(), false);
	Vec2i sbp(size.x - 26, 2);
	Vec2i sbs(24, size.y - 4);
	m_scrollBar = new VerticalScrollBar(this, sbp, sbs);
}

void ScrollText::recalc() {
	int th = getTextDimensions().y;
	int ch = getHeight() - getBordersVert() - getPadding() * 2;
	m_textBase = -(th - ch) + 2;
	m_scrollBar->setRanges(th, ch);
	setTextPos(Vec2i(5, m_textBase));
}

void ScrollText::init() {
	recalc();
	Vec2i sbp(getWidth() - 24 - getBorderRight(), getBorderBottom());
	Vec2i sbs(24, getHeight() - getBordersVert());
	m_scrollBar->setPos(sbp);
	m_scrollBar->setSize(sbs);
	m_scrollBar->ThumbMoved.connect(this, &ScrollText::onScroll);
}

void ScrollText::onScroll(VerticalScrollBar::Ptr sb) {
	int offset = sb->getRangeOffset();
	setTextPos(Vec2i(5, m_textBase - offset));
}

void ScrollText::setText(const string &txt, bool scrollToBottom) {
	const FontMetrics *fm = TextWidget::getTextFont()->getMetrics();
	int width = getSize().x - getPadding() * 2 - 24;
	std::list<string> words, lines;

	string::size_type startPos = 0;
	string::size_type spacePos = txt.find_first_of(' ');
	while (spacePos != string::npos) {
		words.push_back(txt.substr(startPos, spacePos - startPos));
		startPos = spacePos + 1;
		spacePos = txt.find_first_of(' ', startPos);
	}
	words.push_back(txt.substr(startPos));

	string result;
	string currLine;
	do {
		currLine = words.front();
		words.pop_front();
		while (!words.empty()) {
			string testLine = currLine + ' ' + words.front();
			if (fm->getTextDiminsions(testLine).x < width) {
				currLine = testLine;
				words.pop_front();
			} else {
				break;
			}
		}
		result += currLine + "\n";
	} while (!words.empty());
	TextWidget::setText(result);
	recalc();
	
	if (scrollToBottom) {
		m_scrollBar->setOffset(100.f);
	}
}

void ScrollText::render() {
	Widget::renderBgAndBorders(false);
	Vec2i pos = getScreenPos() + Vec2i(getBorderLeft(), getBorderBottom());
	Vec2i size = getSize() - getBordersAll();
	glPushAttrib(GL_SCISSOR_BIT);
		glEnable(GL_SCISSOR_TEST);
		glScissor(pos.x, pos.y, size.x, size.y);
		Container::render();
		TextWidget::renderText();
		glDisable(GL_SCISSOR_TEST);
	glPopAttrib();
}

// =====================================================
//  class TitleBar
// =====================================================

TitleBar::TitleBar(Container::Ptr parent)
		: Container(parent)
		, TextWidget(this)
		, m_title("")
		, m_closeButton(0) {
	//m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::TITLE_BAR);
	m_backgroundStyle.m_colourIndices[0] = g_widgetConfig.getColourIndex(Colour(0u, 0u, 0u, 255u));
	m_backgroundStyle.m_colourIndices[1] = g_widgetConfig.getColourIndex(Colour(31u, 31u, 31u, 255u));
	m_backgroundStyle.m_colourIndices[2] = g_widgetConfig.getColourIndex(Colour(127u, 127u, 127u, 255u));
	m_backgroundStyle.m_colourIndices[3] = g_widgetConfig.getColourIndex(Colour(91u, 91u, 91u, 255u));
	m_backgroundStyle.m_type = BackgroundType::CUSTOM_COLOURS;
	setTextParams(m_title, Vec4f(1.f), g_coreData.getFTMenuFontNormal(), false);
	setTextPos(Vec2i(5, 2));
}

TitleBar::TitleBar(Container::Ptr parent, Vec2i pos, Vec2i size, string title, bool closeBtn)
		: Container(parent, pos, size)
		//, MouseWidget(this)
		, TextWidget(this)
		, m_title(title)
		, m_closeButton(0) {
	///@todo specify in widget.cfg
	//m_backgroundStyle = g_widgetConfig.getBorderStyle(WidgetType::TITLE_BAR);
	//m_borderStyle = g_widgetConfig.getBackgroundStyle(WidgetType::TITLE_BAR);
	m_backgroundStyle.m_colourIndices[0] = g_widgetConfig.getColourIndex(Colour(0u, 0u, 0u, 255u));
	m_backgroundStyle.m_colourIndices[1] = g_widgetConfig.getColourIndex(Colour(31u, 31u, 31u, 255u));
	m_backgroundStyle.m_colourIndices[2] = g_widgetConfig.getColourIndex(Colour(127u, 127u, 127u, 255u));
	m_backgroundStyle.m_colourIndices[3] = g_widgetConfig.getColourIndex(Colour(91u, 91u, 91u, 255u));
	m_backgroundStyle.m_type = BackgroundType::CUSTOM_COLOURS;
	setTextParams(title, Vec4f(1.f), g_coreData.getFTMenuFontNormal(), false);
	setTextPos(Vec2i(5, 2));
	if (closeBtn) {
		int btn_sz = size.y - 4;
		Vec2i pos(size.x - btn_sz - 2, 2);
		m_closeButton = new Button(this, pos, Vec2i(btn_sz));
		m_closeButton->setImage(CoreData::getInstance().getCheckBoxCrossTexture());
	}
}

void TitleBar::render() {
	Widget::renderBgAndBorders();
	TextWidget::renderText();
	Container::render();
}

Vec2i TitleBar::getPrefSize() const { return Vec2i(-1); }
Vec2i TitleBar::getMinSize() const { return Vec2i(-1); }

// =====================================================
//  class BasicDialog
// =====================================================

BasicDialog::BasicDialog(WidgetWindow::Ptr window)
		: Container(window) , MouseWidget(this)
		, m_titleBar(0) , m_content(0)
		, m_button1(0) , m_button2(0)
		, m_buttonCount(0) , m_pressed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::MESSAGE_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::MESSAGE_BOX);
	m_titleBar = new TitleBar(this);
}

BasicDialog::BasicDialog(Container::Ptr parent, Vec2i pos, Vec2i sz)
		: Container(parent, pos, sz)
		, MouseWidget(this)
		, m_titleBar(0) , m_content(0)
		, m_button1(0) , m_button2(0)
		, m_buttonCount(0) , m_pressed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::MESSAGE_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::MESSAGE_BOX);
	m_titleBar = new TitleBar(this);
}

void BasicDialog::init(Vec2i pos, Vec2i size, const string &title, 
					   const string &btn1Text, const string &btn2Text) {
	setPos(pos);
	setSize(size);
	setTitleText(title);
	setButtonText(btn1Text, btn2Text);
}

void BasicDialog::setContent(Widget::Ptr content) {
	m_content = content;
	Vec2i p, s;
	Font *font = g_coreData.getFTMenuFontNormal();
	const FontMetrics *fm = font->getMetrics();

	int a = int(fm->getHeight() + 1.f) + 4;
	p = Vec2i(getBorderLeft(), getHeight() - a - getBorderTop());
	s = Vec2i(getWidth() - getBordersHoriz(), a);

	m_titleBar->setPos(p);
	m_titleBar->setSize(s);

	p = Vec2i(getBorderLeft(), getBorderBottom() + (m_button1 ? 50 : 0));
	s = Vec2i(getWidth() - getBordersHoriz(), getHeight() - a - getBordersVert() - (m_button1 ? 50 : 0));
	m_content->setPos(p);
	m_content->setSize(s);
}

void BasicDialog::setTitleText(const string &text) {
	m_titleBar->setText(text);
}

void BasicDialog::setButtonText(const string &btn1Text, const string &btn2Text) {
	Font *font = g_coreData.getFTMenuFontNormal();
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

void BasicDialog::onButtonClicked(Button::Ptr btn) {
	if (btn == m_button1) {
		Button1Clicked(this);
	} else {
		Button2Clicked(this);
	}
}

bool BasicDialog::mouseDown(MouseButton btn, Vec2i pos) {
	if (m_titleBar->isInside(pos) && btn == MouseButton::LEFT) {
		m_pressed = true;
		m_lastPos = pos;
		return true;
	}
	return false;
}

bool BasicDialog::mouseMove(Vec2i pos) {
	if (m_pressed) {
		Vec2i offset = pos - m_lastPos;
		Vec2i newPos = getPos() + offset;
		setPos(newPos);
		m_lastPos = pos;
		return true;
	}
	return false;
}

bool BasicDialog::mouseUp(MouseButton btn, Vec2i pos) {
	if (m_pressed && btn == MouseButton::LEFT) {
		m_pressed = false;
		return true;
	}
	return false;
}

void BasicDialog::render() {
	renderBgAndBorders();
	Container::render();
}

// =====================================================
//  class MessageDialog
// =====================================================

MessageDialog::MessageDialog(WidgetWindow::Ptr window)
		: BasicDialog(window) {
	m_scrollText = new ScrollText(this);
}

MessageDialog::Ptr MessageDialog::showDialog(Vec2i pos, Vec2i size, const string &title, 
								const string &msg,  const string &btn1Text, const string &btn2Text) {
	MessageDialog::Ptr msgBox = new MessageDialog(&g_widgetWindow);
	g_widgetWindow.setFloatingWidget(msgBox, true);
	msgBox->init(pos, size, title, btn1Text, btn2Text);
	msgBox->setMessageText(msg);
	return msgBox;
}

void MessageDialog::setMessageText(const string &text) {
	setContent(m_scrollText);
	m_scrollText->setText(text);
	m_scrollText->init();
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

InputDialog::InputDialog(WidgetWindow::Ptr window)
		: BasicDialog(window) {
	m_panel = new Panel(this);
	m_panel->setLayoutParams(true, Panel::LayoutDirection::VERTICAL);
	m_panel->setPaddingParams(10, 10);
	m_label = new StaticText(m_panel);
	m_label->setTextParams("", Vec4f(1.f), g_coreData.getFTMenuFontNormal());
	m_inputBox = new InputBox(m_panel);
	m_inputBox->setTextParams("", Vec4f(1.f), g_coreData.getFTMenuFontNormal());
	m_inputBox->InputEntered.connect(this, &InputDialog::onInputEntered);
	m_inputBox->Escaped.connect(this, &InputDialog::onEscaped);
}

InputDialog::Ptr InputDialog::showDialog(Vec2i pos, Vec2i size, const string &title, const string &msg, 
							   const string &btn1Text, const string &btn2Text) {
	InputDialog::Ptr dlg = new InputDialog(&g_widgetWindow);
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

void InputDialog::onInputEntered(TextBox::Ptr) {
	if (!m_inputBox->getText().empty()) {
		Button1Clicked(this);
	}
}

void InputDialog::setMessageText(const string &text) {
	m_label->setText(text);
}

}}

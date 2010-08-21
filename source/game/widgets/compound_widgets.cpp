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
	m_label->setTextParams("Player #", Vec4f(1.f), coreData.getfreeTypeMenuFont(), true);

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

OptionContainer::OptionContainer(Container::Ptr parent, Vec2i pos, Vec2i size, const string &text)
		: Container(parent, pos, size) {
	CoreData &coreData = CoreData::getInstance();
	m_abosulteLabelSize = false;
	m_labelSize = 30;
	int w = int(size.x * float(30) / 100.f);
	m_label = new StaticText(this, Vec2i(0), Vec2i(w, size.y));
	m_label->setTextParams(text, Vec4f(1.f), coreData.getfreeTypeMenuFont(), true);
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

ScrollText::ScrollText(Container::Ptr parent)
		: Panel(parent)
		, MouseWidget(this)
		, TextWidget(this) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::TEXT_BOX);
//	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::TEXT_BOX);
	setAutoLayout(false);
	setPaddingParams(2, 0);
	setTextParams("", Vec4f(1.f), g_coreData.getFTMenuFontNormal(), false);
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

void ScrollText::init() {
	int th = getTextDimensions().y;
	int ch = getHeight() - getBordersVert() - getPadding() * 2;
	m_textBase = -(th - ch);
	m_scrollBar->setRanges(th, ch);
	m_scrollBar->ThumbMoved.connect(this, &ScrollText::onScroll);
	setTextPos(Vec2i(5, m_textBase));

	Vec2i sbp(getWidth() - 24 - getBorderRight(), getBorderBottom());
	Vec2i sbs(24, getHeight() - getBordersVert());
	m_scrollBar->setPos(sbp);
	m_scrollBar->setSize(sbs);
}

void ScrollText::onScroll(VerticalScrollBar::Ptr sb) {
	int offset = sb->getRangeOffset();
	setTextPos(Vec2i(5, m_textBase - offset));
}

void ScrollText::setText(const string &txt) {
	const FontMetrics *fm = TextWidget::getTextFont()->getMetrics();
	int width = getSize().x - getPadding() * 2 - m_scrollBar->getSize().x;
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

MessageDialog::MessageDialog(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Container(parent, pos, size)
		, MouseWidget(this)
		, m_button1(0)
		, m_button2(0)
		, m_buttonCount(0)
		, m_pressed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::MESSAGE_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::MESSAGE_BOX);

	Vec2i p, s;
	Font *font = g_coreData.getFTMenuFontNormal();
	const FontMetrics *fm = font->getMetrics();

	int a = int(fm->getHeight() + 1.f) + 4;
	p = Vec2i(getBorderLeft(), size.y - a - getBorderTop());
	s = Vec2i(size.x - getBordersHoriz(), a);

	m_titleBar = new TitleBar(this, p, s, "", false);

	p = Vec2i(getBorderLeft(), getBorderBottom());
	s = Vec2i(size.x - getBordersHoriz(), size.y - a - getBordersVert());
	m_scrollText = new ScrollText(this, p, s);
	string title = "test text\ntext test\ntesting text\ntexting text\n";
	m_scrollText->setTextParams(title, Vec4f(1.f), font, false);
	m_scrollText->init();
}

MessageDialog::MessageDialog(WidgetWindow::Ptr window)
		: Container(window)
		, MouseWidget(this)
		, m_button1(0)
		, m_button2(0)
		, m_buttonCount(0)
		, m_pressed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::MESSAGE_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::MESSAGE_BOX);
	m_titleBar = new TitleBar(this);
	m_scrollText = new ScrollText(this);
}

void MessageDialog::setSize(const Vec2i &sz) {
	Container::setSize(sz);
	init();
}

void MessageDialog::init() {
	Vec2i p, s;
	Font *font = g_coreData.getFTMenuFontNormal();
	const FontMetrics *fm = font->getMetrics();

	int a = int(fm->getHeight() + 1.f) + 4;
	p = Vec2i(getBorderLeft(), getHeight() - a - getBorderTop());
	s = Vec2i(getWidth() - getBordersHoriz(), a);

	m_titleBar->setPos(p);
	m_titleBar->setSize(s);

	p = Vec2i(getBorderLeft(), getBorderBottom());
	s = Vec2i(getWidth() - getBordersHoriz(), getHeight() - a - getBordersVert());
	m_scrollText->setPos(p);
	m_scrollText->setSize(s);
	m_scrollText->init();
}

void MessageDialog::setTitleText(const string &text) {
	m_titleBar->setText(text);
}

void MessageDialog::setMessageText(const string &text) {
	///@todo get FontMetrics, split text into lines
	m_scrollText->setText(text);
	m_scrollText->init();
}

void MessageDialog::setButtonText(const string &btn1Text, const string &btn2Text) {
	Font *font = g_coreData.getFTMenuFontNormal();
	delete m_button1;
	m_button1 = 0;
	delete m_button2;
	m_button2 = 0;
	if (btn2Text.empty()) {
		m_buttonCount = 1;
	} else {
		m_buttonCount = 2;
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

	m_scrollText->setSize(getWidth() - getBordersHoriz(),
		getHeight() - getBordersVert() - m_titleBar->getHeight() - 50);
	m_scrollText->setPos(Vec2i(getBorderLeft(), 50 + getBorderBottom()));
	m_scrollText->init();
}

void MessageDialog::onButtonClicked(Button::Ptr btn) {
	if (btn == m_button1) {
		Button1Clicked(this);
	} else {
		Button2Clicked(this);
	}
}

bool MessageDialog::mouseDown(MouseButton btn, Vec2i pos) {
	if (m_titleBar->isInside(pos) && btn == MouseButton::LEFT) {
		m_pressed = true;
		m_lastPos = pos;
		return true;
	}
	return false;
}

bool MessageDialog::mouseMove(Vec2i pos) {
	if (m_pressed) {
		Vec2i offset = pos - m_lastPos;
		Vec2i newPos = getPos() + offset;
		setPos(newPos);
		m_lastPos = pos;
		return true;
	}
	return false;
}

bool MessageDialog::mouseUp(MouseButton btn, Vec2i pos) {
	if (m_pressed && btn == MouseButton::LEFT) {
		m_pressed = false;
		return true;
	}
	return false;
}

void MessageDialog::render() {
	renderBgAndBorders();
	Container::render();
}

}}

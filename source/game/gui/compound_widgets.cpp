// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "compound_widgets.h"
#include "core_data.h"
#include "lang.h"

namespace Glest { namespace Widgets {
using Shared::Util::intToStr;
using Global::CoreData;
using Global::Lang;
using Sim::ControlTypeNames;

PlayerSlotWidget::PlayerSlotWidget(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size) {
	Panel::setAutoLayout(false);
	Widget::setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.4f);
	Widget::setPadding(0);
	assert(size.x > 200);
	float size_x = float(size.x - 25);
	float fwidths[] = { size_x * 20.f / 100.f, size_x * 35.f / 100.f, size_x * 35.f / 100.f, size_x * 10.f / 100.f};
	int widths[4];
	for (int i=0; i < 4; ++i) {
		widths[i] = int(fwidths[i]);
	}
	Vec2i cpos(5, 2);
	
	CoreData &coreData = CoreData::getInstance();

	m_label = new StaticText(this, cpos, Vec2i(widths[0], 30));
	m_label->setTextParams("Player #", Vec4f(1.f), coreData.getfreeTypeMenuFont(), true);
	
	cpos.x += widths[0] + 5;
	m_controlList = new DropList(this, cpos, Vec2i(widths[1], 30));
	m_controlList->setBorderSize(0);
	foreach_enum (ControlType, ct) {
		m_controlList->addItem(theLang.get(ControlTypeNames[ct]));
	}
	
	cpos.x += widths[1] + 5;	
	m_factionList = new DropList(this, cpos, Vec2i(widths[2], 30));
	m_factionList->setBorderSize(0);

	cpos.x += widths[2] + 5;	
	m_teamList = new DropList(this, cpos, Vec2i(widths[3], 30));
	for (int i=1; i <= GameConstants::maxPlayers; ++i) {
		m_teamList->addItem(intToStr(i));
	}
	m_teamList->setBorderSize(0);

	m_controlList->SelectionChanged.connect(this, &PlayerSlotWidget::onControlChanged);
	m_factionList->SelectionChanged.connect(this, &PlayerSlotWidget::onFactionChanged);
	m_teamList->SelectionChanged.connect(this, &PlayerSlotWidget::onTeamChanged);
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

ScrollText::ScrollText(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size)
		, MouseWidget(this)
		, TextWidget(this) {
	setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.5f);
	setAutoLayout(false);
	setPaddingParams(2, 0);
	Vec2i sbp(size.x - 26, 2);
	Vec2i sbs(24, size.y - 4);
	m_scrollBar = new VerticalScrollBar(this, sbp, sbs);
}

void ScrollText::init() {
	int th = getTextDimensions().y;
	int ch = getSize().y - getBorderSize() * 2 - getPadding() * 2;
	m_textBase = -(th - ch);
	m_scrollBar->setRanges(th, ch);
	m_scrollBar->ThumbMoved.connect(this, &ScrollText::onScroll);
	setTextPos(Vec2i(5, m_textBase));
}

void ScrollText::onScroll(VerticalScrollBar::Ptr sb) {
	int offset = sb->getRangeOffset();
	setTextPos(Vec2i(5, m_textBase - offset));
}

void ScrollText::render() {
	Widget::renderBgAndBorders(false);
	Vec2i pos = getScreenPos() + Vec2i(getBorderSize());
	Vec2i size = getSize() - Vec2i(getBorderSize()) * 2;
	glPushAttrib(GL_SCISSOR_BIT);
		glEnable(GL_SCISSOR_TEST);
		glScissor(pos.x, pos.y, size.x, size.y);
		Container::render();
		TextWidget::renderText();
		glDisable(GL_SCISSOR_TEST);
	glPopAttrib();
}

TitleBar::TitleBar(Container::Ptr parent, Vec2i pos, Vec2i size, string title, bool closeBtn)
		: Container(parent, pos, size)
		, MouseWidget(this)
		, TextWidget(this)
		, m_title(title)
		, m_closeButton(0) {
	if (closeBtn) {
		m_closeButton = new Button(this);
		///@todo position
		m_closeButton->setImage(CoreData::getInstance().getCheckBoxCrossTexture());
	}
}

}}

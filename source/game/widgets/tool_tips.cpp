// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "tool_tips.h"

#include "widget_window.h"
#include "widget_config.h"
#include "config.h"
#include "lang.h"

#include "leak_dumper.h"

namespace Glest { namespace Widgets {

using namespace Global;

inline Vec4f Vec4ubToVec4f(const Colour &c) {
	return Vec4f(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
}

CommandTipElement::CommandTipElement(Container *parent, const string &msg)
		: StaticText(parent) {
	WidgetConfig &cfg = g_widgetConfig;
	setWidgetStyle(WidgetType::TOOLTIP_ITEM);
	int max_w = g_config.getDisplayWidth() / 5;
	string txt = msg;
	getFont()->getMetrics()->wrapText(txt, max_w);
	Vec2i dims = Vec2i(getFont()->getMetrics()->getTextDiminsions(txt));
	Vec2i txtOffset = Vec2i(getBorderLeft(), getBorderTop());
	TextWidget::setAlignment(Alignment::NONE);
	TextWidget::setText(txt);
	TextWidget::setTextPos(txtOffset);
	TextWidget::setTextColour(Vec4ubToVec4f(cfg.getColour(m_textStyle.m_colourIndex)));
	if (m_textStyle.m_shadow) {
		StaticText::setShadow(Vec4ubToVec4f(cfg.getColour(m_textStyle.m_shadowColourIndex)), 1);
	}
}

// =====================================================
//  class CommandTipItem
// =====================================================

CommandTipItem::CommandTipItem(Container *parent, const DisplayableType *dt, const string &msg)
		: CommandTipElement(parent, msg)
		, ImageWidget(this) {
	WidgetConfig &cfg = g_widgetConfig;
	int imgSz = cfg.getDefaultItemHeight();
	Vec2i imgOffset = Vec2i(getBorderLeft(), getBorderTop());

	int max_w = g_config.getDisplayWidth() / 5 - (imgSz + 4);
	string txt = msg;
	getFont()->getMetrics()->wrapText(txt, max_w);
	Vec2i dims = Vec2i(getFont()->getMetrics()->getTextDiminsions(txt));
	Vec2i txtOffset = imgOffset + Vec2i(imgSz + 4, (imgSz - dims.h) / 2);

	ImageWidget::addImageX(dt->getImage(), imgOffset, Vec2i(imgSz, imgSz));
	TextWidget::setTextPos(txtOffset);
}

void CommandTipItem::render() {
	ImageWidget::renderImage();
	StaticText::render();
}

Vec2i CommandTipItem::getPrefSize() const {
	int imgSz = g_widgetConfig.getDefaultItemHeight();
	Vec2i res = StaticText::getPrefSize();
	res.w += imgSz + 4;
	if (res.h < imgSz + getBordersVert()) {
		res.h = imgSz + getBordersVert();
	}
	return res;
}

// =====================================================
//  class CommandTipReq
// =====================================================

CommandTipReq::CommandTipReq(Container *parent, const DisplayableType *dt, bool ok, const string &msg)
		: CommandTipItem(parent, dt, msg)
		, m_reqMet(ok) {
	WidgetConfig &cfg = g_widgetConfig;
	setWidgetStyle(ok ? WidgetType::TOOLTIP_REQ_OK : WidgetType::TOOLTIP_REQ_NOK);
	TextWidget::setTextColour(Vec4ubToVec4f(cfg.getColour(m_textStyle.m_colourIndex)));
	if (m_textStyle.m_shadow) {
		StaticText::setShadow(Vec4ubToVec4f(cfg.getColour(m_textStyle.m_shadowColourIndex)), 1);
	}
}

void CommandTipReq::setStyle() {
	setWidgetStyle(m_reqMet ? WidgetType::TOOLTIP_REQ_OK : WidgetType::TOOLTIP_REQ_NOK);
}

// =====================================================
//  class CommandTip
// =====================================================

CommandTip::CommandTip(Container *parent)
		: Container(parent) {
	setWidgetStyle(WidgetType::TOOLTIP);
	m_header = new CommandTipHeader(this);
	m_tip = new CommandTipMain(this);
}

void CommandTip::setHeader(const string &header) {
	m_commandName = header;
	m_header->setHeaderText(header);
	if (header.empty()) {
		m_header->setSize(Vec2i(0));
	} else {
		m_header->setSize(m_header->getPrefSize());
	}
	layout();
}

void CommandTip::setTipText(const string &tipText) {
	m_tipText = tipText;
	m_tip->setTipText(tipText);
	m_tip->setSize(m_tip->getPrefSize());
	layout();
}

void CommandTip::addElement(const string &msg) {
	CommandTipElement *elmnt = new CommandTipElement(this, msg);
	elmnt->setSize(elmnt->getPrefSize());
	m_items.push_back(elmnt);
	layout();
}

void CommandTip::addItem(const DisplayableType *dt, const string &msg) {
	CommandTipItem *item = new CommandTipItem(this, dt, msg);
	item->setSize(item->getPrefSize());
	m_items.push_back(item);
	layout();
}

void CommandTip::addReq(const DisplayableType *dt, bool ok, const string &msg) {
	CommandTipReq *ctr = new CommandTipReq(this, dt, ok, msg);
	ctr->setSize(ctr->getPrefSize());
	m_items.push_back(ctr);
	layout();
}

void CommandTip::layout() {
	Vec2i size(0,0);
	Vec2i offset(getBorderLeft(), getBorderTop());
	size = m_header->getSize();
	m_header->setPos(offset);
	offset.y += m_header->getHeight();
	if (!m_tip->getText().empty()) {
		size.h += m_tip->getHeight();
		m_tip->setPos(offset);
		offset.y += m_tip->getHeight();
		if (size.w < m_tip->getWidth()) {
			size.w = m_tip->getWidth();
		}
	}
	foreach (vector<CommandTipElement*>, it, m_items) {
		Vec2i sz = (*it)->getSize();
		(*it)->setPos(offset);
		offset.y += sz.h;
		size.h += sz.h;
		if (size.w < sz.w) {
			size.w = sz.w;
		}
	}
	size += getBordersAll();
	setSize(size);
}

void CommandTip::clearItems() {
	foreach (vector<CommandTipElement*>, it, m_items) {
		delete *it;
	}
	m_items.clear();
	layout();
}

// =====================================================
//  class ToolTip
// =====================================================

WidgetTip::WidgetTip(WidgetWindow *window, Widget* widget, const string &text)
		: Container(window), m_myWidget(widget) {
	m_tipLabel = new StaticText(this);
	setWidgetStyle(WidgetType::TOOLTIP);
	m_tipLabel->setText("");
	m_tipLabel->setAlignment(Alignment::NONE);
	m_tipLabel->setTextPos(Vec2i(getBorderLeft() + 2, getBorderTop() + 2));

	m_tipLabel->setText(text);
	Vec2i dims = m_tipLabel->getTextDimensions(0);
	dims += getBordersAll() + Vec2i(4);
	setSize(dims);
	m_tipLabel->setSize(dims);

	// position sensibly
	Vec2i wPos = m_myWidget->getScreenPos();
	Vec2i wSize = m_myWidget->getSize();
	Vec2i midPoint = wPos + wSize / 2;
	Vec2i pos;
	// prefer under widget
	if (wPos.y + wSize.h + dims.h + 4< g_config.getDisplayHeight()) {
		pos.y = wPos.y + wSize.h + 4;
	} else {
		pos.y = wPos.y - dims.h - 4;
	}
	// prefer to right from centre of widget
	if (midPoint.x + dims.w < g_config.getDisplayWidth()) {
		pos.x = midPoint.x;
	} else {
		pos.x = g_config.getDisplayWidth() - dims.w - 4;
	}
	setPos(pos);
}

// =====================================================
//  Items For Trade Tip
// =====================================================

TradeTipElement::TradeTipElement(Container *parent, const string &msg)
		: StaticText(parent) {
	WidgetConfig &cfg = g_widgetConfig;
	setWidgetStyle(WidgetType::TOOLTIP_ITEM);
	int max_w = g_config.getDisplayWidth() / 5;
	string txt = msg;
	getFont()->getMetrics()->wrapText(txt, max_w);
	Vec2i dims = Vec2i(getFont()->getMetrics()->getTextDiminsions(txt));
	Vec2i txtOffset = Vec2i(getBorderLeft(), getBorderTop());
	TextWidget::setAlignment(Alignment::NONE);
	TextWidget::setText(txt);
	TextWidget::setTextPos(txtOffset);
	TextWidget::setTextColour(Vec4ubToVec4f(cfg.getColour(m_textStyle.m_colourIndex)));
	if (m_textStyle.m_shadow) {
		StaticText::setShadow(Vec4ubToVec4f(cfg.getColour(m_textStyle.m_shadowColourIndex)), 1);
	}
}

// =====================================================
//  class TradeTipItem
// =====================================================

TradeTipItem::TradeTipItem(Container *parent, const DisplayableType *dt, const string &msg)
		: TradeTipElement(parent, msg)
		, ImageWidget(this) {
	WidgetConfig &cfg = g_widgetConfig;
	int imgSz = cfg.getDefaultItemHeight();
	Vec2i imgOffset = Vec2i(getBorderLeft(), getBorderTop());

	int max_w = g_config.getDisplayWidth() / 5 - (imgSz + 4);
	string txt = msg;
	getFont()->getMetrics()->wrapText(txt, max_w);
	Vec2i dims = Vec2i(getFont()->getMetrics()->getTextDiminsions(txt));
	Vec2i txtOffset = imgOffset + Vec2i(imgSz + 4, (imgSz - dims.h) / 2);
	ImageWidget::addImageX(dt->getImage(), imgOffset, Vec2i(imgSz, imgSz));
	TextWidget::setTextPos(txtOffset);
}

void TradeTipItem::render() {
	ImageWidget::renderImage();
	StaticText::render();
}

Vec2i TradeTipItem::getPrefSize() const {
	int imgSz = g_widgetConfig.getDefaultItemHeight();
	Vec2i res = StaticText::getPrefSize();
	res.w += imgSz + 4;
	if (res.h < imgSz + getBordersVert()) {
		res.h = imgSz + getBordersVert();
	}
	return res;
}

// =====================================================
//  class TradeTipReq
// =====================================================

TradeTipReq::TradeTipReq(Container *parent, bool ok, const DisplayableType *dt, const string &msg)
		: TradeTipItem(parent, dt, msg)
		, m_reqMet(ok) {
	WidgetConfig &cfg = g_widgetConfig;
	setWidgetStyle(ok ? WidgetType::TOOLTIP_REQ_OK : WidgetType::TOOLTIP_REQ_NOK);
	TextWidget::setTextColour(Vec4ubToVec4f(cfg.getColour(m_textStyle.m_colourIndex)));
	if (m_textStyle.m_shadow) {
		StaticText::setShadow(Vec4ubToVec4f(cfg.getColour(m_textStyle.m_shadowColourIndex)), 1);
	}
}

void TradeTipReq::setStyle() {
	setWidgetStyle(m_reqMet ? WidgetType::TOOLTIP_REQ_OK : WidgetType::TOOLTIP_REQ_NOK);
}

// =====================================================
//  class TradeTip
// =====================================================

TradeTip::TradeTip(Container *parent)
		: Container(parent) {
	setWidgetStyle(WidgetType::TOOLTIP);
	m_header = new TradeTipHeader(this);
	m_tip = new TradeTipMain(this);
}

void TradeTip::setHeader(const string &header) {
	m_tradeName = header;
	m_header->setHeaderText(header);
	if (header.empty()) {
		m_header->setSize(Vec2i(0));
	} else {
		m_header->setSize(m_header->getPrefSize());
	}
	layout();
}

void TradeTip::setTipText(const string &tipText) {
	m_tipText = tipText;
	m_tip->setTipText(tipText);
	m_tip->setSize(m_tip->getPrefSize());
	layout();
}

void TradeTip::addElement(const string &msg) {
	TradeTipElement *elmnt = new TradeTipElement(this, msg);
	elmnt->setSize(elmnt->getPrefSize());
	m_items.push_back(elmnt);
	layout();
}

void TradeTip::addItem(const DisplayableType *dt, const string &msg) {
	TradeTipItem *item = new TradeTipItem(this, dt, msg);
	item->setSize(item->getPrefSize());
	m_items.push_back(item);
	layout();
}

void TradeTip::addReq(bool ok, const DisplayableType *dt, const string &msg) {
	TradeTipReq *ttr = new TradeTipReq(this, ok, dt, msg);
	ttr->setSize(ttr->getPrefSize());
	m_items.push_back(ttr);
	layout();
}

void TradeTip::layout() {
	Vec2i size(0,0);
	Vec2i offset(getBorderLeft(), getBorderTop());
	size = m_header->getSize();
	m_header->setPos(offset);
	offset.y += m_header->getHeight();
	if (!m_tip->getText().empty()) {
		size.h += m_tip->getHeight();
		m_tip->setPos(offset);
		offset.y += m_tip->getHeight();
		if (size.w < m_tip->getWidth()) {
			size.w = m_tip->getWidth();
		}
	}
	foreach (vector<TradeTipElement*>, it, m_items) {
		Vec2i sz = (*it)->getSize();
		(*it)->setPos(offset);
		offset.y += sz.h;
		size.h += sz.h;
		if (size.w < sz.w) {
			size.w = sz.w;
		}
	}
	size += getBordersAll();
	setSize(size);
}

void TradeTip::clearItems() {
	foreach (vector<TradeTipElement*>, it, m_items) {
		delete *it;
	}
	m_items.clear();
	layout();
}

// =====================================================
//  class FormationTip
// =====================================================

FormationTip::FormationTip(Container *parent)
		: Container(parent) {
	setWidgetStyle(WidgetType::TOOLTIP);
	m_header = new TradeTipHeader(this);
	m_tip = new TradeTipMain(this);
}

void FormationTip::setHeader(const string &header) {
	m_tradeName = header;
	m_header->setHeaderText(header);
	if (header.empty()) {
		m_header->setSize(Vec2i(0));
	} else {
		m_header->setSize(m_header->getPrefSize());
	}
	layout();
}

void FormationTip::setTipText(const string &tipText) {
	m_tipText = tipText;
	m_tip->setTipText(tipText);
	m_tip->setSize(m_tip->getPrefSize());
	layout();
}

void FormationTip::addElement(const string &msg) {
	TradeTipElement *elmnt = new TradeTipElement(this, msg);
	elmnt->setSize(elmnt->getPrefSize());
	m_items.push_back(elmnt);
	layout();
}

void FormationTip::addItem(const DisplayableType *dt, const string &msg) {
	TradeTipItem *item = new TradeTipItem(this, dt, msg);
	item->setSize(item->getPrefSize());
	m_items.push_back(item);
	layout();
}

void FormationTip::addReq(bool ok, const DisplayableType *dt, const string &msg) {
	TradeTipReq *ttr = new TradeTipReq(this, ok, dt, msg);
	ttr->setSize(ttr->getPrefSize());
	m_items.push_back(ttr);
	layout();
}

void FormationTip::layout() {
	Vec2i size(0,0);
	Vec2i offset(getBorderLeft(), getBorderTop());
	size = m_header->getSize();
	m_header->setPos(offset);
	offset.y += m_header->getHeight();
	if (!m_tip->getText().empty()) {
		size.h += m_tip->getHeight();
		m_tip->setPos(offset);
		offset.y += m_tip->getHeight();
		if (size.w < m_tip->getWidth()) {
			size.w = m_tip->getWidth();
		}
	}
	foreach (vector<TradeTipElement*>, it, m_items) {
		Vec2i sz = (*it)->getSize();
		(*it)->setPos(offset);
		offset.y += sz.h;
		size.h += sz.h;
		if (size.w < sz.w) {
			size.w = sz.w;
		}
	}
	size += getBordersAll();
	setSize(size);
}

void FormationTip::clearItems() {
	foreach (vector<TradeTipElement*>, it, m_items) {
		delete *it;
	}
	m_items.clear();
	layout();
}

}}

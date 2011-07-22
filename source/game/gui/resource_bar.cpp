// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "resource_bar.h"

#include "metrics.h"
#include "widget_window.h"
#include "framed_widgets.h"
#include "core_data.h"
#include "resource_type.h"
#include "math_util.h"
#include "config.h"
#include "faction.h"
#include "lang.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Math;

namespace Glest { namespace Gui {

using namespace Global;

void ResourceBarFrame::render() {
	if (g_config.getUiPhotoMode()) {
		return;
	}
	Frame::render();
}

ResourceBarFrame::ResourceBarFrame()
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::SHRINK | ButtonFlags::EXPAND) {
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);
	m_resourceBar = new ResourceBar(this);
	CellStrip::addCells(1);
	m_resourceBar ->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_resourceBar ->setAnchors(a);

	m_titleBar->enableShrinkExpand(false, false);
	Expand.connect(this, &ResourceBarFrame::onExpand);
	Shrink.connect(this, &ResourceBarFrame::onShrink);

	setPinned(g_config.getUiPinWidgets());
}

void ResourceBarFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

void ResourceBarFrame::onExpand(Widget*) {
}

void ResourceBarFrame::onShrink(Widget*) {
}

// =====================================================
// 	class ResourceBar
// =====================================================

ResourceBar::ResourceBar(Container *parent)
		: Widget(parent)
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_faction(0)
		, m_moveOffset(0)
		, m_draggingWidget(false)
		, m_updateCounter(0) {
	setWidgetStyle(WidgetType::RESOURCE_BAR);
}

void ResourceBar::init(const Faction *faction, std::set<const ResourceType*> &types) {
	m_faction = faction;

	TextWidget::setAlignment(Alignment::NONE);
	g_widgetWindow.registerUpdate(this);

	const Font *font = getFont();
	const FontMetrics *fm = font->getMetrics();

	foreach (std::set<const ResourceType*>, it, types) {
		m_resourceTypes.push_back(*it);
	}

	vector<int> reqWidths;
	int total_req = 0;
	foreach_const (vector<const ResourceType*>, it, m_resourceTypes) {
		addImage((*it)->getImage());
		string name = (*it)->getName();
		string tName = g_lang.getTechString(name);
		if (name == tName) {
			tName = formatString(name);
		}
		m_headerStrings.push_back(tName + ": ");
		addText(m_headerStrings.back());
		int w;
		if ((*it)->getClass() == ResourceClass::CONSUMABLE) {
			w = 16 + 4 + int(fm->getTextDiminsions(m_headerStrings.back() + "8000/80000 (8000)").w);
		} else if ((*it)->getClass() == ResourceClass::STATIC) {
			w = 16 + 4 + int(fm->getTextDiminsions(m_headerStrings.back() + "80000").w);
		} else {
			w = 16 + 4 + int(fm->getTextDiminsions(m_headerStrings.back() + "80000/800000").w);
		}
		total_req += w;
		reqWidths.push_back(w);
	}
	int max_width = g_metrics.getScreenW() - 20 - m_parent->getBordersHoriz();
	if (total_req < max_width) {
		// single row
		setSize(Vec2i(total_req, 20));
		m_parent->setSize(Vec2i(total_req + m_parent->getBordersHoriz(), 40 + m_parent->getBordersVert()));

		int x_pos = 5, y_pos = 2;
		for (int i=0; i < m_resourceTypes.size(); ++i) {
			setImageX(0, i, Vec2i(x_pos, y_pos), Vec2i(16, 16));
			setTextPos(Vec2i(x_pos + 20, y_pos), i);
			x_pos += reqWidths[i];
		}
	} else {
		// multi row (only 2 for now)
		///@todo support more than 2 rows?
		int width1 = 0, width2 = 0;
		int stopAt = reqWidths.size() / 2 + 1;
		for (int i=0; i < stopAt; ++i) {
			width1 += reqWidths[i];
		}
		for (int i=stopAt; i < reqWidths.size(); ++i) {
			width2 += reqWidths[i];
		}
		setSize(Vec2i(std::max(width1, width2), 40));
		Vec2i pSize(std::max(width1, width2) + m_parent->getBordersHoriz(), 60 + m_parent->getBordersVert());
		m_parent->setSize(pSize);

		int x_pos = 5, y_pos = 22;
		for (int i=0; i < stopAt; ++i) {
			setImageX(0, i, Vec2i(x_pos, y_pos), Vec2i(16, 16));
			setTextPos(Vec2i(x_pos + 20, y_pos), i);
			x_pos += reqWidths[i];			
		}
		x_pos = 5, y_pos = 2;
		for (int i=stopAt; i < reqWidths.size(); ++i) {
			setImageX(0, i, Vec2i(x_pos, y_pos), Vec2i(16, 16));
			setTextPos(Vec2i(x_pos + 20, y_pos), i);
			x_pos += reqWidths[i];			
		}
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() / 2 - m_parent->getWidth() / 2, 5));
}

ResourceBar::~ResourceBar() {
	g_widgetWindow.unregisterUpdate(this);
}

void ResourceBar::render() {
	Widget::render();
	for (int i=0; i < m_resourceTypes.size(); ++i) {
		renderImage(i);
		renderTextShadowed(i);
	}
}

///@todo fix... this is called 120 times per second
void ResourceBar::update() {
	if (m_updateCounter % 10 == 0) {
		for (int i=0; i < m_resourceTypes.size(); ++i) {
			stringstream ss;
			const ResourceType* &rt = m_resourceTypes[i];
			const StoredResource *res = m_faction->getResource(rt); // amount & balance
			ss << m_headerStrings[i] << res->getAmount();
			if (rt->getClass() == ResourceClass::CONSUMABLE) {
				ss << "/" << m_faction->getStoreAmount(rt) << " (" << res->getBalance() << ")";
			} else if (rt->getClass() != ResourceClass::STATIC) {
				ss << '/' << m_faction->getStoreAmount(rt);
			}
			setText(ss.str(), i);
		}
	}
	++m_updateCounter;
}

bool ResourceBar::mouseDown(MouseButton btn, Vec2i pos) {
	return false;
}

bool ResourceBar::mouseUp(MouseButton btn, Vec2i pos) {
	return false;
}

bool ResourceBar::mouseMove(Vec2i pos) {
	return false;
}


}}//end namespace

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
#include "trade_bar.h"

#include "metrics.h"
#include "widget_window.h"
#include "framed_widgets.h"
#include "core_data.h"
#include "resource_type.h"
#include "trade_command.h"
#include "math_util.h"
#include "world.h"
#include "game.h"
#include "unit.h"
#include "config.h"
#include "faction.h"
#include "lang.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Math;

namespace Glest { namespace Gui_Mandate {

using namespace Global;
using Global::CoreData;

// =====================================================
// 	class TradeBarFrame
// =====================================================

void TradeBarFrame::render() {
	Frame::render();
}

TradeBarFrame::TradeBarFrame(Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::SHRINK | ButtonFlags::EXPAND)
        , m_tradeBar(0){
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);
	m_tradeBar = new TradeBar(this, Vec2i(0,0));
	CellStrip::addCells(1);
	m_tradeBar->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_tradeBar ->setAnchors(a);
	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &TradeBarFrame::onExpand);
	Shrink.connect(this, &TradeBarFrame::onShrink);
	doEnableShrinkExpand(16);
	setPinned(g_config.getUiPinWidgets());
}

void TradeBarFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

void TradeBarFrame::doEnableShrinkExpand(int sz) {
	switch (sz) {
		case 16:
			enableShrinkExpand(false, true);
			break;
		case 24:
			enableShrinkExpand(true, true);
			break;
		case 32:
			enableShrinkExpand(true, false);
			break;
		default: assert(false);
	}
}

void TradeBarFrame::onExpand(Widget*) {
	assert(m_tradeBar->getFuzzySize() != FuzzySize::LARGE);
	FuzzySize sz = m_tradeBar->getFuzzySize();
	++sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_tradeBar->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void TradeBarFrame::onShrink(Widget*) {
	assert(m_tradeBar->getFuzzySize() != FuzzySize::SMALL);
	FuzzySize sz = m_tradeBar->getFuzzySize();
	--sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_tradeBar->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void TradeBarFrame::resetSize() {
    Vec2i size = m_tradeBar->getSize() + getBordersAll() + Vec2i(0, 20);
    if (size != getSize()){
        setSize(size);
    }
}

// =====================================================
// 	class TradeBar
// =====================================================

TradeBar::TradeBar(Container *parent, Vec2i pos)
		: Widget(parent)
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, world(0)
		//, m_faction(0)
		, m_tradeCommands(0)
		, m_imageSize(32)
		, m_tradeOffset(0)
		, m_draggingWidget(false)
		, m_updateCounter(0)
		, m_selectingSecond(false)
		, m_hoverBtn(TradeDisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(TradeDisplaySection::INVALID, invalidIndex)
        , m_fuzzySize(FuzzySize::SMALL)
        , m_toolTip(0) {
    CHECK_HEAP();


	this->world = &g_world;
    m_tradeCommands.resize(g_world.getTechTree()->getResourceTypeCount()*4);
    int init = 0;
    int inita[] = {100, 500, 1000, 2000};
    for (int i = 0; i < g_world.getTechTree()->getResourceTypeCount(); ++i) {
        const ResourceType *rt = g_world.getTechTree()->getResourceType(i);
        for (int j = 0; j < 4; ++j) {
        int value = inita[j];
        m_tradeCommands[init].init(value, rt, Clicks::TWO);
        ++init;
        }
    }

	setWidgetStyle(WidgetType::DISPLAY);
	selectingPos = false;
	TextWidget::setText("");
	for (int i = 0; i < tradeCellCount; ++i) {
		addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}
	layout();
	m_selectedTradeIndex = invalidIndex;
	clear();
    m_toolTip = new TradeTip(WidgetWindow::getInstance());
	m_toolTip->setVisible(false);

    CHECK_HEAP();
}

void TradeBar::setSize() {
	Vec2i sz = m_sizes.tradeSize;
	setVisible(true);
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<TradeBarFrame*>(m_parent)->resetSize();
}

void TradeBar::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastTradeBarSize(sz);
	cfg.setUiLastTradeBarPosX(pos.x);
	cfg.setUiLastTradeBarPosY(pos.y);
}

void TradeBar::reset() {
	Config &cfg = g_config;
	cfg.setUiLastTradeBarSize(2);
	cfg.setUiLastTradeBarPosX(-1);
	cfg.setUiLastTradeBarPosY(-1);
	if (getFuzzySize() == FuzzySize::SMALL) {
		static_cast<TradeBarFrame*>(m_parent)->onExpand(0);
	} else if (getFuzzySize() == FuzzySize::LARGE) {
		static_cast<TradeBarFrame*>(m_parent)->onShrink(0);
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() - 20 - m_parent->getWidth(), 20));
}

void TradeBar::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void TradeBar::render() {
	Widget::render();
	ImageWidget::startBatch();
	Vec4f light(1.f), dark(0.3f, 0.3f, 0.3f, 1.f);
	for (int i=0; i < tradeCellCount; ++i) {
		if (ImageWidget::getImage(i) && i != m_selectedTradeIndex) {
			ImageWidget::renderImage(i, downLighted[i] ? light : dark);
			int ndx = -1;
			if (ndx != -1) {
				ImageWidget::renderImage(ndx);
			}
		}
	}
	if (m_selectedTradeIndex != invalidIndex) {
		assert(ImageWidget::getImage(m_selectedTradeIndex));
		ImageWidget::renderImage(m_selectedTradeIndex, light);
	}
	ImageWidget::endBatch();
}

void TradeBar::computeTradePanel() {
    for (int i = 0, j = 0; i < m_tradeCommands.size(); ++i) {
        const TradeCommand tc = m_tradeCommands[i];
        setDownImage(j, tc.getResourceType()->getImage());
        setTradeCommand(j, tc);
        setDownLighted(j, tc.getResourceType()->getImage());
        ++j;
    }
}

void TradeBar::computeTradeTip(const TradeCommand *tc, const ResourceType *rt) {
	getTradeTip()->clearItems();
	tc->describe(m_faction, this->getTradeTip(), rt);
	//this->setToolTipText("Resource", "Value");

	string resource = tc->getResourceType()->getName();
	int value = tc->getTradeValue();
	stringstream ss;
	ss << value;
	string trade = ss.str();
	this->setToolTipText(resource, trade);

	resetTipPos();
}

void TradeBar::computeTradeInfo(int posTrade) {
	WIDGET_LOG( __FUNCTION__ << "( " << posTrade << " )");
    if (!m_selectingSecond) {
        //if (posTrade == cancelPos) {
            //this->setToolTipText("", g_lang.get("Cancel"));
        //} else {
            const TradeCommand *tc = &m_tradeCommands[posTrade];
            if (tc != 0) {
                const ResourceType *rt = tc->getResourceType();
                computeTradeTip(tc, rt);
                //this->setToolTipText("Resource", "Value");
            }
        //}
    } else {
        if (posTrade == cancelPos) {
            this->setToolTipText("", g_lang.get("Return"));
            return;
        }
	}
}

void TradeBar::setToolTipText(const string &hdr, const string &tip, TradeDisplaySection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
	Vec2i a_offset;
	if (i_section == TradeDisplaySection::TRADE) {
		a_offset = m_tradeOffset;
	}
	resetTipPos(a_offset);
}

void TradeBar::resetTipPos(Vec2i i_offset) {
	if (m_toolTip->isEmpty()) {
		m_toolTip->setVisible(false);
		return;
	}
	Vec2i ttPos = getScreenPos() + i_offset;
	if (ttPos.x > g_metrics.getScreenW() / 2) {
		ttPos.x -= (m_toolTip->getWidth() + getBorderLeft() + 3);
	} else {
		ttPos.x += (getWidth() - getBorderRight() + 3);
	}
	if (ttPos.y + m_toolTip->getHeight() > g_metrics.getScreenH()) {
		int offset = g_metrics.getScreenH() - (ttPos.y + m_toolTip->getHeight());
		ttPos.y += offset;
	}
	//ttPos.y -= (m_toolTip->getHeight() + 5);
	m_toolTip->setPos(ttPos);
	m_toolTip->setVisible(true);
}

void TradeBar::onFirstTierSelect(int posTrade) {
	WIDGET_LOG( __FUNCTION__ << "( " << posTrade << " )");
	if (posTrade == cancelPos) {
        resetState(false);
	} else {
        m_selectingSecond = true;
        const Faction *fac = g_world.getThisFaction();
        Faction *f = fac->getUnit(0)->getFaction();
        const TradeCommand tc = m_tradeCommands[posTrade];
        const ResourceType *rt = tc.getResourceType();
        int tradeAmount = tc.getTradeValue();
        const StoredResource *sr = f->getSResource(rt);
        int storedAmount = sr->getAmount();
        if (storedAmount>=tradeAmount) {
        f->incResourceAmount(rt, tradeAmount);
        }
    }
    computeTradePanel();
}

void TradeBar::onSecondTierSelect(int posTrade) {
	WIDGET_LOG( __FUNCTION__ << "( " << posTrade << " )");
	if (posTrade == cancelPos) {
		resetState(false);
	} else {
        const Faction *fac = g_world.getThisFaction();
        Faction *f = fac->getUnit(0)->getFaction();
        const TradeCommand tc = m_tradeCommands[posTrade];
        const ResourceType *rt = tc.getResourceType();
        int tradeAmount = tc.getTradeValue();
        const StoredResource *sr = f->getSResource(rt);
        int storedAmount = sr->getAmount();
        if (storedAmount>=tradeAmount) {
        f->incResourceAmount(rt, tradeAmount);
    }
	}
	computeTradePanel();
}

void TradeBar::tradeButtonPressed(int posTrade) {
	WIDGET_LOG( __FUNCTION__ << "( " << posTrade << " )");
    if (m_selectingSecond) {
        onSecondTierSelect(posTrade);
    } else {
        onFirstTierSelect(posTrade);
    }
    computeTradePanel();
    resetState();
    activePos = posTrade;
    computeTradeInfo(activePos);
    resetState();
}

void TradeBar::resetState(bool redoDisplay) {
	m_selectingSecond = false;
	selectingPos = false;
	activePos = -1;
	//dragging = false;
	//needSelectionUpdate = false;
	//g_program.getMouseCursor().setAppearance(MouseAppearance::DEFAULT);
	if (redoDisplay) {
		computeTradePanel();
	}
}

void TradeBar::layout() {
	int x = 0;
	int y = 0;

	const Font *font = getSmallFont();
	int fontIndex = m_textStyle.m_smallFontIndex != -1 ? m_textStyle.m_smallFontIndex : m_textStyle.m_fontIndex;

	if (m_fuzzySize== FuzzySize::SMALL) {
		m_imageSize = 32;
		font = getSmallFont();
		fontIndex = m_textStyle.m_smallFontIndex != -1 ? m_textStyle.m_smallFontIndex : m_textStyle.m_fontIndex;
	} else if (m_fuzzySize == FuzzySize::MEDIUM) {
		m_imageSize = 48;
		font = getFont();
		fontIndex = m_textStyle.m_fontIndex;
	} else if (m_fuzzySize == FuzzySize::LARGE) {
		m_imageSize = 64;
		font = getBigFont();
		fontIndex = m_textStyle.m_largeFontIndex != -1 ? m_textStyle.m_largeFontIndex : m_textStyle.m_fontIndex;
	}
	m_fontMetrics = font->getMetrics();

	x = 0;
	y = 0;
	m_tradeOffset = Vec2i(x, y);
	for (int i = 0; i < tradeCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}
    y += m_imageSize;
	m_sizes.tradeSize = Vec2i(x, y);
}

void TradeBar::clear() {
	for (int i=0; i < tradeCellCount; ++i) {
		downLighted[i]= true;
		ImageWidget::setImage(0, i);
	}
	setSelectedTradePos(invalidIndex);
}

void TradeBar::setSelectedTradePos(int i) {
	if (m_selectedTradeIndex == i) {
		return;
	}
	if (m_selectedTradeIndex != invalidIndex) {
		// shrink
		int ndx = m_selectedTradeIndex;
		Vec2i pos = getImagePos(ndx);
		Vec2i size = getImageSize(ndx);
		pos += Vec2i(3);
		size -= Vec2i(6);
		ImageWidget::setImageX(0, ndx, pos, size);
	}
	m_selectedTradeIndex = i;
	if (m_selectedTradeIndex != invalidIndex) {
		// enlarge
		int ndx = m_selectedTradeIndex;
		Vec2i pos = getImagePos(ndx);
		Vec2i size = getImageSize(ndx);
		pos -= Vec2i(3);
		size += Vec2i(6);
		ImageWidget::setImageX(0, ndx, pos, size);
	}
}

TradeDisplayButton TradeBar::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	Vec2i offsets[1] = { m_tradeOffset };
	int counts[1] = { tradeCellCount };

	for (int i=0; i < 1; ++i) {
		pos = i_pos - offsets[i];

		if (pos.y >= 0 && pos.y < m_imageSize * cellHeightCount) {
			int cellX = pos.x / m_imageSize;
			int cellY = (pos.y / m_imageSize) % cellHeightCount;
			int index = cellY * cellWidthCount + cellX;
			if (index >= 0 && index < counts[i]) {
				if (ImageWidget::getImage(i * cellHeightCount * cellWidthCount + index)) {
					return TradeDisplayButton(TradeDisplaySection(i), index);
				}
				return TradeDisplayButton(TradeDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return TradeDisplayButton(TradeDisplaySection::INVALID, invalidIndex);
}

bool TradeBar::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
			if (m_hoverBtn.m_section == TradeDisplaySection::TRADE) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else {
				m_pressedBtn = TradeDisplayButton(TradeDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return false;
}

bool TradeBar::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != TradeDisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
				if (m_hoverBtn == m_pressedBtn) {
					if (m_hoverBtn.m_section == TradeDisplaySection::TRADE) {
						tradeButtonPressed(m_hoverBtn.m_index);
					}
					m_pressedBtn = TradeDisplayButton(TradeDisplaySection::INVALID, invalidIndex);
					return true;
				}
			}
			m_pressedBtn = TradeDisplayButton(TradeDisplaySection::INVALID, invalidIndex);
		}
	}
	return false;
}

ostream& operator<<(ostream &stream, const TradeDisplayButton &btn) {
	return stream << "Section: " << btn.m_section << " index: " << btn.m_index;
}

bool TradeBar::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		TradeDisplayButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == TradeDisplaySection::TRADE) {
				computeTradeInfo(currBtn.m_index);
			} else {
				setToolTipText("Resource", "Value");
			}
			m_hoverBtn = currBtn;
			return true;
		} else {
		}
	} else {
		setToolTipText("", "");
	}
	return false;
}

void TradeBar::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = TradeDisplayButton(TradeDisplaySection::INVALID, invalidIndex);
	setToolTipText("", "");
    invalidateActivePos();
}

}}//end namespace

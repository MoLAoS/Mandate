// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "map_display.h"
#include "metrics.h"
#include "command_type.h"
#include "widget_window.h"
#include "core_data.h"
#include "user_interface.h"
#include "world.h"
#include "config.h"
#include "sim_interface.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest { namespace Gui_Mandate {

using Global::CoreData;

// =====================================================
//  class MapDisplayFrame
// =====================================================

MapDisplayFrame::MapDisplayFrame(UserInterface *ui, Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::SHRINK | ButtonFlags::EXPAND)
		, m_mapDisplay(0)
		, m_ui(ui) {
	m_ui = ui;
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);

	m_mapDisplay = new MapDisplay(this, ui, Vec2i(0,0));
	CellStrip::addCells(1);
	m_mapDisplay->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_mapDisplay->setAnchors(a);
	setPos(pos);

	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &MapDisplayFrame::onExpand);
	Shrink.connect(this, &MapDisplayFrame::onShrink);
	setPinned(g_config.getUiPinWidgets());
}

void MapDisplayFrame::resetSize() {
	if (m_mapDisplay->isVisible()) {
		if (!isVisible()) {
			setVisible(true);
		}
		Vec2i size = m_mapDisplay->getSize() + getBordersAll() + Vec2i(0, 20);
		if (size != getSize()){
			setSize(size);
		}
	} else {
		setVisible(false);
	}
}

void MapDisplayFrame::onExpand(Widget*) {
	assert(m_mapDisplay->getFuzzySize() != FuzzySize::LARGE);
	FuzzySize sz = m_mapDisplay->getFuzzySize();
	++sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_mapDisplay->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void MapDisplayFrame::onShrink(Widget*) {
	assert(m_mapDisplay->getFuzzySize() != FuzzySize::SMALL);
	FuzzySize sz = m_mapDisplay->getFuzzySize();
	--sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_mapDisplay->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void MapDisplayFrame::render() {
	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject() && g_config.getUiPhotoMode()) {
		return;
	}
	Frame::render();
}

void MapDisplayFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

// =====================================================
// 	class MapDisplay
// =====================================================

MapDisplay::MapDisplay(Container *parent, UserInterface *ui, Vec2i pos)
		: Widget(parent, pos, Vec2i(192, 500))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_mapBuilds(0)
		, m_logo(-1)
		, m_imageSize(32)
		, m_hoverBtn(MapDisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(MapDisplaySection::INVALID, invalidIndex)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);
	TextWidget::setAlignment(Alignment::NONE);
	for (int i = 0; i < commandCellCount; ++i) {
		ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}
	m_logo = 0;
	building = false;
	layout();
	m_selectedCommandIndex = invalidIndex;
	clear();
	m_toolTip = new CommandTip(WidgetWindow::getInstance());
	m_toolTip->setVisible(false);
	CHECK_HEAP();
}

void MapDisplay::init(Tileset *tileset) {
	m_tileset = tileset;
	m_mapBuilds.resize(10);
	for (int i = 0, j = 0; i < 10; ++i) {
        m_mapBuilds[j].init(m_tileset->getObjectType(i), Clicks::TWO);
        ++j;
	}
}

void MapDisplay::layout() {
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
	m_sizes.logoSize = Vec2i(m_imageSize * 6, m_imageSize * 6);
	int titleYpos = std::max((m_imageSize - int(m_fontMetrics->getHeight() + 1.f)) / 2, 0);

	x = 0;
	y = m_imageSize / 4 + int(m_fontMetrics->getHeight());
	m_commandOffset = Vec2i(x, y);
	for (int i = 0; i < commandCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}
	y += m_imageSize;
	m_sizes.commandSize = Vec2i(x, y);
	if (m_logo != invalidIndex) {
		ImageWidget::setImageX(0, m_logo, Vec2i(0, 0), Vec2i(m_imageSize * 6, m_imageSize * 6));
	}
}

void MapDisplay::computeBuildPanel() {
    for (int i = 0; i < getBuildCount(); ++i) {
        setDownImage(i, m_tileset->getObjectType(i)->getImage());
        setDownLighted(i, true);
    }
}

void MapDisplay::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastFactionDisplaySize(sz);
	cfg.setUiLastFactionDisplayPosX(pos.x);
	cfg.setUiLastFactionDisplayPosY(pos.y);
}

void MapDisplay::reset() {
	Config &cfg = g_config;
	cfg.setUiLastFactionDisplaySize(2);
	cfg.setUiLastFactionDisplayPosX(-1);
	cfg.setUiLastFactionDisplayPosY(-1);
	if (getFuzzySize() == FuzzySize::SMALL) {
		static_cast<MapDisplayFrame*>(m_parent)->onExpand(0);
	} else if (getFuzzySize() == FuzzySize::LARGE) {
		static_cast<MapDisplayFrame*>(m_parent)->onShrink(0);
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() - 20 - m_parent->getWidth(), 20));
}

void MapDisplay::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void MapDisplay::setSize() {
	Vec2i sz = m_sizes.commandSize;
	setVisible(true);
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<MapDisplayFrame*>(m_parent)->resetSize();
}

void MapDisplay::setSelectedCommandPos(int i) {
	if (m_selectedCommandIndex == i) {
		return;
	}
	if (m_selectedCommandIndex != invalidIndex) {
		// shrink
		int ndx = m_selectedCommandIndex;
		Vec2i pos = getImagePos(ndx);
		Vec2i size = getImageSize(ndx);
		pos += Vec2i(3);
		size -= Vec2i(6);
		ImageWidget::setImageX(0, ndx, pos, size);
	}
	m_selectedCommandIndex = i;
	if (m_selectedCommandIndex != invalidIndex) {
		assert(!m_ui->getSelection()->isEmpty());
		// enlarge
		int ndx = m_selectedCommandIndex;
		Vec2i pos = getImagePos(ndx);
		Vec2i size = getImageSize(ndx);
		pos -= Vec2i(3);
		size += Vec2i(6);
		ImageWidget::setImageX(0, ndx, pos, size);
	}
}

void MapDisplay::setToolTipText2(const string &hdr, const string &tip, MapDisplaySection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
	Vec2i a_offset;
    a_offset = m_commandOffset;
	resetTipPos(a_offset);
}

void MapDisplay::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	for (int i=0; i < commandCellCount; ++i) {
		downLighted[i]= true;
		ImageWidget::setImage(0, i);
	}
	setSelectedCommandPos(invalidIndex);
}

void MapDisplay::render() {
	if (!isVisible()) {
		return;
	}
    if (g_config.getUiPhotoMode()) {
        return;
    }
    if (m_logo == -1) {
        setSize();
        RUNTIME_CHECK( !isVisible() );
        return;
    }
	Widget::render();
	ImageWidget::startBatch();
	Vec4f light(1.f), dark(0.3f, 0.3f, 0.3f, 1.f);
	for (int i = 0; i < commandCellCount; ++i) {
		if (ImageWidget::getImage(i)) {
			ImageWidget::renderImage(i, light);
		}
	}
	for (int i=0; i < commandCellCount; ++i) {
		if (ImageWidget::getImage(i) && i != m_selectedCommandIndex) {
			ImageWidget::renderImage(i, downLighted[i] ? light : dark);
			int ndx = -1;
			if (ndx != -1) {
				ImageWidget::renderImage(ndx);
			}
		}
	}
	if (m_selectedCommandIndex != invalidIndex) {
		assert(ImageWidget::getImage(m_selectedCommandIndex));
		ImageWidget::renderImage(m_selectedCommandIndex, light);
	}
	ImageWidget::endBatch();
}

MapDisplayButton MapDisplay::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	Vec2i offsets[1] = { m_commandOffset };
	int counts[1] = { commandCellCount };

	for (int i=0; i < 1; ++i) {
		pos = i_pos - offsets[i];

		if (pos.y >= 0 && pos.y < m_imageSize * cellHeightCount) {
			int cellX = pos.x / m_imageSize;
			int cellY = (pos.y / m_imageSize) % cellHeightCount;
			int index = cellY * cellWidthCount + cellX;
			if (index >= 0 && index < counts[i]) {
				if (ImageWidget::getImage(i * cellHeightCount * cellWidthCount + index)) {
					return MapDisplayButton(MapDisplaySection(i), index);
				}
				return MapDisplayButton(MapDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return MapDisplayButton(MapDisplaySection::INVALID, invalidIndex);
}

void MapDisplay::computeBuildInfo(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
	if (posDisplay == m_ui->invalidPos) {
		setToolTipText2("", "");
		return;
	}
    MapBuild mb = getMapBuild(posDisplay);
    string header = "Build: " + mb.getMapObjectType()->getName();
    setToolTipText2(header, "");
}

void MapDisplay::onFirstTierSelect(int posBuild) {
	WIDGET_LOG( __FUNCTION__ << "( " << posBuild << " )");
	if (posBuild == m_ui->cancelPos) {
        m_ui->resetState(false);
	} else {
        MapBuild mb = m_mapBuilds[posBuild];
        m_ui->setActivePos(posBuild);
        g_program.getMouseCursor().setAppearance(MouseAppearance::CMD_ICON, mb.getMapObjectType()->getImage());
        currentMapBuild = mb;
	}
	if (building == false) {
        m_ui->computeDisplay();
    }
}

void MapDisplay::buildButtonPressed(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
    onFirstTierSelect(posDisplay);
    if (building == false) {
        m_ui->computeDisplay();
    }
    m_ui->setActivePos(posDisplay);
    building = true;
    computeBuildInfo(m_ui->getActivePos());
}

bool MapDisplay::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
			if (m_hoverBtn.m_section == MapDisplaySection::COMMANDS) {
				m_pressedBtn = m_hoverBtn;
				return true;
            }
            m_pressedBtn = MapDisplayButton(MapDisplaySection::INVALID, invalidIndex);
        }
    }
	return false;
}

bool MapDisplay::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != MapDisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
				if (m_hoverBtn == m_pressedBtn) {
					if (m_hoverBtn.m_section == MapDisplaySection::COMMANDS) {
						buildButtonPressed(m_hoverBtn.m_index);
					}
					m_pressedBtn = MapDisplayButton(MapDisplaySection::INVALID, invalidIndex);
					return true;
				}
			}
			m_pressedBtn = MapDisplayButton(MapDisplaySection::INVALID, invalidIndex);
		}
	}
	return false;
}

bool MapDisplay::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		m_hoverBtn = computeIndex(pos, true);
        return false;
	}
	return mouseDown(btn, pos);
}

void MapDisplay::resetTipPos(Vec2i i_offset) {
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

bool MapDisplay::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (Widget::isInsideBorders(pos)) {
		MapDisplayButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == MapDisplaySection::COMMANDS) {
				computeBuildInfo(currBtn.m_index);
			} else {
				setToolTipText2("", "");
			}
			m_hoverBtn = currBtn;
			return true;
		} else {
		}
	} else {
		setToolTipText2("", "");
	}
	return false;
}

void MapDisplay::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = MapDisplayButton(MapDisplaySection::INVALID, invalidIndex);
	setToolTipText2("", "");
	m_ui->invalidateActivePos();
}

}}//end namespace

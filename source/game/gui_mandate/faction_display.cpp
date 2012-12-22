// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "faction_display.h"
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
//  class FactionDisplayFrame
// =====================================================

FactionDisplayFrame::FactionDisplayFrame(UserInterface *ui, Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::SHRINK | ButtonFlags::EXPAND)
		, m_factionDisplay(0)
		, m_ui(ui) {
	m_ui = ui;
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);

	m_factionDisplay = new FactionDisplay(this, ui, Vec2i(0,0));
	CellStrip::addCells(1);
	m_factionDisplay->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_factionDisplay->setAnchors(a);
	setPos(pos);

	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &FactionDisplayFrame::onExpand);
	Shrink.connect(this, &FactionDisplayFrame::onShrink);
	setPinned(g_config.getUiPinWidgets());
}

void FactionDisplayFrame::resetSize() {
	if (m_factionDisplay->isVisible()) {
		if (!isVisible()) {
			setVisible(true);
		}
		Vec2i size = m_factionDisplay->getSize() + getBordersAll() + Vec2i(0, 20);
		if (size != getSize()){
			setSize(size);
		}
	} else {
		setVisible(false);
	}
}

void FactionDisplayFrame::onExpand(Widget*) {
	assert(m_factionDisplay->getFuzzySize() != FuzzySize::LARGE);
	FuzzySize sz = m_factionDisplay->getFuzzySize();
	++sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_factionDisplay->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void FactionDisplayFrame::onShrink(Widget*) {
	assert(m_factionDisplay->getFuzzySize() != FuzzySize::SMALL);
	FuzzySize sz = m_factionDisplay->getFuzzySize();
	--sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_factionDisplay->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void FactionDisplayFrame::render() {
	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject() && g_config.getUiPhotoMode()) {
		return;
	}
	Frame::render();
}

void FactionDisplayFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

// =====================================================
// 	class FactionDisplay
// =====================================================

FactionDisplay::FactionDisplay(Container *parent, UserInterface *ui, Vec2i pos)
		: Widget(parent, pos, Vec2i(192, 500))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_factionBuilds(0)
		, m_faction(0)
		, m_logo(-1)
		, m_imageSize(32)
		, m_hoverBtn(FactionDisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(FactionDisplaySection::INVALID, invalidIndex)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);
	TextWidget::setAlignment(Alignment::NONE);
	TextWidget::setAlignment(Alignment::NONE);
	TextWidget::setText("");
    TextWidget::addText("");
	for (int i = 0; i < commandCellCount; ++i) {
		ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}
	const Texture2D *logoTex = (g_world.getThisFaction()) ? g_world.getThisFaction()->getLogoTex() : 0;
	if (logoTex) {
		m_logo = ImageWidget::addImageX(logoTex, Vec2i(0, 0), Vec2i(192,192));
	}
	layout();
	m_selectedCommandIndex = invalidIndex;
	clear();
	m_toolTip = new CommandTip(WidgetWindow::getInstance());
	m_toolTip->setVisible(false);
	CHECK_HEAP();
}

void FactionDisplay::init(const Faction *faction, std::set<const UnitType*> &types) {
	m_faction = faction;

	foreach (std::set<const UnitType*>, it, types) {
		m_unitTypes.push_back(*it);
	}

    int buildResize = 0;
	for (int i = 0; i < getBuildingCount(); ++i) {
	    if (getBuilding(i)->hasTag("faction")) {
	        buildResize++;
	    }
	}
    m_factionBuilds.resize(buildResize);
	for (int i = 0, j = 0; i < getBuildingCount(); ++i) {
	    if (getBuilding(i)->hasTag("faction")) {
	        m_factionBuilds[j].init(getBuilding(i), Clicks::TWO);
	        ++j;
	    }
	}
}

void FactionDisplay::layout() {
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

	TextWidget::setTextPos(Vec2i(m_imageSize * 5 / 4, titleYpos), 0);
	TextWidget::setTextPos(Vec2i(m_imageSize * 5 / 4, titleYpos), 1);

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
	for (int i=0; i < 2; ++i) {
		setTextFont(fontIndex, i);
	}
	if (m_logo != invalidIndex) {
		ImageWidget::setImageX(0, m_logo, Vec2i(0, 0), Vec2i(m_imageSize * 6, m_imageSize * 6));
	}
}

void FactionDisplay::computeFactionCommandPanel() {

}

void FactionDisplay::computeBuildPanel() {
    for (int i = 0; i < getBuildCount(); ++i) {
        setDownImage(i, getFactionBuild(i).getUnitType()->getImage());
        setDownLighted(i, true);
    }
}

void FactionDisplay::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastFactionDisplaySize(sz);
	cfg.setUiLastFactionDisplayPosX(pos.x);
	cfg.setUiLastFactionDisplayPosY(pos.y);
}

void FactionDisplay::reset() {
	Config &cfg = g_config;
	cfg.setUiLastFactionDisplaySize(2);
	cfg.setUiLastFactionDisplayPosX(-1);
	cfg.setUiLastFactionDisplayPosY(-1);
	if (getFuzzySize() == FuzzySize::SMALL) {
		static_cast<FactionDisplayFrame*>(m_parent)->onExpand(0);
	} else if (getFuzzySize() == FuzzySize::LARGE) {
		static_cast<FactionDisplayFrame*>(m_parent)->onShrink(0);
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() - 20 - m_parent->getWidth(), 20));
}

void FactionDisplay::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void FactionDisplay::setSize() {
	Vec2i sz = m_sizes.commandSize;
	setVisible(true);
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<FactionDisplayFrame*>(m_parent)->resetSize();
}

void FactionDisplay::setSelectedCommandPos(int i) {
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

void FactionDisplay::setPortraitTitle(const string title) {
	if (TextWidget::getText(0).empty() && title.empty()) {
		return;
	}
	TextWidget::setText(title, 0);
}

void FactionDisplay::setPortraitText(const string &text) {
	if (TextWidget::getText(1).empty() && text.empty()) {
		return;
	}
	string str = formatString(text);

	int lines = 1;
	foreach_const (string, it, str) {
		if (*it == '\n') ++lines;
	}
	int yPos = m_imageSize * 5 / 4;
	TextWidget::setTextPos(Vec2i(5, yPos), 1);
	TextWidget::setText(str, 1);
}

void FactionDisplay::setToolTipText2(const string &hdr, const string &tip, FactionDisplaySection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
	Vec2i a_offset;
    a_offset = m_commandOffset;
	resetTipPos(a_offset);
}

void FactionDisplay::addToolTipReq(const DisplayableType *dt, bool ok, const string &txt) {
	m_toolTip->addReq(dt, ok, txt);
	resetTipPos();
}

// misc
void FactionDisplay::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );

	for (int i=0; i < commandCellCount; ++i) {
		downLighted[i]= true;
		commandTypes[i]= NULL;
		commandClasses[i]= CmdClass::NULL_COMMAND;
		ImageWidget::setImage(0, i);
	}

	setSelectedCommandPos(invalidIndex);
	setPortraitTitle("");
}

void FactionDisplay::render() {
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

    stringstream ss;
    int count = getBuildingCount();
    ss << "Faction: " << count;
    string faction = m_faction->getType()->getName();
    setPortraitTitle(faction);
	//TextWidget::setText("Faction", 1);

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
	if (!TextWidget::getText(0).empty()) {
		TextWidget::renderTextShadowed(0);
	}
}

FactionDisplayButton FactionDisplay::computeIndex(Vec2i i_pos, bool screenPos) {
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
					return FactionDisplayButton(FactionDisplaySection(i), index);
				}
				return FactionDisplayButton(FactionDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return FactionDisplayButton(FactionDisplaySection::INVALID, invalidIndex);
}

void FactionDisplay::computeBuildTip(FactionBuild fb) {
    getCommandTip()->clearItems();
	fb.describe(m_faction, getBuildTip(), fb.getUnitType());
	resetTipPos();
}

void FactionDisplay::computeBuildInfo(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
	if (posDisplay == m_ui->invalidPos) {
		setToolTipText2("", "");
		return;
	}
    FactionBuild fb = getFactionBuild(posDisplay);
    computeBuildTip(fb);
    string header = "Build: " + fb.getUnitType()->getName();
    setToolTipText2(header, "");

}

void FactionDisplay::onFirstTierSelect(int posBuild) {
	WIDGET_LOG( __FUNCTION__ << "( " << posBuild << " )");
	if (posBuild == m_ui->cancelPos) {
        m_ui->resetState(false);
	} else {
        FactionBuild fb = m_factionBuilds[posBuild];
		const ProducibleType *pt = fb.getUnitType();
        if (getFaction()->reqsOk(pt)) {
            m_ui->setActivePos(posBuild);
            g_program.getMouseCursor().setAppearance(MouseAppearance::CMD_ICON, fb.getUnitType()->getImage());
            currentFactionBuild = fb;
        }
	}
    m_ui->computeDisplay();
}

void FactionDisplay::buildButtonPressed(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
    onFirstTierSelect(posDisplay);
    m_ui->computeDisplay();
    m_ui->setActivePos(posDisplay);
    building = true;
    computeBuildInfo(m_ui->getActivePos());
}

bool FactionDisplay::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
			if (m_hoverBtn.m_section == FactionDisplaySection::COMMANDS) {
				m_pressedBtn = m_hoverBtn;
				return true;
            }
            m_pressedBtn = FactionDisplayButton(FactionDisplaySection::INVALID, invalidIndex);
        }
    }
	return false;
}

bool FactionDisplay::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != FactionDisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
				if (m_hoverBtn == m_pressedBtn) {
					if (m_hoverBtn.m_section == FactionDisplaySection::COMMANDS) {
						buildButtonPressed(m_hoverBtn.m_index);
					}
					m_pressedBtn = FactionDisplayButton(FactionDisplaySection::INVALID, invalidIndex);
					return true;
				}
			}
			m_pressedBtn = FactionDisplayButton(FactionDisplaySection::INVALID, invalidIndex);
		}
	}
	return false;
}

bool FactionDisplay::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		m_hoverBtn = computeIndex(pos, true);
        return false;
	}
	return mouseDown(btn, pos);
}

void FactionDisplay::resetTipPos(Vec2i i_offset) {
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

bool FactionDisplay::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (Widget::isInsideBorders(pos)) {
		FactionDisplayButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == FactionDisplaySection::COMMANDS) {
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

void FactionDisplay::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = FactionDisplayButton(FactionDisplaySection::INVALID, invalidIndex);
	setToolTipText2("", "");
	m_ui->invalidateActivePos();
}

}}//end namespace

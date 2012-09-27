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
#include "item_display.h"

#include "metrics.h"
#include "command_type.h"
#include "widget_window.h"
#include "core_data.h"
#include "user_interface.h"
#include "world.h"
#include "config.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest { namespace Gui {

using Global::CoreData;

// =====================================================
//  class ItemDisplayFrame
// =====================================================

ItemDisplayFrame::ItemDisplayFrame(UserInterface *ui, Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::SHRINK | ButtonFlags::EXPAND)
		, m_display(0)
		, m_ui(ui) {
	m_ui = ui;
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);

	m_display = new ItemWindow(this, ui, Vec2i(0,0));
	CellStrip::addCells(1);
	m_display->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_display->setAnchors(a);
	setPos(pos);

	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &ItemDisplayFrame::onExpand);
	Shrink.connect(this, &ItemDisplayFrame::onShrink);
	setPinned(g_config.getUiPinWidgets());
}

void ItemDisplayFrame::resetSize() {
	if (m_display->isVisible()) {
		if (!isVisible()) {
			setVisible(true);
		}
		Vec2i size = m_display->getSize() + getBordersAll() + Vec2i(0, 20);
		if (size != getSize()){
			setSize(size);
		}
	} else {
		setVisible(false);
	}
}

void ItemDisplayFrame::onExpand(Widget*) {
	assert(m_display->getFuzzySize() != FuzzySize::LARGE);
	FuzzySize sz = m_display->getFuzzySize();
	++sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_display->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void ItemDisplayFrame::onShrink(Widget*) {
	assert(m_display->getFuzzySize() != FuzzySize::SMALL);
	FuzzySize sz = m_display->getFuzzySize();
	--sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_display->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void ItemDisplayFrame::render() {
	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject() && g_config.getUiPhotoMode()) {
		return;
	}
	Frame::render();
}

void ItemDisplayFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

// =====================================================
// 	class ItemWindow
// =====================================================

ItemWindow::ItemWindow(Container *parent, UserInterface *ui, Vec2i pos)
		: Widget(parent, pos, Vec2i(192, 500))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_logo(-1)
		, m_imageSize(32)
		, m_hoverBtn(ItemDisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(ItemDisplaySection::INVALID, invalidIndex)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);
	for (int i = 0; i < selectionCellCount; ++i) {
		ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}
	TextWidget::setAlignment(Alignment::NONE);
	TextWidget::setText("");
	TextWidget::addText("");

	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");
    TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");

	for (int i = 0; i < buttonCellCount; ++i) {
		ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < 9; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	const Texture2D* overlayImages[3] = {
		g_widgetConfig.getTickTexture(),
		g_widgetConfig.getCrossTexture(),
		g_widgetConfig.getQuestionTexture()
	};
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

void ItemWindow::layout() {
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
	m_portraitOffset = Vec2i(x, y);
	for (int i = 0; i < selectionCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, i, Vec2i(x, y), Vec2i(m_imageSize));
		x += m_imageSize;
	}
	y += m_imageSize;
	m_sizes.portraitSize = Vec2i(x, y);

	int titleYpos = std::max((m_imageSize - int(m_fontMetrics->getHeight() + 1.f)) / 2, 0);
	TextWidget::setTextPos(Vec2i(m_imageSize * 5 / 4, titleYpos), 0);

	x = 0;
	y = m_imageSize + m_imageSize / 4 + int(m_fontMetrics->getHeight()) * 6;
	m_commandOffset = Vec2i(x, y);
	for (int i = 0; i < buttonCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, selectionCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}
	y += m_imageSize;
	m_sizes.commandSize = Vec2i(x, y+32+(m_imageSize*7));

    x = 40;
    y = 72;
    TextWidget::setTextPos(Vec2i(x, y), 2);
    ImageWidget::setImageX(0, selectionCellCount + buttonCellCount + 0, Vec2i(x-40,y), Vec2i(m_imageSize));
    y += m_imageSize;
    TextWidget::setTextPos(Vec2i(x, y), 3);
    ImageWidget::setImageX(0, selectionCellCount + buttonCellCount + 1, Vec2i(x-40,y), Vec2i(m_imageSize));
    y += m_imageSize;
    TextWidget::setTextPos(Vec2i(x, y), 4);
    ImageWidget::setImageX(0, selectionCellCount + buttonCellCount + 2, Vec2i(x-40,y), Vec2i(m_imageSize));
    y += m_imageSize;
    TextWidget::setTextPos(Vec2i(x, y), 5);
    ImageWidget::setImageX(0, selectionCellCount + buttonCellCount + 3, Vec2i(x-40,y), Vec2i(m_imageSize));
    y += m_imageSize;
    TextWidget::setTextPos(Vec2i(x, y), 6);
    ImageWidget::setImageX(0, selectionCellCount + buttonCellCount + 4, Vec2i(x-40,y), Vec2i(m_imageSize));
    y += m_imageSize;
    TextWidget::setTextPos(Vec2i(x, y), 7);
    ImageWidget::setImageX(0, selectionCellCount + buttonCellCount + 5, Vec2i(x-40,y), Vec2i(m_imageSize));
    y += m_imageSize;
    TextWidget::setTextPos(Vec2i(x, y), 8);
    ImageWidget::setImageX(0, selectionCellCount + buttonCellCount + 6, Vec2i(x-40,y), Vec2i(m_imageSize));
    y += m_imageSize;
    TextWidget::setTextPos(Vec2i(x, y), 9);
    ImageWidget::setImageX(0, selectionCellCount + buttonCellCount + 7, Vec2i(x-40,y), Vec2i(m_imageSize));

	for (int i=0; i < 10; ++i) {
		setTextFont(fontIndex, i);
	}

	if (m_logo != invalidIndex) {
		ImageWidget::setImageX(0, m_logo, Vec2i(0, 0), Vec2i(m_imageSize * 6, m_imageSize * 6));
	}
}

void ItemWindow::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastDisplaySize(sz);
	cfg.setUiLastDisplayPosX(pos.x);
	cfg.setUiLastDisplayPosY(pos.y);
}

void ItemWindow::reset() {
	Config &cfg = g_config;
	cfg.setUiLastDisplaySize(2);
	cfg.setUiLastDisplayPosX(-1);
	cfg.setUiLastDisplayPosY(-1);
	if (getFuzzySize() == FuzzySize::SMALL) {
		static_cast<DisplayFrame*>(m_parent)->onExpand(0);
	} else if (getFuzzySize() == FuzzySize::LARGE) {
		static_cast<DisplayFrame*>(m_parent)->onShrink(0);
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() - 20 - m_parent->getWidth(), 20));
}

void ItemWindow::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
	setPortraitText(getPortraitText());
}

void ItemWindow::setSize() {
	Vec2i sz = m_sizes.commandSize;
	if (m_ui->getSelection()->isEmpty()) {
		if (m_ui->getSelectedObject()) {
			sz = m_sizes.portraitSize;
		} else {
			if (m_logo != invalidIndex) {
				sz = m_sizes.logoSize;
			} else {
				setVisible(false);
				static_cast<ItemDisplayFrame*>(m_parent)->resetSize();
				return;
			}
		}
	} else {
		if (!m_ui->getSelection()->isComandable()) {
			sz = m_sizes.portraitSize;
		} else {
            sz = m_sizes.commandSize;
		}
	}
	setVisible(true);
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<ItemDisplayFrame*>(m_parent)->resetSize();
}

void ItemWindow::setSelectedCommandPos(int i) {
	if (m_selectedCommandIndex == i) {
		return;
	}
	if (m_selectedCommandIndex != invalidIndex) {
		// shrink
		int ndx = m_selectedCommandIndex + selectionCellCount;
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
		int ndx = m_selectedCommandIndex + selectionCellCount;
		Vec2i pos = getImagePos(ndx);
		Vec2i size = getImageSize(ndx);
		pos -= Vec2i(3);
		size += Vec2i(6);
		ImageWidget::setImageX(0, ndx, pos, size);
	}
}

void ItemWindow::setPortraitTitle(const string title) {
	if (TextWidget::getText(0).empty() && title.empty()) {
		return;
	}
	TextWidget::setText(title, 0);
}

void ItemWindow::setPortraitText(const string &text) {
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

/*void ItemWindow::setHelmetLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("Helmet") : ""), 2);
}
void ItemWindow::setPauldronsLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("Pauldrons") : ""), 3);
}
void ItemWindow::setCuirassLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("Cuirass") : ""), 4);
}
void ItemWindow::setGauntletsLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("Gauntlets") : ""), 5);
}
void ItemWindow::setRightHandLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("RightHand") : ""), 6);
}
void ItemWindow::setLeftHandLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("LeftHand") : ""), 7);
}
void ItemWindow::setGreavesLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("Greaves") : ""), 8);
}
void ItemWindow::setBootsLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("Boots") : ""), 9);
}*/

void ItemWindow::setHelmetLabel(bool v) {
	TextWidget::setText("Helmet", 2);
}
void ItemWindow::setPauldronsLabel(bool v) {
	TextWidget::setText("Pauldrons", 3);
}
void ItemWindow::setCuirassLabel(bool v) {
	TextWidget::setText("Cuirass", 4);
}
void ItemWindow::setGauntletsLabel(bool v) {
	TextWidget::setText("Gauntlets", 5);
}
void ItemWindow::setRightHandLabel(bool v) {
	TextWidget::setText("RightHand", 6);
}
void ItemWindow::setLeftHandLabel(bool v) {
	TextWidget::setText("LeftHand", 7);
}
void ItemWindow::setGreavesLabel(bool v) {
	TextWidget::setText("Greaves", 8);
}
void ItemWindow::setBootsLabel(bool v) {
	TextWidget::setText("Boots", 9);
}

void ItemWindow::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	for (int i=0; i < selectionCellCount; ++i) {
		ImageWidget::setImage(0, i);
	}

	for (int i=0; i < buttonCellCount; ++i) {
		downLighted[i]= true;
		ImageWidget::setImage(0, selectionCellCount + i);
	}
	setSelectedCommandPos(invalidIndex);
	setPortraitTitle("");
	setPortraitText("");
}

void ItemWindow::render() {
	if (!isVisible()) {
		return;
	}

	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject()) {
		if (g_config.getUiPhotoMode()) {
			return;
		}
		if (m_logo == -1) {
			setSize();
			RUNTIME_CHECK( !isVisible() );
			return;
		}
	}

	Widget::render();

	ImageWidget::startBatch();
	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject() && m_logo != -1) {
		//ImageWidget::renderImage(m_logo);
	}

	Vec4f light(1.f), dark(0.3f, 0.3f, 0.3f, 1.f);
	for (int i = 0; i < selectionCellCount; ++i) {
		if (ImageWidget::getImage(i)) {
			ImageWidget::renderImage(i, light);
		}
	}
	for (int i=0; i < buttonCellCount; ++i) {
		if (ImageWidget::getImage(i + selectionCellCount) && i != m_selectedCommandIndex) {
			ImageWidget::renderImage(i + selectionCellCount, downLighted[i] ? light : dark);
			int ndx = -1;
			if (ndx != -1) {
				ImageWidget::renderImage(ndx);
			}
		}
	}
	if (m_selectedCommandIndex != invalidIndex) {
		assert(ImageWidget::getImage(m_selectedCommandIndex + selectionCellCount));
		ImageWidget::renderImage(m_selectedCommandIndex + selectionCellCount, light);
	}

	for (int i=0; i < 8; ++i) {
		if (ImageWidget::getImage(i + selectionCellCount + buttonCellCount) && i != m_selectedCommandIndex) {
			ImageWidget::renderImage(i + selectionCellCount + buttonCellCount, downLighted[i] ? light : dark);
			int ndx = -1;
			if (ndx != -1) {
				ImageWidget::renderImage(ndx);
			}
		}
	}

	ImageWidget::endBatch();
	if (!TextWidget::getText(0).empty()) {
		TextWidget::renderTextShadowed(0);
	}
	if (!TextWidget::getText(1).empty()) {
		TextWidget::renderTextShadowed(1);
	}
	if (!TextWidget::getText(2).empty()) {
		TextWidget::renderTextShadowed(2);
	}
	if (!TextWidget::getText(3).empty()) {
		TextWidget::renderTextShadowed(3);
	}
	if (!TextWidget::getText(4).empty()) {
		TextWidget::renderTextShadowed(4);
	}
	if (!TextWidget::getText(5).empty()) {
		TextWidget::renderTextShadowed(5);
	}
	if (!TextWidget::getText(6).empty()) {
		TextWidget::renderTextShadowed(6);
	}
	if (!TextWidget::getText(7).empty()) {
		TextWidget::renderTextShadowed(7);
	}
	if (!TextWidget::getText(8).empty()) {
		TextWidget::renderTextShadowed(8);
	}
	if (!TextWidget::getText(9).empty()) {
		TextWidget::renderTextShadowed(9);
	}
}

void ItemWindow::computeEquipmentPanel() {
    if (m_ui->getSelection()->isComandable()) {
        setHelmetLabel(true);
        setPauldronsLabel(true);
        setCuirassLabel(true);
        setGauntletsLabel(true);
        setRightHandLabel(true);
        setLeftHandLabel(true);
        setGreavesLabel(true);
        setBootsLabel(true);
    }
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        const FactionType *ft = u->getFaction()->getType();
        if (u->isBuilt()) {
            for (int i = 0; i < ft->getItemImagesCount(); ++i) {
                setDownImage(i + selectionCellCount + buttonCellCount, ft->getItemImage(i));
                setDownLighted(i, true);
            }
        }
    }
}

void ItemWindow::computeButtonsPanel() {
    /*if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        const FactionType *ft = u->getFaction()->getType();
        if (u->isBuilt()) {
            for (int i = 0; i < ft->getItemImagesCount(); ++i) {
                setDownImage(i, ft->getItemImage(i));
                setDownLighted(i, true);
            }
        }
    }*/
}

void ItemWindow::computeSelectionPanel() {
	if (m_ui->getSelection()->getCount() == 1) {
		const Unit *unit = m_ui->getSelection()->getFrontUnit();
		string name = unit->getFullName();
		name = g_lang.getTranslatedFactionName(unit->getFaction()->getType()->getName(), name);
		IF_DEBUG_EDITION(
			name += ": " + intToStr(unit->getId());
		)
		setPortraitTitle(name);
		setUpImage(0, m_ui->getSelection()->getUnit(0)->getType()->getImage());
	}
}

ItemDisplayButton ItemWindow::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	Vec2i offsets[2] = { m_portraitOffset, m_commandOffset };
	int counts[2] = { selectionCellCount, buttonCellCount };

	for (int i=0; i < 5; ++i) {
		pos = i_pos - offsets[i];

		if (pos.y >= 0 && pos.y < m_imageSize * cellHeightCount) {
			int cellX = pos.x / m_imageSize;
			int cellY = (pos.y / m_imageSize) % cellHeightCount;
			int index = cellY * cellWidthCount + cellX;
			if (index >= 0 && index < counts[i]) {
				if (ImageWidget::getImage(i * cellHeightCount * cellWidthCount + index)) {
					return ItemDisplayButton(ItemDisplaySection(i), index);
				}
				return ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
}

bool ItemWindow::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
			if (m_hoverBtn.m_section == ItemDisplaySection::BUTTONS) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else if (m_hoverBtn.m_section == ItemDisplaySection::SELECTION) {
				RUNTIME_CHECK(m_ui->getSelection()->getCount() > m_hoverBtn.m_index);
				const Unit *unit = m_ui->getSelection()->getUnit(m_hoverBtn.m_index);
				RUNTIME_CHECK(unit != 0);
				if (m_ui->getInput().isCtrlDown()) {
					if (m_ui->getInput().isShiftDown()) {
						m_ui->getSelection()->unSelectAllOfType(unit->getType());
					} else {
						m_ui->getSelection()->unSelect(unit);
					}
				} else if (m_ui->getInput().isShiftDown()) {
					m_ui->getSelection()->unSelectAllNotOfType(unit->getType());
				} else {
					m_ui->getSelection()->clear();
					m_ui->getSelection()->select(const_cast<Unit*>(unit));
				}
				m_ui->computeDisplay();
				m_ui->computePortraitInfo(m_hoverBtn.m_index);
				m_pressedBtn = m_hoverBtn = ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
				return true;
			} else {
				m_pressedBtn = ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return false;
}

bool ItemWindow::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != ItemDisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
				if (m_hoverBtn == m_pressedBtn) {
					if (m_hoverBtn.m_section == ItemDisplaySection::BUTTONS) {
						m_ui->commandButtonPressed(m_hoverBtn.m_index);
					}
					m_pressedBtn = ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
					return true;
				}
			}
			m_pressedBtn = ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
		}
	}
	return false;
}

bool ItemWindow::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (Widget::isInsideBorders(pos)) {
		m_hoverBtn = computeIndex(pos, true);
        return false;
	}
	return mouseDown(btn, pos);
}

void ItemWindow::resetTipPos(Vec2i i_offset) {
	if (m_toolTip->isEmpty()) {
		m_toolTip->setVisible(false);
		return;
	}
	Vec2i ctPos = getScreenPos() + i_offset;
	if (ctPos.x > g_metrics.getScreenW() / 2) {
		ctPos.x -= (m_toolTip->getWidth() + getBorderLeft() + 3);
	} else {
		ctPos.x += (getWidth() - getBorderRight() + 3);
	}
	if (ctPos.y + m_toolTip->getHeight() > g_metrics.getScreenH()) {
		int offset = g_metrics.getScreenH() - (ctPos.y + m_toolTip->getHeight());
		ctPos.y += offset;
	}
	m_toolTip->setPos(ctPos);
	m_toolTip->setVisible(true);
}

ostream& operator<<(ostream &stream, const ItemDisplayButton &btn) {
	return stream << "Section: " << btn.m_section << " index: " << btn.m_index;
}

bool ItemWindow::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		ItemDisplayButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == ItemDisplaySection::SELECTION) {
				m_ui->computePortraitInfo(currBtn.m_index);
			} else {

			}
			m_hoverBtn = currBtn;
			return true;
		} else {
		}
	} else {

	}
	return false;
}

void ItemWindow::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);

	m_ui->invalidateActivePos();
}

}}

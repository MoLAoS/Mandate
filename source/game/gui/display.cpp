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
#include "display.h"

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
//  class DisplayFrame
// =====================================================

DisplayFrame::DisplayFrame(UserInterface *ui, Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::SHRINK | ButtonFlags::EXPAND)
		, m_display(0)
		, m_ui(ui) {
	m_ui = ui;
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);

	m_display = new Display(this, ui, Vec2i(0,0));
	CellStrip::addCells(1);
	m_display->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_display->setAnchors(a);
	setPos(pos);

	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &DisplayFrame::onExpand);
	Shrink.connect(this, &DisplayFrame::onShrink);
	setPinned(g_config.getUiPinWidgets());
}

void DisplayFrame::resetSize() {
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

void DisplayFrame::onExpand(Widget*) {
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

void DisplayFrame::onShrink(Widget*) {
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

void DisplayFrame::render() {
	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject() && g_config.getUiPhotoMode()) {
		return;
	}
	Frame::render();
}

void DisplayFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

// =====================================================
// 	class Display
// =====================================================

Display::Display(Container *parent, UserInterface *ui, Vec2i pos)
		: Widget(parent, pos, Vec2i(192, 500))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_logo(-1)
		, m_imageSize(32)
		, m_hoverBtn(DisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(DisplaySection::INVALID, invalidIndex)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);

	for (int i = 0; i < selectionCellCount; ++i) { // selection potraits
		ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}
	TextWidget::setAlignment(Alignment::NONE);
	TextWidget::setText(""); // (0) unit title
	TextWidget::addText(""); // (1) unit text
	TextWidget::addText(""); // (2) queued orders text (to display below progress bar if present)
	TextWidget::addText(""); // (3) progress bar
	for (int i = 0; i < commandCellCount; ++i) { // command buttons
		ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}
	TextWidget::addText(""); // (4) 'Transported' label
	for (int i = 0; i < transportCellCount; ++i) { // loaded unit portraits
		ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}
	const Texture2D* overlayImages[3] = {
		g_widgetConfig.getTickTexture(),
		g_widgetConfig.getCrossTexture(),
		g_widgetConfig.getQuestionTexture()
	};
	m_autoRepairOn = addImageX(overlayImages[0], Vec2i(0), Vec2i(m_imageSize));
	m_autoRepairOff = addImageX(overlayImages[1], Vec2i(0), Vec2i(m_imageSize));
	m_autoRepairMixed = addImageX(overlayImages[2], Vec2i(0), Vec2i(m_imageSize));

	m_autoAttackOn = addImageX(overlayImages[0], Vec2i(0), Vec2i(m_imageSize));
	m_autoAttackOff = addImageX(overlayImages[1], Vec2i(0), Vec2i(m_imageSize));
	m_autoAttackMixed = addImageX(overlayImages[2], Vec2i(0), Vec2i(m_imageSize));

	m_autoFleeOn =  addImageX(overlayImages[0], Vec2i(0), Vec2i(m_imageSize));
	m_autoFleeOff = addImageX(overlayImages[1], Vec2i(0), Vec2i(m_imageSize));
	m_autoFleeMixed = addImageX(overlayImages[2], Vec2i(0), Vec2i(m_imageSize));

	// -loadmap doesn't have any faction
	const Texture2D *logoTex = (g_world.getThisFaction()) ? g_world.getThisFaction()->getLogoTex() : 0;
	if (logoTex) {
		m_logo = ImageWidget::addImageX(logoTex, Vec2i(0, 0), Vec2i(192,192));
	}
	layout();

	m_selectedCommandIndex = invalidIndex;
	setProgressBar(-1);
	clear();
	//setSize();

	m_toolTip = new CommandTip(WidgetWindow::getInstance());
	m_toolTip->setVisible(false);

	CHECK_HEAP();
}

void Display::layout() {
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
	for (int i = 0; i < selectionCellCount; ++i) { // selection potraits
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
	TextWidget::setTextPos(Vec2i(m_imageSize * 5 / 4, titleYpos), 0); // (0) unit title

	Vec2i arPos, aaPos, afPos;
	x = 0;
	y = m_imageSize + m_imageSize / 4 + int(m_fontMetrics->getHeight()) * 6;
	m_commandOffset = Vec2i(x, y);
	for (int i = 0; i < commandCellCount; ++i) { // command buttons
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, selectionCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		if (i == UserInterface::autoRepairPos) {
			arPos = Vec2i(x, y);
		} else if (i == UserInterface::autoAttackPos) {
			aaPos = Vec2i(x, y);
		} else if (i == UserInterface::autoFleePos) {
			afPos = Vec2i(x, y);
		}
		x += m_imageSize;
	}
	y += m_imageSize;
	m_sizes.commandSize = Vec2i(x, y);

	x = 0;
	y += m_imageSize * 3 / 2;
	TextWidget::setTextPos(Vec2i(x, y), 4); // (4) 'Transported' label
	y += int(m_fontMetrics->getHeight() + 1.f);
	m_carryImageOffset = Vec2i(x, y);
	for (int i = 0; i < transportCellCount; ++i) { // loaded unit portraits
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, selectionCellCount + commandCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}
	y += m_imageSize;
	m_sizes.transportSize = Vec2i(x, y);

	for (int i=0; i < 5; ++i) {
		setTextFont(fontIndex, i);
	}

	setImageX(0, m_autoRepairOn, arPos, Vec2i(m_imageSize));
	setImageX(0, m_autoRepairOff, arPos, Vec2i(m_imageSize));
	setImageX(0, m_autoRepairMixed, arPos, Vec2i(m_imageSize));

	setImageX(0, m_autoAttackOn, aaPos, Vec2i(m_imageSize));
	setImageX(0, m_autoAttackOff, aaPos, Vec2i(m_imageSize));
	setImageX(0, m_autoAttackMixed, aaPos, Vec2i(m_imageSize));

	setImageX(0, m_autoFleeOn, afPos, Vec2i(m_imageSize));
	setImageX(0, m_autoFleeOff, afPos, Vec2i(m_imageSize));
	setImageX(0, m_autoFleeMixed, afPos, Vec2i(m_imageSize));

	if (m_logo != invalidIndex) {
		ImageWidget::setImageX(0, m_logo, Vec2i(0, 0), Vec2i(m_imageSize * 6, m_imageSize * 6));
	}
}

void Display::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastDisplaySize(sz);
	cfg.setUiLastDisplayPosX(pos.x);
	cfg.setUiLastDisplayPosY(pos.y);
}

void Display::reset() {
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

void Display::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
	setPortraitText(getPortraitText());
	setOrderQueueText(getOrderQueueText());
}

void Display::setSize() {
	Vec2i sz = m_sizes.commandSize;
	if (m_ui->getSelection()->isEmpty()) {
		if (m_ui->getSelectedObject()) {
			sz = m_sizes.portraitSize;
		} else {
			if (m_logo != invalidIndex) {
				sz = m_sizes.logoSize;
			} else {
				setVisible(false);
				static_cast<DisplayFrame*>(m_parent)->resetSize();
				return;
			}
		}
	} else {
		if (!m_ui->getSelection()->isComandable()) {
			sz = m_sizes.portraitSize;
		} else {
			if (m_ui->getSelection()->hasTransported()) {
				sz = m_sizes.transportSize;
			}
		}
	} 
	setVisible(true);
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<DisplayFrame*>(m_parent)->resetSize(); ///@todo construct with DisplayFrame as parent
}

void Display::setProgressBar(int i) {
	m_progress = i;
	if (i >= 0) {
		TextWidget::setText(intToStr(i) + "%", 3);
		Vec2i sz = getTextDimensions(3);
		m_progPrecentPos = (getWidth() - 2 * m_imageSize) / 2 - (sz.x / 2);
	} else {
		TextWidget::setText("", 3);
	}
}

void Display::setSelectedCommandPos(int i) {
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

void Display::setPortraitTitle(const string title) {
	if (TextWidget::getText(0).empty() && title.empty()) {
		return;
	}
	TextWidget::setText(title, 0);
}

void Display::setPortraitText(const string &text) {
	//WIDGET_LOG( __FUNCTION__ << "( \"" << text << "\" )" );
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

	m_progressPos = Vec2i(m_imageSize, yPos + lines * int(m_fontMetrics->getHeight() + 1.f));
}

void Display::setOrderQueueText(const string &i_text) {
	if (TextWidget::getText(2).empty() && i_text.empty()) {
		return;
	}
	int y = m_progressPos.y;
	if (m_progress != -1) {
		y += int(m_fontMetrics->getHeight()) + 3;
	}
	TextWidget::setTextPos(Vec2i(5, y), 2);
	TextWidget::setText(i_text, 2);
}

void Display::setLoadInfo(const string &str) {
	if (TextWidget::getText(4).empty() && str.empty()) {
		return;
	}
	TextWidget::setText(str, 4);
}

void Display::setToolTipText2(const string &hdr, const string &tip, DisplaySection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
	Vec2i a_offset;
	if (i_section == DisplaySection::SELECTION) {
		a_offset = m_portraitOffset;
	} else if (i_section == DisplaySection::TRANSPORTED) {
		a_offset = m_carryImageOffset;
	} else {
		a_offset = m_commandOffset;
	}
	resetTipPos(a_offset);
}

void Display::addToolTipReq(const DisplayableType *dt, bool ok, const string &txt) {
	m_toolTip->addReq(dt, ok, txt);
	resetTipPos();
}

void Display::setTransportedLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("Transported") : ""), 4);
}

// misc
void Display::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	for (int i=0; i < selectionCellCount; ++i) {
		ImageWidget::setImage(0, i);
	}

	for (int i=0; i < commandCellCount; ++i) {
		downLighted[i]= true;
		commandTypes[i]= NULL;
		commandClasses[i]= CmdClass::NULL_COMMAND;
		ImageWidget::setImage(0, selectionCellCount + i);
	}

	for (int i=0; i < transportCellCount; ++i) {
		ImageWidget::setImage(0, selectionCellCount + commandCellCount + i);
	}

	setSelectedCommandPos(invalidIndex);
	setPortraitTitle("");
	setPortraitText("");
	setProgressBar(-1);
	setOrderQueueText("");
	setLoadInfo("");
}

const Vec3f progressBarBg = Vec3f(0.3f);
const Vec3f progressBarFg1 = Vec3f(0.f, 0.5f, 0.f);
const Vec3f progressBarFg2 = Vec3f(0.f, 0.1f, 0.f);

void Display::renderProgressBar() {
	const int h = int(m_fontMetrics->getHeight() + 2);
	int w = getWidth() - 2 * m_imageSize;

	int bw = m_progress;
	Vec2i pos = getScreenPos() + m_progressPos;

	// bar (green bit)
	glBegin(GL_QUADS);
		glColor3fv(progressBarFg2.ptr());
		glVertex2i(pos.x, pos.y); // bottom left
		glVertex2i(pos.x, pos.y + h); // top left
		glColor3fv(progressBarFg1.ptr());
		glVertex2i(pos.x + bw, pos.y + h); // top right
		glVertex2i(pos.x + bw, pos.y); // bottom right
	glEnd();

	// transp bar
	glBegin(GL_QUADS);
		glColor3fv(progressBarBg.ptr());
		glVertex2i(pos.x + bw, pos.y); // bottom left
		glVertex2i(pos.x + bw, pos.y + h); // top left
		glVertex2i(pos.x + w, pos.y + h); // top right
		glVertex2i(pos.x + w, pos.y); // bottom right
	glEnd();

	// text
	setTextPos(m_progressPos + Vec2i(m_progPrecentPos, 0), 3);
	renderText(3);
}

int Display::getImageOverlayIndex(AutoCmdFlag f, AutoCmdState s) {
	if (s == AutoCmdState::INVALID || s == AutoCmdState::NONE) {
		return -1;
	}
	///@todo put in array...
	switch (f) {
		case AutoCmdFlag::REPAIR:
			if (s == AutoCmdState::ALL_ON) {
				return m_autoRepairOn;
			} else if (s == AutoCmdState::ALL_OFF) {
				return m_autoRepairOff;
			} else if (s == AutoCmdState::MIXED) {
				return m_autoRepairMixed;
			} else {
				assert(false);
				return -1;
			}
		case AutoCmdFlag::ATTACK:
			if (s == AutoCmdState::ALL_ON) {
				return m_autoAttackOn;
			} else if (s == AutoCmdState::ALL_OFF) {
				return m_autoAttackOff;
			} else if (s == AutoCmdState::MIXED) {
				return m_autoAttackMixed;
			} else {
				assert(false);
				return -1;
			}

		case AutoCmdFlag::FLEE:
			if (s == AutoCmdState::ALL_ON) {
				return m_autoFleeOn;
			} else if (s == AutoCmdState::ALL_OFF) {
				return m_autoFleeOff;
			} else if (s == AutoCmdState::MIXED) {
				return m_autoFleeMixed;
			} else {
				assert(false);
				return -1;
			}
	}
	return -1;
}

void Display::render() {
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
		// faction logo
		ImageWidget::renderImage(m_logo);
	}

	Vec4f light(1.f), dark(0.3f, 0.3f, 0.3f, 1.f);
	for (int i = 0; i < selectionCellCount; ++i) {
		if (ImageWidget::getImage(i)) {
			ImageWidget::renderImage(i, light);
		}
	}
	for (int i=0; i < commandCellCount; ++i) {
		if (ImageWidget::getImage(i + selectionCellCount) && i != m_selectedCommandIndex) {
			ImageWidget::renderImage(i + selectionCellCount, downLighted[i] ? light : dark);
			int ndx = -1;
			if (i == UserInterface::autoRepairPos) {
				AutoCmdState state = m_ui->getSelection()->getAutoCmdState(AutoCmdFlag::REPAIR);
				ndx = getImageOverlayIndex(AutoCmdFlag::REPAIR, state);
			} else if (i == UserInterface::autoAttackPos) {
				AutoCmdState state = m_ui->getSelection()->getAutoCmdState(AutoCmdFlag::ATTACK);
				ndx = getImageOverlayIndex(AutoCmdFlag::ATTACK, state);
			} else if (i == UserInterface::autoFleePos) {
				AutoCmdState state = m_ui->getSelection()->getAutoCmdState(AutoCmdFlag::FLEE);
				ndx = getImageOverlayIndex(AutoCmdFlag::FLEE, state);
			}
			if (ndx != -1) {
				ImageWidget::renderImage(ndx);
			}
		}
	}
	if (m_selectedCommandIndex != invalidIndex) {
		assert(ImageWidget::getImage(m_selectedCommandIndex + selectionCellCount));
		ImageWidget::renderImage(m_selectedCommandIndex + selectionCellCount, light);
	}
	for (int i=0; i < transportCellCount; ++i) {
		if (ImageWidget::getImage(i + selectionCellCount + commandCellCount)) {
			ImageWidget::renderImage(i + selectionCellCount + commandCellCount, light);
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
	if (!TextWidget::getText(4).empty()) {
		TextWidget::renderTextShadowed(4);
	}
	if (m_progress >= 0) {
		renderProgressBar();
	} else if (!TextWidget::getText(3).empty()) {
		TextWidget::renderTextShadowed(3);
	}
}

DisplayButton Display::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	Vec2i offsets[3] = { m_portraitOffset, m_commandOffset, m_carryImageOffset };
	int counts[3] = { selectionCellCount, commandCellCount, transportCellCount };
	
	for (int i=0; i < 3; ++i) {	
		pos = i_pos - offsets[i];

		if (pos.y >= 0 && pos.y < m_imageSize * cellHeightCount) {
			int cellX = pos.x / m_imageSize;
			int cellY = (pos.y / m_imageSize) % cellHeightCount;
			int index = cellY * cellWidthCount + cellX;
			if (index >= 0 && index < counts[i]) {
				if (ImageWidget::getImage(i * cellHeightCount * cellWidthCount + index)) {
					return DisplayButton(DisplaySection(i), index);
				}
				return DisplayButton(DisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return DisplayButton(DisplaySection::INVALID, invalidIndex);
}

bool Display::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
			if (m_hoverBtn.m_section == DisplaySection::COMMANDS) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else if (m_hoverBtn.m_section == DisplaySection::TRANSPORTED) {
				return true;
			} else if (m_hoverBtn.m_section == DisplaySection::SELECTION) {
				RUNTIME_CHECK(m_ui->getSelection()->getCount() > m_hoverBtn.m_index);
				const Unit *unit = m_ui->getSelection()->getUnit(m_hoverBtn.m_index);
				RUNTIME_CHECK(unit != 0);
				if (m_ui->getInput().isCtrlDown()) {
					if (m_ui->getInput().isShiftDown()) {
						// remove all of ndx's type from selection
						m_ui->getSelection()->unSelectAllOfType(unit->getType());
					} else {
						// remove ndx from selection
						m_ui->getSelection()->unSelect(unit);
					}
				} else if (m_ui->getInput().isShiftDown()) {
					// select all of ndx's type in current selection
					m_ui->getSelection()->unSelectAllNotOfType(unit->getType());
				} else {
					m_ui->getSelection()->clear();
					m_ui->getSelection()->select(const_cast<Unit*>(unit));
				}
				m_ui->computeDisplay();
				m_ui->computePortraitInfo(m_hoverBtn.m_index);
				m_pressedBtn = m_hoverBtn = DisplayButton(DisplaySection::INVALID, invalidIndex);
				return true;
			} else {
				m_pressedBtn = DisplayButton(DisplaySection::INVALID, invalidIndex);
			}
		}
	}
	// nothing 'tangible' clicked, let event through
	return false;
}

bool Display::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != DisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
				if (m_hoverBtn == m_pressedBtn) {
					if (m_hoverBtn.m_section == DisplaySection::COMMANDS) {
						m_ui->commandButtonPressed(m_hoverBtn.m_index);
					}
					m_pressedBtn = DisplayButton(DisplaySection::INVALID, invalidIndex);
					return true;
				}
			}
			m_pressedBtn = DisplayButton(DisplaySection::INVALID, invalidIndex);
		}
	}
	return false;
}

bool Display::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (Widget::isInsideBorders(pos)) {
		m_hoverBtn = computeIndex(pos, true);
		if (m_hoverBtn.m_section == DisplaySection::TRANSPORTED) {
			m_ui->unloadRequest(m_hoverBtn.m_index);
			m_hoverBtn = DisplayButton(DisplaySection::INVALID, invalidIndex);
			return true;
		}
	}
	return mouseDown(btn, pos);
}

void Display::resetTipPos(Vec2i i_offset) {
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

ostream& operator<<(ostream &stream, const DisplayButton &btn) {
	return stream << "Section: " << btn.m_section << " index: " << btn.m_index;
}

bool Display::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (Widget::isInsideBorders(pos)) {
		DisplayButton currBtn = computeIndex(pos, true);
		//Vec2i lPos = pos - getScreenPos();
		//stringstream ss;
		//ss << "local pos " << lPos << " index = " << currBtn.m_index;
		//g_console.addLine(ss.str());
		if (currBtn != m_hoverBtn) {
			// change stuff
			if (currBtn.m_section == DisplaySection::SELECTION) {
				m_ui->computePortraitInfo(currBtn.m_index);
			} else if (currBtn.m_section == DisplaySection::COMMANDS) {
				m_ui->computeCommandInfo(currBtn.m_index);
			} else if (currBtn.m_section == DisplaySection::TRANSPORTED) {
				setToolTipText2("", g_lang.get("TransportInfo"), DisplaySection::TRANSPORTED);
			} else {
				setToolTipText2("", "");
			}
			m_hoverBtn = currBtn;
			return true;
		} else {
			// don't do anything
		}
	} else {
		setToolTipText2("", "");
	}
	return false;
}

void Display::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = DisplayButton(DisplaySection::INVALID, invalidIndex);
	setToolTipText2("", "");
	m_ui->invalidateActivePos();
	//m_toolTip->setVisible(false);
}

}}//end namespace

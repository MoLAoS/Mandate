// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest { namespace Gui {

using Global::CoreData;

// =====================================================
// 	class Display
// =====================================================

Display::Display(UserInterface *ui, Vec2i pos)
		: Widget(WidgetWindow::getInstance(), pos, Vec2i(195, 500))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_logo(-1)
		, m_imageSize(32)
		, m_draggingWidget(false)
		, m_moveOffset(Vec2i(0))
		, m_hoverBtn(DisplaySection::INVALID, invalidPos)
		, m_pressedBtn(DisplaySection::INVALID, invalidPos)
		, m_toolTip(0) {
	Widget::setWidgetStyle(WidgetType::DISPLAY);
	int x = getBorderLeft();
	int y = getBorderTop();
	m_sizes.logoSize = Vec2i(getBordersHoriz() + 192, getBordersVert() + 192);
	m_upImageOffset = Vec2i(x, y);
	for (int i = 0; i < selectionCellCount; ++i) { // selection potraits
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = getBorderLeft();
		}
		ImageWidget::addImageX(0, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}
	y += m_imageSize;
	m_sizes.portraitSize = Vec2i(x + getBorderRight(), y + getBorderBottom());

	TextWidget::setAlignment(Alignment::NONE);
	TextWidget::setText(""); // (0) unit title
	TextWidget::addText(""); // (1) unit text
	TextWidget::addText(""); // (2) queued orders text (to display below progress bar if present)
	TextWidget::addText(""); // (3) progress bar

	const FontMetrics *fm = getFont()->getMetrics();

	///@todo fix: centre text with image
	TextWidget::setTextPos(Vec2i(getBorderLeft() + 40, getBorderTop() + m_imageSize / 4), 0);

	Vec2i arPos, aaPos, afPos;
	x = getBorderLeft();
	y = getBorderTop() + m_imageSize + m_imageSize / 4 + int(fm->getHeight()) * 6;
	m_downImageOffset = Vec2i(x, y);
	for (int i = 0; i < commandCellCount; ++i) { // command buttons
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = getBorderLeft();
		}
		ImageWidget::addImageX(0, Vec2i(x,y), Vec2i(m_imageSize));
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
	m_sizes.commandSize = Vec2i(x + getBorderRight(), y + getBorderBottom());

	x = getBorderLeft();
	y += int(m_imageSize * 0.5f);
	TextWidget::addText(""); // (4) 'Transported' label
	TextWidget::setTextPos(Vec2i(x, y), 4);
	y += int(fm->getHeight() + 1.f);
	m_carryImageOffset = Vec2i(x, y);
	for (int i = 0; i < transportCellCount; ++i) { // loaded unit portraits
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = getBorderLeft();
		}
		ImageWidget::addImageX(0, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}
	y += m_imageSize;
	m_sizes.transportSize = Vec2i(x + getBorderRight(), y + getBorderBottom());

	const Texture2D* overlayImages[3] = {
		g_widgetConfig.getTickTexture(),
		g_widgetConfig.getCrossTexture(),
		g_widgetConfig.getQuestionTexture()
	};
	m_autoRepairOn = addImageX(overlayImages[0], arPos, Vec2i(m_imageSize));
	m_autoRepairOff = addImageX(overlayImages[1], arPos, Vec2i(m_imageSize));
	m_autoRepairMixed = addImageX(overlayImages[2], arPos, Vec2i(m_imageSize));

	m_autoAttackOn = addImageX(overlayImages[0], aaPos, Vec2i(m_imageSize));
	m_autoAttackOff = addImageX(overlayImages[1], aaPos, Vec2i(m_imageSize));
	m_autoAttackMixed = addImageX(overlayImages[2], aaPos, Vec2i(m_imageSize));

	m_autoFleeOn =  addImageX(overlayImages[0], afPos, Vec2i(m_imageSize));
	m_autoFleeOff = addImageX(overlayImages[1], afPos, Vec2i(m_imageSize));
	m_autoFleeMixed = addImageX(overlayImages[2], afPos, Vec2i(m_imageSize));

	// -loadmap doesn't have any faction
	const Texture2D *logoTex = (g_world.getThisFaction()) ? g_world.getThisFaction()->getLogoTex() : 0;
	if (logoTex) {
		m_logo = ImageWidget::addImageX(logoTex, Vec2i(getBorderLeft(), getBorderTop()), Vec2i(192,192));
	}

	m_selectedCommandIndex = invalidPos;
	setProgressBar(-1);
	clear();
	setSize();

	m_toolTip = new CommandTip(WidgetWindow::getInstance());
	m_toolTip->setVisible(false);
}

void Display::setSize() {
	Vec2i sz = m_sizes.commandSize;
	if (m_ui->getSelection()->isEmpty()) {
		if (m_ui->getSelectedObject()) {
			sz = m_sizes.portraitSize;
		} else {
			// -loadmap doesn't have any faction
			if (g_world.getThisFaction() && g_world.getThisFaction()->getLogoTex()) {
				sz = m_sizes.logoSize;
			} else {
				setVisible(false);
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
}

void Display::setProgressBar(int i) {
	m_progress = i;
	if (i >= 0) {
		TextWidget::setText(intToStr(i) + "%", 3);
		Vec2i sz = getTextDimensions(3);
		m_progPrecentPos = 50 - sz.x / 2;
	} else {
		TextWidget::setText("", 3);
	}
}

void Display::setDownSelectedPos(int i) {
	if (m_selectedCommandIndex == i) {
		return;
	}
	if (m_selectedCommandIndex != invalidPos) {
		// shrink
		int ndx = m_selectedCommandIndex + selectionCellCount;
		Vec2i pos = getImagePos(ndx);
		Vec2i size = getImageSize(ndx);
		pos += Vec2i(3);
		size -= Vec2i(6);
		ImageWidget::setImageX(0, ndx, pos, size);
	}
	m_selectedCommandIndex = i;
	if (m_selectedCommandIndex != invalidPos) {
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

void trimTrailingNewlines(string &str) {
	while (!str.empty() && *(str.end() - 1) == '\n') {
		str.erase(str.end() - 1);
	}
}

void Display::setPortraitTitle(const string title) {
	if (TextWidget::getText(0).empty() && title.empty()) {
		return;
	}
	//string str = formatString(title);	
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
	int yPos = getBorderTop() + 40;
	TextWidget::setTextPos(Vec2i(5, yPos), 1);
	TextWidget::setText(str, 1);
	m_progressPos = Vec2i(14, yPos + lines * int(getFont()->getMetrics()->getHeight() + 1.f));
}

void Display::setOrderQueueText(const string &i_text) {
	if (TextWidget::getText(2).empty() && i_text.empty()) {
		return;
	}
	int y = m_progressPos.y;
	if (m_progress != -1) {
		y += int(getFont()->getMetrics()->getHeight()) + 3;
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
		a_offset = m_upImageOffset;
	} else if (i_section == DisplaySection::TRANSPORTED) {
		a_offset = m_carryImageOffset;
	} else {
		a_offset = m_downImageOffset;
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

	setDownSelectedPos(invalidPos);
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
	const int h = 15;
	const int w = 100;

	int bw = m_progress;
	Vec2i pos = getPos() + m_progressPos;

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
	if (m_selectedCommandIndex != invalidPos) {
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
	Vec2i offsets[3] = { m_upImageOffset, m_downImageOffset, m_carryImageOffset };
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
				return DisplayButton(DisplaySection::INVALID, invalidPos);
			}
		}
	}
	return DisplayButton(DisplaySection::INVALID, invalidPos);
}

bool Display::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (pos.y < myPos.y + getBorderTop()) {
			m_moveOffset = myPos - pos;
			m_draggingWidget = true;
			return true;
		}
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
				m_pressedBtn = m_hoverBtn = DisplayButton(DisplaySection::INVALID, invalidPos);
				return true;
			} else {
				m_pressedBtn = DisplayButton(DisplaySection::INVALID, invalidPos);
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

	if (m_draggingWidget) {
		m_draggingWidget = false;
		return true;
	}
	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != DisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
				if (m_hoverBtn == m_pressedBtn) {
					if (m_hoverBtn.m_section == DisplaySection::COMMANDS) {
						m_ui->commandButtonPressed(m_hoverBtn.m_index);
					}
					m_pressedBtn = DisplayButton(DisplaySection::INVALID, invalidPos);
					return true;
				}
			}
			m_pressedBtn = DisplayButton(DisplaySection::INVALID, invalidPos);
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
			m_hoverBtn = DisplayButton(DisplaySection::INVALID, invalidPos);
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

	if (m_draggingWidget) {
		setPos(pos + m_moveOffset);
		return true;
	}

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
	m_hoverBtn = DisplayButton(DisplaySection::INVALID, invalidPos);
	setToolTipText2("", "");
	m_ui->invalidateActivePos();
	//m_toolTip->setVisible(false);
}

}}//end namespace

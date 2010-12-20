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
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest { namespace Gui {

using Global::CoreData;

void setFancyBorder(BorderStyle &style) {
	style.setSizes(0);
	style.m_sizes[Border::TOP] = 15;
	style.m_sizes[Border::LEFT] = 3;
	
	style.m_type = BorderType::CUSTOM_CORNERS;
	int opaqueIndex = g_widgetConfig.getColourIndex(Colour(255, 255, 255, 127));
	int transparantIndex = g_widgetConfig.getColourIndex(Colour(255, 255, 255, 0));
	style.m_colourIndices[Corner::TOP_LEFT]		= opaqueIndex;
	style.m_colourIndices[Corner::TOP_RIGHT]	= transparantIndex;
	style.m_colourIndices[Corner::BOTTOM_RIGHT] = transparantIndex;
	style.m_colourIndices[Corner::BOTTOM_LEFT]	= transparantIndex;
}

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
		, m_draggingWidget(false)
		, m_moveOffset(Vec2i(0))
		, m_hoverBtn(DisplaySection::INVALID, invalidPos)
		, m_pressedBtn(DisplaySection::INVALID, invalidPos)
		, m_toolTip(0) {
	setFancyBorder(m_borderStyle);

	colors[0] = Vec3f(1.f, 1.f, 1.f);
	colors[1] = Vec3f(1.f, 0.5f, 0.5f);
	colors[2] = Vec3f(0.f, 1.f, 0.f);
	colors[3] = Vec3f(0.7f, 0.7f, 0.7f);

	currentColor = 0;

	int iconSize = 32;

	m_font = g_coreData.getFTDisplayFont();

	int x = getBorderLeft();
	int y = getHeight() - getBorderTop();
	m_upImageOffset = Vec2i(x, y);
	for (int i = 0; i < selectionCellCount; ++i) { // selection potraits
		if (i % cellWidthCount == 0) {
			y -= iconSize;
			x = getBorderLeft();
		}
		addImageX(0, Vec2i(x,y), Vec2i(iconSize));
		x += iconSize;
	}
	setTextParams("", Vec4f(1.f), m_font, false); // (0) unit title
	setTextShadowColour(Vec4f(0.f, 0.f, 0.f, 1.f));
	addText(""); // (1) unit text
	addText(""); // (2) queued orders text (to display below progress bar if present)
	addText(""); // (3) progress bar
	setTextPos(Vec2i(40, getHeight() - (iconSize + iconSize / 4)), 0);

	Vec2i arPos, aaPos, afPos;
	x = getBorderLeft();
	y = getHeight() - getBorderTop() - (iconSize + iconSize / 4) - int(m_font->getMetrics()->getHeight()) * 6;
	m_downImageOffset = Vec2i(x, y);
	for (int i = 0; i < commandCellCount; ++i) { // command buttons
		if (i % cellWidthCount == 0) {
			y -= iconSize;
			x = getBorderLeft();
		}
		addImageX(0, Vec2i(x,y), Vec2i(iconSize));
		if (i == UserInterface::autoRepairPos) {
			arPos = Vec2i(x, y);
		} else if (i == UserInterface::autoAttackPos) {
			aaPos = Vec2i(x, y);
		} else if (i == UserInterface::autoFleePos) {
			afPos = Vec2i(x, y);
		}
		x += iconSize;
	}

	x = getBorderLeft();
	y -= (iconSize + iconSize / 4 /* + int(m_font->getMetrics()->getHeight()) * 8*/);
	addText(""); // (4) 'Transported' label
	setTextPos(Vec2i(x, y + 5), 4);
	m_carryImageOffset = Vec2i(x, y);
	for (int i = 0; i < transportCellCount; ++i) { // loaded unit portraits
		if (i % cellWidthCount == 0) {
			y -= iconSize;
			x = getBorderLeft();
		}
		addImageX(0, Vec2i(x,y), Vec2i(iconSize));
		x += iconSize;
	}

	const Texture2D* overlayImages[3] = {
		g_coreData.getGreenTickOverlay(),
		g_coreData.getRedCrossOverlay(),
		g_coreData.getQuestionOverlay()
	};
	m_autoRepairOn = addImageX(overlayImages[0], arPos, Vec2i(iconSize));
	m_autoRepairOff = addImageX(overlayImages[1], arPos, Vec2i(iconSize));
	m_autoRepairMixed = addImageX(overlayImages[2], arPos, Vec2i(iconSize));

	m_autoAttackOn = addImageX(overlayImages[0], aaPos, Vec2i(iconSize));
	m_autoAttackOff = addImageX(overlayImages[1], aaPos, Vec2i(iconSize));
	m_autoAttackMixed = addImageX(overlayImages[2], aaPos, Vec2i(iconSize));

	m_autoFleeOn =  addImageX(overlayImages[0], afPos, Vec2i(iconSize));
	m_autoFleeOff = addImageX(overlayImages[1], afPos, Vec2i(iconSize));
	m_autoFleeMixed = addImageX(overlayImages[2], afPos, Vec2i(iconSize));

	// -loadmap doesn't have any faction
	const Texture2D *logoTex = (g_world.getThisFaction()) ? g_world.getThisFaction()->getLogoTex() : 0;
	if (logoTex) {
		m_logo = addImageX(logoTex, Vec2i(3,0), Vec2i(192,192));
	}

	m_selectedCommandIndex = invalidPos;
	setProgressBar(-1);
	clear();
	setSize();

	m_toolTip = new ToolTip(getParent());
	m_toolTip->setVisible(false);
}

void Display::setSize() {
	const int width = 192 + 3;
	const int bigHeight = 500;
	const int smallHeight = 192 + 15;
	Vec2i sz(width, bigHeight);
	if (m_ui->getSelection()->isEmpty()) {
		if (m_ui->getSelectedObject()) {
			sz = Vec2i(width, smallHeight);
		} else {
			// -loadmap doesn't have any faction
			if (g_world.getThisFaction() && g_world.getThisFaction()->getLogoTex()) {
				sz = Vec2i(width, smallHeight);
			} else {
				setVisible(false);
				return;
			}
		}
	} else {
		if (!m_ui->getSelection()->isComandable()) {
			sz = Vec2i(width, smallHeight);
		}
	}
	setVisible(true);
	Vec2i pos = getPos();
	Vec2i size = getSize();
	if (size != sz) {
		if (size.y == bigHeight) {
			pos.y += (bigHeight - smallHeight);
			Vec2i iPos = ImageWidget::getImagePos(0);
			iPos.y -= (bigHeight - smallHeight);
			ImageWidget::setImageX(0, 0, iPos, Vec2i(32,32));
			Vec2i tPos = TextWidget::getTextPos(0);
			tPos.y -= (bigHeight - smallHeight);
			TextWidget::setTextPos(tPos, 0);
		} else {
			assert(size.y == smallHeight);
			pos.y -= (bigHeight - smallHeight);
			Vec2i iPos = ImageWidget::getImagePos(0);
			iPos.y += (bigHeight - smallHeight);
			ImageWidget::setImageX(0, 0, iPos, Vec2i(32,32));
			Vec2i tPos = TextWidget::getTextPos(0);
			tPos.y += (bigHeight - smallHeight);
			TextWidget::setTextPos(tPos, 0);
		}
		Widget::setPos(pos);
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
		setImageX(0, ndx, pos, size);
	}
	m_selectedCommandIndex = i;
	if (m_selectedCommandIndex != invalidPos) {
		// enlarge
		int ndx = m_selectedCommandIndex + selectionCellCount;
		Vec2i pos = getImagePos(ndx);
		Vec2i size = getImageSize(ndx);
		pos -= Vec2i(3);
		size += Vec2i(6);
		setImageX(0, ndx, pos, size);
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
	string str = formatString(title);	
	TextWidget::setText(str, 0);
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
	int yPos = getHeight() - 40 - getBorderTop() - int(m_font->getMetrics()->getHeight()) * lines;
	TextWidget::setTextPos(Vec2i(5, yPos), 1);
	TextWidget::setText(str, 1);
	m_progressPos = Vec2i(14, yPos - 20);
}

void Display::setOrderQueueText(const string &i_text) {
	if (TextWidget::getText(2).empty() && i_text.empty()) {
		return;
	}
	int y = TextWidget::getTextPos(1).y - int(m_font->getMetrics()->getHeight());
	if (m_progress != -1) {
		y = m_progressPos.y - int(m_font->getMetrics()->getHeight()) - 3;
	}
	TextWidget::setTextPos(Vec2i(5, y), 2);
	TextWidget::setText(i_text, 2);
}

void Display::setLoadInfo(const string &str) {
	if (TextWidget::getText(3).empty() && str.empty()) {
		return;
	}
	int y;
	if (!TextWidget::getText(2).empty()) {
		y = TextWidget::getTextPos(2).y - int(m_font->getMetrics()->getHeight());
	} else {
		y = TextWidget::getTextPos(1).y - int(m_font->getMetrics()->getHeight());
	}
	TextWidget::setTextPos(Vec2i(5, y), 3);
	TextWidget::setText(str, 3);
}

void Display::setToolTipText(const string &i_txt, DisplaySection i_section) {
	//WIDGET_LOG( __FUNCTION__ << "( \"" << i_txt << "\" )");
	string str = i_txt;
	trimTrailingNewlines(str);
	const FontMetrics *fm = m_toolTip->getTextFont()->getMetrics();
	fm->wrapText(str, 32 * cellWidthCount);

	m_toolTip->setText(str);
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

void Display::setTransportedLabel(bool v) {
	TextWidget::setText((v ? g_lang.get("Transported") : ""), 4);
}

// misc
void Display::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	for (int i=0; i < selectionCellCount; ++i) {
		setImage(0, i);
	}

	for (int i=0; i < commandCellCount; ++i) {
		downLighted[i]= true;
		commandTypes[i]= NULL;
		commandClasses[i]= CommandClass::NULL_COMMAND;
		setImage(0, selectionCellCount + i);
	}

	for (int i=0; i < transportCellCount; ++i) {
		setImage(0, selectionCellCount + commandCellCount + i);
	}

	setDownSelectedPos(invalidPos);
	setPortraitTitle("");
	setPortraitText("");
	setProgressBar(-1);
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
	
	renderBgAndBorders();
	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject() && m_logo != -1) {
		// faction logo
		ImageWidget::renderImage(m_logo);
	}

	Vec4f light(1.f), dark(0.3f, 0.3f, 0.3f, 1.f);
	ImageWidget::startBatch();
	for (int i = 0; i < selectionCellCount; ++i) {
		if (getImage(i)) {
			renderImage(i, light);
		}
	}
	for (int i=0; i < commandCellCount; ++i) {
		if (getImage(i + selectionCellCount) && i != m_selectedCommandIndex) {
			renderImage(i + selectionCellCount, downLighted[i] ? light : dark);
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
				renderImage(ndx);
			}
		}
	}
	if (m_selectedCommandIndex != invalidPos) {
		assert(getImage(m_selectedCommandIndex + selectionCellCount));
		renderImage(m_selectedCommandIndex + selectionCellCount, light);
	}
	for (int i=0; i < transportCellCount; ++i) {
		if (getImage(i + selectionCellCount + commandCellCount)) {
			renderImage(i + selectionCellCount + commandCellCount, light);
		}
	}
	ImageWidget::endBatch();
	if (!TextWidget::getText(0).empty()) {
		renderTextShadowed(0);
	}
	if (!TextWidget::getText(1).empty()) {
		renderTextShadowed(1);
	}
	if (!TextWidget::getText(2).empty()) {
		renderTextShadowed(2);
	}
	if (!TextWidget::getText(4).empty()) {
		renderTextShadowed(4);
	}
	if (m_progress >= 0) {
		renderProgressBar();
	} else if (!TextWidget::getText(3).empty()) {
		renderTextShadowed(3);
	}
}

DisplayButton Display::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos() - Vec2i(getBorderLeft(), getBorderBottom());
	}
	Vec2i pos = i_pos;
	Vec2i offsets[3] = {
		m_upImageOffset, m_downImageOffset, m_carryImageOffset
	};
	int counts[3] = { selectionCellCount, commandCellCount, transportCellCount };
	for (int i=0; i < 3; ++i) {
		pos.y = i_pos.y - (offsets[i].y - cellHeightCount * imageSize);
		if (pos.y >= 0 && pos.y < imageSize * cellHeightCount) {
	int cellX = pos.x / imageSize;
	int cellY = (pos.y / imageSize) % cellHeightCount;
	int index = (cellHeightCount - cellY - 1) * cellWidthCount + cellX;
			if (index >= 0 && index < counts[i]) {
				if (getImage(i * cellHeightCount * cellWidthCount + index)) {
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
		if (pos.y > myPos.y + mySize.y - getBorderTop()) {
			m_moveOffset = myPos - pos;
			m_draggingWidget = true;
			return true;
		}

		if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
		&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
			Vec2i tPos = pos - myPos - Vec2i(getBorderLeft(), getBorderBottom());

			m_hoverBtn = computeIndex(tPos);
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
			if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
			&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
				Vec2i tPos = pos - myPos - Vec2i(getBorderLeft(), getBorderBottom());
				m_hoverBtn = computeIndex(tPos);
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

	if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
	&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
		Vec2i tPos = pos - myPos - Vec2i(getBorderLeft(), getBorderBottom());
		m_hoverBtn = computeIndex(tPos);
		if (m_hoverBtn.m_section == DisplaySection::TRANSPORTED) {
			m_ui->unloadRequest(m_hoverBtn.m_index);
			m_hoverBtn = DisplayButton(DisplaySection::INVALID, invalidPos);
			return true;
		}
	}
	return mouseDown(btn, pos);
}

void Display::resetTipPos(Vec2i i_offset) {
	if (m_toolTip->getText().empty()) {
		m_toolTip->setVisible(false);
		return;
	}
	Vec2i ttPos = getScreenPos() + i_offset;
	if (ttPos.x > g_metrics.getScreenW() / 2) {
		ttPos.x -= (m_toolTip->getWidth() + 5);
	} else {
		ttPos.x += (getWidth() + 5);
	}
	if (ttPos.y < m_toolTip->getHeight()) {
		ttPos.y += (m_toolTip->getHeight() - ttPos.y + 10);
	}
	ttPos.y -= (m_toolTip->getHeight() + 5);
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

	if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
	&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
		Vec2i tPos = pos - myPos - Vec2i(getBorderLeft(), getBorderBottom());

		DisplayButton currBtn = computeIndex(tPos);
		
		if (currBtn != m_hoverBtn) {
			// change stuff
			if (currBtn.m_section == DisplaySection::SELECTION) {
				m_ui->computePortraitInfo(currBtn.m_index);
			} else if (currBtn.m_section == DisplaySection::COMMANDS) {
				m_ui->computeCommandInfo(currBtn.m_index);
			} else if (currBtn.m_section == DisplaySection::TRANSPORTED) {
				setToolTipText(g_lang.get("TransportInfo"), DisplaySection::TRANSPORTED);
			} else {
				setToolTipText("");
			}
			m_hoverBtn = currBtn;
			return true;
		} else {
			// don't do anything
		}
	} else {
		setToolTipText("");
		}
	return false;
}

void Display::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = DisplayButton(DisplaySection::INVALID, invalidPos);
	setToolTipText("");
	m_ui->invalidateActivePos();
	//m_toolTip->setVisible(false);
}

// =====================================================
// 	class ResourceBar
// =====================================================

ResourceBar::ResourceBar(const Faction *faction, std::set<const ResourceType*> &types)
		: Widget(static_cast<Container*>(WidgetWindow::getInstance()))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_faction(faction)
		, m_moveOffset(0)
		, m_draggingWidget(false) {
	setFancyBorder(m_borderStyle);

	g_widgetWindow.registerUpdate(this);

	Font *font = g_coreData.getFTDisplayFont();
	const FontMetrics *fm = font->getMetrics();
	setTextFont(font);
	setTextColour(Vec4f(1.f));
	setTextShadowColour(Vec4f(0.f, 0.f, 0.f, 1.f));
	setTextCentre(false);

	foreach (std::set<const ResourceType*>, it, types) {
		m_resourceTypes.push_back(*it);
	}

	vector<int> reqWidths;
	int total_req = 0;
	foreach_const (vector<const ResourceType*>, it, m_resourceTypes) {
		addImage((*it)->getImage());
		m_headerStrings.push_back(formatString((*it)->getName()) + ": ");
		addText(m_headerStrings.back());
		int w;
		if ((*it)->getClass() == ResourceClass::CONSUMABLE) {
			w = 20 + int(fm->getTextDiminsions(m_headerStrings.back() + "000/0000 (000)").x);
		} else if ((*it)->getClass() == ResourceClass::STATIC) {
			w = 20 + int(fm->getTextDiminsions(m_headerStrings.back() + "0000").x);
		} else {
			w = 20 + int(fm->getTextDiminsions(m_headerStrings.back() + "0000/00000").x);
		}
		total_req += w;
		reqWidths.push_back(w);
	}
	int max_width = g_metrics.getScreenW() - 20;
	if (total_req < max_width) {
		// single row
		setSize(Vec2i(total_req + getBorderLeft(), 20 + getBorderTop()));
		setPos(Vec2i(g_metrics.getScreenW() / 2 - getWidth() / 2, g_metrics.getScreenH() - getHeight() - 5));
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
		setSize(Vec2i(std::max(width1, width2) + getBorderLeft(), 40 + getBorderTop()));
		setPos(Vec2i(g_metrics.getScreenW() / 2 - getWidth() / 2, g_metrics.getScreenH() - getHeight() - 5));
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
}

ResourceBar::~ResourceBar() {
	g_widgetWindow.unregisterUpdate(this);
}

void ResourceBar::render() {
	if (g_config.getUiPhotoMode()) {
		return;
	}
	renderBgAndBorders(false);
	for (int i=0; i < m_resourceTypes.size(); ++i) {
		renderImage(i);
		renderTextShadowed(i);
	}
}

void ResourceBar::update() {
	for (int i=0; i < m_resourceTypes.size(); ++i) {
		stringstream ss;
		const ResourceType* &rt = m_resourceTypes[i];
		const Resource *res = m_faction->getResource(rt); // amount & balance
		ss << m_headerStrings[i] << res->getAmount();
		if (rt->getClass() == ResourceClass::CONSUMABLE) {
			ss << "/" << m_faction->getStoreAmount(rt) << " (" << res->getBalance() << ")";
		} else if (rt->getClass() != ResourceClass::STATIC) {
			ss << '/' << m_faction->getStoreAmount(rt);
		}
		setText(ss.str(), i);
	}
}

bool ResourceBar::mouseDown(MouseButton btn, Vec2i pos) {
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (pos.y > myPos.y + mySize.y - getBorderTop()) {
		m_moveOffset = myPos - pos;
		m_draggingWidget = true;
		return true;
	}
	// nothing 'tangible' clicked, let event through
	return false;
}

bool ResourceBar::mouseUp(MouseButton btn, Vec2i pos) {
	if (m_draggingWidget) {
		m_draggingWidget = false;
		return true;
	}
	return false;
}

bool ResourceBar::mouseMove(Vec2i pos) {
	Vec2i myPos = getScreenPos();

	if (m_draggingWidget) {
		setPos(pos + m_moveOffset);
		return true;
	}
	// let event through
	return false;
}


}}//end namespace

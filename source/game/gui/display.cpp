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

#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest { namespace Gui {

using Global::CoreData;

// =====================================================
// 	class Display
// =====================================================

Display::Display(UserInterface *ui, Vec2i pos, Vec2i size)
		: Widget(WidgetWindow::getInstance(), pos, size)
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_draggingWidget(false)
		, m_moveOffset(Vec2i(0)) {
	m_borderStyle.setSizes(0);
	m_borderStyle.m_sizes[Border::TOP] = 15;
	m_borderStyle.m_sizes[Border::LEFT] = 3;
	
	m_borderStyle.m_type = BorderType::CUSTOM_CORNERS;
	int opaqueIndex = g_widgetConfig.getColourIndex(Colour(255, 255, 255, 127));
	int transparantIndex = g_widgetConfig.getColourIndex(Colour(255, 255, 255, 0));
	m_borderStyle.m_colourIndices[Corner::TOP_LEFT]		= opaqueIndex;
	m_borderStyle.m_colourIndices[Corner::TOP_RIGHT]	= transparantIndex;
	m_borderStyle.m_colourIndices[Corner::BOTTOM_RIGHT] = transparantIndex;
	m_borderStyle.m_colourIndices[Corner::BOTTOM_LEFT]	= transparantIndex;

	colors[0] = Vec3f(1.f, 1.f, 1.f);
	colors[1] = Vec3f(1.f, 0.5f, 0.5f);
	colors[2] = Vec3f(0.f, 1.f, 0.f);
	colors[3] = Vec3f(0.7f, 0.7f, 0.7f);

	currentColor = 0;

	m_font = g_coreData.getFTDisplayFont();

	int x = getBorderLeft();
	int y = getHeight() - getBorderTop();
	m_upImageOffset = Vec2i(x, y);
	for (int i = 0; i < upCellCount; ++i) { // 'up' images (selection potraits)
		if (i % cellSideCount == 0) {
			y -= 32;
			x = getBorderLeft();
		}
		addImageX(0, Vec2i(x,y), Vec2i(32,32));
		x += 32;
	}

	x = getBorderLeft();
	y = getHeight() - getBorderTop() - 40 - int(m_font->getMetrics()->getHeight()) * 10;
	m_downImageOffset = Vec2i(x, y);
	for (int i = 0; i < downCellCount; ++i) { // 'down' images (command buttons)
		if (i % cellSideCount == 0) {
			y -= 32;
			x = getBorderLeft();
		}
		addImageX(0, Vec2i(x,y), Vec2i(32,32));
		x += 32;
	}
	setTextParams("", Vec4f(1.f), m_font, false); // unit title
	setTextShadowColour(Vec4f(0.f, 0.f, 0.f, 1.f));
	addText(""); // unit text
	addText(""); // command text
	addText(""); // progress bar

	setTextPos(Vec2i(40, size.y - 40), 0);

	downSelectedPos = invalidPos;
	setProgressBar(-1);
	clear();
}

void Display::setProgressBar(int i) {
	m_progress = i;
	if (i >= 0) {
		TextWidget::setText(intToStr(i) + "%", 3);
		Vec2i sz = getTextDimensions(3);
		m_progPrecentPos = 50 - sz.x / 2;
	}
}

void Display::setDownSelectedPos(int i) {
	if (downSelectedPos == i) {
		return;
	}
	if (downSelectedPos != invalidPos) {
		// shrink
		int ndx = downSelectedPos + upCellCount;
		Vec2i pos = getImagePos(ndx);
		Vec2i size = getImageSize(ndx);
		pos += Vec2i(3);
		size -= Vec2i(6);
		setImageX(0, ndx, pos, size);
	}
	downSelectedPos = i;
	if (downSelectedPos != invalidPos) {
		// enlarge
		int ndx = downSelectedPos + upCellCount;
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

void Display::setTitle(const string title) {
	if (this->title.empty() && title.empty()) {
		return;
	}	string str = Util::formatString(title);
	this->title = str;
	
	TextWidget::setText(str, 0);
}

void Display::setText(const string &text) {
	if (this->text.empty() && text.empty()) {
		return;
	}
	string str = Util::formatString(text);
	this->text= str;
	int lines = 1;
	foreach_const (string, it, str) {
		if (*it == '\n') ++lines;
	}
	int yPos = getHeight() - 40 - getBorderTop() - int(m_font->getMetrics()->getHeight()) * lines;
	TextWidget::setTextPos(Vec2i(5, yPos), 1);
	TextWidget::setText(str, 1);
	m_progressPos = Vec2i(14, yPos - 20);
}

void Display::setInfoText(const string &infoText) {
	if (this->infoText.empty() && infoText.empty()) {
		return;
	}
	string str = Util::formatString(infoText);
	trimTrailingNewlines(str);
	this->infoText = str;

	int lines = 1;
	foreach_const (string, it, str) {
		if (*it == '\n') ++lines;
	}
	int yPos = getHeight() - getBorderTop() - imageSize * cellSideCount - 48
		- (lines + 10) * int(m_font->getMetrics()->getHeight());
	TextWidget::setTextPos(Vec2i(5, yPos), 2);
	TextWidget::setText(str, 2);
}

// misc
void Display::clear(){
	for(int i=0; i<upCellCount; ++i){
		upImages[i]= NULL;
		setImage(0, i);
	}

	for(int i=0; i<downCellCount; ++i){
		downImages[i]= NULL;
		downLighted[i]= true;
		commandTypes[i]= NULL;
		commandClasses[i]= CommandClass::NULL_COMMAND;
		setImage(0, upCellCount + i);
	}

	for(int i=0; i<carryCellCount; ++i){
		carryImages[i]= NULL;
	}

	setDownSelectedPos(invalidPos);
	setTitle("");
	setText("");
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

void Display::render() {
	renderBgAndBorders();
	Vec4f light(1.f), dark(0.3f, 0.3f, 0.3f, 1.f);
	ImageWidget::startBatch();
	for (int i = 0; i < upCellCount; ++i) {
		if (getImage(i)) {
			renderImage(i, light);
		}
	}
	for (int i=0; i < downCellCount; ++i) {
		if (getImage(i + upCellCount) && i != downSelectedPos) {
			renderImage(i + upCellCount, downLighted[i] ? light : dark);
		}
	}
	if (downSelectedPos != invalidPos) {
		assert(getImage(downSelectedPos + upCellCount));
		renderImage(downSelectedPos + upCellCount, light);
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
	if (m_progress >= 0) {
		renderProgressBar();
	}
}

int Display::computeIndex(Vec2i imgOffset, Vec2i pos) {
	pos.y = pos.y - (imgOffset.y - cellSideCount * imageSize);

	if (pos.y < 0 || pos.y >= imageSize * cellSideCount) {
		return invalidPos;
	}

	int cellX = pos.x / imageSize;
	int cellY = (pos.y / imageSize) % cellSideCount;
	int index = (cellSideCount - cellY - 1) * cellSideCount + cellX;

	if (index < 0 || index >= cellSideCount * cellSideCount) {
		index = invalidPos;
	}
	return index;
}

bool Display::mouseDown(MouseButton btn, Vec2i pos) {
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (pos.y > myPos.y + mySize.y - getBorderTop()) {
		m_moveOffset = myPos - pos;
		m_draggingWidget = true;
		return true;
	}

	if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
	&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
		Vec2i tPos = pos - myPos - Vec2i(getBorderLeft(), getBorderBottom());
		int ndx = computeIndex(m_downImageOffset, tPos);
		if (getImage(upCellCount + ndx)) {
			m_ui->commandButtonPressed(ndx);
			return true;
		}
	}
	// nothing 'tangible' clicked, let event through
	return false;
}

bool Display::mouseUp(MouseButton btn, Vec2i pos) {
	//Vec2i myPos = getScreenPos();
	//Vec2i mySize = getSize();

	if (m_draggingWidget) {
		m_draggingWidget = false;
		return true;
	}

	//if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
	//&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
	//}
	return true;
}

bool Display::mouseMove(Vec2i pos) {
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (m_draggingWidget) {
		setPos(pos + m_moveOffset);
		return true;
	}

	if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
	&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
		Vec2i tPos = pos - myPos - Vec2i(getBorderLeft(), getBorderBottom());
		int ndx = computeIndex(m_downImageOffset, tPos);
		if (ndx != invalidPos && getImage(upCellCount + ndx)) {
			m_ui->computeInfoString(ndx);
			return true;
		}
	}
	// let event through
	return false;
}

}}//end namespace

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
	
	//m_borderStyle.setSolid(g_widgetConfig.getColourIndex(Colour(191, 191, 191, 127)));

	m_borderStyle.m_type = BorderType::CUSTOM_CORNERS;
	m_borderStyle.m_colourIndices[Corner::TOP_LEFT]		= g_widgetConfig.getColourIndex(Colour(255, 255, 255, 127));
	m_borderStyle.m_colourIndices[Corner::TOP_RIGHT]	= g_widgetConfig.getColourIndex(Colour(0, 0, 0, 0));
	m_borderStyle.m_colourIndices[Corner::BOTTOM_RIGHT] = g_widgetConfig.getColourIndex(Colour(0, 0, 0, 0));
	m_borderStyle.m_colourIndices[Corner::BOTTOM_LEFT]	= g_widgetConfig.getColourIndex(Colour(0, 0, 0, 0));

	colors[0]= Vec3f(1.f, 1.f, 1.f);
	colors[1]= Vec3f(1.f, 0.5f, 0.5f);
	colors[2]= Vec3f(0.f, 1.f, 0.f);
	colors[3]= Vec3f(0.7f, 0.7f, 0.7f);

	currentColor= 0;

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
	y = getHeight() - getBorderTop() - int(m_font->getMetrics()->getHeight()) * 10;
	m_downImageOffset = Vec2i(x, y);
	for (int i = 0; i < downCellCount; ++i) { // 'down' images (command buttons)
		if (i % cellSideCount == 0) {
			y -= 32;
			x = getBorderLeft();
		}
		addImageX(0, Vec2i(x,y), Vec2i(32,32));
		x += 32;
	}
	setTextParams("", Vec4f(1.f), m_font, false);
	addText("");
	addText("");

	setTextPos(Vec2i(40, size.y - 40), 0);
//	setTextPos(Vec2i(5, size.y - 160), 1); // dynamic set ?
//	setTextPos(Vec2i(5, y), 2); // dynamic set ?

	clear();
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
}

void Display::setInfoText(const string infoText) {
	if (this->infoText.empty() && infoText.empty()) {
		return;
	}
	string str = Util::formatString(infoText);
	this->infoText = str;

	int lines = 1;
	foreach_const (string, it, str) {
		if (*it == '\n') ++lines;
	}
	int yPos = getHeight() - getBorderTop() - imageSize * cellSideCount 
		- (lines + 10) * int(m_font->getMetrics()->getHeight());
	TextWidget::setTextPos(Vec2i(5, yPos), 2);
	TextWidget::setText(str, 2);
}


//misc
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

	downSelectedPos= invalidPos;
	setTitle("");
	setText("");
//	setInfoText("");
	progressBar= -1;
}

int Display::computeDownIndex(int x, int y){
	y= y-(downY-cellSideCount*imageSize);

	if(y>imageSize*cellSideCount){
		return invalidPos;
	}

	int cellX= x/imageSize;
	int cellY= (y/imageSize) % cellSideCount;
	int index= (cellSideCount-cellY-1)*cellSideCount+cellX;;

	if(index<0 || index>=downCellCount || downImages[index]==NULL){
		index= invalidPos;
	}

	return index;
}

int Display::computeCarryIndex(int x, int y){
	y= y-(carryY-cellSideCount*imageSize);

	if(y>imageSize*cellSideCount){
		return invalidPos;
	}

	int cellX= x/imageSize;
	int cellY= (y/imageSize) % cellSideCount;
	int index= (cellSideCount-cellY-1)*cellSideCount+cellX;;

	if(index<0 || index>=carryCellCount || carryImages[index]==NULL){
		index= invalidPos;
	}

	return index;
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
		if (getImage(i + upCellCount)) {
			renderImage(i + upCellCount, downLighted[i] ? light : dark);
		}
	}
	ImageWidget::endBatch();
	if (!TextWidget::getText(0).empty()) {
		renderText(0);
	}
	if (!TextWidget::getText(1).empty()) {
		renderText(1);
	}
	if (!TextWidget::getText(2).empty()) {
		renderText(2);
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
			//m_ui->???
			return true;
		}
		// let event through (for now)
		return false;
	}
	return true;
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
	}

	if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderBottom()
	&& pos.x < myPos.x + mySize.x - getBorderRight() && pos.y < myPos.y + mySize.y - getBorderTop()) {
		Vec2i tPos = pos - myPos - Vec2i(getBorderLeft(), getBorderBottom());
		int ndx = computeIndex(m_downImageOffset, tPos);
		if (ndx != invalidPos && getImage(upCellCount + ndx)) {
			m_ui->computeInfoString(ndx);
			return true;
		}
		// let event through (for now)
		return false;
	}
	return true;
}

}}//end namespace

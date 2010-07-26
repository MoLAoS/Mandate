// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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

#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest { namespace Gui {

// =====================================================
// 	class Display
// =====================================================

Display::Display(Vec2i pos, Vec2i size)
		: Widget(WidgetWindow::getInstance(), pos, size)
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_draggingWidget(false)
		, m_moveOffset(Vec2i(0)) {
	m_borderStyle.setSizes(3);
	m_borderStyle.m_sizes[Border::TOP] = 15;
	m_borderStyle.setSolid(g_widgetConfig.getColourIndex(Colour(191, 191, 191, 127)));

	colors[0]= Vec3f(1.f, 1.f, 1.f);
	colors[1]= Vec3f(1.f, 0.5f, 0.5f);
	colors[2]= Vec3f(0.f, 1.f, 0.f);
	colors[3]= Vec3f(0.7f, 0.7f, 0.7f);

	currentColor= 0;

	int x;// = getBorderLeft();
	int y = getSize().y - getBorderTop();
	for (int i = 0; i < upCellCount; ++i) {
		if (i % cellSideCount == 0) {
			y -= 32;
			x = getBorderLeft();
		}
		addImageX(0, Vec2i(x,y), Vec2i(32,32));
		x += 32;
	}
	clear();
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
	}

	for(int i=0; i<carryCellCount; ++i){
		carryImages[i]= NULL;
	}

	downSelectedPos= invalidPos;
	title.clear();
	text.clear();
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
	for (int i = 0; i < upCellCount; ++i) {
		if (getImage(i)) {
			renderImage(i);
		}
	}
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
		// let event through (for now)
		return false;
	}
	return true;
}

}}//end namespace

// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Jaagup Repän <jrepan@gmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "components.h"

#include <cassert>
#include <algorithm>

#include "renderer.h"
#include "metrics.h"
#include "core_data.h"
#include "platform_util.h"
#include "window.h"
#include "font.h"
#include "math_util.h"
#include "renderer.h"
#include "widget_config.h"

#include "leak_dumper.h"

using std::max;
using namespace Shared::Graphics;
using namespace Glest::Widgets;

namespace Glest { 
using namespace Global;

namespace Graphics {

// =====================================================
// class GraphicComponent
// =====================================================

float GraphicComponent::anim = 0.f;
float GraphicComponent::fade = 0.f;
const float GraphicComponent::animSpeed = 0.02f;
const float GraphicComponent::fadeSpeed = 0.01f;

GraphicComponent::GraphicComponent() : x(0), y(0), w(0), h(0), text(), font(NULL) {
   enabled = true;
   visible = true;
}

void GraphicComponent::init(int x, int y, int w, int h) {
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
//	font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	enabled = true;
	visible = true;
}

bool GraphicComponent::mouseMove(int x, int y) {
	return
		x > this->x &&
		y > this->y &&
		x < this->x + w &&
		y < this->y + h;
}

bool GraphicComponent::mouseClick(int x, int y) {
	return mouseMove(x, y);
}

void GraphicComponent::update() {
	fade += fadeSpeed;
	anim += animSpeed;
	if (fade > 1.f) fade = 1.f;
	if (anim > 1.f) anim = 0.f;
}

void GraphicComponent::resetFade() {
	fade = 0.f;
}

// ===========================================================
// 	class GraphicProgressBar
// ===========================================================

GraphicProgressBar::GraphicProgressBar() : GraphicComponent(), progress(0), font(0) {}

void GraphicProgressBar::init(int x, int y, int w, int h) {
	GraphicComponent::init(x, y, w, h);

	// choose appropriate font size
	setFont(g_widgetConfig.getMenuFont());
	//if (h <= 20) {
	//	setFont(g_widgetConfig.getMenuFont()[FontSize::SMALL]);
	//} else if (h < 35) {
	//	setFont(g_widgetConfig.getMenuFont()[FontSize::NORMAL]);
	//} else {
	//	setFont(g_widgetConfig.getMenuFont()[FontSize::BIG]);
	//}
}

void GraphicProgressBar::render() {
	g_renderer.renderProgressBar(progress,	x, y, w, h, getFont());
}

}}//end namespace

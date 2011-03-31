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
#include "console.h"

#include "lang.h"
#include "config.h"
#include "program.h"
#include "game_constants.h"

#include "sound_renderer.h"
#include "core_data.h"

#include "leak_dumper.h"

#include <limits>

using std::numeric_limits;

#if defined(WIN32) || defined(WIN64)
	#define snprintf _snprintf
#endif

using std::numeric_limits;

namespace Glest { namespace Gui {

// =====================================================
// 	class Console
// =====================================================

Console::Console(Container* parent, int maxLines, bool fromTop)
		: Widget(parent)
		, TextWidget(this)
		, maxLines(maxLines)
		, fromTop(fromTop) {
	setWidgetStyle(WidgetType::CONSOLE);
	//config
#	if _GAE_DEBUG_EDITION_
		maxLines = 20;
		timeout = numeric_limits<float>::infinity();
#	else
		timeout= float(Config::getInstance().getUiConsoleTimeout());
#	endif
	timeElapsed = 0.0f;
	m_rootWindow->registerUpdate(this);
}

Console::~Console() {
	m_rootWindow->unregisterUpdate(this);
}

void Console::addStdMessage(const string &s){
	addLine(g_lang.get(s));
}

void Console::addStdMessage(const string &s, const string &param1, const string &param2, const string &param3) {
	string msg = g_lang.get(s);
	size_t bufsize = msg.size() + param1.size()  + param2.size()  + param3.size() + 32;
	char *buf = new char[bufsize];
	snprintf(buf, bufsize - 1, msg.c_str(), param1.c_str(), param2.c_str(), param3.c_str());
	addLine(buf);
	delete[] buf;
}

void Console::addLine(string line, bool playSound) {
	if (playSound) {
		g_soundRenderer.playFx(g_coreData.getClickSoundA());
	}
	lines.push_front(MessageTimePair(Message(line), timeElapsed));
	if (lines.size() > maxLines) {
		lines.pop_back();
	}
}

void Console::addDialog(string speaker, Colour colour, string text, bool playSound) {
	if (playSound) {
		g_soundRenderer.playFx(g_coreData.getClickSoundA());
	}
	Message msg(speaker, colour);
	msg.push_back(TextInfo(text));
	lines.push_front(MessageTimePair(msg, timeElapsed));
	if (lines.size() > maxLines) {
		lines.pop_back();
	}
}

Colour toColour(const Vec3f &c) {
	Colour res;
	res.r = clamp(unsigned(c.r * 255), 0u, 255u);
	res.g = clamp(unsigned(c.g * 255), 0u, 255u);
	res.b = clamp(unsigned(c.b * 255), 0u, 255u);
	res.a = 255u;
	return res;
}

void Console::addDialog(string speaker, Vec3f c, string text, bool playSound) {
	Colour colour = toColour(c);
	addDialog(speaker, colour, text, playSound);
}

void Console::update(){
	timeElapsed += 1.f / WORLD_FPS;

	if (!lines.empty()) {
		if(lines.back().second < timeElapsed - timeout){
			lines.pop_back();
		}
	}
}

void Console::render() {
	const FontMetrics *fm = getFont()->getMetrics();
	int lineSize = int(fm->getHeight() + 1.f);
	const Vec2i pos = getScreenPos() + Vec2i(getBorderLeft(), getBorderTop());

	TextWidget::startBatch(getFont());
	Colour black(0u, 0u, 0u, 255u);
	for (int i=0; i < lines.size(); ++i) {
		int x = pos.x;
		int y = pos.y;
		y += fromTop ? i * lineSize : maxLines * lineSize - i * lineSize;
		Message &msg = lines[i].first;
		foreach (Message, snippet, msg) {
			renderText(snippet->text, x + 2, y + 2, black, getFont());
			renderText(snippet->text, x, y, snippet->colour, getFont());
			x += int(fm->getTextDiminsions(snippet->text).x + 1.f);
		}
 	}
	TextWidget::endBatch();
}

}}//end namespace

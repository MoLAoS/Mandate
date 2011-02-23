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

Console::Console(Container* parent, int maxLines, int y_pos, bool fromTop)
		: Widget(parent, Vec2i(0), Vec2i(0))
		, TextWidget(this)
		, maxLines(maxLines)
		, yPos(y_pos)
		, fromTop(fromTop) {
	//config
#	if _GAE_DEBUG_EDITION_
		maxLines = 20;
		timeout = numeric_limits<float>::infinity();
#	else
		timeout= float(Config::getInstance().getUiConsoleTimeout());
#	endif
	timeElapsed = 0.0f;
	if (fromTop) {
		yPos = Metrics::getInstance().getScreenH() - y_pos;
	}
	m_font = g_widgetConfig.getGameFont()[FontSize::NORMAL];

	g_widgetWindow.registerUpdate(this);
}

Console::~Console() {
	g_widgetWindow.unregisterUpdate(this);
}

void Console::addStdMessage(const string &s){
	addLine(Lang::getInstance().get(s));
}

void Console::addStdMessage(const string &s, const string &param1, const string &param2, const string &param3) {
	string msg = Lang::getInstance().get(s);
	size_t bufsize = msg.size() + param1.size()  + param2.size()  + param3.size() + 32;
	char *buf = new char[bufsize];
	snprintf(buf, bufsize - 1, msg.c_str(), param1.c_str(), param2.c_str(), param3.c_str());
	addLine(buf);
	delete[] buf;
}

void Console::addLine(string line, bool playSound){
	if (playSound) {
		SoundRenderer::getInstance().playFx(CoreData::getInstance().getClickSoundA());
	}
	lines.push_front(MessageTimePair(Message(line), timeElapsed));
	if (lines.size() > maxLines) {
		lines.pop_back();
	}
}

void Console::addDialog(string speaker, Colour colour, string text, bool playSound) {
	addDialog(speaker, Vec3f(colour.r / 255.f, colour.g / 255.f, colour.b / 255.f), text, playSound);
}

void Console::addDialog(string speaker, Vec3f colour, string text, bool playSound) {
	if (playSound) {
		SoundRenderer::getInstance().playFx(CoreData::getInstance().getClickSoundA());
	}
	Message msg(speaker, colour);
	msg.push_back(TextInfo(text));
	lines.push_front(MessageTimePair(msg, timeElapsed));
	if (lines.size() > maxLines) {
		lines.pop_back();
	}
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
	const FontMetrics *fm = m_font->getMetrics();

	TextWidget::startBatch(m_font);
	Vec4f black(0.f, 0.f, 0.f, 1.f);
	for (int i=0; i < lines.size(); ++i) {
		int x = 20;
		int y = yPos + (fromTop ? -(int(lines.size()) - i - 1) : i) * int(fm->getHeight() + 1.f);
		Message &msg = lines[i].first;
		foreach (Message, snippet, msg) {
			renderText(snippet->text, x + 2, y - 2, black, m_font);
			renderText(snippet->text, x, y, snippet->colour, m_font);
			x += int(fm->getTextDiminsions(snippet->text).x + 1.f);
		}
 	}
	TextWidget::endBatch();
}

}}//end namespace

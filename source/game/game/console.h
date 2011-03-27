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

#ifndef _GLEST_GAME_CONSOLE_H_
#define _GLEST_GAME_CONSOLE_H_

#include <utility>
#include <string>
#include <deque>
#include <vector>

using std::string;
using std::deque;
using std::pair;
using std::vector;

#include "vec.h"
#include "widgets.h"

using Shared::Math::Vec3f;

namespace Glest { namespace Gui {
using namespace Widgets;

// =====================================================
// 	class Console
//
//	In-game console that shows various types of messages
// =====================================================

class Console : public Widget, public TextWidget {
public:
	struct TextInfo {
		string text;
		Colour colour;
		TextInfo(const string &text) : text(text), colour(255u) {}
		TextInfo(const string &text, Colour colour) : text(text), colour(colour) {}
	};
	struct Message : public vector<TextInfo> {
		Message(const string &txt, Colour col) { push_back(TextInfo(txt, col)); }
		Message(const string &txt) { push_back(TextInfo(txt)); }
	};
	typedef pair<Message, float> MessageTimePair;
	typedef deque<MessageTimePair> Lines;
	typedef Lines::const_iterator LineIterator;

private:
	float timeElapsed;
	Lines lines;

	//config
	int maxLines;
	float timeout;

	bool fromTop;

public:
	Console(Container* parent, int maxLines = 5, bool fromTop = false);
	~Console();

	int getReqHeight() {
		return (int(getFont()->getMetrics()->getHeight() + 1.f) * maxLines);
	}

	virtual void update() override;
	virtual void render() override;
	virtual string descType() const override { return "Console"; }

	void addStdMessage(const string &s);
	void addStdMessage(const string &s, const string &param1, const string &param2 = "", const string &param3 = "");
	void addLine(string line, bool playSound=false);
	void addDialog(string speaker, Vec3f colour, string text, bool playSound=false);
	void addDialog(string speaker, Colour colour, string text, bool playSound=false);

};

}}//end namespace

#endif

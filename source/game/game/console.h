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

using Shared::Math::Vec3f;

namespace Glest{ namespace Game{

// =====================================================
// 	class Console
//
//	In-game console that shows various types of messages
// =====================================================

class Console {
public:
	struct TextInfo {
		string text;
		Vec3f colour;
		TextInfo(const string &text) : text(text), colour(1.f) {}
		TextInfo(const string &text, Vec3f colour) : text(text), colour(colour) {}
	};
	struct Message : public vector<TextInfo> {
		Message(const string &txt, Vec3f col) { push_back(TextInfo(txt, col)); }
		Message(const string &txt) { push_back(TextInfo(txt)); }
	};
	typedef pair<Message, float> MessageTimePair;
	typedef deque<MessageTimePair> Lines;
	typedef Lines::const_iterator LineIterator;

private:
	float timeElapsed;
	Lines lines;

	//this should be deleted from here someday
	bool won, lost;

	//config
	int maxLines;
	float timeout;

	int yPos;
	bool fromTop;

public:
	Console(int maxLines = 5, int yPos = 20, bool fromTop = false);

	int getLineCount() const		{return lines.size();}
	Message getLine(int i) const	{return lines[i].first;}
	int getYPos() const				{ return yPos; }
	bool isFromTop() const			{ return fromTop; }


	void addStdMessage(const string &s);
	void addStdMessage(const string &s, const string &param1, const string &param2 = "", const string &param3 = "");
	void addLine(string line, bool playSound=false);
	void addDialog(string speaker, Vec3f colour, string text);
	void update();
	bool isEmpty();
};

}}//end namespace

#endif

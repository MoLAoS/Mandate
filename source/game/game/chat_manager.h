// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_CHATMANAGER_H_
#define _GLEST_GAME_CHATMANAGER_H_

#include <string>

#include "keymap.h"

namespace Glest { namespace Sim {
	class SimulationInterface;
}}
using Glest::Sim::SimulationInterface;
using std::string;
using Shared::Platform::Key;

namespace Glest { namespace Game {

class Console;

// =====================================================
// class ChatManager
// =====================================================

class ChatManager {
private:
	static const int maxTextLenght;

private:
	SimulationInterface *iSim;
	const Keymap &keymap;
	bool editEnabled;
	bool teamMode;
	Console* console;
	int thisTeamIndex;
	string text;

public:
	ChatManager(SimulationInterface *si, const Keymap &keymap);
	void init(Console* console, int thisTeamIndex);

	bool keyDown(const Key &key);
	void keyPress(char c);
	void updateNetwork();

	bool getEditEnabled() const {return editEnabled;}
	bool getTeamMode() const {return teamMode;}
	string getText() const  {return text;}
};

}}//end namespace

#endif

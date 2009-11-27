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
#include "chat_manager.h"

#include "window.h"
#include "console.h"
#include "network_manager.h"
#include "lang.h"
#include "keymap.h"
#include "script_manager.h"

#include "leak_dumper.h"


using namespace Shared::Platform;

namespace Glest { namespace Game {

// =====================================================
// class ChatManager
// =====================================================

const int ChatManager::maxTextLenght = 256;

ChatManager::ChatManager(const Keymap &keymap) :
		keymap(keymap),
		editEnabled(false),
		teamMode(false),
		console(NULL),
		thisTeamIndex(-1),
		text() {
}

void ChatManager::init(Console* console, int thisTeamIndex) {
	this->console = console;
	this->thisTeamIndex = thisTeamIndex;
}

/** @return true if the keystroke was used or will be used when keyPressed is called. */
bool ChatManager::keyDown(const Key &key) {
	Lang &lang = Lang::getInstance();
	bool keyUsed = false;


	//set chat mode
	if (!editEnabled) {
		bool oldTeamMode = teamMode;
		//all
		if (keymap.isMapped(key, ucChatAudienceAll)) {
			teamMode = false;
		//team
		} else if (keymap.isMapped(key, ucChatAudienceTeam)) {
			teamMode = true;
		//toggle
		} else if (keymap.isMapped(key, ucChatAudienceToggle)) {
			teamMode = !teamMode;
		}

		if (teamMode != oldTeamMode) {
			if (teamMode) {
				console->addLine(lang.get("ChatMode") + ": " + lang.get("All"));
			} else {
				console->addLine(lang.get("ChatMode") + ": " + lang.get("Team"));
			}
		} else if (keymap.isMapped(key, ucEnterChatMode)) {
			editEnabled = true;
			text.clear();
		} else {
			return false;
		}
		return true;
	}

	
	if (key == keyReturn && editEnabled) {
		editEnabled = false;
		if (!text.empty()) {
			if ( text[0] == '~' ) {
				string codeline = text.substr(1);
				console->addLine("Lua > " + codeline);
				ScriptManager::doSomeLua(codeline);
			} else {
				GameNetworkInterface *gameNetworkInterface = NetworkManager::getInstance().getGameNetworkInterface();
				console->addLine(gameNetworkInterface->getHostName() + ": " + text);
				gameNetworkInterface->sendTextMessage(text, teamMode ? thisTeamIndex : -1);
			}
		}
	} else if (key == keyBackspace) {
		if (!text.empty()) {
			text.erase(text.end() - 1);
		}
	} else if (key == keyEscape) {
		editEnabled = false;
	} else {
		return key.getAscii() >= ' ' && key.getAscii() <= 0x79;
	}
	return true;
}

void ChatManager::keyPress(char c) {
	if (editEnabled && text.size() < maxTextLenght) {
		//space is the first meaningful code
		if (c >= ' ') {
			text += c;
		}
	}
}

void ChatManager::updateNetwork() {
	GameNetworkInterface *gameNetworkInterface = NetworkManager::getInstance().getGameNetworkInterface();
	string text;
	string sender;

	if (!gameNetworkInterface->getChatText().empty()) {
		int teamIndex = gameNetworkInterface->getChatTeamIndex();

		if (teamIndex == -1 || teamIndex == thisTeamIndex) {
			console->addLine(gameNetworkInterface->getChatSender() + ": " + gameNetworkInterface->getChatText(), true);
		}
	}
}

}}//end namespace

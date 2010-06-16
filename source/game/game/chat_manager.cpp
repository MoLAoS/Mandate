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

#include "pch.h"
#include "chat_manager.h"

#include "window.h"
#include "console.h"
#include "network_interface.h"
#include "lang.h"
#include "keymap.h"
#include "script_manager.h"
#include "config.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Glest::Net;

namespace Glest { namespace Gui {

// =====================================================
// class ChatManager
// =====================================================

const int ChatManager::maxTextLenght = 256;

ChatManager::ChatManager(SimulationInterface *si, const Keymap &keymap)
		: iSim(si)
		, keymap(keymap)
		, editEnabled(false)
		, teamMode(false)
		, console(NULL)
		, thisTeamIndex(-1)
		, text() {
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
	
	if (key == KeyCode::RETURN && editEnabled) {
		editEnabled = false;
		if (!text.empty()) {
			IF_DEBUG_EDITION(
				if ( text[0] == '~' ) {
					string codeline = text.substr(1);
					console->addLine("Lua > " + codeline);
					ScriptManager::doSomeLua(codeline);
				} else {
			)
			console->addLine(g_config.getNetPlayerName() + ": " + text);
			NetworkInterface *netInterface = iSim->asNetworkInterface();
			if (netInterface) {
				netInterface->sendTextMessage(text, teamMode ? thisTeamIndex : -1);
			}
			IF_DEBUG_EDITION(
				}
			)
		}
	} else if (key == KeyCode::BACK_SPACE) {
		if (!text.empty()) {
			text.erase(text.end() - 1);
		}
	} else if (key == KeyCode::ESCAPE) {
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


}}//end namespace

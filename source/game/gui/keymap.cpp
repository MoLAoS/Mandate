// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include <stdexcept>
#include <sstream>
#include <fstream>

#include "keymap.h"
#include "util.h"
#include "FSFactory.hpp"

#include "leak_dumper.h"

using std::stringstream;
using std::ofstream;
using std::endl;
using std::ios_base;

namespace Glest { namespace Gui {

using namespace Shared::PhysFS;
using Shared::Util::toLower;

static const char *modNames[4] = {"Shift", "Ctrl", "Alt", "Meta"};
	
// =====================================================
// 	class Keymap::Entry
// =====================================================
void Keymap::Entry::init(const string &str) {
	key = KeyCode::NONE;
	mod = 0;
	if(str.empty()) {
		return;
	}
	size_t lastPlus = str.rfind('+');
	if(lastPlus == string::npos) {
		key = Key::findByName(str.c_str());
	} else {
		key = Key::findByName(str.substr(lastPlus + 1).c_str());
		//string modStr = str.substr(0, lastPos - 1);
		size_t start = 0;
		for(size_t plus = str.find('+'); plus != string::npos; plus = str.find('+', start)) {
			string modStr = str.substr(start, plus - start);
			int i;
			for(i = 0; i < 4; ++i) {
				if(!strcasecmp(modStr.c_str(),modNames[i])) {
					mod = mod | (1 << i);
					break;
				}
			}
			if(i == 4) {
				throw range_error(string() + "Invalid key modifier name: " + modStr);
			}
			start = plus + 1;
		}
	}
}

string Keymap::Entry::toString() const {
	stringstream ret;
	if(key) {
		if(mod) {
			for(int i = 0; i < 4; ++i) {
				if((1 << i) & mod) {
					switch(i) {
						case 0: if(key == KeyCode::LEFT_SHIFT || key == KeyCode::RIGHT_SHIFT) continue; break;
						case 1: if(key == KeyCode::LEFT_CTRL  || key == KeyCode::RIGHT_CTRL)  continue; break;
						case 2: if(key == KeyCode::LEFT_ALT   || key == KeyCode::RIGHT_ALT)   continue; break;
						case 3: if(key == KeyCode::LEFT_META  || key == KeyCode::RIGHT_META)  continue; break;
					}
					ret << modNames[i] << "+";
				}
			}
		}
		ret << Key::getName((KeyCode)key);
	}
	return ret.str();
}

// =====================================================
// 	class Keymap::EntryPair
// =====================================================

void Keymap::EntryPair::init(const string &str) {
	string s;
	s.reserve(str.size());
	clear();

	// remove whitespace
	for(int j = 0; j < str.size(); ++j) {
		if(!isspace(str[j])) {
			s += str[j];
		}
	}

	if(s.empty()) {
		return;
	}

	// split if it contains a comma
	size_t comma = s.find(',');
	if(comma != string::npos) {
		if(s.size() > comma + 1) {
			b.init(s.substr(comma + 1));
		}
		if(comma > 0) {
			s = s.substr(0, comma);
		}
	}
	a.init(s);
}

string Keymap::EntryPair::toString() const {
	string astr = a.toString();
	string bstr = b.toString();
	if(!astr.empty() && !bstr.empty()) {
		return astr + ", " + bstr;
	}
	return astr + bstr;
}

// =====================================================
// 	class Keymap
// =====================================================

#pragma pack(push, 1)
/*	  name,					prim_key,	prim_key mods,	sec_key,	sec_key mods */
const Keymap::UserCommandInfo Keymap::commandInfo[ucCount] = {
	{"None",					0,				0,			0,				0},
	{"ChatAudienceAll",			0,				0,			0,				0},
	{"ChatAudienceTeam",		0,				0,			0,				0},
	{"ChatAudienceToggle",		KeyCode::H,		0,			0,				0},
	{"EnterChatMode",			KeyCode::RETURN,0,			0,				0},
	{"MenuMain",				0,				0,			0,				0},
	{"MenuQuit",				KeyCode::ESCAPE,0,			0,				0},
	{"MenuSave",				KeyCode::Z,		0,			0,				0},
	{"MenuLoad",				0,				0,			0,				0},
	{"QuitNow",					0,				0,			0,				0},
	{"QuickSave",				0,				0,			0,				0},
	{"QuickLoad",				0,				0,			0,				0},
	{"PauseOn",					0,				0,			0,				0},
	{"PauseOff",				0,				0,			0,				0},
	{"PauseToggle",				KeyCode::P,		0,			0,				0},
	{"SpeedInc",				KeyCode::EQUAL,	0,	KeyCode::KEYPAD_PLUS,	0},
	{"SpeedDec",				KeyCode::MINUS,	0,	KeyCode::KEYPAD_MINUS,	0},
	{"SpeedReset",				0,				0,			0,				0},
	{"NetworkStatusOn",			0,				0,			0,				0},
	{"NetworkStatusOff",		0,				0,			0,				0},
	{"NetworkStatusToggle",		KeyCode::N,		0,			0,				0},
	{"SaveScreenshot",			KeyCode::E,		0,			0,				0},
	{"CycleDisplayColor",		KeyCode::C,		0,			0,				0},
	{"CameraCycleMode",			KeyCode::F,		0,			0,				0},
	{"CameraZoomIn",			KeyCode::PAGE_UP,	0,		0,				0},
	{"CameraZoomOut",			KeyCode::PAGE_DOWN,	0,		0,				0},
	{"CameraZoomReset",			0,				0,			0,				0},
	{"CameraPitchUp",			KeyCode::W,		bkmShift,	0,				0},
	{"CameraPitchDown",			KeyCode::S,		bkmShift,	0,				0},
	{"CameraRotateLeft",		KeyCode::A,		bkmShift,	0,				0},
	{"CameraRotateRight",		KeyCode::D,		bkmShift,	0,				0},
	{"CameraAngleReset",		0,				0,			0,				0},
	{"CameraZoomAndAngleReset",	0,				0,			0,				0},
	{"CameraPosLeft",			KeyCode::ARROW_LEFT,	0,	0,				0},
	{"CameraPosRight",			KeyCode::ARROW_RIGHT,	0,	0,				0},
	{"CameraPosUp",				KeyCode::ARROW_UP,		0,	0,				0},
	{"CameraPosDown",			KeyCode::ARROW_DOWN,	0,	0,				0},
	{"CameraGotoSelection",		KeyCode::SPACE,	0,			0,				0},
	{"CameraGotoLastEvent",		KeyCode::SPACE,	bkmShift,	0,				0},
	{"SelectNextIdleHarvester",	KeyCode::I,		0,			0,				0},
	{"SelectNextIdleBuilder",	0,				0,			0,				0},
	{"SelectNextIdleRepairer",	0,				0,			0,				0},
	{"SelectNextIdleWorker",	0,				0,			0,				0},
	{"SelectNextIdleRestorer",	0,				0,			0,				0},
	{"SelectNextIdleProducer",	0,				0,			0,				0},
	{"SelectNextProducer",		0,				0,			0,				0},
	{"SelectNextDamaged",		KeyCode::D,		0,			0,				0},
	{"SelectNextBuiltBuilding",	KeyCode::B,		0,			0,				0},
	{"SelectNextStore",			KeyCode::T,		0,			0,				0},
	{"Attack",					KeyCode::A,		0,			0,				0},
	{"Stop",					KeyCode::S,		0,			0,				0},
	{"Move",					KeyCode::M,		0,			0,				0},
	{"Replenish",				0,				0,			0,				0},
	{"Guard",					KeyCode::G,		0,			0,				0},
	{"Follow",					0,				0,			0,				0},
	{"Patrol",					0,				0,			0,				0},
	{"Rotate",					KeyCode::R,		0,			0,				0},
	{"LuaConsole",				KeyCode::BACK_QUOTE, 0,		0,				0},
	{"CycleShaders",			KeyCode::QUOTE, 0,			0,				0}
};
#pragma pack(pop)

Keymap::Keymap(const Input &input, const char* fileName) :
		input(input), lang(Lang::getInstance()) {
	for(int i = ucNone; i != ucCount; ++i) {
		entries.push_back(EntryPair(commandInfo[i]));
		//entries[i] = EntryPair(commandInfo[i]);
	}
	
	if(fileName && Shared::Util::fileExists(fileName)) {
		load(fileName);
	}
}

void Keymap::reinit() {
	entryCmdMap.clear();
	for(int i = ucNone; i != ucCount; ++i) {
		EntryPair &ep = entries[i];
		if(ep.getA().getKey()) {
			entryCmdMap[ep.getA()] = (UserCommand)i;
		}
		if(ep.getB().getKey()) {
			entryCmdMap[ep.getB()] = (UserCommand)i;
		}
	}
}

void Keymap::load(const char *path) {
	Properties p;
	p.load(path);
 	const Properties::PropertyMap &pm = p.getPropertyMap();
	Properties::PropertyMap::const_iterator it;
	for(int i = ucNone; i != ucCount; ++i) {
		string cmdName = toLower(getCommandName((UserCommand)i));		
		it = pm.find(cmdName);
		if(it != pm.end()) {
			try {
				entries[i].init(it->second);
			} catch (runtime_error &e) {
				stringstream str;
				str << "Failed to parse key map file " << path << ". Failed entry:" << endl
					<< cmdName << " = " << it->second << endl << e.what();
				throw runtime_error(str.str());
			}
		}
	}
	reinit();
}

void Keymap::save(const char *path) {
	try {
		ostream *out = FSFactory::getInstance()->getOStream(path);
		size_t maxSize = 0;
		for(int i = ucNone + 1; i != ucCount; ++i) {
			size_t size = strlen(getCommandName((UserCommand)i));
			if(size > maxSize) {
				maxSize = size;
			}
		}
		for(int i = ucNone + 1; i != ucCount; ++i) {
			const char* name = getCommandName((UserCommand)i);
			size_t size = strlen(name);
			*out << getCommandName((UserCommand)i);
			for(int j = size; j < maxSize; ++j) {
				*out << " ";
			}
			*out << " = " << entries[i].toString() << endl;
		}
		delete out;
	} catch (runtime_error &e) {
		stringstream str;
		str << "Failed to save key map file: " << path << endl << e.what();
		throw runtime_error(str.str());
	}
}

bool Keymap::isMapped(Key key, UserCommand cmd) const {
	assert(cmd >= 0 && cmd < ucCount);
	KeyCode keyCode = key.getCode();
	if(keyCode <= KeyCode::UNKNOWN) {
		return false;
	}
	return entries[cmd].matches(keyCode, getCurrentMods());
}

UserCommand Keymap::getCommand(Key key) const {
	KeyCode keyCode = key.getCode();
	if(keyCode > KeyCode::UNKNOWN) {
		map<Entry, UserCommand>::const_iterator i = entryCmdMap.find(
				Entry(keyCode, getCurrentMods()));
		return i == entryCmdMap.end() ? ucNone : i->second;
	}
	return ucNone;
}

int Keymap::getCurrentMods() const {
	return	  (input.isShiftDown()	? bkmShift	: 0)
			| (input.isCtrlDown()	? bkmCtrl	: 0)
			| (input.isAltDown()	? bkmAlt	: 0)
			| (input.isMetaDown()	? bkmMeta	: 0);
}

const char* Keymap::getCommandName(UserCommand cmd) {
	assert(cmd >= 0 && cmd < ucCount);
	return commandInfo[cmd].name;
}

}}//end namespace

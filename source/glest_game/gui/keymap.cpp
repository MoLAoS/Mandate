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

#include "leak_dumper.h"

using std::stringstream;
using std::ofstream;
using std::endl;
using std::ios_base;

namespace Glest { namespace Game {

static const char *modNames[4] = {"Shift", "Ctrl", "Alt", "Meta"};
	
// =====================================================
// 	class Keymap::Entry
// =====================================================
void Keymap::Entry::init(const string &str) {
	key = keyNone;
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
						case 0: if(key == keyLShift || key == keyRShift) continue; break;
						case 1: if(key == keyLCtrl  || key == keyRCtrl)  continue; break;
						case 2: if(key == keyLAlt   || key == keyRAlt)   continue; break;
						case 3: if(key == keyLMeta  || key == keyRMeta)  continue; break;
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

const Keymap::UserCommandInfo Keymap::commandInfo[ucCount] = {
	{"None",					0,			0,			0,			0},
	{"ChatAudienceAll",			0,			0,			0,			0},
	{"ChatAudienceTeam",		0,			0,			0,			0},
	{"ChatAudienceToggle",		keyH,		0,			0,			0},
	{"EnterChatMode",			keyReturn,	0,			0,			0},
	{"MenuMain",				0,			0,			0,			0},
	{"MenuQuit",				keyEscape,	0,			0,			0},
	{"MenuSave",				keyZ,		0,			0,			0},
	{"MenuLoad",				0,			0,			0,			0},
	{"QuitNow",					0,			0,			0,			0},
	{"QuickSave",				0,			0,			0,			0},
	{"QuickLoad",				0,			0,			0,			0},
	{"PauseOn",					0,			0,			0,			0},
	{"PauseOff",				0,			0,			0,			0},
	{"PauseToggle",				keyP,		0,			0,			0},
	{"SpeedInc",				keyEquals,	0,			keyKPPlus,	0},
	{"SpeedDec",				keyMinus,	0,			keyKPMinus,	0},
	{"SpeedReset",				0,			0,			0,			0},
	{"NetworkStatusOn",			0,			0,			0,			0},
	{"NetworkStatusOff",		0,			0,			0,			0},
	{"NetworkStatusToggle",		keyN,		0,			0,			0},
	{"SaveScreenshot",			keyE,		0,			0,			0},
	{"CycleDisplayColor",		keyC,		0,			0,			0},
	{"CameraCycleMode",			keyF,		0,			0,			0},
	{"CameraZoomIn",			keyPageUp,	0,			0,			0},
	{"CameraZoomOut",			keyPageDown,0,			0,			0},
	{"CameraZoomReset",			0,			0,			0,			0},
	{"CameraPitchUp",			keyW,		bkmShift,	0,			0},
	{"CameraPitchDown",			keyS,		bkmShift,	0,			0},
	{"CameraRotateLeft",		keyA,		bkmShift,	0,			0},
	{"CameraRotateRight",		keyD,		bkmShift,	0,			0},
	{"CameraAngleReset",		0,			0,			0,			0},
	{"CameraZoomAndAngleReset",	0,			0,			0,			0},
	{"CameraPosLeft",			keyLeft,	0,			0,			0},
	{"CameraPosRight",			keyRight,	0,			0,			0},
	{"CameraPosUp",				keyUp,		0,			0,			0},
	{"CameraPosDown",			keyDown,	0,			0,			0},
	{"CameraGotoSelection",		keySpace,	0,			0,			0},
	{"CameraGotoLastEvent",		keySpace,	bkmShift,	0,			0},
	{"SelectNextIdleHarvester",	keyI,		0,			0,			0},
	{"SelectNextIdleBuilder",	0,			0,			0,			0},
	{"SelectNextIdleRepairer",	0,			0,			0,			0},
	{"SelectNextIdleWorker",	0,			0,			0,			0},
	{"SelectNextIdleRestorer",	0,			0,			0,			0},
	{"SelectNextIdleProducer",	0,			0,			0,			0},
	{"SelectNextProducer",		keyR,		0,			0,			0},
	{"SelectNextDamaged",		keyD,		0,			0,			0},
	{"SelectNextBuiltBuilding",	keyB,		0,			0,			0},
	{"SelectNextStore",			keyT,		0,			0,			0},
	{"Attack",					keyA,		0,			0,			0},
	{"Stop",					keyS,		0,			0,			0},
	{"Move",					keyM,		0,			0,			0},
	{"Replenish",				0,			0,			0,			0},
	{"Guard",					keyG,		0,			0,			0},
	{"Follow",					0,			0,			0,			0},
	{"Patrol",					0,			0,			0,			0}
};
#pragma pack(pop)

Keymap::Keymap(const Input &input, const char* fileName) :
		input(input), lang(Lang::getInstance()) {
	for(int i = ucNone; i != ucCount; ++i) {
		entries.push_back(EntryPair(commandInfo[i]));
		//entries[i] = EntryPair(commandInfo[i]);
	}
	
	if(fileName) {
		std::ifstream in(fileName, ios_base::in);
		if(!in.fail()) {
			in.close();
			load(fileName);
		}
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
		string cmdName(getCommandName((UserCommand)i));
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
		} /*else {
			stringstream str;
			str << "Failed to parse key map file " << path << ". Invalid command name: "
					<< cmdName << ".";
			throw runtime_error(str.str());
		}*/
	}

	reinit();
}

void Keymap::save(const char *path) {
	try {
		ofstream out(path, ios_base::out | ios_base::trunc);
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
			out << getCommandName((UserCommand)i);
			for(int j = size; j < maxSize; ++j) {
				out << " ";
			}
			out << " = " << entries[i].toString() << endl;
		}
	} catch (runtime_error &e) {
		stringstream str;
		str << "Failed to save key map file: " << path << endl << e.what();
		throw runtime_error(str.str());
	}
}

}}//end namespace

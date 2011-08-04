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
// 	class HotKey
// =====================================================

void HotKey::init(const string &str) {
	m_keyCode = KeyCode::NONE;
	m_modFlags = 0;
	if (str.empty()) {
		return;
	}
	size_t lastPlus = str.rfind('+');
	if (lastPlus == string::npos) {
		m_keyCode = Key::findByName(str.c_str());
	} else {
		m_keyCode = Key::findByName(str.substr(lastPlus + 1).c_str());
		//string modStr = str.substr(0, lastPos - 1);
		size_t start = 0;
		for (size_t plus = str.find('+'); plus != string::npos; plus = str.find('+', start)) {
			string modStr = str.substr(start, plus - start);
			int i;
			for (i = 0; i < 4; ++i) {
				if (!strcasecmp(modStr.c_str(),modNames[i])) {
					m_modFlags |= (1 << i);
					break;
				}
			}
			if (i == 4) {
				throw range_error(string() + "Invalid key modifier name: " + modStr);
			}
			start = plus + 1;
		}
	}
}

string HotKey::toString() const {
	stringstream ret;
	if (m_keyCode) {
		if (m_modFlags) {
			for (int i = 0; i < 4; ++i) {
				if ((1 << i) & m_modFlags) {
					switch (i) {
						case 0: if (m_keyCode == KeyCode::LEFT_SHIFT || m_keyCode == KeyCode::RIGHT_SHIFT) continue;
						case 1: if (m_keyCode == KeyCode::LEFT_CTRL  || m_keyCode == KeyCode::RIGHT_CTRL)  continue;
						case 2: if (m_keyCode == KeyCode::LEFT_ALT   || m_keyCode == KeyCode::RIGHT_ALT)   continue;
						case 3: if (m_keyCode == KeyCode::LEFT_META  || m_keyCode == KeyCode::RIGHT_META)  continue;
					}
					ret << modNames[i] << "+";
				}
			}
		}
		ret << Key::getName(m_keyCode);
	}
	return ret.str();
}

// =====================================================
// 	class HotKeyAssignment
// =====================================================

void HotKeyAssignment::init(const string &str) {
	string s;
	s.reserve(str.size());
	//clear();

	// remove whitespace
	for (int j = 0; j < str.size(); ++j) {
		if (!isspace(str[j])) {
			s += str[j];
		}
	}

	if (s.empty()) {
		m_hotKey1.clear();
		m_hotKey2.clear();
	} else {
		// split if it contains a comma
		size_t comma = s.find(',');
		if (comma != string::npos) {
			if (s.size() > comma + 1) {
				m_hotKey2.init(s.substr(comma + 1));
			}
			if (comma > 0) {
				s = s.substr(0, comma);
			}
		} else {
			m_hotKey2.clear();
		}
		m_hotKey1.init(s);
	}
	Modified(this);
}

void HotKeyAssignment::setHotKey1(HotKey hk) {
	if (hk == m_hotKey1) {
		return;
	}
	m_hotKey1 = hk;
	if (!m_hotKey1.isSet() && m_hotKey2.isSet()) {
		m_hotKey1 = m_hotKey2;
		m_hotKey2.clear();
	}
	Modified(this);
}

void HotKeyAssignment::setHotKey2(HotKey hk) {
	if (hk == m_hotKey2) {
		return;
	}
	m_hotKey2 = hk;
	if (!m_hotKey1.isSet() && m_hotKey2.isSet()) {
		m_hotKey1 = m_hotKey2;
		m_hotKey2.clear();
	}
	Modified(this);
}

void HotKeyAssignment::clear() {
	if (m_hotKey1.isSet() || m_hotKey2.isSet()) {
		m_hotKey1.clear();
		m_hotKey2.clear();
		Modified(this);
	}
}

string HotKeyAssignment::toString() const {
	string astr = m_hotKey1.toString();
	string bstr = m_hotKey2.toString();
	if (!astr.empty() && !bstr.empty()) {
		return astr + ", " + bstr;
	}
	return astr + bstr;
}

// =====================================================
// 	class Keymap
// =====================================================

#pragma pack(push, 1)
/*	prim_key,				prim_key mods,			sec_key,				sec_key mods */
const AssignmentInfo Keymap::commandInfo[UserCommand::COUNT] = {
	{0,						0,						0,						0}, // NONE
	{0,						0,						0,						0}, // CHAT_AUDIENCE_ALL
	{0,						0,						0,						0}, // CHAT_AUDIENCE_TEAM
	{KeyCode::H,			0,						0,						0}, // CHAT_AUDIENCE_TOGGLE
	{KeyCode::RETURN,		0,						0,						0}, // SHOW_CHAT_DIALOG
	{KeyCode::ESCAPE,		0,						0,						0}, // QUIT_GAME
	{KeyCode::Z,			0,						0,						0}, // SAVE_GAME
	{0,						0,						0,						0}, // PAUSE_GAME
	{0,						0,						0,						0}, // RESUME_GAME
	{KeyCode::P,			0,						0,						0}, // TOGGLE_PAUSE
	{KeyCode::EQUAL,		0,						KeyCode::KEYPAD_PLUS,	0}, // INC_SPEED
	{KeyCode::MINUS,		0,						KeyCode::KEYPAD_MINUS,	0}, // DEC_SPEED
	{0,						0,						0,						0}, // RESET_SPEED
	{KeyCode::E,			0,						0,						0}, // SAVE_SCREENSHOT
	{KeyCode::PAGE_UP,		0,						0,						0}, // ZOOM_CAMERA_IN
	{KeyCode::PAGE_DOWN,	0,						0,						0}, // ZOOM_CAMERA_OUT
	{KeyCode::W,			ModKeys::SHIFT,			0,						0}, // PITCH_CAMERA_UP
	{KeyCode::S,			ModKeys::SHIFT,			0,						0}, // PITCH_CAMERA_DOWN
	{KeyCode::A,			ModKeys::SHIFT,			0,						0}, // ROTATE_CAMERA_LEFT
	{KeyCode::D,			ModKeys::SHIFT,			0,						0}, // ROTATE_CAMERA_RIGHT
	{0,						0,						0,						0}, // CAMERA_RESET_ZOOM
	{0,						0,						0,						0}, // CAMERA_RESET_ANGLE
	{0,						0,						0,						0}, // CAMERA_RESET
	{KeyCode::ARROW_LEFT,	0,						0,						0}, // MOVE_CAMERA_LEFT
	{KeyCode::ARROW_RIGHT,	0,						0,						0}, // MOVE_CAMERA_RIGHT
	{KeyCode::ARROW_UP,		0,						0,						0}, // MOVE_CAMERA_UP
	{KeyCode::ARROW_DOWN,	0,						0,						0}, // MOVE_CAMERA_DOWN
	{KeyCode::SPACE,		0,						0,						0}, // GOTO_SELECTION
	{KeyCode::SPACE,		ModKeys::SHIFT,			0,						0}, // GOTO_LAST_EVENT
	{KeyCode::I,			0,						0,						0}, // SELECT_IDLE_HARVESTER
	{0,						0,						0,						0}, // SELECT_IDLE_BUILDER
	{0,						0,						0,						0}, // SELECT_IDLE_REPAIRER
	{0,						0,						0,						0}, // SELECT_IDLE_WORKER
	{0,						0,						0,						0}, // SELECT_IDLE_RESTORER
	{0,						0,						0,						0}, // SELECT_IDLE_PRODUCER
	{0,						0,						0,						0}, // SELECT_NEXT_PRODUCER
	{KeyCode::D,			0,						0,						0}, // SELECT_NEXT_DAMAGED
	{KeyCode::B,			0,						0,						0}, // SELECT_NEXT_BUILT_BUILDING
	{KeyCode::T,			0,						0,						0}, // SELECT_NEXT_STORE
	{KeyCode::A,			0,						0,						0}, // ATTACK
	{KeyCode::S,			0,						0,						0}, // STOP
	{KeyCode::M,			0,						0,						0}, // MOVE
	{0,						0,						0,						0}, // REPAIR
	{KeyCode::G,			0,						0,						0}, // GUARD
	{0,						0,						0,						0}, // FOLLOW
	{0,						0,						0,						0}, // PATROL
	{KeyCode::R,			0,						0,						0}, // ROTATE_BUILDING
	{KeyCode::BACK_QUOTE,	0,						0,						0}, // SHOW_LUA_CONSOLE
	{KeyCode::QUOTE,		0,						0,						0}, // CYCLE_SHADERS
	{KeyCode::SEMI_COLON,	0,						0,						0}  // TOGGLE_TEAM_TINT
};
#pragma pack(pop)

Keymap::Keymap(const Input &input, const char* fileName)
		: m_input(input)
		, m_lang(Lang::getInstance())
		, m_filename(fileName)
		, m_dirty(false) {
	foreach_enum(UserCommand, uc) {
		if (uc == UserCommand::ROTATE_BUILDING) {
			DEBUG_HOOK();
		}
		m_entries.push_back(HotKeyAssignment(uc, commandInfo[uc]));
	}

	if (FSFactory::fileExists(m_filename.c_str())) {
		load(m_filename.c_str());
	}

	for (UserCommand uc(1); uc != UserCommand::COUNT; ++uc) {
		m_entries[uc].Modified.connect(this, &Keymap::onAssignmentModified);
		HotKey hk1 = m_entries[uc].getHotKey1();
		if (hk1.isSet()) {
			m_hotKeyCmdMap[hk1] = uc;
		}
		HotKey hk2 = m_entries[uc].getHotKey2();
		if (hk2.isSet()) {
			m_hotKeyCmdMap[hk2] = uc;
		}
		m_cmdHotKeyMap[uc] = HotKeyPair(hk1, hk2);
	}
}

void Keymap::onAssignmentModified(HotKeyAssignment *assignment) {
	UserCommand uCmd = assignment->getUserCommand();
	HotKeyPair foo = m_cmdHotKeyMap[uCmd];
	if (foo.first.isSet()) {
		assert(m_hotKeyCmdMap[foo.first] == uCmd);
		m_hotKeyCmdMap.erase(foo.first);
	}
	if (foo.second.isSet()) {
		assert(m_hotKeyCmdMap[foo.second] == uCmd);
		m_hotKeyCmdMap.erase(foo.second);
	}
	if (assignment->getHotKey1().isSet()) {
		m_hotKeyCmdMap[assignment->getHotKey1()] = uCmd;
	}
	if (assignment->getHotKey2().isSet()) {
		m_hotKeyCmdMap[assignment->getHotKey2()] = uCmd;
	}
	m_cmdHotKeyMap[uCmd] = HotKeyPair(assignment->getHotKey1(), assignment->getHotKey2());
	save();
}

//void Keymap::reinit() {
//	m_hotKeyCmdMap.clear();
//	foreach_enum (UserCommand, uc) {
//		HotKeyAssignment &hka = m_entries[uc];
//		if (hka.getHotKey1().isSet()) {
//			m_hotKeyCmdMap[hka.getHotKey1()] = uc;
//		}
//		if (hka.getHotKey2().isSet()) {
//			m_hotKeyCmdMap[hka.getHotKey2()] = uc;
//		}
//	}
//}

void Keymap::load(const char *path) {
	if (!path) {
		path = m_filename.c_str();
	}
	Properties p;
	p.load(path);

 	const Properties::PropertyMap &pm = p.getPropertyMap();
	Properties::PropertyMap::const_iterator it;
	for (UserCommand uc(1); uc < UserCommand::COUNT; ++uc) {
		string cmdName = toLower(getCommandName(uc));
		it = pm.find(cmdName);
		if (it != pm.end()) {
			try {
				//if (uc == UserCommand::ROTATE_BUILDING) {
				//	DEBUG_HOOK();
				//}
				m_entries[uc].init(it->second);
			} catch (runtime_error &e) {
				stringstream str;
				str << "Failed to parse key map file " << path << ". Failed entry:" << endl
					<< cmdName << " = " << it->second << endl << e.what();
				throw runtime_error(str.str());
			}
		}
	}
	//reinit();
	if (m_dirty) {
		m_dirty = false;
		DirtyModified(false);
	}
}

void Keymap::save(const char *path) {
	if (!path) {
		path = m_filename.c_str();
	}
	try {
		ostream *out = g_fileFactory.getOStream(path);
		size_t maxSize = 0;
		for (UserCommand uc(1); uc < UserCommand::COUNT; ++uc) {
			size_t size = strlen(getCommandName(uc));
			if (size > maxSize) {
				maxSize = size;
			}
		}
		for (UserCommand uc(1); uc < UserCommand::COUNT; ++uc) {
			const char* name = getCommandName(uc);
			size_t size = strlen(name);
			*out << getCommandName(uc);
			for(int i = size; i < maxSize; ++i) {
				*out << " ";
			}
			*out << " = " << m_entries[uc].toString() << endl;
		}
		delete out;
		if (m_dirty) {
			m_dirty = false;
			DirtyModified(false);
		}
	} catch (runtime_error &e) {
		stringstream str;
		str << "Failed to save key map file: " << path << endl << e.what();
		throw runtime_error(str.str());
	}
}

//bool Keymap::isMapped(Key key, UserCommand cmd) const {
//	assert(cmd > UserCommand::INVALID && cmd < UserCommand::COUNT);
//	KeyCode keyCode = key.getCode();
//	if (keyCode <= KeyCode::UNKNOWN) {
//		return false;
//	}
//	return entries[cmd].matches(keyCode, getCurrentMods());
//}

UserCommand Keymap::getCommand(KeyCode keyCode, int modFlags) const {
	if (keyCode > KeyCode::UNKNOWN) {
		HotKeyCommandMap::const_iterator i = m_hotKeyCmdMap.find(HotKey(keyCode, modFlags));
		return i == m_hotKeyCmdMap.end() ? UserCommand::NONE : (UserCommand::Enum)i->second;
	}
	return UserCommand::NONE;
}

UserCommand Keymap::getCommand(Key key) const {
	return getCommand(key.getCode(), getCurrentMods());
}

UserCommand Keymap::getCommand(HotKey entry) const {
	return getCommand(entry.getKey(), entry.getMod());
}

int Keymap::getCurrentMods() const {
	return	  (m_input.isShiftDown() ? ModKeys::SHIFT : 0)
			| (m_input.isCtrlDown()	 ? ModKeys::CTRL  : 0)
			| (m_input.isAltDown()   ? ModKeys::ALT   : 0)
			| (m_input.isMetaDown()  ? ModKeys::META  : 0);
}

const char* Keymap::getCommandName(UserCommand cmd) {
	assert(cmd > UserCommand::INVALID && cmd < UserCommand::COUNT);
	return UserCommandNames[cmd];
	//return commandInfo[cmd].name;
}

}}//end namespace

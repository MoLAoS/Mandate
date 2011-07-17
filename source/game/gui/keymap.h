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

#ifndef _GLEST_GAME_KEYMAP_H_
#define _GLEST_GAME_KEYMAP_H_

#include "properties.h"
#include "input.h"
#include "lang.h"
#include "sigslot.h"

using Shared::Util::Properties;
using namespace Shared::Platform;
using Shared::Platform::Key;
using Shared::Platform::KeyCode;
using Glest::Global::Lang;

namespace Glest { namespace Gui {

// =====================================================
// 	enum UserCommand
// =====================================================

STRINGY_ENUM( UserCommand,
	NONE,

	// Chat
	CHAT_AUDIENCE_ALL,
	CHAT_AUDIENCE_TEAM,
	CHAT_AUDIENCE_TOGGLE,
	SHOW_CHAT_DIALOG,

	// 
	QUIT_GAME,
	SAVE_GAME,

	// game speed/pause
	PAUSE_GAME,
	RESUME_GAME,
	TOGGLE_PAUSE,
	INC_SPEED,
	DEC_SPEED,
	RESET_SPEED,

	SAVE_SCREENSHOT,

	// camera
	ZOOM_CAMERA_IN,
	ZOOM_CAMERA_OUT,
	PITCH_CAMERA_UP,
	PITCH_CAMERA_DOWN,
	ROTATE_CAMERA_LEFT,
	ROTATE_CAMERA_RIGHT,
	CAMERA_RESET_ZOOM,
	CAMERA_RESET_ANGLE,
	CAMERA_RESET,
	MOVE_CAMERA_LEFT,
	MOVE_CAMERA_RIGHT,
	MOVE_CAMERA_UP,
	MOVE_CAMERA_DOWN,
	GOTO_SELECTION,
	GOTO_LAST_EVENT,

	// select stuff
	SELECT_IDLE_HARVESTER,
	SELECT_IDLE_BUILDER,
	SELECT_IDLE_REPAIRER,
	SELECT_IDLE_WORKER,
	SELECT_IDLE_RESTORER,
	SELECT_IDLE_PRODUCER,
	SELECT_NEXT_PRODUCER,
	SELECT_NEXT_DAMAGED,
	SELECT_NEXT_BUILT_BUILDING,
	SELECT_NEXT_STORE,

	// Commands
	ATTACK,
	STOP,
	MOVE,
	REPAIR,
	GUARD,
	FOLLOW,
	PATROL,

	// misc
	ROTATE_BUILDING,
	SHOW_LUA_CONSOLE,
	CYCLE_SHADERS,
	TOGGLE_TEAM_TINT
);

struct ModKeys {
	enum { SHIFT = 1, CTRL = 2, ALT = 4, META = 8 };
};

struct AssignmentInfo {
	unsigned short defKey1;
	unsigned short defMod1;
	unsigned short defKey2;
	unsigned short defMod2;
};

/** 
 * A single key map entry specifying a KeyCode and a set of modifiers.  Modifiers use the values
 * of the BasicKeyModifier enum as bit masks for each modifier key.
 */
class HotKey {
private:
	KeyCode m_keyCode;
	int     m_modFlags;

public:
	HotKey() : m_keyCode(KeyCode::NONE), m_modFlags(0) {}
	HotKey(KeyCode key, int mod) : m_keyCode(key), m_modFlags(mod) {}
	HotKey(const HotKey &v) : m_keyCode(v.m_keyCode), m_modFlags(v.m_modFlags) {}
	HotKey(const string &str) { init(str); }

	bool operator<(const HotKey &arg) const {
		return m_keyCode != arg.m_keyCode ? m_keyCode < arg.m_keyCode : m_modFlags < arg.m_modFlags;
	}
	bool operator==(const HotKey &arg) const {
		return m_keyCode == arg.m_keyCode && m_modFlags == arg.m_modFlags;
	}
	bool operator!=(const HotKey &arg) const {
		return !(*this == arg);
	}
	bool matches(KeyCode key, int mod) const {
		return m_keyCode == key && m_modFlags == mod;
	}
	bool isSet() const { return m_keyCode != KeyCode::NONE; }
	KeyCode getKey() const			{return m_keyCode;}
	int getMod() const				{return m_modFlags;}
	void clear() 					{m_keyCode = KeyCode::NONE; m_modFlags = 0;}
	void init(const string &str);
	string toString() const;
};

class HotKeyAssignment {
private:
	UserCommand  m_userCommand;
	HotKey       m_hotKey1;
	HotKey       m_hotKey2;

public:
	HotKeyAssignment(UserCommand userCommand, const AssignmentInfo &info)
			: m_userCommand(userCommand)
			, m_hotKey1(KeyCode(info.defKey1), info.defMod1)
			, m_hotKey2(KeyCode(info.defKey2), info.defMod2) {
	}
	HotKeyAssignment(UserCommand userCommand)
			: m_userCommand(userCommand)
			, m_hotKey1(KeyCode::NONE, 0)
			, m_hotKey2(KeyCode::NONE, 0) {
	}
	HotKeyAssignment(const HotKeyAssignment &v)
			: m_userCommand(v.m_userCommand)
			, m_hotKey1(v.m_hotKey1)
			, m_hotKey2(v.m_hotKey2) {
	}
	HotKeyAssignment()
			: m_userCommand(UserCommand::NONE)
			, m_hotKey1(KeyCode::NONE, 0)
			, m_hotKey2(KeyCode::NONE, 0) {
	}

	bool matches(KeyCode keyCode, int mod) const {
		return m_hotKey1.matches(keyCode, mod) || m_hotKey2.matches(keyCode, mod);
	}

	UserCommand getUserCommand() const { return m_userCommand; }
	HotKey getHotKey1() const { return m_hotKey1; }
	HotKey getHotKey2() const { return m_hotKey2; }
	
	void setHotKey1(HotKey hk);
	void setHotKey2(HotKey hk);

	bool isSet() const { return m_hotKey1.isSet() || m_hotKey2.isSet(); }

	void clear();
	void init(const string &str);
	string toString() const;

	sigslot::signal<HotKeyAssignment*> Modified;
};


// =====================================================
// 	class Keymap
// =====================================================

class Keymap : public sigslot::has_slots {
public:
	typedef vector<HotKeyAssignment>      HotKeyAssignments;
	typedef map<HotKey, UserCommand>      HotKeyCommandMap;
	typedef pair<HotKey, HotKey>          HotKeyPair;
	typedef map<UserCommand, HotKeyPair>  CommandHotKeyMap;

private:
	const Input&             m_input;
	const Lang&              m_lang;
	HotKeyAssignments        m_entries; // all Assignments, indexed by UserCommand
	HotKeyCommandMap         m_hotKeyCmdMap; // hotkey to user-command map
	CommandHotKeyMap         m_cmdHotKeyMap; // user-command to hotkeys map (for convenience of maintenaince)
	string                   m_filename;
	bool                     m_dirty;

	static const AssignmentInfo commandInfo[UserCommand::COUNT];

private:
	Keymap(const Keymap &);
	Keymap &operator=(const Keymap &);

public:
	Keymap(const Input &input, const char* fileName);
	//bool isMapped(Key key, UserCommand cmd) const;	
	UserCommand getCommand(HotKey hotKey) const;
	UserCommand getCommand(KeyCode key, int modFlags) const;
	UserCommand getCommand(Key key) const;
	int getCurrentMods() const;
	HotKeyAssignment& getAssignment(UserCommand uc) { return m_entries[uc]; }

	void load(const char *path = 0);
	void save(const char *path = 0);

	static const char* getCommandName(UserCommand cmd);

	sigslot::signal<bool> DirtyModified;

	void onAssignmentModified(HotKeyAssignment *assignment);
//
//private:
//	void reinit();
};

}}//end namespace

#endif // _GLEST_GAME_KEYMAP_H_

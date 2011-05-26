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

using Shared::Util::Properties;
using namespace Shared::Platform;
using Shared::Platform::Key;
using Shared::Platform::KeyCode;
using Glest::Global::Lang;

namespace Glest { namespace Gui {

// =====================================================
// 	class Keymap
// =====================================================
enum UserCommand {
	ucNone,
	ucChatAudienceAll,
	ucChatAudienceTeam,
	ucChatAudienceToggle,
	ucEnterChatMode,
	ucMenuMain,
	ucMenuQuit,
	ucMenuSave,
	ucMenuLoad,
	ucQuitNow,
	ucQuickSave,
	ucQuickLoad,
	ucPauseOn,
	ucPauseOff,
	ucPauseToggle,
	ucSpeedInc,
	ucSpeedDec,
	ucSpeedReset,
	ucNetworkStatusOn,
	ucNetworkStatusOff,
	ucNetworkStatusToggle,
	ucSaveScreenshot,
	ucCameraZoomIn,
	ucCameraZoomOut,
	ucCameraPitchUp,
	ucCameraPitchDown,
	ucCameraRotateLeft,
	ucCameraRotateRight,
	ucCameraZoomReset,
	ucCameraAngleReset,
	ucCameraZoomAndAngleReset,
	ucCameraPosLeft,
	ucCameraPosRight,
	ucCameraPosUp,
	ucCameraPosDown,
	ucCameraGotoSelection,
	ucCameraGotoLastEvent,
	ucSelectNextIdleHarvester,
	ucSelectNextIdleBuilder,
	ucSelectNextIdleRepairer,
	ucSelectNextIdleWorker,
	ucSelectNextIdleRestorer,
	ucSelectNextIdleProducer,
	ucSelectNextProducer,
	ucSelectNextDamaged,
	ucSelectNextBuiltBuilding,
	ucSelectNextStore,
	ucAttack,
	ucStop,
	ucMove,
	ucReplenish,
	ucGuard,
	ucFollow,
	ucPatrol,
	ucRotate,
	ucLuaConsole,
	ucCycleShaders,

	ucCount
};

class Keymap {
public:
	enum BasicKeyModifier {
		bkmNone		= 0x00,
		bkmShift	= 0x01,
		bkmCtrl		= 0x02,
		bkmAlt		= 0x04,
		bkmMeta		= 0x08,
	};

#pragma pack(push, 1)

	struct UserCommandInfo {
		const char *name;
		unsigned short defKey1;
		unsigned char defMod1;
		unsigned short defKey2;
		unsigned char defMod2;
	};

#pragma pack(pop)

	/** 
	 * A single key map entry specifying a KeyCode and a set of modifiers.  Modifiers use the values
	 * of the BasicKeyModifier enum as bit masks for each modifier key.
	 */
	class Entry {
	private:
		KeyCode key;
		int mod;

	public:
		Entry(KeyCode key, int mod) : key(key), mod(mod) {}
		Entry(const Entry &v) : key(v.key), mod(v.mod) {}

		bool operator <(const Entry &arg) const {
			return key != arg.key ? key < arg.key : mod < arg.mod;
		}
		bool operator ==(const Entry &arg) const {
			return key == arg.key && mod == arg.mod;
		}
		bool matches(KeyCode key, int mod) const {
			return this->key == key && this->mod == mod;
		}
		KeyCode getKey() const			{return key;}
		int getMod() const				{return mod;}
		void clear() 					{key = KeyCode::NONE; mod = bkmNone;}
		void init(const string &str);
		string toString() const;
	};
	
	class EntryPair {
		Entry a;
		Entry b;

	public:
		EntryPair(const UserCommandInfo &info) 
				: a((KeyCode)info.defKey1, (BasicKeyModifier)info.defMod1)
				, b((KeyCode)info.defKey2, (BasicKeyModifier)info.defMod2) {
		}
		EntryPair(const EntryPair &v) : a(v.a), b(v.b) {}

		bool matches(KeyCode keyCode, int mod) const {
			return a.matches(keyCode, mod) || b.matches(keyCode, mod);
		}
		const Entry &getA() const	{return a;}
		const Entry &getB() const	{return b;}
		void clear() 				{a.clear(); b.clear();}
		void init(const string &str);
		string toString() const;
	};

private:
	const Input &input;
	const Lang &lang;
	vector<EntryPair> entries;
	map<Entry, UserCommand> entryCmdMap;

	static const UserCommandInfo commandInfo[ucCount];

private:
	Keymap(const Keymap &);
	Keymap &operator=(const Keymap &);

public:
	Keymap(const Input &input, const char* fileName);
	bool isMapped(Key key, UserCommand cmd) const;	
	UserCommand getCommand(Key key) const;	
	int getCurrentMods() const;

	void load(const char *path);
	void save(const char *path);

	static const char* getCommandName(UserCommand cmd);

private:
	void reinit();
};

}}//end namespace

#endif // _GLEST_GAME_KEYMAP_H_

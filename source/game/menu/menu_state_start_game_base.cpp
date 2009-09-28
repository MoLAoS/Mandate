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
#include "menu_state_start_game_base.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "game.h"
#include "network_manager.h"
#include "xml_parser.h"

#include "leak_dumper.h"

namespace Glest { namespace Game {

using namespace Shared::Util;

// =====================================================
// 	class MenuStateStartGameBase
// =====================================================

MenuStateStartGameBase::MenuStateStartGameBase(Program &program, MainMenu *mainMenu, const string &stateName) :
		MenuState(program, mainMenu, stateName) {
	msgBox = NULL;
}

// ============ PROTECTED ===========================


void MenuStateStartGameBase::loadMapInfo(string file, MapInfo *mapInfo) {

	struct MapFileHeader {
		int32 version;
		int32 maxPlayers;
		int32 width;
		int32 height;
		int32 altFactor;
		int32 waterLevel;
		int8 title[128];
	};

	Lang &lang = Lang::getInstance();

	try {
		FILE *f = fopen(file.c_str(), "rb");

		if(!f) {
			throw runtime_error(strerror(errno));
		}

		MapFileHeader header;
		fread(&header, sizeof(MapFileHeader), 1, f);
		mapInfo->size.x = header.width;
		mapInfo->size.y = header.height;
		mapInfo->players = header.maxPlayers;
		mapInfo->desc = lang.get("MaxPlayers") + ": " + intToStr(mapInfo->players) + "\n";
		mapInfo->desc += lang.get("Size") + ": " + intToStr(mapInfo->size.x) + " x " + intToStr(mapInfo->size.y);
		fclose(f);
	} catch (exception e) {
		throw runtime_error("Error loading map file: " + file + '\n' + e.what());
	}
}

}}//end namespace

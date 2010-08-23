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
#include "projectConfig.h"
#include "game_util.h"

#include "util.h"
#include "lang.h"
#include "game_constants.h"
#include "config.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Util {
using Global::Lang;

const string mailString= "contact_game@glest.org";
const string gaeMailString= CONTACT_STRING;

const string glestVersionString= "v3.2.2";

#if _GAE_DEBUG_EDITION_ && MAD_SYNC_CHECKING
#	error MAD_SYNC_CHECKING and _GAE_DEBUG_EDITION_ not a good idea
#elif _GAE_DEBUG_EDITION_
	const string gaeVersionString = string(VERSION_STRING) + "_debug_ed";
#elif MAD_SYNC_CHECKING
	const string gaeVersionString = string(VERSION_STRING) + "_sync_test";
#else
	const string gaeVersionString = VERSION_STRING;
#endif

string getCrashDumpFileName(){
	return "gae" + gaeVersionString + ".dmp";
}

string getNetworkVersionString(){
	return gaeVersionString;
}

string getAboutString1(int i){
	switch(i){
		case 0: return "Glest Advanced Engine " + gaeVersionString;
		case 1: return "Built: " + string(__DATE__);
		case 2: return "Copyright 2001-2008 The Glest Team";
		case 3: return "Copyright 2008-2010 The GAE Team";
		default: throw runtime_error("AboutString1 #" + intToStr(i) + " does not exist!");
	}
}

string getAboutString2(int i){
	switch(i){
		case 0: return "Web: http://sf.net/apps/trac/glestae, http://glest.org";
		case 1: return "Mail: " + gaeMailString;
		case 2: return "Irc: irc://irc.freenode.net/glest";
		default: throw runtime_error("AboutString2 #" + intToStr(i) + " does not exist!");
	}
}

int getGlestTeamMemberCount() {
	return 7;
}

string getGlestTeamMemberName(int i) {
	switch(i){
		case 0: return "Martiño Figueroa";
		case 1: return "José Luis González";
		case 2: return "Tucho Fernández";
		case 3: return "José Zanni";
		case 4: return "Félix Menéndez";
		case 5: return "Marcos Caruncho";
		case 6: return "Matthias Braun";
		default: throw runtime_error("Glest Team Member " + intToStr(i) + " does not exist!");
	}
}

// Font metrics are broken for characters with diacratic... so we get measurements with these.
string getGlestTeamMemberNameNoDiacritics(int i) {
	switch(i){
		case 0: return "Martino Figueroa";
		case 1: return "Jose Luis Gonzalez";
		case 2: return "Tucho Fernandez";
		case 3: return "Jose Zanni";
		case 4: return "Felix Menendez";
		case 5: return "Marcos Caruncho";
		case 6: return "Matthias Braun";
		default: throw runtime_error("Glest Team Member " + intToStr(i) + " does not exist!");
	}
}

string getGlestTeamMemberRole(int i){
	Lang &l= Lang::getInstance();

	switch(i){
		case 0: return l.get("Programming");
		case 1: return l.get("SoundAndMusic");
		case 2: return l.get("3dAnd2dArt");
		case 3: return l.get("2dArtAndWeb");
		case 4: return l.get("Animation");
		case 5: return l.get("3dArt");
		case 6: return l.get("LinuxPort");
		default: throw runtime_error("Glest Team Member " + intToStr(i) + " does not exist!");
	}
}

int getGAETeamMemberCount() {
	return 5;
}

string getGAETeamMemberName(int i) {
	switch (i) {
		case 0: return "Daniel Santos";
		case 1: return "James McCulloch";
		case 2: return "Nathan Turner";
		case 3: return "Frank Tetzel";
		case 4: return "Eric Wilson";
		default: throw runtime_error("GAE Team Memeber " + intToStr(i) + " does not exist!");
	}
}

string getGAETeamMemberRole(int i) {
	return g_lang.get("Programming");
}

int getGAEContributorCount() {
	return 2;
}

string getGAEContributorName(int i) {
	switch (i) {
		case 0: return "Jaagup Repän";
		case 1: return "Titus Tscharntke";
		default: throw runtime_error("GAE Contributor " + intToStr(i) + " does not exist!");
	}
}

string formatString(const string &str) {
	string outStr = str;

	if (!outStr.empty()) {
		outStr[0] = toupper(outStr[0]);
	}

	bool afterSeparator = false;
	for (int i = 0; i < str.size(); ++i) {
		if (outStr[i] == '_') {
			outStr[i] = ' ';
		} else if (afterSeparator) {
			outStr[i] = toupper(outStr[i]);
			afterSeparator = false;
		}
		if (outStr[i] == '\n' || outStr[i] == '(' || outStr[i] == ' ') {
			afterSeparator = true;
		}
	}
	return outStr;
}

}}//end namespace

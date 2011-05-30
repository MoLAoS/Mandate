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
#include "projectConfig.h"
#include "game_util.h"

#include "util.h"
#include "lang.h"
#include "game_constants.h"
#include "config.h"
#include "version.h"

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
	const string gaeVersionString = string(VERSION_STRING) + "_dev_edition";
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

int getAboutStringCount() { return 7; }

string getAboutString(int i){
	switch(i){
		case 0: return "Glest Advanced Engine " + gaeVersionString;
		case 1: return "Built: " + string(build_date);
		case 2: return "Copyright 2001-2008 The Glest Team";
		case 3: return "Copyright 2008-2011 The GAE Team";
		case 4: return "Web: http://glestae.sf.net/";
		case 5: return "Mail: " + gaeMailString;
		case 6: return "Irc: irc://irc.freenode.net/glest";
		default: throw runtime_error("AboutString1 #" + intToStr(i) + " does not exist!");
	}
}

int getGlestTeamMemberCount() {
	return 7;
}

string getGlestTeamMemberField(int i, TeamMemberField field) {
	Lang &l= Lang::getInstance();
	switch(i) {
		case 0: return field == TeamMemberField::NAME ? "Marti�o Figueroa"   : l.get("Programming");
		case 1: return field == TeamMemberField::NAME ? "Jos� Luis Gonz�lez" : l.get("SoundAndMusic");
		case 2: return field == TeamMemberField::NAME ? "Tucho Fern�ndez"    : l.get("3dAnd2dArt");
		case 3: return field == TeamMemberField::NAME ? "Jos� Zanni"         : l.get("2dArtAndWeb");
		case 4: return field == TeamMemberField::NAME ? "F�lix Men�ndez"     : l.get("Animation");
		case 5: return field == TeamMemberField::NAME ? "Marcos Caruncho"    : l.get("3dArt");
		case 6: return field == TeamMemberField::NAME ? "Matthias Braun"     : l.get("LinuxPort");
		default: throw runtime_error("Glest Team Member " + intToStr(i) + " does not exist!");
	}
}

int getGAETeamMemberCount() {
	return 7;
}

string getGAETeamMemberField(int i, TeamMemberField field) {
	Lang &l= Lang::getInstance();
	switch (i) {
		case 0: return field == TeamMemberField::NAME ? "Daniel Santos"    : l.get("Programming");
		case 1: return field == TeamMemberField::NAME ? "Nathan Turner"    : l.get("Programming");
		case 2: return field == TeamMemberField::NAME ? "James McCulloch"  : l.get("Programming");
		case 3: return field == TeamMemberField::NAME ? "Frank Tetzel"     : l.get("Programming");
		case 4: return field == TeamMemberField::NAME ? "Jaagup Rep�n"     : l.get("Programming");
		case 5: return field == TeamMemberField::NAME ? "Titus Tscharntke" : l.get("Programming");
		case 6: return field == TeamMemberField::NAME ? "Eric Wilson"      : l.get("Programming");
		default: throw runtime_error("GAE Team Memeber " + intToStr(i) + " does not exist!");
	}
}

}}//end namespace

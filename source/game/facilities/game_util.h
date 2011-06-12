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

#ifndef _GLEST_GAME_GAMEUTIL_H_
#define _GLEST_GAME_GAMEUTIL_H_

#include <string>
#include <vector>

#include "util.h"

using std::string;
using Shared::Util::sharedLibVersionString;

namespace Glest { namespace Util {

extern const string mailString;
extern const string gaeMailString;
extern const string glestVersionString;
extern const string gaeVersionString;

string getNetworkVersionString();

int getAboutStringCount();
string getAboutString(int i);

WRAPPED_ENUM( TeamMemberField, NAME, ROLE );

int getGlestTeamMemberCount();
string getGlestTeamMemberField(int i, TeamMemberField field);

int getGAETeamMemberCount();
string getGAETeamMemberField(int i, TeamMemberField field);

int getContributorCount();
string getContributorField(int i, TeamMemberField field);

string getCrashDumpFileName();

}} // namespace Glest::Util

#endif

// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <cstring>

#include "game_settings.h"
#include "random.h"
#include "timer.h"
#include "leak_dumper.h"

using Shared::Util::Random;
using Shared::Platform::Chrono;

namespace Glest { namespace Game {

GameSettings::GameSettings() {}

GameSettings::GameSettings(const XmlNode *node) {
	description = node->getChildStringValue("description");
	mapPath = node->getChildStringValue("mapPath");
	tilesetPath = node->getChildStringValue("tilesetPath");
	techPath = node->getChildStringValue("techPath");
	scenarioPath = node->getChildStringValue("scenarioPath");
	thisFactionIndex = node->getChildIntValue("thisFactionIndex");
	factionCount = node->getChildIntValue("factionCount");
	fogOfWar = node->getChildBoolValue("fogOfWar");

	XmlNode *factionsNode = node->getChild("factions");
	assert(GameConstants::maxPlayers >= factionsNode->getChildCount());
	for (int i = 0; i < factionsNode->getChildCount(); ++i) {
		XmlNode *factionNode = factionsNode->getChild("faction", i);

		factionTypeNames[i] = factionNode->getChildStringValue("type");
		playerNames[i] = factionNode->getChildStringValue("playerName");
		factionControls[i] = enum_cast<ControlType>(factionNode->getChildIntValue("control"));
		teams[i] = factionNode->getChildIntValue("team");
		startLocationIndex[i] = factionNode->getChildIntValue("startLocationIndex");
	}
}

void GameSettings::save(XmlNode *node) const {
	node->addChild("description", description);
	node->addChild("mapPath", mapPath);
	node->addChild("tilesetPath", tilesetPath);
	node->addChild("techPath", techPath);
	node->addChild("scenarioPath", scenarioPath);
	node->addChild("thisFactionIndex", thisFactionIndex);
	node->addChild("factionCount", factionCount);
	node->addChild("fogOfWar", fogOfWar);

	XmlNode *factionsNode = node->addChild("factions");
	for (int i = 0; i < factionCount; ++i) {
		XmlNode *factionNode = factionsNode->addChild("faction");
		factionNode->addChild("type", factionTypeNames[i]);
		factionNode->addChild("playerName", playerNames[i]);
		factionNode->addChild("control", factionControls[i]);
		factionNode->addChild("team", teams[i]);
		factionNode->addChild("startLocationIndex", startLocationIndex[i]);
	}
}

void GameSettings::randomizeLocs(int maxPlayers) {
	bool *slotUsed = new bool[maxPlayers];
	Random rand;

	memset(slotUsed, 0, sizeof(bool)*maxPlayers);
	rand.init(Shared::Platform::Chrono::getCurMillis ());

	for(int i = 0; i < maxPlayers; i++) {
		int slot = rand.randRange(0, 3 - i);
		for(int j = slot; j < slot + maxPlayers; j++) {
			int k = j % maxPlayers;
			if(!slotUsed[j % maxPlayers]) {
				slotUsed[j % maxPlayers] = true;
				startLocationIndex[k] = i;
				break;
			}
		}
	}
	delete [] slotUsed;
}

}}//end namespace

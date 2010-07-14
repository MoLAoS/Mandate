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

namespace Glest { namespace Sim {

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
		colourIndices[i] = factionNode->getChildIntValue("colourIndex");
	}
}

void GameSettings::clear() {
	description = "";
	mapPath = "";
	tilesetPath = "";
	techPath = "";
	scenarioPath = "";

	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		factionTypeNames[i] = "";
		playerNames[i] = "";
		factionControls[i] = ControlType::CLOSED;
		resourceMultipliers[i] = 1.f;
		teams[i] = -1;
		startLocationIndex[i] = -1;
		colourIndices[i] = -1;
	}
	thisFactionIndex = -1;
	defaultUnits = true;
	defaultResources = true;
	defaultVictoryConditions = true;
	fogOfWar = true;
	randomStartLocs = false;
}

void GameSettings::compact() {
	bool slotFlags[GameConstants::maxPlayers];
	int count = 0;
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		slotFlags[i] = factionControls[i] != ControlType::CLOSED;
		if (slotFlags[i]) ++count;
	}
	factionCount = count;
	for (int i = 0; i < GameConstants::maxPlayers; ) {
		if (!slotFlags[i]) { // need to shuffle everything from here down one slot
			bool done = true; // if everything else is false, we're finished
			for (int j = i + 1; j < GameConstants::maxPlayers; ++j) {
				factionTypeNames[j - 1] = factionTypeNames[j];
				playerNames[j - 1] = playerNames[j];
				factionControls[j - 1] = factionControls[j];
				resourceMultipliers[j - 1] = resourceMultipliers[j];
				teams[j - 1] = teams[j];
				startLocationIndex[j - 1] = startLocationIndex[j];
				colourIndices[j - 1] = colourIndices[j];
				if (thisFactionIndex == j) --thisFactionIndex;
				slotFlags[j - 1] = slotFlags[j];
				if (slotFlags[j]) done = false;
			}
			// copied last entry down, but didn't copy over it, reset
			int k = GameConstants::maxPlayers - 1;
			factionTypeNames[k] = "";
			playerNames[k] = "";
			factionControls[k] = ControlType::CLOSED;
			resourceMultipliers[k] = 1.f;
			teams[k] = -1;
			startLocationIndex[k] = -1;
			slotFlags[k] = false;

			if (done) break;
		} else {
			++i;
		}
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
	node->addChild("randomStartLocs", randomStartLocs);

	XmlNode *factionsNode = node->addChild("factions");
	for (int i = 0; i < factionCount; ++i) {
		XmlNode *factionNode = factionsNode->addChild("faction");
		factionNode->addChild("type", factionTypeNames[i]);
		factionNode->addChild("playerName", playerNames[i]);
		factionNode->addChild("control", factionControls[i]);
		factionNode->addChild("team", teams[i]);
		factionNode->addChild("startLocationIndex", startLocationIndex[i]);
		factionNode->addChild("colourIndex", colourIndices[i]);
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

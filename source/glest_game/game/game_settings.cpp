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

#include "game_settings.h"
#include "random.h"

using Shared::Util::Random;

namespace Glest { namespace Game {

GameSettings::GameSettings(const XmlNode *node){
	description = node->getChildStringValue("description");
	mapPath = node->getChildStringValue("mapPath");
	tilesetPath = node->getChildStringValue("tilesetPath");
	techPath = node->getChildStringValue("techPath");
	thisFactionIndex = node->getChildIntValue("thisFactionIndex");
	factionCount = node->getChildIntValue("factionCount");

	XmlNode *factionsNode = node->getChild("factions");
	assert(GameConstants::maxPlayers == factionsNode->getChildCount());
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		XmlNode *factionNode = factionsNode->getChild("faction", i);

		factionTypeNames[i] = factionNode->getChildStringValue("type");
		factionControls[i] = (ControlType)factionNode->getChildIntValue("control");
		teams[i] = factionNode->getChildIntValue("team");
		startLocationIndex[i] = factionNode->getChildIntValue("startLocationIndex");
	}
}

void GameSettings::save(XmlNode *node) const {
	node->addChild("description", description);
	node->addChild("mapPath", mapPath);
	node->addChild("tilesetPath", tilesetPath);
	node->addChild("techPath", techPath);
	node->addChild("thisFactionIndex", thisFactionIndex);
	node->addChild("factionCount", factionCount);

	XmlNode *factionsNode = node->addChild("factions");
	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
		XmlNode *factionNode = factionsNode->addChild("faction");
		factionNode->addChild("type", factionTypeNames[i]);
		factionNode->addChild("control", factionControls[i]);
		factionNode->addChild("team", teams[i]);
		factionNode->addChild("startLocationIndex", startLocationIndex[i]);
	}
}

void GameSettings::randomizeLocs() {
	bool slotUsed[GameConstants::maxPlayers];
	Random rand;

	memset(slotUsed, 0, sizeof(slotUsed));
	rand.init(1234);

	for(int i = 0; i < GameConstants::maxPlayers; i++) {
		int slot = rand.randRange(0, 3 - i);
		for(int j = slot; j < slot + GameConstants::maxPlayers; j++) {
			int k = j % GameConstants::maxPlayers;
			if(!slotUsed[j % GameConstants::maxPlayers]) {
				slotUsed[j % GameConstants::maxPlayers] = true;
				startLocationIndex[k] = i;
				break;
			}
		}
	}
}

}}//end namespace

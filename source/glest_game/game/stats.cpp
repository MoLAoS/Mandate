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
#include "stats.h"

#include "leak_dumper.h"


namespace Glest{ namespace Game{

Stats::PlayerStats::PlayerStats(){
	victory= false;

	kills= 0;
	deaths= 0;
	unitsProduced= 0;
	resourcesHarvested= 0;
}

// =====================================================
// class Stats
// =====================================================

void Stats::init(int factionCount, int thisFactionIndex, const string& description){
	this->thisFactionIndex= thisFactionIndex;
	this->factionCount= factionCount;
	this->description= description;
}

void Stats::load(const XmlNode *node) {
	for(int i = 0; i < factionCount; ++i) {
		const XmlNode *n = node->getChild("player", i);
		playerStats[i].control = (ControlType)n->getChildIntValue("control");
		playerStats[i].factionTypeName = n->getChildStringValue("factionTypeName");
		playerStats[i].teamIndex = n->getChildIntValue("teamIndex");
		playerStats[i].victory = n->getChildBoolValue("victory");
		playerStats[i].kills = n->getChildIntValue("kills");
		playerStats[i].deaths = n->getChildIntValue("deaths");
		playerStats[i].unitsProduced = n->getChildIntValue("unitsProduced");
		playerStats[i].resourcesHarvested = n->getChildIntValue("resourcesHarvested");
	}
}

void Stats::save(XmlNode *node) const {
	for(int i = 0; i < factionCount; ++i) {
		XmlNode *n = node->addChild("player");
		n->addChild("control", playerStats[i].control);
		n->addChild("factionTypeName", playerStats[i].factionTypeName);
		n->addChild("teamIndex", playerStats[i].teamIndex);
		n->addChild("victory", playerStats[i].victory);
		n->addChild("kills", playerStats[i].kills);
		n->addChild("deaths", playerStats[i].deaths);
		n->addChild("unitsProduced", playerStats[i].unitsProduced);
		n->addChild("resourcesHarvested", playerStats[i].resourcesHarvested);
	}
}

void Stats::setVictorious(int playerIndex){
	playerStats[playerIndex].victory= true;
}

void Stats::kill(int killerFactionIndex, int killedFactionIndex){
	playerStats[killerFactionIndex].kills++;
	playerStats[killedFactionIndex].deaths++;
}

void Stats::produce(int producerFactionIndex){
	playerStats[producerFactionIndex].unitsProduced++;
}

void Stats::harvest(int harvesterFactionIndex, int amount){
	playerStats[harvesterFactionIndex].resourcesHarvested+= amount;
}

}}//end namespace

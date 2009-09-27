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

#ifndef _GLEST_GAME_STATS_H_
#define _GLEST_GAME_STATS_H_

#include <string>

#include "game_constants.h"
#include "faction.h"
#include "xml_parser.h"
#include "game_settings.h"

using std::string;
using Shared::Xml::XmlNode;

namespace Glest { namespace Game {

// =====================================================
// 	class Stats
// =====================================================

/** Player statistics that are shown after the game ends */
class Stats {
public:
	struct PlayerStats {
		PlayerStats();

		bool victory;
		int kills;
		int deaths;
		int unitsProduced;
		int resourcesHarvested;
	};

private:
	GameSettings gs;
	PlayerStats playerStats[GameConstants::maxPlayers];

public:
	Stats(const GameSettings &gs) : gs(gs), playerStats() {}
	void load(const XmlNode *n);
	void save(XmlNode *n) const;

	const GameSettings & getGameSettings() const	{return gs;}
	bool getVictory(int i) const					{return playerStats[i].victory;}
	int getKills(int i) const						{return playerStats[i].kills;}
	int getDeaths(int i) const						{return playerStats[i].deaths;}
	int getUnitsProduced(int i) const				{return playerStats[i].unitsProduced;}
	int getResourcesHarvested(int i) const			{return playerStats[i].resourcesHarvested;}


	void setVictorious(int i)						{playerStats[i].victory = true;}
	void produce(int i)								{playerStats[i].unitsProduced++;}
	void harvest(int i, int amount)					{playerStats[i].resourcesHarvested += amount;}
	void kill(int killerIndex, int killedIndex) {
		if(killerIndex != killedIndex) {
			playerStats[killerIndex].kills++;
		}
		playerStats[killedIndex].deaths++;
	}
};

}}//end namespace

#endif

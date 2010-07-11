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

#ifndef _GLEST_GAME_STATS_H_
#define _GLEST_GAME_STATS_H_

#include <string>

#include "game_constants.h"
#include "faction.h"
#include "xml_parser.h"
#include "game_settings.h"

using std::string;
using Shared::Xml::XmlNode;

namespace Glest { namespace Sim {

class SimulationInterface;

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
	PlayerStats playerStats[GameConstants::maxPlayers];

	SimulationInterface *iSim;

public:
	Stats(SimulationInterface *si) : playerStats(), iSim(si) {}
	void load(const XmlNode *n);
	void save(XmlNode *n) const;

	bool getVictory(int i) const					{assert(i >= 0); return playerStats[i].victory;}
	int getKills(int i) const						{assert(i >= 0); return playerStats[i].kills;}
	int getDeaths(int i) const						{assert(i >= 0); return playerStats[i].deaths;}
	int getUnitsProduced(int i) const				{assert(i >= 0); return playerStats[i].unitsProduced;}
	int getResourcesHarvested(int i) const			{assert(i >= 0); return playerStats[i].resourcesHarvested;}


	void setVictorious(int i)						{assert(i >= 0); playerStats[i].victory = true;}
	void produce(int i)								{assert(i >= 0); playerStats[i].unitsProduced++;}
	void harvest(int i, int amount)					{assert(i >= 0); playerStats[i].resourcesHarvested += amount;}
	void kill(int killerIndex, int killedIndex) {
		if (killerIndex == -1) {
			return; // glestimal
		}
		if(killerIndex != killedIndex) {
			playerStats[killerIndex].kills++;
		}
		playerStats[killedIndex].deaths++;
	}
};

}}//end namespace

#endif

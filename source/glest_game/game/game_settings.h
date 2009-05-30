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

#ifndef _GLEST_GAME_GAMESETTINGS_H_
#define _GLEST_GAME_GAMESETTINGS_H_

#include <string>
#include "game_constants.h"
#include "xml_parser.h"

using std::string;
using Shared::Xml::XmlNode;

namespace Glest { namespace Game {

// =====================================================
//	class GameSettings
// =====================================================

class GameSettings {
private:
	string description;
	string mapPath;
	string tilesetPath;
	string techPath;
	string factionTypeNames[GameConstants::maxPlayers]; //faction names
	string playerNames[GameConstants::maxPlayers];

	ControlType factionControls[GameConstants::maxPlayers];

	int thisFactionIndex;
	int factionCount;
	int teams[GameConstants::maxPlayers];
	int startLocationIndex[GameConstants::maxPlayers];

public:
	GameSettings(){}
	GameSettings(const XmlNode *node);
	// use default copy ctor
	//GameSettings(const GameSettings &gs);

	//get
	const string &getDescription() const						{return description;}
	const string &getMapPath() const 							{return mapPath;}
	const string &getTilesetPath() const						{return tilesetPath;}
	const string &getTechPath() const							{return techPath;}
	const string &getFactionTypeName(int i) const				{return factionTypeNames[i];}
	const string &getPlayerName(int i) const					{return playerNames[i];}
	ControlType getFactionControl(int i) const					{return factionControls[i];}
	int getThisFactionIndex() const								{return thisFactionIndex;}
	int getFactionCount() const									{return factionCount;}
	int getTeam(int i) const									{return teams[i];}
	int getStartLocationIndex(int i) const						{return startLocationIndex[i];}

	//set
	void setDescription(const string& description)				{this->description = description;}
	void setMapPath(const string& mapPath)						{this->mapPath = mapPath;}
	void setTilesetPath(const string& tilesetPath)				{this->tilesetPath = tilesetPath;}
	void setTechPath(const string& techPath)					{this->techPath = techPath;}
	void setFactionTypeName(int i, const string& name)			{this->factionTypeNames[i] = name;}
	void setPlayerName(int i, const string &name)				{this->playerNames[i] = name;}
	void setFactionControl(int i, ControlType controller)		{this->factionControls[i]= controller;}
	void setThisFactionIndex(int thisFactionIndex) 				{this->thisFactionIndex = thisFactionIndex;}
	void setFactionCount(int factionCount)						{this->factionCount = factionCount;}
	void setTeam(int i, int team)								{this->teams[i] = team;}
	void setStartLocationIndex(int i, int startLocationIndex)	{this->startLocationIndex[i] = startLocationIndex;}

	//misc
	void randomizeLocs();
	void save(XmlNode *node) const;
};

}}//end namespace

#endif

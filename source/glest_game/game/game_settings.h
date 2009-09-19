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
	string map;
	string tileset;
	string tech;
	string mapPath;
	string tilesetPath;
	string techPath;
	string factionTypeNames[GameConstants::maxPlayers]; //faction names
	string playerNames[GameConstants::maxPlayers];
	string scenario;
	string scenarioDir;

	ControlType factionControls[GameConstants::maxPlayers];

	int thisFactionIndex;
	int factionCount;
	int teams[GameConstants::maxPlayers];
	int startLocationIndex[GameConstants::maxPlayers];

   bool defaultUnits;
   bool defaultResources;
   bool defaultVictoryConditions;

public:
	GameSettings(){}
	GameSettings(const XmlNode *node);
	// use default copy ctor
	//GameSettings(const GameSettings &gs);

	//get
	const string &getDescription() const						{return description;}
	const string &getMap() const 								{return map;}
	const string &getTileset() const							{return tileset;}
	const string &getTech() const								{return tech;}
	const string &getMapPath() const 							{return mapPath;}
	const string &getTilesetPath() const						{return tilesetPath;}
	const string &getTechPath() const							{return techPath;}
	const string &getScenario() const							{return scenario;}
	const string &getScenarioDir() const						{return scenarioDir;}
   
	const string &getFactionTypeName(int i) const				{return factionTypeNames[i];}
	const string &getPlayerName(int i) const					{return playerNames[i];}
	ControlType getFactionControl(int i) const					{return factionControls[i];}
	int getThisFactionIndex() const								{return thisFactionIndex;}
	int getFactionCount() const									{return factionCount;}
	int getTeam(int i) const									{return teams[i];}
	int getStartLocationIndex(int i) const						{return startLocationIndex[i];}

   bool getDefaultUnits() const				{return defaultUnits;}
   bool getDefaultResources() const			{return defaultResources;}
   bool getDefaultVictoryConditions() const	{return defaultVictoryConditions;}

	//set
	void setDescription(const string& description)				{this->description = description;}
	void setMap(const string& map)						{this->map = map; this->mapPath = "maps/" + map + ".gbm";}
	void setTileset(const string& tileset)				{this->tileset = tileset; this->tilesetPath = "tilesets/" + tileset;}
	void setTech(const string& tech)					{this->tech = tech; this->techPath = "techs/" + tech;}
	void setScenario(const string& scenario)							{this->scenario= scenario;}
	void setScenarioDir(const string& scenarioDir)						{this->scenarioDir= scenarioDir;}
	void setFactionTypeName(int i, const string& name)			{this->factionTypeNames[i] = name;}
	void setPlayerName(int i, const string &name)				{this->playerNames[i] = name;}
	void setFactionControl(int i, ControlType controller)		{this->factionControls[i]= controller;}
	void setThisFactionIndex(int thisFactionIndex) 				{this->thisFactionIndex = thisFactionIndex;}
	void setFactionCount(int factionCount)						{this->factionCount = factionCount;}
	void setTeam(int i, int team)								{this->teams[i] = team;}
	void setStartLocationIndex(int i, int startLocationIndex)	{this->startLocationIndex[i] = startLocationIndex;}

	void setDefaultUnits(bool defaultUnits) 						{this->defaultUnits= defaultUnits;}
	void setDefaultResources(bool defaultResources) 				{this->defaultResources= defaultResources;}
	void setDefaultVictoryConditions(bool defaultVictoryConditions) {this->defaultVictoryConditions= defaultVictoryConditions;}

	//misc
	void randomizeLocs(int maxPlayers);
	void save(XmlNode *node) const;
};

}}//end namespace

#endif

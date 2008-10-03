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

namespace Glest{ namespace Game{

// =====================================================
//	class GameSettings
// =====================================================

class GameSettings{
private:
	string description;
	string mapPath;
	string tilesetPath;
	string techPath;
	string factionTypeNames[GameConstants::maxPlayers]; //faction names

	ControlType factionControls[GameConstants::maxPlayers];

	int thisFactionIndex;
	int factionCount;
	int teams[GameConstants::maxPlayers];
	int startLocationIndex[GameConstants::maxPlayers];

public:
	GameSettings(){}
	GameSettings(const XmlNode *node);
	//get
	const string &getDescription() const						{return description;}
	const string &getMapPath() const 							{return mapPath;}
	const string &getTilesetPath() const						{return tilesetPath;}
	const string &getTechPath() const							{return techPath;}
	const string &getFactionTypeName(int factionIndex) const	{return factionTypeNames[factionIndex];}

	ControlType getFactionControl(int factionIndex) const	{return factionControls[factionIndex];}

	int getThisFactionIndex() const							{return thisFactionIndex;}
	int getFactionCount() const								{return factionCount;}
	int getTeam(int factionIndex) const						{return teams[factionIndex];}
	int getStartLocationIndex(int factionIndex) const		{return startLocationIndex[factionIndex];}

	//set
	void setDescription(const string& description)		{this->description= description;}
	void setMapPath(const string& mapPath)				{this->mapPath= mapPath;}
	void setTilesetPath(const string& tilesetPath)		{this->tilesetPath= tilesetPath;}
	void setTechPath(const string& techPath)			{this->techPath= techPath;}

	void setFactionTypeName(int factionIndex, const string& factionTypeName)	{this->factionTypeNames[factionIndex]= factionTypeName;}

	void setFactionControl(int factionIndex, ControlType controller)			{this->factionControls[factionIndex]= controller;}

	void setThisFactionIndex(int thisFactionIndex) 							{this->thisFactionIndex= thisFactionIndex;}
	void setFactionCount(int factionCount)									{this->factionCount= factionCount;}
	void setTeam(int factionIndex, int team)								{this->teams[factionIndex]= team;}
	void setStartLocationIndex(int factionIndex, int startLocationIndex)	{this->startLocationIndex[factionIndex]= startLocationIndex;}

	void randomizeLocs();

	void save(XmlNode *node) const;
};

}}//end namespace

#endif

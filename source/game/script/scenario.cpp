// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "scenario.h"

#include "logger.h"
#include "xml_parser.h"
#include "util.h"
#include "game_util.h"
#include "leak_dumper.h"
#include "simulation_enums.h"
#include "sim_interface.h"
#include "lang.h"

using std::exception;
using namespace Shared::Xml;
using namespace Shared::Util;

namespace Glest { namespace Script {

using namespace Util;
using Sim::SimulationInterface;
using Global::Lang;

// =====================================================
//	class Scenario
// =====================================================

Scenario::~Scenario(){

}

void Scenario::load(const string &path){
	try{
		string name= dirname(basename(path));
		g_logger.addProgramMsg("Scenario: " + formatString(name), true);

		//parse xml
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *scenarioNode= xmlTree.getRootNode();
		const XmlNode *scriptsNode= scenarioNode->getChild("scripts");

		for(int i= 0; i<scriptsNode->getChildCount(); ++i){
			const XmlNode *scriptNode = scriptsNode->getChild(i);

			scripts.push_back(Script(getFunctionName(scriptNode), scriptNode->getText()));
		}
	}
	//Exception handling (conversions and so on);
	catch(const exception &e){
		throw runtime_error("Error: " + path + "\n" + e.what());
	}
}

/*string Scenario::getScenarioPath(const string &scenarioPath) {
	return scenarioPath + "/" + basename(scenarioPath) + ".xml";
}*/

void Scenario::loadScenarioInfo(string scenario, string category, ScenarioInfo *scenarioInfo) {
	XmlTree xmlTree;
	//gae/scenarios/[category]/[scenario]/[scenario].xml
	xmlTree.load("gae/scenarios/" + category + "/" + scenario + "/" + scenario + ".xml");

	const XmlNode *scenarioNode = xmlTree.getRootNode();
	const XmlNode *difficultyNode = scenarioNode->getChild("difficulty");
	scenarioInfo->difficulty = difficultyNode->getAttribute("value")->getIntValue();

	if (scenarioInfo->difficulty < dVeryEasy || scenarioInfo->difficulty > dInsane) {
		throw std::runtime_error("Invalid difficulty");
	}

	scenarioInfo->fogOfWar = scenarioNode->getOptionalBoolValue("fog-of-war", true);
	scenarioInfo->shroudOfDarkness = scenarioNode->getOptionalBoolValue("shroud-of-darkness", true);

	const XmlNode *playersNode = scenarioNode->getChild("players");
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		const XmlNode *playerNode;
		try{
			playerNode = playersNode->getChild("player", i);
		}catch(runtime_error err){
			// old scenario -> has only 4 players
			scenarioInfo->factionControls[i] = strToControllerType("closed");
			continue;
		}
		ControlType factionControl = strToControllerType(playerNode->getAttribute("control")->getValue());
		string factionTypeName;

		scenarioInfo->factionControls[i] = factionControl;

		if (factionControl != ControlType::CLOSED) {
			int teamIndex = playerNode->getAttribute("team")->getIntValue();
			XmlAttribute *nameAttrib = playerNode->getAttribute("name", false);
			XmlAttribute *resMultAttrib = playerNode->getAttribute("resource-multiplier", false);
			if (nameAttrib) {
				scenarioInfo->playerNames[i] = nameAttrib->getValue();
			} else if (factionControl == ControlType::HUMAN) {
				scenarioInfo->playerNames[i] = Config::getInstance().getNetPlayerName();
			} else {
				scenarioInfo->playerNames[i] = "CPU Player";
			}
			if (resMultAttrib) {
				scenarioInfo->resourceMultipliers[i] = resMultAttrib->getFloatValue();
			} else {
				if (factionControl == ControlType::CPU_MEGA) {
					scenarioInfo->resourceMultipliers[i] = 4.f;
				}
				else if (factionControl == ControlType::CPU_ULTRA) {
					scenarioInfo->resourceMultipliers[i] = 3.f;
				}
				else {
					scenarioInfo->resourceMultipliers[i] = 1.f;
				}
			}
			if (teamIndex < 1 || teamIndex > GameConstants::maxPlayers) {
				throw runtime_error("Team out of range: " + intToStr(teamIndex));
			}

			scenarioInfo->teams[i] = playerNode->getAttribute("team")->getIntValue();
			scenarioInfo->factionTypeNames[i] = playerNode->getAttribute("faction")->getValue();
		}
	}
	scenarioInfo->mapName = scenarioNode->getChild("map")->getAttribute("value")->getValue();
	scenarioInfo->tilesetName = scenarioNode->getChild("tileset")->getAttribute("value")->getValue();
	scenarioInfo->techTreeName = scenarioNode->getChild("tech-tree")->getAttribute("value")->getValue();
	scenarioInfo->defaultUnits = scenarioNode->getChild("default-units")->getAttribute("value")->getBoolValue();
	scenarioInfo->defaultResources = scenarioNode->getChild("default-resources")->getAttribute("value")->getBoolValue();
	scenarioInfo->defaultVictoryConditions = scenarioNode->getChild("default-victory-conditions")->getAttribute("value")->getBoolValue();

	//add player info
	scenarioInfo->desc = g_lang.get("Player") + ": ";
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		if (scenarioInfo->factionControls[i] == ControlType::HUMAN) {
			scenarioInfo->desc += formatString(scenarioInfo->factionTypeNames[i]);
			break;
		}
	}

	//add misc info
	string difficultyString = "Difficulty" + intToStr(scenarioInfo->difficulty);

	scenarioInfo->desc += "\n";
	scenarioInfo->desc += g_lang.get("Difficulty") + ": " + g_lang.get(difficultyString) + "\n";
	scenarioInfo->desc += g_lang.get("Map") + ": " + formatString(scenarioInfo->mapName) + "\n";
	scenarioInfo->desc += g_lang.get("Tileset") + ": " + formatString(scenarioInfo->tilesetName) + "\n";
	scenarioInfo->desc += g_lang.get("TechTree") + ": " + formatString(scenarioInfo->techTreeName) + "\n";
}

void Scenario::loadGameSettings(string scenario, string category, const ScenarioInfo *scenarioInfo) {
	GameSettings *gs = &g_simInterface.getGameSettings();
	gs->clear();

	string scenarioPath = "gae/scenarios/" + category + "/" + scenario;
	// map in scenario dir ?
	string test = scenarioPath + "/" + scenarioInfo->mapName;
	if (fileExists(test + ".gbm") || fileExists(test + ".mgm")) {
		gs->setMapPath(test);
	} else {
		gs->setMapPath(string("maps/") + scenarioInfo->mapName);
	}
	gs->setDescription(formatString(scenario));
	gs->setTilesetPath(string("tilesets/") + scenarioInfo->tilesetName);
	gs->setTechPath(string("techs/") + scenarioInfo->techTreeName);
	gs->setScenarioPath(scenarioPath);
	gs->setDefaultUnits(scenarioInfo->defaultUnits);
	gs->setDefaultResources(scenarioInfo->defaultResources);
	gs->setDefaultVictoryConditions(scenarioInfo->defaultVictoryConditions);

	int factionCount = 0;
	for (int i = 0; i < GameConstants::maxPlayers; ++i) {
		ControlType ct = static_cast<ControlType>(scenarioInfo->factionControls[i]);
		if (ct != ControlType::CLOSED) {
			if (ct == ControlType::HUMAN) {
				gs->setThisFactionIndex(factionCount);
			}
			gs->setFactionControl(factionCount, ct);
			gs->setPlayerName(factionCount, scenarioInfo->playerNames[i]);
			gs->setTeam(factionCount, scenarioInfo->teams[i] - 1);
			gs->setStartLocationIndex(factionCount, i);
			gs->setFactionTypeName(factionCount, scenarioInfo->factionTypeNames[i]);
			gs->setResourceMultiplier(factionCount, scenarioInfo->resourceMultipliers[i]);
			gs->setColourIndex(i, i);
			factionCount++;
		}
	}
	gs->setFogOfWar(scenarioInfo->fogOfWar);
	gs->setShroudOfDarkness(scenarioInfo->shroudOfDarkness);
	gs->setFactionCount(factionCount);
}

//REFACTOR: Delete. Use ControlTypeNames.match()
ControlType Scenario::strToControllerType(const string &str) {
	if (str == "closed") {
		return ControlType::CLOSED;
	} else if (str == "cpu-easy") {
		return ControlType::CPU_EASY;
	} else if (str == "cpu") {
		return ControlType::CPU;
	} else if (str == "cpu-ultra") {
		return ControlType::CPU_ULTRA;
	} else if (str == "cpu-mega") {
		return ControlType::CPU_MEGA;
	} else if (str == "human") {
		return ControlType::HUMAN;
	}

	throw std::runtime_error("Unknown controller type: " + str);
}


string Scenario::getFunctionName(const XmlNode *scriptNode){
	string name= scriptNode->getName();

	for(int i= 0; i<scriptNode->getAttributeCount(); ++i){
		name+= "_" + scriptNode->getAttribute(i)->getValue();
	}
	return name;
}

}}//end namespace

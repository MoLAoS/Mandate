// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//                2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SCRIPT_MANAGER_H_
#define _GLEST_GAME_SCRIPT_MANAGER_H_

#include "trigger_manager.h"

namespace Glest { namespace Script {

using Sim::CmdResult;
using Sim::CmdResultNames;

class PlayerModifiers {
private:
	bool winner;
	bool aiEnabled;
	bool consumeEnabled;

public:
	PlayerModifiers() : winner(false), aiEnabled(true), consumeEnabled(true) { }

	bool getWinner() const		{return winner;}
	bool getAiEnabled() const	{return aiEnabled;}
	bool getConsumeEnabled() const {return consumeEnabled;}

	void enableAi(bool v)		{aiEnabled = v;}
	void enableConsume(bool v)	{consumeEnabled = v;}
	void setAsWinner()			{winner = true;}
};

struct LuaCmdResult {
	enum Enum {
		INSUFFICIENT_SPACE = -9,
		RESOURCE_NOT_FOUND,
		PRODUCIBLE_NOT_FOUND,
		NO_COMMAND_FOR_TARGET,
		NO_CAPABLE_COMMAND,
		INVALID_FACTION_INDEX,
		INVALID_POSITION,
		INVALID_COMMAND_CLASS,
		INVALID_UNIT_ID,
		OK // == 0
	};

	Enum val;
	LuaCmdResult() : val(OK) {}
	LuaCmdResult(int i) : val(Enum(i)) {}

	string getString(bool unitCommand = false) {
		switch (val) {
			case INSUFFICIENT_SPACE:
				return "Insufficient space near position.";
			case RESOURCE_NOT_FOUND:
				return "Resource type not found.";
			case PRODUCIBLE_NOT_FOUND:
				return "Producible type not found.";
			case NO_COMMAND_FOR_TARGET:
				return "No command to use with that target.";
			case NO_CAPABLE_COMMAND:
				return "No command capable of doing that.";
			case INVALID_FACTION_INDEX:
				return "Invalid faction index.";
			case INVALID_POSITION:
				return "Invalid position.";
			case INVALID_COMMAND_CLASS:
				return "Invalid command class.";
			case INVALID_UNIT_ID:
				return "Invalid unit id";
			case OK:
				return "Ok.";
			default:
				if (unitCommand) {
					CmdResult res(val);
					return string("CmdResult == ") + CmdResultNames[res];
				} else {
					return "Result = " + intToStr(val);
				}
		}
	}
};

// =====================================================
//	class ScriptManager
// =====================================================

//REFACTOR: namespace ScriptManager [hide all the private stuff in the cpp]
class ScriptManager {
public:
	//lua
	static string code;
	static LuaScript luaScript;

	//misc
	static LuaConsole *luaConsole;

	//last created unit & last dead unit
	static struct UnitInfo {
		string name;
		int id;
	} latestCreated, latestCasualty;

	// end game state
	static bool gameOver;
	static PlayerModifiers playerModifiers[GameConstants::maxPlayers];

	static vector<ScriptTimer> timers;
	static vector<ScriptTimer> newTimerQueue;
	static set<string> definedEvents;
	static TriggerManager triggerManager;

	static const int messageWrapCount;
	static const int displayTextWrapCount;

	static map<string, Vec3f> actorColours;

public:
	static void cleanUp();
	static void initGame();

	static void doSomeLua(const string &code);

	static bool getGameOver() 											{return gameOver;}
	static const PlayerModifiers *getPlayerModifiers(int factionIndex) 	{return &playerModifiers[factionIndex];}

	//events
	static void onResourceHarvested(const Unit *unit);
	static void onUnitCreated(const Unit* unit);
	static void onUnitDied(const Unit* unit);

	static void update();

	static void onTrigger(const string &name, int unitId, int userData=0);
	static void unitMoved(Unit *unit) { triggerManager.unitMoved(unit); }
	static void commandCallback(const Unit *unit) { triggerManager.commandCallback(unit); }
	static void onHPBelowTrigger(const Unit *unit) { triggerManager.onHPBelow(unit); }
	static void onHPAboveTrigger(const Unit *unit) { triggerManager.onHPAbove(unit); }
	static void onAttackedTrigger(const Unit *unit){ triggerManager.onAttacked(unit); }

	static void addErrorMessage(const char *txt=NULL, bool quietly = true);
	static void addErrorMessage(const string &txt) {
		addErrorMessage(txt.c_str());
	}

	static int panicFunc(LuaHandle* luaHandle);

private:
	typedef const char* c_str;

public:
	static bool extractArgs(LuaArguments &args, c_str caller, c_str format, ...);

private:
	//
	// LUA callbacks
	//

	// unit trigger helper...
	static void doUnitTrigger(int id, const string &cond, const string &evnt, int ud);

	// Timers, Triggers, Events...
	static int setTimer(LuaHandle* luaHandle);			// Game.setTimer()
	static int stopTimer(LuaHandle* luaHandle);			// Game.stopTimer()
	static int registerRegion(LuaHandle* luaHandle);	// Game.resgisterRegion()
	static int registerEvent(LuaHandle* luaHandle);		// Game.resgisterEvent()
	static int setUnitTrigger(LuaHandle* luaHandle);	// Unit:setTrigger()
	static int setUnitTriggerX(LuaHandle* luaHandle);	// Unit:setTrigger()
	static int setFactionTrigger(LuaHandle* luaHandle);	// Faction:setTrigger()
	//static int setTeamTrigger(LuaHandle* luaHandle);	//
	static int removeUnitPosTriggers(LuaHandle* luaHandle);

	// messages
	static int showMessage(LuaHandle* luaHandle);		// Gui.showMessage()
	static int setDisplayText(LuaHandle* luaHandle);	// Gui.setDisplayText()
	static int clearDisplayText(LuaHandle* luaHandle);	// Gui.clearDisplayText()
	static int consoleMsg(LuaHandle* luaHandle);		// Gui.consoleMsg()
	static int addActor(LuaHandle* luaHandle);
	static int addDialog(LuaHandle* luaHandle);

	// gui
	static int lockInput(LuaHandle* luaHandle);			// Gui.lockInput()
	static int unlockInput(LuaHandle* luaHandle);		// Gui.unlockInput()
	static int setCameraPosition(LuaHandle* luaHandle);	// Gui.setCameraPosition()
	static int setCameraAngles(LuaHandle* luaHandle);
	static int setCameraDestination(LuaHandle* luaHandle);
	static int setCameraMotion(LuaHandle* luaHandle);
	static int unfogMap(LuaHandle *luaHandle);			// Gui.unfogMap()

	// create/damage/destroy units / hand-out resources/upgrades
	static int createUnit(LuaHandle* luaHandle);		// Faction:createUnit()
	static int giveResource(LuaHandle* luaHandle);		// Faction:giveResource()
	static int giveUpgrade(LuaHandle* luaHandle);
	static int damageUnit(LuaHandle* luaHandle);
	static int destroyUnit(LuaHandle* luaHandle);

	// commands
	static int givePositionCommand(LuaHandle* luaHandle);	// Unit:giveCommand()
	static int giveBuildCommand(LuaHandle* luaHandle);		// Unit:giveCommand()
	static int giveTargetCommand(LuaHandle * luaHandle);	// Unit:giveCommand()
	static int giveStopCommand(LuaHandle * luaHandle);		// Unit:giveCommand()
	static int giveProductionCommand(LuaHandle* luaHandle);	// Unit:giveCommand()
	static int giveUpgradeCommand(LuaHandle* luaHandle);	// Unit:giveCommand()

	// prototype AI helper functions... these DON'T belong here...
	static int initSurveyor(LuaHandle* luaHandle);
	static int findLocationForBuilding(LuaHandle* luaHandle);
	static int findResourceLocation(LuaHandle* luaHandle);

	// ditto ... and should be one getPlayer() and then [with player = getPlayer(0);] ndx = player:getIndex(); col = player:getColour(), etc
	static int getPlayerCount(LuaHandle* luaHandle);
	static int getHumanPlayerIndex(LuaHandle* luaHandle);
	static int getPlayerName(LuaHandle* luaHandle);
	static int getPlayerTeam(LuaHandle* luaHandle);
	static int getPlayerColour(LuaHandle* luaHandle);

	// game flow
	static int setPlayerAsWinner(LuaHandle* luaHandle);		// Faction:setWinnerFlag()
	static int endGame(LuaHandle* luaHandle);				// Game.end()

	// AI
	static int disableAi(LuaHandle* luaHandle);				// Faction:disableAi()
	static int enableAi(LuaHandle* luaHandle);				// Faction:enableAi()
	static int disableConsume(LuaHandle* luaHandle);
	static int enableConsume(LuaHandle* luaHandle);
	static int increaseStore(LuaHandle* luaHandle);

	static int debugLog(LuaHandle* luaHandle);				// Game.debugLog()

	// queries
	static int playerName(LuaHandle* luaHandle);			// Faction:getPlayerName()
	static int factionTypeName(LuaHandle* luaHandle);		// Faction:getName()
	static int startLocation(LuaHandle* luaHandle);			// Faction:getStartLocation()
	static int resourceAmount(LuaHandle* luaHandle);		// Faction:getResourceAmount()

	static int scenarioDir(LuaHandle* luaHandle);			// Scenario.getPath()
	//static int campaignDir(LuaHandle* luaHandle);			// Scenario.getCampaignPath()

	static int unitPosition(LuaHandle* luaHandle);			// Unit:getPosition()
	static int unitFaction(LuaHandle* luaHandle);			// Unit:getFaction()

	static int unitCount(LuaHandle* luaHandle);				// Faction:getUnitCount()
	static int unitCountOfType(LuaHandle* luaHandle);		// Faction:getUnitTypeCount()

	static int lastCreatedUnitName(LuaHandle* luaHandle);	// ?? deprecate, use 'created' unitEvent ??
	static int lastCreatedUnit(LuaHandle* luaHandle);		// ?? deprecate, use 'created' unitEvent ??

	static int lastDeadUnitName(LuaHandle* luaHandle);		// deprecate, use unitEvent
	static int lastDeadUnit(LuaHandle* luaHandle);			// deprecate, use unitEvent

	IF_DEBUG_EDITION(
		static int hilightRegion(LuaHandle *luaHandle);
		static int hilightCell(LuaHandle *luaHandle);
		static int clearHilights(LuaHandle *luaHandle);
		static int debugSet(LuaHandle *luaHandle);
		static int setFarClip(LuaHandle *luaHandle);
	)

	// Over-rides... normal Lua functions redefined
	static int dofile(LuaHandle *luaHandle);
};

}}//end namespace

#endif

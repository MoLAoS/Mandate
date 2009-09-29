// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SCRIPT_MANAGER_H_
#define _GLEST_GAME_SCRIPT_MANAGER_H_

#include <string>
#include <queue>
#include <set>

#include "lua_script.h"
#include "vec.h"
#include "timer.h"

#include "components.h"
#include "game_constants.h"

using std::string;
using std::queue;
using std::set;
using Shared::Graphics::Vec2i;
using Shared::Platform::Chrono;
using namespace Shared::Lua;

namespace Glest{ namespace Game{

class World;
class Unit;
class GameCamera;
class GameSettings;
class Game;


// =====================================================
//	class ScriptTimer
// =====================================================

class ScriptTimer {
private:
	string name;
	bool real;
	bool periodic;
	bool active;
	int64 targetTime;
	int64 interval;

public:
	ScriptTimer(const string &name, bool real, int interval, bool periodic);

	const string &getName()	const	{return name;}
	bool isPeriodic() const			{return periodic;}
	bool isAlive() const			{return active;}
	bool isReady() const;

	void kill()						{active = false;}
	void reset();
};

// =====================================================
//	class ScriptManagerMessage
// =====================================================

class ScriptManagerMessage {
private:
	string text;
	string header;

public:
	ScriptManagerMessage(string text, string header) : text(text), header(header) {}
	const string &getText() const	{return text;}
	const string &getHeader() const	{return header;}
};

class PlayerModifiers {
private:
	bool winner;
	bool aiEnabled;

public:
	PlayerModifiers();

	bool getWinner() const		{return winner;}
	bool getAiEnabled() const	{return aiEnabled;}

	void disableAi()			{aiEnabled = false;}
	void setAsWinner()			{winner = true;}
};

// =====================================================
//	class ScriptManager
// =====================================================

class ScriptManager {

	//friend class World; // Lua Debugging, using setDisplayTest()

private:
	typedef queue<ScriptManagerMessage> MessageQueue;

	//lua
	static string code;
	static LuaScript luaScript;

	//misc
	static MessageQueue messageQueue;
	static GraphicMessageBox messageBox;
	static string displayText;
	
	//last created unit
	static string lastCreatedUnitName;
	static int lastCreatedUnitId;

	//last dead unit
	static string lastDeadUnitName;
	static int lastDeadUnitId;

	// end game state
	static bool gameOver;
	static PlayerModifiers playerModifiers[GameConstants::maxPlayers];

	static vector<ScriptTimer> timers;
	static vector<ScriptTimer> newTimerQueue;

	static set<string> definedEvents;

	//static ScriptManager* thisScriptManager;

	static const int messageWrapCount;
	static const int displayTextWrapCount;

public:
	static void init ();

	//message box functions
	static bool getMessageBoxEnabled() 									{return !messageQueue.empty();}
	static GraphicMessageBox* getMessageBox()							{return &messageBox;}
	static string getDisplayText() 										{return displayText;}
	static bool getGameOver() 											{return gameOver;}
	static const PlayerModifiers *getPlayerModifiers(int factionIndex) 	{return &playerModifiers[factionIndex];}	

	//events
	static void onMessageBoxOk();
	static void onResourceHarvested();
	static void onUnitCreated(const Unit* unit);
	static void onUnitDied(const Unit* unit);
	static void onTimer();

private:
	static string wrapString(const string &str, int wrapCount);
	static string describeLuaStack ( LuaArguments &args );
	static void luaCppCallError ( const string &func, const string &expected, const string &received, 
											const string extra = "Wrong number of parameters." ) ;

	//
	// LUA callbacks
	//
	
	// commands
	static int setTimer(LuaHandle* luaHandle);
	static int stopTimer(LuaHandle* luaHandle);
	static int showMessage(LuaHandle* luaHandle);
	static int setDisplayText(LuaHandle* luaHandle);
	static int clearDisplayText(LuaHandle* luaHandle);
	static int setCameraPosition(LuaHandle* luaHandle);
	static int createUnit(LuaHandle* luaHandle);
	static int giveResource(LuaHandle* luaHandle);
	static int givePositionCommand(LuaHandle* luaHandle);
	static int giveTargetCommand ( LuaHandle * luaHandle );
	static int giveStopCommand ( LuaHandle * luaHandle );
	static int giveProductionCommand(LuaHandle* luaHandle);
	static int giveUpgradeCommand(LuaHandle* luaHandle);
	static int disableAi(LuaHandle* luaHandle);
	static int setPlayerAsWinner(LuaHandle* luaHandle);
	static int endGame(LuaHandle* luaHandle);
	static int debugLog ( LuaHandle* luaHandle );
	static int consoleMsg ( LuaHandle* luaHandle );

	// queries
	static int getPlayerName(LuaHandle* luaHandle);
	static int getFactionTypeName(LuaHandle* luaHandle);
	static int getScenarioDir(LuaHandle* luaHandle);
	static int getStartLocation(LuaHandle* luaHandle);
	static int getUnitPosition(LuaHandle* luaHandle);
	static int getUnitFaction(LuaHandle* luaHandle);
	static int getResourceAmount(LuaHandle* luaHandle);
	static int getLastCreatedUnitName(LuaHandle* luaHandle);
	static int getLastCreatedUnitId(LuaHandle* luaHandle);
	static int getLastDeadUnitName(LuaHandle* luaHandle);
	static int getLastDeadUnitId(LuaHandle* luaHandle);
	static int getUnitCount(LuaHandle* luaHandle);
	static int getUnitCountOfType(LuaHandle* luaHandle);
};

}}//end namespace

#endif

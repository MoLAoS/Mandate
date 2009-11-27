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

#ifndef _GLEST_GAME_SCRIPT_MANAGER_H_
#define _GLEST_GAME_SCRIPT_MANAGER_H_

#include <string>
#include <queue>
#include <set>
#include <map>
#include <limits>

#include "lua_script.h"
#include "vec.h"
#include "timer.h"

#include "components.h"
#include "game_constants.h"
#include "logger.h"

using namespace std;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Vec4i;
using Shared::Platform::Chrono;
using namespace Shared::Lua;

namespace Glest{ namespace Game {

class World;
class Unit;
class GameCamera;
class GameSettings;
class Game;
class UnitType;

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
	ScriptTimer(const string &name, bool real, int interval, bool periodic)
		: name(name), real(real), periodic(periodic), interval(interval), active(true) {
			reset();
	}

	const string &getName()	const	{return name;}
	bool isPeriodic() const			{return periodic;}
	bool isAlive() const			{return active;}
	bool isReady() const;

	void kill()						{active = false;}
	void reset();
};

// =====================================================
//	class Region, and Derivitives
// =====================================================

struct Region {
	virtual bool isInside(const Vec2i &pos) const = 0;
};

struct Rect : public Region {
	int x, y, w, h; // top-left coords + width and height

	Rect() : x(0), y(0), w(0), h(0) { }
	Rect(const int v) : x(v), y(v), w(v), h(v) { }
	Rect(const Vec4i &v) : x(v.x), y(v.y), w(v.z), h(v.w) { }
	Rect(const int x, const int y, const int w, const int h) : x(x), y(y), w(w), h(h) { }

	virtual bool isInside(const Vec2i &pos) const {
		return pos.x >= x && pos.y >= y && pos.x < x + w && pos.y < y + h;
	}
};

struct Circle : public Region {
	int x, y; // centre
	float radius;

	Circle() : x(-1), y(-1), radius(numeric_limits<float>::quiet_NaN()) { }
	Circle(const Vec2i &pos, const float radius) : x(pos.x), y(pos.y), radius(radius) { }
	Circle(const int x, const int y, const float r) : x(x), y(y), radius(r) { }

	virtual bool isInside(const Vec2i &pos) const {
		return pos.dist(Vec2i(x,y)) <= radius;
	}
};

struct CompoundRegion : public Region {
	vector<Region*> regions;

	CompoundRegion() { }
	CompoundRegion(Region *ptr) { regions.push_back(ptr); }

	template<typename InIter>
	void addRegions(InIter start, InIter end) {
		copy(start,end,regions.end());
	}

	virtual bool isInside(const Vec2i &pos) const {
		for ( vector<Region*>::const_iterator it = regions.begin(); it != regions.end(); ++it ) {
			if ( (*it)->isInside(pos) ) {
				return true;
			}
		}
		return false;
	}
};

struct PosTrigger {
	Region *region;
	string evnt;
	int user_dat;
	PosTrigger() : region(NULL), evnt(""), user_dat(0) {}
};

struct Trigger {
	string evnt;
	int user_dat;
	Trigger() : evnt(""), user_dat(0) {}
};

// =====================================================
//	class TriggerManager
// =====================================================

class TriggerManager {
	typedef map<string,Region*>			Regions; 
	typedef set<string>					Events;
	typedef vector<PosTrigger>			PosTriggers;
	typedef map<int,PosTriggers>		PosTriggerMap;
	typedef map<int,Trigger>			TriggerMap;

	Events  events;
	Regions regions;

	PosTriggerMap	unitPosTriggers;
	PosTriggerMap	factionPosTriggers;
	TriggerMap		attackedTriggers;
	TriggerMap		hpBelowTriggers;
	TriggerMap		hpAboveTriggers;
	TriggerMap		commandCallbacks;
	TriggerMap		deathTriggers;

	World *world;
public:
	TriggerManager() : world(NULL) {}
	~TriggerManager();

	void reset(World *world);
	bool registerRegion(const string &name, const Rect &rect);
	int  registerEvent(const string &name);

	const Region* getRegion(string &name) { 
		Regions::iterator it = regions.find(name);
		if ( it == regions.end() ) return NULL;
		else return it->second;
	}

	int  addUnitPosTrigger(int unitId, const string &region, const string &eventName, int userData=0);
	int  addFactionPosTrigger(int ndx, const string &region, const string &eventName, int userData);

	int  addCommandCallback(int unitId, const string &eventName, int userData=0);
	int  addHPBelowTrigger(int unitId, int threshold, const string &eventName, int userData=0);
	int  addHPAboveTrigger(int unitId, int threshold, const string &eventName, int userData=0);
	int  addDeathTrigger(int unitId, const string &eventName, int userData=0);

	// must be called any time a unit is 'put' in cells (created, moved, 
	void unitMoved(const Unit *unit);
	void unitDied(const Unit *unit);
	void commandCallback(const Unit *unit);
	void onHPBelow(const Unit *unit);
	void onHPAbove(const Unit *unit);
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
	PlayerModifiers() : winner(false), aiEnabled(true) { }

	bool getWinner() const		{return winner;}
	bool getAiEnabled() const	{return aiEnabled;}

	void disableAi()			{aiEnabled = false;}
	void setAsWinner()			{winner = true;}
};

// =====================================================
//	class ScriptManager
// =====================================================

class ScriptManager {
private:
	typedef queue<ScriptManagerMessage> MessageQueue;

	//lua
	static string code;
	static LuaScript luaScript;

	//misc
	static MessageQueue messageQueue;
	static GraphicMessageBox messageBox;
	static string displayText;

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

	static Game *game;
	static World *world;

public:
	static void init(Game *game);

	static void doSomeLua(string &code);

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
	static void onTrigger(const string &name, int unitId, int userData=0);
	static void unitMoved(Unit *unit) { triggerManager.unitMoved(unit); }
	static void commandCallback(const Unit *unit) { triggerManager.commandCallback(unit); }
	static void onHPBelowTrigger(const Unit *unit) { triggerManager.onHPBelow(unit); }
	static void onHPAboveTrigger(const Unit *unit) { triggerManager.onHPAbove(unit); }

	static void addErrorMessage(const char *txt=NULL, bool quietly = false);
	static void addErrorMessage(const string &txt) {
		addErrorMessage(txt.c_str());
	}

	static int panicFunc(LuaHandle* luaHandle);

private:
	static string wrapString(const string &str, int wrapCount);
	static bool extractArgs(LuaArguments &args, const char *caller, const char *format, ...);

	//
	// LUA callbacks
	//

	// unit trigger helper...
	static void doUnitTrigger(int id, string &cond, string &evnt, int ud);

	// Timers, Triggers, Events...
	static int setTimer(LuaHandle* luaHandle);			// Game.setTimer()
	static int stopTimer(LuaHandle* luaHandle);			// Game.stopTimer()
	static int registerRegion(LuaHandle* luaHandle);	// Game.resgisterRegion()
	static int registerEvent(LuaHandle* luaHandle);		// Game.resgisterEvent()
	static int setUnitTrigger(LuaHandle* luaHandle);	// Unit:setTrigger()
	static int setUnitTriggerX(LuaHandle* luaHandle);	// Unit:setTrigger()
	static int setFactionTrigger(LuaHandle* luaHandle);	// Faction:setTrigger()
	//static int setTeamTrigger(LuaHandle* luaHandle);	// 

	// messages
	static int showMessage(LuaHandle* luaHandle);		// Gui.showMessage()
	static int setDisplayText(LuaHandle* luaHandle);	// Gui.setDisplayText()
	static int clearDisplayText(LuaHandle* luaHandle);	// Gui.clearDisplayText()
	static int consoleMsg(LuaHandle* luaHandle);		// Gui.consoleMsg()

	// gui
	static int lockInput(LuaHandle* luaHandle);			// Gui.lockInput()
	static int unlockInput(LuaHandle* luaHandle);		// Gui.unlockInput()
	static int setCameraPosition(LuaHandle* luaHandle);	// Gui.setCameraPosition()
	static int unfogMap(LuaHandle *luaHandle);			// Gui.unfogMap()

	// create units / hand-out resources
	static int createUnit(LuaHandle* luaHandle);		// Faction:createUnit()
	static int giveResource(LuaHandle* luaHandle);		// Faction:giveResource()

	// commands
	static int givePositionCommand(LuaHandle* luaHandle);	// Unit:givePosCommand()
	static int giveTargetCommand(LuaHandle * luaHandle);	// Unit:giveTargetCommand()
	static int giveStopCommand(LuaHandle * luaHandle);		// Unit:giveStopCommand()
	static int giveProductionCommand(LuaHandle* luaHandle);	// Unit:givePorduceCommand()
	static int giveUpgradeCommand(LuaHandle* luaHandle);	// Unit:giveUpgradeCommand()
	//static int giveBuildCommand(LuaHanfle* luaHandle);	// Unit:giveBuildCommand()

	// game flow
	static int disableAi(LuaHandle* luaHandle);				// Faction:disableAi()
	static int setPlayerAsWinner(LuaHandle* luaHandle);		// Faction:setWinnerFlag()
	static int endGame(LuaHandle* luaHandle);				// Game.end()

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
	static int lastDeadUnit(LuaHandle* luaHandle);		// deprecate, use unitEvent

#	if DEBUG_RENDERING_ENABLED

	static int hilightRegion(LuaHandle *luaHandle);
	static int hilightCell(LuaHandle *luaHandle);
	static int clearHilights(LuaHandle *luaHandle);

#	endif

};

}}//end namespace

#endif

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

#include "pch.h"

#include <stdarg.h>

#include "script_manager.h"
#include "conversion.h"

#include "world.h"
#include "lang.h"
#include "game_camera.h"
#include "game.h"
#include "renderer.h"

#include "leak_dumper.h"
#include "sim_interface.h"

using namespace Glest::Sim;

#if _GAE_DEBUG_EDITION_
#	include "renderer.h"
#	include "debug_renderer.h"
#endif

namespace Glest { namespace Script {

using namespace Shared::Util::Conversion;

TriggerManager ScriptManager::triggerManager;

// =====================================================
//	class ScriptManager
// =====================================================

// ========== statics ==========

const int ScriptManager::messageWrapCount= 30;
const int ScriptManager::displayTextWrapCount= 64;

string				ScriptManager::code;
LuaScript			ScriptManager::luaScript;
bool				ScriptManager::gameOver;
PlayerModifiers		ScriptManager::playerModifiers[GameConstants::maxPlayers];
vector<ScriptTimer> ScriptManager::timers;
vector<ScriptTimer> ScriptManager::newTimerQueue;
set<string>			ScriptManager::definedEvents;

LuaConsole*			ScriptManager::luaConsole;

map<string, Vec3f> ScriptManager::actorColours;

ScriptManager::UnitInfo		ScriptManager::latestCreated,
							ScriptManager::latestCasualty;

#define LUA_FUNC(x) luaScript.registerFunction(x, #x)
#if _GAE_DEBUG_EDITION_
#	define DEBUG_FUNC(x) LUA_FUNC(x)
#else
#	define DEBUG_FUNC(x)
#endif

void ScriptManager::cleanUp() {
	code = "";
	gameOver = false;
	timers.clear();
	newTimerQueue.clear();
	definedEvents.clear();
	triggerManager.reset();
	latestCreated.id = -1;
	latestCasualty.id = -1;
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		playerModifiers[i] = PlayerModifiers();
	}
}

int getSubfaction(LuaHandle *luaHandle);
int getSubfactionRestrictions(LuaHandle *luaHandle);
int getFrameCount(LuaHandle *luaHandle);

int getCellType(LuaHandle *luaHandle);
int getCellHeight(LuaHandle *luaHandle);
int getTileHeight(LuaHandle *luaHandle);
int getWaterLevel(LuaHandle *luaHandle);

int setTime(LuaHandle *luaHandle);

int returnTableTest(LuaHandle *luaHandle);

void ScriptManager::initGame() {
	const Scenario*	scenario = g_world.getScenario();

	cleanUp();

	luaScript.startUp();
	luaScript.atPanic(panicFunc);
	assert(!luaScript.isDefined("startup")); // making sure old code is gone
	luaConsole = g_userInterface.getLuaConsole();

	//register functions

	// debug info / testing
	LUA_FUNC(getSubfaction);
	LUA_FUNC(getSubfactionRestrictions);
	LUA_FUNC(getFrameCount);

	LUA_FUNC(getCellHeight);
	LUA_FUNC(getCellType);
	LUA_FUNC(getTileHeight);
	LUA_FUNC(getWaterLevel);
	LUA_FUNC(setTime);

	LUA_FUNC(returnTableTest);



	// AI helpers
	LUA_FUNC(initSurveyor);
	LUA_FUNC(findLocationForBuilding);
	LUA_FUNC(findResourceLocation);

	LUA_FUNC(getPlayerCount);
	LUA_FUNC(getHumanPlayerIndex);
	LUA_FUNC(getPlayerName);
	LUA_FUNC(getPlayerTeam);
	LUA_FUNC(getPlayerColour);

	// Game control
	LUA_FUNC(setPlayerAsWinner);
	LUA_FUNC(endGame);

	// Gui control
	LUA_FUNC(lockInput);
	LUA_FUNC(unlockInput);
	LUA_FUNC(setCameraPosition);
	LUA_FUNC(setCameraDestination);
	LUA_FUNC(setCameraAngles);
	LUA_FUNC(setCameraMotion);
	LUA_FUNC(unfogMap);

	// messaging
	LUA_FUNC(showMessage);
	LUA_FUNC(setDisplayText);
	LUA_FUNC(clearDisplayText);
	LUA_FUNC(addActor);
	LUA_FUNC(addDialog);

	// create and give ...
	LUA_FUNC(createUnit);
	LUA_FUNC(giveResource);
	LUA_FUNC(giveUpgrade);
	LUA_FUNC(damageUnit);
	LUA_FUNC(destroyUnit);

	// commands
	LUA_FUNC(givePositionCommand);
	LUA_FUNC(giveBuildCommand);
	LUA_FUNC(giveProductionCommand);
	LUA_FUNC(giveStopCommand);
	LUA_FUNC(giveTargetCommand);
	LUA_FUNC(giveUpgradeCommand);

	// timers
	LUA_FUNC(setTimer);
	LUA_FUNC(stopTimer);

	// regions, events and triggers
	LUA_FUNC(registerRegion);
	LUA_FUNC(registerEvent);
	LUA_FUNC(setUnitTrigger);
	LUA_FUNC(setUnitTriggerX);
	LUA_FUNC(setFactionTrigger);
	LUA_FUNC(removeUnitPosTriggers);

	// queries
	LUA_FUNC(playerName);
	LUA_FUNC(factionTypeName);
	LUA_FUNC(scenarioDir);
	LUA_FUNC(startLocation);
	LUA_FUNC(unitPosition);
	LUA_FUNC(unitFaction);
	LUA_FUNC(resourceAmount);
	LUA_FUNC(lastCreatedUnitName);
	LUA_FUNC(lastCreatedUnit);
	LUA_FUNC(lastDeadUnitName);
	LUA_FUNC(lastDeadUnit);
	LUA_FUNC(unitCount);
	LUA_FUNC(unitCountOfType);

	// debug
	LUA_FUNC(debugLog);
	LUA_FUNC(consoleMsg);
	DEBUG_FUNC(hilightRegion);
	DEBUG_FUNC(hilightCell);
	DEBUG_FUNC(clearHilights);
	DEBUG_FUNC(debugSet);
	DEBUG_FUNC(setFarClip);

	LUA_FUNC(dofile);
	
	LUA_FUNC(disableAi);
	LUA_FUNC(enableAi);
	LUA_FUNC(disableConsume);
	LUA_FUNC(enableConsume);
	LUA_FUNC(increaseStore);
	
	IF_DEBUG_EDITION(
		luaScript.luaDoLine("dofile('dev_edition.lua')");
	)

	if (!scenario) {
		return;
	}
	//load code
	for(int i = 0; i < scenario->getScriptCount(); ++i) {
		const Script* script= scenario->getScript(i);
		bool ok;
		if (script->getName().substr(0,9) == "unitEvent" || script->getName() == "resourceHarvested") {
			ok = luaScript.loadCode("function " + script->getName() + "(unit_id, user_data)" + script->getCode() + "\nend\n", script->getName());
		} else {
			ok = luaScript.loadCode("function " + script->getName() + "()" + script->getCode() + "\nend\n", script->getName());
		}
		if (!ok) {
			addErrorMessage();
		}

	}

	// get globs, startup, unitDied, unitDiedOfType_xxxx (archer, worker, etc...) etc.
	//  need unit names of all loaded factions
	// put defined function names in definedEvents, check membership before doing luaCall()
	set<string> funcNames;
	funcNames.insert("startup");
	funcNames.insert("unitDied");
	funcNames.insert("unitCreated");
	funcNames.insert("resourceHarvested");
	for (int i=0; i < g_world.getFactionCount(); ++i) {
		const FactionType *f = g_world.getFaction(i)->getType();
		for (int j=0; j < f->getUnitTypeCount(); ++j) {
			const UnitType *ut = f->getUnitType(j);
			funcNames.insert("unitCreatedOfType_" + ut->getName());
		}
	}
	for (set<string>::iterator it = funcNames.begin(); it != funcNames.end(); ++it) {
		if (luaScript.isDefined(*it)) {
			definedEvents.insert(*it);
		}
	}

	//call startup function
	if (definedEvents.find("startup") != definedEvents.end()) {
		if(!luaScript.luaCall("startup")){
			addErrorMessage();
		}
	} else {
		addErrorMessage("Warning, no startup script defined", true);
	}
}

int returnTableTest(LuaHandle *l) {
	LuaArguments args(l);

	args.startReturnTable();
		args.returnString("What is the meaning of life?");
		args.returnInt(42);
	args.endReturnTable();

	return args.getReturnCount();
}

int setTime(LuaHandle *l) {
	LuaArguments args(l);
	float time;
	if (ScriptManager::extractArgs(args, "setTime", "flt", &time)) {
		time = clamp(time, 0.f, 23.999f);
		const_cast<TimeFlow*>(g_world.getTimeFlow())->setTime(time);
		string timeStr = g_world.getTimeFlow()->describeTime();
		ScriptManager::luaConsole->addOutput("Time set, time now: " + timeStr);
	}
	return args.getReturnCount();
}

int getSubfaction(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	int ndx;
	if (ScriptManager::extractArgs(args, "getSubfaction", "int", &ndx)) {
		if (ndx >= 0 && ndx < g_world.getFactionCount()) {
			const Faction *f = g_world.getFaction(ndx);
			const FactionType *ft = f->getType();
			string name = ft->getSubfaction(f->getSubfaction());
			string res = "Faction " + intToStr(ndx) + " [" + ft->getName() + "] = '" + name + "'";
			ScriptManager::luaConsole->addOutput(res);
		} else {
			ScriptManager::luaConsole->addOutput("Faction index out of range:" + intToStr(ndx));
		}
	}
	return args.getReturnCount();
}

int getSubfactionRestrictions(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	string facName, reqName;
	if (ScriptManager::extractArgs(args, "getSubfactionRestrictions", "str,str", &facName, &reqName)) {
		try {
			string res;
			const FactionType *ft = g_world.getTechTree()->getFactionType(facName);
			const UpgradeType *ut = ft->getUpgradeType(reqName);
			if (ut->getSubfactionsReqs() == -1) {
				res = "all";
			} else {			
				for (int i=0; i < ft->getSubfactionCount(); ++i) {
					if (ut->isAvailableInSubfaction(i)) {
						res += ft->getSubfaction(i) + " ";
					}
				}
			}
			ScriptManager::luaConsole->addOutput("Available in: " + res);
		} catch (runtime_error &e) {
			ScriptManager::luaConsole->addOutput(e.what());
		}
	}
	return args.getReturnCount();
}

int getFrameCount(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	args.returnInt(g_world.getFrameCount());
	return args.getReturnCount();
}

int getCellType(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	Vec2i pos;
	if (ScriptManager::extractArgs(args, "getCellType", "v2i", &pos)) {
		if (g_map.isInside(pos)) {
			SurfaceType surfType = g_map.getCell(pos)->getType();
			args.returnString(formatString(SurfaceTypeNames[surfType]));
		} else {
			ScriptManager::addErrorMessage("Error: getCellType(pos), pos is not on map.");
		}
	}
	return args.getReturnCount();
}

int getCellHeight(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	Vec2i pos;
	if (ScriptManager::extractArgs(args, "getCellHeight", "v2i", &pos)) {
		if (g_map.isInside(pos)) {
			float res = g_map.getCell(pos)->getHeight();
			args.returnString(toStr(res));
		} else {
			ScriptManager::addErrorMessage("Error: getCellHeight(pos), pos is not on map.");
		}
	}
	return args.getReturnCount();
}

int getTileHeight(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	Vec2i pos;
	if (ScriptManager::extractArgs(args, "getTileHeight", "v2i", &pos)) {
		if (g_map.isInsideTile(pos)) {
			float res = g_map.getTileHeight(pos);
			args.returnString(toStr(res));
		} else {
			ScriptManager::addErrorMessage("Error: getTileHeight(pos), pos is not on map.");
		}
	}
	return args.getReturnCount();
}

int getWaterLevel(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	float res = g_map.getWaterLevel();
	args.returnString(toStr(res));
	return args.getReturnCount();
}

// ========================== events ===============================================

void ScriptManager::onResourceHarvested(const Unit *unit) {
	if (definedEvents.find("resourceHarvested") != definedEvents.end()) {
		if (!luaScript.luaCallback("resourceHarvested", unit->getId(), 0)) {
			addErrorMessage();
		}
	}
}

void ScriptManager::onUnitCreated(const Unit* unit) {
	latestCreated.name = unit->getType()->getName();
	latestCreated.id = unit->getId();
	if (definedEvents.find("unitCreated") != definedEvents.end()) {
		if (!luaScript.luaCall("unitCreated")) {
			addErrorMessage();
		}
	}
	if (definedEvents.find("unitCreatedOfType_"+latestCreated.name) != definedEvents.end()) {
		if (!luaScript.luaCall("unitCreatedOfType_"+latestCreated.name)) {
			addErrorMessage();
		}
	}
}

void ScriptManager::onUnitDied(const Unit* unit) {
	latestCasualty.name = unit->getType()->getName();
	latestCasualty.id = unit->getId();
	if (definedEvents.find("unitDied") != definedEvents.end()) {
		if (!luaScript.luaCall("unitDied")) {
			addErrorMessage();
		}
	}
	triggerManager.unitDied(unit);
}

void ScriptManager::onTrigger(const string &name, int unitId, int userData) {
	if (!luaScript.luaCallback("unitEvent_" + name, unitId, userData)) {
		addErrorMessage("unitEvent_" + name + "(id:" + intToStr(unitId) + ", userData:"
			+ intToStr(userData) +"): call failed.");
		addErrorMessage();
	}
}

void ScriptManager::update() {
	// when a timer is ready, call the corresponding lua function
	// and remove the timer, or reset to repeat.

	vector<ScriptTimer>::iterator timer = timers.begin();

	while (timer != timers.end()) {
		if (timer->isReady()) {
			if (timer->isAlive()) {
				if (!luaScript.luaCall("timer_" + timer->getName())) {
					timer->kill();
					addErrorMessage();
				}
			}
			if (timer->isPeriodic() && timer->isAlive()) {
				timer->reset();
			} else {
				timer = timers.erase(timer); //returns next element
				continue;
			}
		}
		++timer;
	}
	for (vector<ScriptTimer>::iterator it = newTimerQueue.begin(); it != newTimerQueue.end(); ++it) {
		timers.push_back (*it);
	}
	newTimerQueue.clear();
}

// =============== util ===============

void ScriptManager::doSomeLua(const string &code) {
	if (!luaScript.luaDoLine(code)) {
		addErrorMessage();
	}
}


// =============== Error handling bits ===============

void ScriptManager::addErrorMessage(const char *txt, bool quietly) {
	string err = txt ? txt : luaScript.getLastError();
	g_logger.logError(err);

	if (World::isConstructed()) {
		luaConsole->addOutput(err);

		if (!quietly) {
			g_simInterface.pause();
			g_gameState.addScriptMessage("Script Error", err);
		}
	}
}

/** Extracts arguments for Lua callbacks.
  * <p>uses a string description of the expected arguments in arg_desc, then takes pointers the
  * variables of the corresponding types to populate with the results</p>
  * <p>Uses va_args obviously, so there is no type checking, be careful.</p>
  * @param luaArgs the LuaArguments object to extract arguments from
  * @param caller if NULL, errors will not be reported, otherwise this is name of the lua callback
  *  that called this function, used for error reporting.
  * @param arg_desc argument descriptor, a string of the form "[xxx[,xxx[,etc]]]" where xxx can be any of
  * <ul><li>'int' for an integer</li>
  *		<li>'flt' for a float</li>
  *		<li>'str' for a string</li>
  *		<li>'bln' for a boolean</li>
  *		<li>'v2i' for a Vec2i</li>
  *		<li>'v3i' for a Vec3i</li>
  *		<li>'v4i' for a Vec4i</li></ul>
  * @param ... a pointer to the appropriate type for each argument specified in arg_desc
  * @return true if arguments were extracted successfully, false if there was an error
  */
bool ScriptManager::extractArgs(LuaArguments &luaArgs, const char *caller, const char *arg_desc, ...) {
	if (!*arg_desc) {
		if (luaArgs.getArgumentCount()) {
			if (caller) {
				addErrorMessage(string(caller) + "(): expected 0 arguments, got "
						+ intToStr(luaArgs.getArgumentCount()));
			}
			return false;
		} else {
			return true;
		}
	}
	const char *ptr = arg_desc;
	int expected = 1;
	while (*ptr) {
		if (*ptr++ == ',') expected++;
	}
	if (expected != luaArgs.getArgumentCount()) {
		if (caller) {
			addErrorMessage(string(caller) + "(): expected " + intToStr(expected)
				+ " arguments, got " + intToStr(luaArgs.getArgumentCount()));
		}
		return false;
	}
	va_list vArgs;
	va_start(vArgs, arg_desc);
	char *tmp = strcpy (new char[strlen(arg_desc) + 1], arg_desc);
	char *tok = strtok(tmp, ",");
	try {
		while (tok) {
			if (strcmp(tok,"int") == 0) {
				*va_arg(vArgs,int*) = luaArgs.getInt(-expected);
			} else if (strcmp(tok,"str") == 0) {
				*va_arg(vArgs,string*) = luaArgs.getString(-expected);
			} else if (strcmp(tok,"bln") == 0) {
				*va_arg(vArgs,bool*) = luaArgs.getBoolean(-expected);
			} else if (strcmp(tok,"v2i") == 0) {
				*va_arg(vArgs,Vec2i*) = luaArgs.getVec2i(-expected);
			} else if (strcmp(tok,"v3i") == 0) {
				*va_arg(vArgs,Vec3i*) = luaArgs.getVec3i(-expected);
			} else if (strcmp(tok,"v4i") == 0) {
				*va_arg(vArgs,Vec4i*) = luaArgs.getVec4i(-expected);
			} else if (strcmp(tok,"flt") == 0) {
				*va_arg(vArgs,float*) = luaArgs.getFloat(-expected);
			} else {
				throw runtime_error("ScriptManager::extractArgs() passed bad arg_desc string");
			}
			--expected;
			tok = strtok(NULL, ",");
		}
	} catch (LuaError e) {
		if (caller) {
			addErrorMessage("Error: " + string(caller) + "() " + e.desc());
		}
		va_end(vArgs);
		delete[] tmp;
		return false;
	}
	delete[] tmp;
	return true;
}

int ScriptManager::panicFunc(LuaHandle *luaHandle) {
	g_logger.logError("Fatal Error: Lua panic.");
	return 0;
}

// ========================== lua callbacks ===============================================

int ScriptManager::debugLog(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	string msg;
	if (extractArgs(args, "debugLog", "str", &msg)) {
		g_logger.logError(msg);
	}
	return args.getReturnCount();
}

int ScriptManager::consoleMsg(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	string msg;
	if (extractArgs(args, "consoleMsg", "str", &msg)) {
		luaConsole->addOutput(msg);
	}
	return args.getReturnCount();
}

int ScriptManager::setTimer(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name, type;
	int period;
	bool repeat;
	if (extractArgs(args, "setTimer", "str,str,int,bln", &name, &type, &period, &repeat)) {
		if (type == "real") {
			newTimerQueue.push_back(ScriptTimer(name, true, period, repeat));
		} else if (type == "game") {
			newTimerQueue.push_back(ScriptTimer(name, false, period, repeat));
		} else {
			addErrorMessage("setTimer(): invalid type '" + type + "'");
		}
	}
	return args.getReturnCount(); // == 0
}

int ScriptManager::stopTimer(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name;
	if (extractArgs(args, "stopTimer", "str", &name)) {
		vector<ScriptTimer>::iterator i;
		bool killed = false;
		for (i = timers.begin(); i != timers.end(); ++i) {
			if (i->getName() == name) {
				i->kill();
				killed = true;
				break;
			}
		}
		if (!killed) {
			addErrorMessage("stopTimer(): timer '" + name + "' not found.");
		}
	}
	return args.getReturnCount();
}

int ScriptManager::registerRegion(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name;
	Vec4i rect;
	if (extractArgs(args, "registerRegion", "str,v4i", &name, &rect)) {
		if (!triggerManager.registerRegion(name, rect)) {
			stringstream ss;
			ss << "registerRegion(): with name='" << name << "' at " << rect
				<< "failed, a region with that name is already registered.";
			addErrorMessage(ss.str());
		}
	}
	return args.getReturnCount();
}

int ScriptManager::registerEvent(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name, condition;
	if (extractArgs(args, "registerEvent", "str", &name)) {
		if (triggerManager.registerEvent(name) == -1) {
			addErrorMessage("registerEvent(): event '" + name + "' is already registered");
		}
	}
	return args.getReturnCount();
}

/** @return 0 if ok, -1 if bad unit id, -2 if event not found  */
int ScriptManager::setUnitTriggerX(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id, ud;
	string cond, evnt;
	if (extractArgs(args, "setUnitTriggerX", "int,str,str,int", &id, &cond, &evnt, &ud)) {
		doUnitTrigger(id, cond, evnt, ud);
	}
	return args.getReturnCount();
}

int ScriptManager::setUnitTrigger(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string cond, evnt;
	if (extractArgs(args, "setUnitTrigger", "int,str,str", &id, &cond, &evnt)) {
		doUnitTrigger(id, cond, evnt, 0);
	}
	return args.getReturnCount();
}

string triggerResultToString(SetTriggerRes res) {
	switch (res) {
		case SetTriggerRes::OK:
			return "Ok.";
		case SetTriggerRes::BAD_UNIT_ID:
			return "Invalid unit id.";
		case SetTriggerRes::BAD_FACTION_INDEX:
			return "Invalid faction index.";
		case SetTriggerRes::UNKNOWN_EVENT:
			return "Unknown event.";
		case SetTriggerRes::UNKNOWN_REGION:
			return "Unknown region.";
		case SetTriggerRes::INVALID_THRESHOLD:
			return "Invalid threshold.";
		case SetTriggerRes::DUPLICATE_TRIGGER:
			return "Duplicate trigger.";
		case SetTriggerRes::INVALID:
		default:
			return "Invalid.";
	}
}

void ScriptManager::doUnitTrigger(int id, const string &cond, const string &evnt, int ud) {
	SetTriggerRes res;
	if (cond == "attacked") {
		res = triggerManager.addAttackedTrigger(id, evnt, ud);
	} else if (cond == "death") {
		res = triggerManager.addDeathTrigger(id, evnt, ud);
	} else if (cond == "enemy_sighted") { // nop
	} else if (cond == "command_callback") {
		res = triggerManager.addCommandCallback(id,evnt, ud);
	} else { // 'complex' conditions
		size_t ePos = cond.find('=');
		if (ePos != string::npos) {
			string key = cond.substr(0, ePos);
			string val = cond.substr(ePos+1);
			if (key == "hp_below") {
				int threshold = atoi(val.c_str());
				if (threshold >= 1) {
					res = triggerManager.addHPBelowTrigger(id, threshold, evnt, ud);
				}
			} else if (key == "hp_above") {
				int threshold = atoi(val.c_str());
				if (threshold >= 1) {
					res = triggerManager.addHPAboveTrigger(id, threshold, evnt, ud);
				}
			} else if (key == "region") {
				res = triggerManager.addUnitPosTrigger(id,val,evnt,ud);
			}
		}
	}
	if (res == SetTriggerRes::INVALID) {
		addErrorMessage("setUnitTrigger(): invalid condition = '" + cond + "'");
	} else if (res != SetTriggerRes::OK) {
		addErrorMessage("setUnitTrigger(): " + triggerResultToString(res));
	}
}

int ScriptManager::removeUnitPosTriggers(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	if (extractArgs(args, "removeUnitPosTriggers", "int", &id)) {
		if (triggerManager.removeUnitPosTriggers(id)) {
			args.returnBool(true);
		} else {
			args.returnBool(false);
		}
	} else {
		args.returnBool(false);
	}
	return args.getReturnCount();
}

int ScriptManager::setFactionTrigger(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int ndx, ud;
	string cond, evnt;
	if (extractArgs(args, "setFactionTrigger", "int,str,str,int", &ndx, &cond, &evnt, &ud)) {
		size_t ePos = cond.find('=');
		//bool did_something = false;
		SetTriggerRes res;
		if (ePos != string::npos) {
			string key = cond.substr(0, ePos);
			string val = cond.substr(ePos+1);
			if (key == "region") {
				res = triggerManager.addFactionPosTrigger(ndx,val,evnt, ud);
			}
		}
		if (res == SetTriggerRes::INVALID) {
			addErrorMessage("setFactionTrigger(): invalid condition = '" + cond + "'");
		} else if (res != SetTriggerRes::OK) {
			addErrorMessage("setFactionTrigger(): " + triggerResultToString(res));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::showMessage(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string txt, hdr;
	if ( extractArgs(args, "showMessage", "str,str", &txt, &hdr) ) {
		//g_gameState.pause (); // this needs to be optional, default false
		g_gameState.addScriptMessage(hdr, txt);
	}
	return args.getReturnCount();
}

int ScriptManager::setDisplayText(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string txt;
	if (extractArgs(args,"setDisplayText", "str", &txt)) {
		string msg = g_lang.getScenarioString(txt);
		g_gameState.setScriptDisplay(msg);
	}
	return args.getReturnCount();
}

int ScriptManager::clearDisplayText(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	g_gameState.setScriptDisplay("");
	return args.getReturnCount();
}

///@todo move to somewhere sensible
bool isHex(string s) {
	foreach (string, c, s) {
		if (!(isdigit(*c) || ((*c >= 'a' && *c <= 'f') || (*c >= 'A' && *c <= 'F')))) {
			return false;
		}
	}
	return true;
}

Vec3f extractColour(string s) {
	Vec3f colour;
	if (s.size() > 6) { // trim pre/post-fixes
		if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
			s = s.substr(2);
		} else if (s[0] == '#') {
			s = s.substr(1);
		} else if (s[s.size()-1] == 'h' || s[s.size()-1] == 'H') {
			s = s.substr(0, s.size() - 1);
		}
	}
	if (s.size() == 6 && isHex(s)) {
		string tmp = s.substr(0, 2);
		colour.r = float(Conversion::strToInt(tmp, 16));
		tmp = s.substr(2, 2);
		colour.g = float(Conversion::strToInt(tmp, 16));
		tmp = s.substr(4, 2);
		colour.b = float(Conversion::strToInt(tmp, 16));
	} else {
		throw runtime_error("bad colour string " + s);
	}
	return colour;
}

int ScriptManager::addActor(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name, colour;
	int r,g,b;
	Vec3f gl_colour;
	if (extractArgs(args, 0, "str,str", &name, &colour)) {
		gl_colour = extractColour(colour);
	} else if (extractArgs(args,"addActor", "str,int,int,int", &name, &r, &g, &b)) {
		gl_colour.r = float(r);
		gl_colour.g = float(g);
		gl_colour.b = float(b);
	}
	gl_colour /= 255.f;
	actorColours[name] = gl_colour;
	return args.getReturnCount();
}

int ScriptManager::addDialog(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string actor, msg;
	if (extractArgs(args, "addDialog", "str,str", &actor, &msg)) {
		if (actorColours.find(actor) == actorColours.end()) {
			actorColours[actor] = Vec3f(1.f);
		}
		g_userInterface.getDialogConsole()->addDialog(
			actor + " : ", actorColours[actor], g_lang.getScenarioString(msg));
	}
	return args.getReturnCount();
}

int ScriptManager::lockInput(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	g_gameState.lockInput();
	return args.getReturnCount();
}

int ScriptManager::unlockInput(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	g_gameState.unlockInput();
	return args.getReturnCount();
}

int ScriptManager::unfogMap(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	Vec4i area;
	int time;
	if (extractArgs(args, "unfogMap", "int,v4i", &time, &area)) {
		g_world.unfogMap(area,time);
	}
	return args.getReturnCount();
}

int ScriptManager::setCameraPosition(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	Vec2i pos;
	if (extractArgs(args, "setCameraPosition", "v2i", &pos)) {
		g_camera.centerXZ((float)pos.x, (float)pos.y);
	}
	return args.getReturnCount();
}

int ScriptManager::setCameraAngles(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int hAngle, vAngle;
	if (extractArgs(args, 0, "int,int", &hAngle, &vAngle)) {
		g_camera.setAngles(float(hAngle), float(vAngle));
	} else if (extractArgs(args, "setCameraPosition", "int", &hAngle)) {
		g_camera.setAngles(float(hAngle), FLOATINFINITY);
	}
	return args.getReturnCount();
}

int ScriptManager::setCameraDestination(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	Vec2i pos;
	int height, hAngle, vAngle;
	if (extractArgs(args, 0, "int,int,int,int,int", &pos.x, &pos.y, &height, &hAngle, &vAngle)) {
		g_camera.setDest(pos, height, float(hAngle), float(vAngle));
	} else if (extractArgs(args, 0, "int,int,int,int", &pos.x, &pos.y, &height, &hAngle)) {
		g_camera.setDest(pos, height, float(hAngle));
	} else if (extractArgs(args, 0, "int,int,int", &pos.x, &pos.y, &height)) {
		g_camera.setDest(pos, height);
	} else if (extractArgs(args, "setCameraPosition", "v2i", &pos)) {
		g_camera.setDest(pos);
	}
	return args.getReturnCount();
}

int ScriptManager::setCameraMotion(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	Vec2i move;
	Vec2i angle;
	int linFCount, angFCount;
	int linFDelay, angFDelay;
	if (extractArgs(args, "setCameraMotion", "v2i,v2i,int,int,int,int", &move, &angle, &linFCount, &angFCount, &linFDelay, &angFDelay)) {
		g_camera.setCameraMotion(move, angle, linFCount, angFCount, linFDelay, angFDelay);
	}
	return args.getReturnCount();
}

int ScriptManager::createUnit(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string type;
	int fNdx;
	Vec2i pos;
	bool precise;
	if (!extractArgs(args, 0, "str,int,v2i,bln", &type, &fNdx, &pos, &precise)) {
		precise = false;
		if (!extractArgs(args, "createUnit", "str,int,v2i", &type, &fNdx, &pos)) {
			args.returnInt(-1);
			return args.getReturnCount();
		}
	}
	int id = g_world.createUnit(type, fNdx, pos, precise);
	if (id < 0) {
		LuaCmdResult res(id);
		addErrorMessage("Error: createUnit() " + res.getString());
	}
	args.returnInt(id);
	return args.getReturnCount();
}

int ScriptManager::giveResource(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string resource;
	int fNdx, amount;
	if (extractArgs(args, "giveResource", "str,int,int", &resource, &fNdx, &amount)) {
		int err = g_world.giveResource(resource, fNdx, amount);
		if (err < 0) {
			LuaCmdResult res(err);
			addErrorMessage("Error: giveResource() " + res.getString());
		}
	}
	return args.getReturnCount();
}

int ScriptManager::givePositionCommand(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string cmd;
	Vec2i pos;
	if (extractArgs(args, "givePositionCommand", "int,str,v2i", &id, &cmd, &pos)) {
		int res = g_world.givePositionCommand(id, cmd, pos);
		args.returnBool((res == 0 ? true : false));
		if (res < 0) {
			LuaCmdResult lcres(res);
			addErrorMessage("Error: givePositionCommand() " + lcres.getString());
		}
	} else {
		args.returnBool(false);
	}
	return args.getReturnCount();
}

int ScriptManager::giveBuildCommand(LuaHandle * luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string cmd, buildType;
	Vec2i pos;
	if (extractArgs(args, "giveBuildCommand", "int,str,str,v2i", &id, &cmd, &buildType, &pos)) {
		int res = g_world.giveBuildCommand(id, cmd, buildType, pos);
		args.returnBool((res == 0 ? true : false));
		if (res < 0) {
			LuaCmdResult lcres(res);
			addErrorMessage("Error: giveBuildCommand() " + lcres.getString());
		}
	} else {
		args.returnBool(false);
	}
	return args.getReturnCount();
}

int ScriptManager::initSurveyor(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int ndx;
	if (extractArgs(args, "initSurveyor", "int", &ndx)) {
		if (ndx < 0 || ndx >= g_gameSettings.getFactionCount()) {
			addErrorMessage("Error: initSurveyor() faction index out of range: " + intToStr(ndx));
		} else {
			g_world.initSurveyor(g_world.getFaction(ndx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::findLocationForBuilding(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int ndx;
	string buildingType, locationType;
	Vec2i result(-1);
	if (extractArgs(args, "findLocationForBuilding", "int,str,str", &ndx, &buildingType, &locationType)) {
		LocationType locType = LocationTypeNames.match(locationType.c_str());
		const Faction *faction = 0;
		const UnitType *bType = 0;
		if (ndx < 0 || ndx >= g_gameSettings.getFactionCount()) {
			addErrorMessage("Error: findLocationForBuilding() faction index out of range: " + intToStr(ndx));
		}
		faction = g_world.getFaction(ndx);
		if (locType == LocationType::INVALID) {
			addErrorMessage("Error: findLocationForBuilding() location type '" + locationType + "' unknown.");
		} else if (!(bType = faction->getType()->getUnitType(buildingType))) {
			addErrorMessage("Error: findLocationForBuilding() building type '" + buildingType + "' unknown.");
		} else {
			Surveyor *surveyor = g_world.getSurveyor(g_world.getFaction(ndx));
			result = surveyor->findLocationForBuilding(bType, locType);
			if (result == Vec2i(-1)) {
				addErrorMessage("Error: findLocationForBuilding() could not find location for building.");
			}
		}
	}
	args.returnVec2i(result);
	return args.getReturnCount();
}

int ScriptManager::findResourceLocation(LuaHandle * luaHandle) {
	LuaArguments args(luaHandle);
	Vec2i result(-1);
	int ndx;
	string resource;
	if (extractArgs(args, "findResourceLocation", "int,str", &ndx, &resource)) {
		if (ndx < 0 || ndx >= g_gameSettings.getFactionCount()) {
			addErrorMessage("Error: findResourceLocation() faction index out of range: " + intToStr(ndx));
		} else {
			try {
				const ResourceType *rt = g_world.getTechTree()->getResourceType(resource);
				if (rt->getClass() == ResourceClass::TECHTREE || rt->getClass() == ResourceClass::TILESET) {
					result = g_world.getSurveyor(ndx)->findResourceLocation(rt);
				} else {
					addErrorMessage("Error: findResourceLocation() resource is static or consumable: " + resource);
				}
			} catch (runtime_error &e){
				addErrorMessage("Error: findResourceLocation() resource type not found: " + resource);
			}
		}
	}
	args.returnVec2i(result);
	return args.getReturnCount();
}

int ScriptManager::getPlayerCount(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	args.returnInt(g_gameSettings.getFactionCount());
	return args.getReturnCount();
}

int ScriptManager::getHumanPlayerIndex(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	args.returnInt(g_world.getThisFactionIndex());
	return args.getReturnCount();
}

int ScriptManager::getPlayerName(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int ndx;
	const GameSettings &gs = g_gameSettings;
	if (extractArgs(args, "getPlayerName", "int", &ndx)) {
		if (ndx < 0 || ndx >= gs.getFactionCount()) {
			addErrorMessage("Error: getPlayerName() faction index out of range: " + intToStr(ndx));
		} else {
			args.returnString(gs.getPlayerName(ndx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::getPlayerTeam(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int ndx;
	const GameSettings &gs = g_gameSettings;
	if (extractArgs(args, "getPlayerTeam", "int", &ndx)) {
		if (ndx < 0 || ndx >= gs.getFactionCount()) {
			addErrorMessage("Error: getPlayerTeam() faction index out of range: " + intToStr(ndx));
		} else {
			args.returnInt(gs.getTeam(ndx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::getPlayerColour(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int ndx;
	const GameSettings &gs = g_gameSettings;
	if (extractArgs(args, "getPlayerColour", "int", &ndx)) {
		if (ndx < 0 || ndx >= gs.getFactionCount()) {
			addErrorMessage("Error: getPlayerColour() faction index out of range: " + intToStr(ndx));
		} else {
			args.returnString(factionColourStrings[gs.getColourIndex(ndx)]);
		}
	}
	return args.getReturnCount();
}

int ScriptManager::giveTargetCommand(LuaHandle * luaHandle) {
	LuaArguments args(luaHandle);
	int id, id2;
	string cmd;
	if (extractArgs(args, "giveTargetCommand", "int,str,int", &id, &cmd, &id2)) {
		int res = g_world.giveTargetCommand(id, cmd, id2);
		args.returnBool((res == 0 ? true : false));
		if (res < 0) {
			LuaCmdResult lcres(res);
			addErrorMessage("Error: giveTargetCommand() " + lcres.getString());
		}
	} else {
		args.returnBool(false);
	}
	return args.getReturnCount();
}

int ScriptManager::giveStopCommand(LuaHandle * luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string cmd;
	if (extractArgs(args, "giveStopCommand", "int,str", &id, &cmd)) {
		int res = g_world.giveStopCommand(id, cmd);
		args.returnBool((res == 0 ? true : false));
		if (res < 0) {
			LuaCmdResult lcres(res);
			addErrorMessage("Error: giveStopCommand() " + lcres.getString());
		}
	} else {
		args.returnBool(false);
	}
	return args.getReturnCount();
}

int ScriptManager::giveProductionCommand(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string prod;
	if (extractArgs(args, "giveProductionCommand", "int,str", &id, &prod)) {
		int res = g_world.giveProductionCommand(id, prod);
		args.returnBool((res == 0 ? true : false));
		if (res < 0) {
			LuaCmdResult lcres(res);
			addErrorMessage("Error: giveProductionCommand() " + lcres.getString());
		}
	} else {
		args.returnBool(false);
	}
	return args.getReturnCount();
}

int ScriptManager::giveUpgradeCommand(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string prod;
	if (extractArgs(args, "giveUpgradeCommand", "int,str", &id, &prod)) {
		int res = g_world.giveUpgradeCommand(id, prod);
		args.returnBool((res == 0 ? true : false));
		if (res < 0) {
			LuaCmdResult lcres(res);
			addErrorMessage("Error: giveUpgradeCommand() " + lcres.getString());
		}
	} else {
		args.returnBool(false);
	}
	return args.getReturnCount();
}

int ScriptManager::disableAi(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if (extractArgs(args, "disableAi", "int", &fNdx)) {
		if (fNdx >= 0 && fNdx < g_gameSettings.getFactionCount()) {
			playerModifiers[fNdx].enableAi(false);
		} else {
			addErrorMessage("Error: disableAi(): Invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::enableAi(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if (extractArgs(args, "enableAi", "int", &fNdx)) {
		if (fNdx >= 0 && fNdx < g_gameSettings.getFactionCount()) {
			playerModifiers[fNdx].enableAi(true);
		} else {
			addErrorMessage("Error: enableAi(): Invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::disableConsume(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if (extractArgs(args, "disableConsume", "int", &fNdx)) {
		if (fNdx >= 0 && fNdx < g_gameSettings.getFactionCount()) {
			playerModifiers[fNdx].enableConsume(false);
		} else {
			addErrorMessage("Error: disableConsume(): Invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::enableConsume(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if (extractArgs(args, "enableConsume", "int", &fNdx)) {
		if (fNdx >= 0 && fNdx < g_gameSettings.getFactionCount()) {
			playerModifiers[fNdx].enableConsume(true);
		} else {
			addErrorMessage("Error: enableConsume(): Invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::increaseStore(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string resource;
	int fNdx, amount;
	if (extractArgs(args, "increaseStore", "str,int,int", &resource, &fNdx, &amount)) {
		if (fNdx >= 0 && fNdx < g_gameSettings.getFactionCount()) {
			try {
				const ResourceType *rt = g_world.getTechTree()->getResourceType(resource);
				g_world.getFaction(fNdx)->addStore(rt, amount);
			} catch (runtime_error &e) {
				addErrorMessage("Error: increaseStore(): Invalid resource type" + resource);
			}
		} else {
			addErrorMessage("Error: increaseStore(): Invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::setPlayerAsWinner(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if (extractArgs(args, "setPlayerAsWinner", "int", &fNdx)) {
		if (fNdx >= 0 && fNdx < g_gameSettings.getFactionCount()) {
			playerModifiers[fNdx].setAsWinner();
		} else {
			addErrorMessage("Error: setPlayerAsWinner(): invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::endGame(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	gameOver = true;
	return args.getReturnCount();
}

// Queries
int ScriptManager::playerName(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if (extractArgs(args, "playerName", "int", &fNdx)) {
		if (fNdx >= 0 && fNdx < g_gameSettings.getFactionCount()) {
			string playerName= g_gameSettings.getPlayerName(fNdx);
			args.returnString(playerName);
		} else {
			addErrorMessage("playerName(): invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::factionTypeName(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if (extractArgs(args, "factionTypeName", "int", &fNdx)) {
		if (fNdx >= 0 && fNdx < g_gameSettings.getFactionCount()) {
			string factionTypeName = g_gameSettings.getFactionTypeName(fNdx);
			args.returnString(factionTypeName);
		} else {
			addErrorMessage("Error: factionTypeName(): invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::scenarioDir(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	args.returnString(g_gameSettings.getScenarioPath());
	return args.getReturnCount();
}

int ScriptManager::startLocation(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if (extractArgs(args, "startLocation", "int", &fNdx)) {
		Vec2i pos= g_world.getStartLocation(fNdx);
		if (pos == Vec2i(-1)) {
			addErrorMessage("Error: startLocation(): invalid faction index " + intToStr(fNdx));
		}
		args.returnVec2i(pos);
	}
	return args.getReturnCount();
}

int ScriptManager::unitPosition(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	if (extractArgs(args, "unitPosition", "int", &id)) {
		Vec2i pos= g_world.getUnitPosition(id);
		if (pos == Vec2i(-1)) {
			addErrorMessage("Error: unitPosition(): Can not find unit=" + intToStr(id) + " to get position");
		}
		args.returnVec2i(pos);
	}
	return args.getReturnCount();
}

int ScriptManager::unitFaction(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	if (extractArgs(args, "unitFaction", "int", &id)) {
		int factionIndex = g_world.getUnitFactionIndex(id);
		if (id < 0) {
			LuaCmdResult lcres(id);
			addErrorMessage("Error: unitFaction(): " + lcres.getString());
		}
		args.returnInt(factionIndex);
	}
	return args.getReturnCount();
}

int ScriptManager::resourceAmount(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string resource;
	int fNdx;
	if (extractArgs(args, "resourceAmount", "str,int", &resource, &fNdx)) {
		int amount = g_world.getResourceAmount(resource, fNdx);
		if (amount < 0) {
			LuaCmdResult lcres(amount);
			addErrorMessage("Error: resourceAmount(): " + lcres.getString());
		}
		args.returnInt(amount);
	}
	return args.getReturnCount();
}

int ScriptManager::lastCreatedUnitName(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	if (latestCreated.id == -1) {
		addErrorMessage("Error: lastCreatedUnitName(): called before any units created.");
	}
	args.returnString(latestCreated.name);
	return args.getReturnCount();
}

int ScriptManager::lastCreatedUnit(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	if (latestCreated.id == -1) {
		addErrorMessage("Error: lastCreatedUnit(): called before any units created.");
	}
	args.returnInt(latestCreated.id);
	return args.getReturnCount();
}

int ScriptManager::lastDeadUnitName(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	if (latestCasualty.id == -1) {
		addErrorMessage("Error: lastDeadUnitName(): called before any units died.");
	}
	args.returnString(latestCasualty.name);
	return args.getReturnCount();
}

int ScriptManager::lastDeadUnit(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	if (latestCasualty.id == -1) {
		addErrorMessage("Error: lastDeadUnit(): called before any units died.");
	}
	args.returnInt(latestCasualty.id);
	return args.getReturnCount();
}

int ScriptManager::unitCount(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if (extractArgs(args, "unitCount", "int", &fNdx)) {
		int amount = g_world.getUnitCount(fNdx);
		if (amount == -1) {
			addErrorMessage("unitCount() invalid faction index " + intToStr(fNdx));
		}
		args.returnInt(amount);
	}
	return args.getReturnCount();
}

int ScriptManager::unitCountOfType(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	string type;
	if (extractArgs(args, "unitCountOfType", "int,str", &fNdx, &type)) {
		int amount = g_world.getUnitCountOfType(fNdx, type);
		if (amount < 0) {
			LuaCmdResult lcres(amount);
			addErrorMessage("Error: unitCountOfType(): " + lcres.getString());
		}
		args.returnInt(amount);
	}
	return args.getReturnCount();
}

int ScriptManager::giveUpgrade(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	string upgrade;
	if (extractArgs(args, "giveUpgrade", "int,str", &fNdx, &upgrade)) {
		if (fNdx >= 0 && fNdx < g_world.getFactionCount()) {
			Faction *faction = g_world.getFaction(fNdx);
			const FactionType *ft = faction->getType();
			try {
				const UpgradeType *ut = ft->getUpgradeType(upgrade);
				faction->startUpgrade(ut);
				faction->finishUpgrade(ut);
			} catch (runtime_error &e) {
				addErrorMessage("Error: giveUpgrade(): invalid upgrade '" + upgrade + "'");
			}
		} else {
			addErrorMessage("Error: giveUpgrade(): invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::damageUnit(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int unitId, hp;
	if (extractArgs(args, "damageUnit", "int,int", &unitId, &hp)) {
		Unit *unit = g_world.getUnit(unitId);
		if (unit) {
			if (hp > 0) {
				g_world.damage(unit, hp);
			} else {
				addErrorMessage("damageUnit(): invalid hp amount " + intToStr(hp));
			}
		} else {
			addErrorMessage("damageUnit(): invalid unit id " + intToStr(unitId));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::destroyUnit(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int unitId;
	bool zap;
	bool good = false;
	if (extractArgs(args, 0,"int,bln", &unitId, &zap)) {
		good = true;
	} else if (extractArgs(args, "destroyUnit", "int", &unitId)) {
		zap = false;
		good = true;
	}
	if (good) {
		Unit *unit = g_world.getUnit(unitId);
		if (unit) {
			if (!unit->isAlive()) {
				addErrorMessage("destroyUnit(): unit with id " + intToStr(unitId) + " is already dead.");
			} else if (!zap) {
				g_world.damage(unit, unit->getHp());
			} else {
				g_world.damage(unit, unit->getHp());
				unit->undertake();
				g_world.getUnitFactory().deleteUnit(unit);
			}
		} else {
			addErrorMessage("destroyUnit(): invalid unit id " + intToStr(unitId));
		}
	}
	return args.getReturnCount();
}


IF_DEBUG_EDITION(

	int ScriptManager::hilightRegion(LuaHandle *luaHandle) {
		LuaArguments args(luaHandle);
		string region;
		if (extractArgs(args, "hilightRegion", "str", &region)) {
			const Region *r = triggerManager.getRegion(region);
			if (r) {
				const Rect *rect = static_cast<const Rect*>(r);
				for (int y = rect->y; y < rect->y + rect->h; ++y) {
					for (int x = rect->x; x < rect->x + rect->w; ++x) {
						g_debugRenderer.addCellHighlight(Vec2i(x,y));
					}
				}
			} else {
				addErrorMessage("hilightRegion(): region '" + region + "' not found");
			}
		}
		return args.getReturnCount();
	}

	int ScriptManager::hilightCell(LuaHandle *luaHandle) {
		LuaArguments args(luaHandle);
		Vec2i cell;
		if (extractArgs(args, "hilightCell", "v2i", &cell)) {
			g_debugRenderer.addCellHighlight(cell);
		}
		return args.getReturnCount();
	}

	int ScriptManager::clearHilights(LuaHandle *luaHandle) {
		LuaArguments args(luaHandle);
		g_debugRenderer.clearCellHilights();
		return args.getReturnCount();
	}

	int ScriptManager::debugSet(LuaHandle *luaHandle) {
		LuaArguments args(luaHandle);
		string line;
		if (extractArgs(args, "debugSet", "str", &line)) {
			g_debugRenderer.commandLine(line);
		}
		return args.getReturnCount();
	}

	int ScriptManager::setFarClip(LuaHandle *luaHandle) {
		LuaArguments args(luaHandle);
		int clip;
		if (extractArgs(args, "setFarClip", "int", &clip)) {
			Renderer::getInstance().setFarClip(float(clip));
		}
		return args.getReturnCount();
	}
) // DEBUG_EDITION

int ScriptManager::dofile(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	string path;
	if (extractArgs(args, "dofile", "str", &path)) {
		try {
			FileOps *f = g_fileFactory.getFileOps();
			f->openRead(path.c_str());
			int size = f->fileSize();
			char *someLua = new char[size + 1];
			f->read(someLua, size, 1);
			someLua[size] = '\0';
			delete f;
				if (!luaScript.luaDoLine(someLua)) {
					addErrorMessage();
				}
			delete [] someLua;
		} catch (runtime_error &e) {
			addErrorMessage(e.what());
		}
	}
	return args.getReturnCount();
}

}}//end namespace

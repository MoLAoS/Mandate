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

#include <stdarg.h>

#include "script_manager.h"

#include "world.h"
#include "lang.h"
#include "game_camera.h"
#include "game.h"

#include "leak_dumper.h"

#if DEBUG_RENDERING_ENABLED
#	include "debug_renderer.h"
#endif

using namespace Shared::Platform;
using namespace Shared::Lua;

namespace Glest { namespace Game {

//TODO move these somewhere sensible [after branch is re-intergrated]
ostream& operator<<(ostream &lhs, Vec2i &rhs) {
	return lhs << "(" << rhs.x << ", " << rhs.y << ")";
}
ostream& operator<<(ostream &lhs, Vec4i &rhs) {
	return lhs << "(" << rhs.x << ", " << rhs.y << ", " << rhs.z << ", " << rhs.w << ")";
}

// =====================================================
//	class ScriptTimer
// =====================================================

bool ScriptTimer::isReady() const {
	if (real) {
		return Chrono::getCurMillis() >= targetTime;
	} else {
		return theWorld.getFrameCount() >= targetTime;
	}
}

void ScriptTimer::reset() {
	if (real) {
		targetTime = Chrono::getCurMillis() + interval;
	} else {
		targetTime = theWorld.getFrameCount() + interval;
	}
}

// =====================================================
//	class TriggerManager
// =====================================================

TriggerManager::~TriggerManager() {
	for ( Regions::iterator it = regions.begin(); it != regions.end(); ++it ) {
		delete it->second;
	}
}
	
void TriggerManager::reset(World *world) {
	for ( Regions::iterator it = regions.begin(); it != regions.end(); ++it ) {
		delete it->second;
	}
	regions.clear();
	events.clear();
	unitPosTriggers.clear();
	attackedTriggers.clear();
	hpBelowTriggers.clear();
	hpAboveTriggers.clear();
	commandCallbacks.clear();
	this->world = world;
}

bool TriggerManager::registerRegion(const string &name, const Rect &rect) {
	if ( regions.find(name) != regions.end() ) return false;
 	Region *region = new Rect(rect);
 	regions[name] = region;
 	return true;
 }

int TriggerManager::registerEvent(const string &name) {
	if ( events.find(name) != events.end() ) return -1;
	events.insert(name);
	return 0;
}

int TriggerManager::addCommandCallback(int unitId, const string &eventName, int userData) {
	Unit *unit = theWorld.findUnitById(unitId);
	if ( !unit ) return -1;
	unit->setCommandCallback();
	commandCallbacks[unitId].evnt = eventName;
	commandCallbacks[unitId].user_dat = userData;
	return 0;
}

int TriggerManager::addHPBelowTrigger(int unitId, int threshold, const string &eventName, int userData) {
	Unit *unit = theWorld.findUnitById(unitId);
	if ( !unit ) return -1;
	if ( unit->getHp() < threshold ) return -2;
	unit->setHPBelowTrigger(threshold);
	hpBelowTriggers[unit->getId()].evnt = eventName;
	hpBelowTriggers[unit->getId()].user_dat = userData;
	return 0;
}

int TriggerManager::addHPAboveTrigger(int unitId, int threshold, const string &eventName, int userData) {
	Unit *unit = theWorld.findUnitById(unitId);
	if ( !unit ) return -1;
	if ( unit->getHp() > threshold ) return -2;
	unit->setHPAboveTrigger(threshold);
	hpAboveTriggers[unit->getId()].evnt = eventName;
	hpAboveTriggers[unit->getId()].user_dat = userData;
	return 0;
}

int TriggerManager::addDeathTrigger(int unitId, const string &eventName, int userData) {
	Unit *unit = theWorld.findUnitById(unitId);
	if ( !unit ) return -1;
	if ( !unit->isAlive() ) return -2;
	deathTriggers[unitId].evnt = eventName;
	deathTriggers[unitId].user_dat = userData;
	return 0;
}
void TriggerManager::unitMoved(const Unit *unit) {
	// check id
	int id = unit->getId();
	PosTriggerMap::iterator tmit = unitPosTriggers.find(id);
	
	if ( tmit != unitPosTriggers.end() ) {
		PosTriggers &triggers = tmit->second;
	start:
		PosTriggers::iterator it = triggers.begin();
 		while ( it != triggers.end() ) {
			if ( it->region->isInside(unit->getPos()) ) {
				// setting another trigger on this unit in response to this trigger will cause our 
				// iterators here to go bad... :(
				string evnt = it->evnt;
				int ud = it->user_dat;
				it = triggers.erase(it);
				ScriptManager::onTrigger(evnt, id, ud);
				goto start;
 			} else {
 				++it;
 			}
		}
	}
	tmit = factionPosTriggers.find(unit->getFactionIndex());
	if ( tmit != factionPosTriggers.end() ) {
		PosTriggers &triggers = tmit->second;
	start2:
		PosTriggers::iterator it = triggers.begin();
 		while ( it != triggers.end() ) {
			if ( it->region->isInside(unit->getPos()) ) {
				string evnt = it->evnt;
				int ud = it->user_dat;
				it = triggers.erase(it);
				ScriptManager::onTrigger(evnt, id, ud);
				goto start2;
 			} else {
 				++it;
 			}
		}
	}
}

void TriggerManager::unitDied(const Unit *unit) {
	const int &id = unit->getId();
	unitPosTriggers.erase(unit->getId());
	commandCallbacks.erase(unit->getId());
	attackedTriggers.erase(unit->getId());
	hpBelowTriggers.erase(unit->getId());
	hpAboveTriggers.erase(unit->getId());
	TriggerMap::iterator it = deathTriggers.find(unit->getId());
	if ( it != deathTriggers.end() ) {
		ScriptManager::onTrigger(it->second.evnt, unit->getId(), it->second.user_dat);
		deathTriggers.erase(it);
	}	
}

void TriggerManager::commandCallback(const Unit *unit) {
	TriggerMap::iterator it = commandCallbacks.find(unit->getId());
	if ( it == commandCallbacks.end() ) return;
	string evnt = it->second.evnt;
	int ud = it->second.user_dat;
	commandCallbacks.erase(it);
	ScriptManager::onTrigger(evnt, unit->getId(), ud);
}

void TriggerManager::onHPBelow(const Unit *unit) {
	TriggerMap::iterator it = hpBelowTriggers.find(unit->getId());
	if ( it == hpBelowTriggers.end() ) return;
	string evnt = it->second.evnt;
	int ud = it->second.user_dat;
	hpBelowTriggers.erase(it);
	ScriptManager::onTrigger(evnt, unit->getId(), ud);
}

void TriggerManager::onHPAbove(const Unit *unit) {
	TriggerMap::iterator it = hpAboveTriggers.find(unit->getId());
	if ( it == hpAboveTriggers.end() ) {
		return;
	}
	string evnt = it->second.evnt;
	int ud = it->second.user_dat;
	hpAboveTriggers.erase(it);
	ScriptManager::onTrigger(evnt, unit->getId(), ud);
}

/** @return 0 if ok, -1 if bad unit id, -2 if event not found, -3 region not found,
  * -4 unit already has a trigger for this region,event pair */
int TriggerManager::addUnitPosTrigger	(int unitId, const string &region, const string &eventName, int userData) {
	//theLogger.add("adding unit="+intToStr(unitId)+ ", event=" + eventName + " trigger");
	Unit *unit = theWorld.findUnitById(unitId);
	if ( !unit ) return -1;
	if ( events.find(eventName) == events.end() ) return -2;
	Region *rgn = NULL;
	if ( regions.find(region) != regions.end() ) rgn = regions[region];
	if ( !rgn ) return -3;
	if ( unitPosTriggers.find(unitId) == unitPosTriggers.end() ) {
		unitPosTriggers.insert(pair<int,PosTriggers>(unitId,PosTriggers()));
 	}
	PosTriggers &triggers = unitPosTriggers.find(unitId)->second;
	PosTriggers::iterator it = triggers.begin();
	for ( ; it != triggers.end(); ++it ) {
		if ( it->region == rgn && it->evnt == eventName ) {
			return -4;
		}
 	}
	triggers.push_back(PosTrigger());
	triggers.back().region = rgn;
	triggers.back().evnt = eventName;
	triggers.back().user_dat = userData;
	return 0;
}

/** @return 0 if ok, -1 if bad index id, -2 if event not found, -3 region not found,
  * -4 faction already has a trigger for this region,event pair */
int TriggerManager::addFactionPosTrigger (int ndx, const string &region, const string &eventName, int userData) {
	//theLogger.add("adding unit="+intToStr(unitId)+ ", event=" + eventName + " trigger");
	if ( ndx < 0 || ndx >= GameConstants::maxPlayers ) return -1;
	if ( events.find(eventName) == events.end() ) return -2;
	Region *rgn = NULL;
	if ( regions.find(region) != regions.end() ) rgn = regions[region];
	if ( !rgn ) return -3;

	if ( factionPosTriggers.find(ndx) == factionPosTriggers.end() ) {
		factionPosTriggers.insert(pair<int,PosTriggers>(ndx,PosTriggers()));
 	}
	PosTriggers &triggers = factionPosTriggers.find(ndx)->second;
	PosTriggers::iterator it = triggers.begin();
	for ( ; it != triggers.end(); ++it ) {
		if ( it->region == rgn && it->evnt == eventName ) {
			return -4;
		}
 	}
	triggers.push_back(PosTrigger());
	triggers.back().region = rgn;
	triggers.back().evnt = eventName;
	triggers.back().user_dat = userData;
	return 0;
}

TriggerManager ScriptManager::triggerManager;

// =====================================================
//	class ScriptManager
// =====================================================

// ========== statics ==========

const int ScriptManager::messageWrapCount= 30;
const int ScriptManager::displayTextWrapCount= 64;

string				ScriptManager::code;
LuaScript			ScriptManager::luaScript;
GraphicMessageBox	ScriptManager::messageBox;
string				ScriptManager::displayText;
bool				ScriptManager::gameOver;
PlayerModifiers		ScriptManager::playerModifiers[GameConstants::maxPlayers];
vector<ScriptTimer> ScriptManager::timers;
vector<ScriptTimer> ScriptManager::newTimerQueue;
set<string>			ScriptManager::definedEvents;

Game  *ScriptManager::game  = NULL;
World *ScriptManager::world = NULL;

ScriptManager::MessageQueue ScriptManager::messageQueue;
ScriptManager::UnitInfo		ScriptManager::latestCreated,
							ScriptManager::latestCasualty;

#define LUA_FUNC(x) luaScript.registerFunction(x, #x)

void ScriptManager::init(Game *g) {
	game = g;
	world = game->getWorld();
	const Scenario*	scenario = world->getScenario();
	assert(scenario);
	luaScript.startUp();
	luaScript.atPanic(panicFunc);
	assert(!luaScript.isDefined("startup")); // making sure old code is gone

	//register functions
	LUA_FUNC(setTimer);
	LUA_FUNC(stopTimer);

	LUA_FUNC(registerRegion);
	LUA_FUNC(registerEvent);
	LUA_FUNC(setUnitTrigger);
	LUA_FUNC(setUnitTriggerX);
	LUA_FUNC(setFactionTrigger);
	
	LUA_FUNC(showMessage);
	LUA_FUNC(setDisplayText);
	LUA_FUNC(clearDisplayText);

	LUA_FUNC(lockInput);
	LUA_FUNC(unlockInput);
	LUA_FUNC(setCameraPosition);
	LUA_FUNC(unfogMap);

	LUA_FUNC(createUnit);
	LUA_FUNC(giveResource);
	LUA_FUNC(givePositionCommand);
	LUA_FUNC(giveProductionCommand);
	LUA_FUNC(giveStopCommand);
	LUA_FUNC(giveTargetCommand);
	LUA_FUNC(giveUpgradeCommand);
	LUA_FUNC(disableAi);
	LUA_FUNC(setPlayerAsWinner);
	LUA_FUNC(endGame);
	LUA_FUNC(debugLog);
	LUA_FUNC(consoleMsg);

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
	
#	if DEBUG_RENDERING_ENABLED

	LUA_FUNC(hilightRegion);
	LUA_FUNC(hilightCell);
	LUA_FUNC(clearHilights);

#	endif

	//setup message box
	messageBox.init("", Lang::getInstance().get("Ok"));
	messageBox.setEnabled(false);

	//last created unit
	latestCreated.id = -1;
	latestCasualty.id = -1;
	gameOver= false;

	//load code
	for(int i= 0; i<scenario->getScriptCount(); ++i){
		const Script* script= scenario->getScript(i);
		bool ok;
		if ( script->getName().substr(0,9) == "unitEvent" ) {
			ok = luaScript.loadCode("function " + script->getName() + "(unit_id)" + script->getCode() + "\nend\n", script->getName());
		} else {
			ok = luaScript.loadCode("function " + script->getName() + "()" + script->getCode() + "\nend\n", script->getName());
		}
		if ( !ok ) {
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
	for ( int i=0; i < theWorld.getFactionCount(); ++i ) {
		const FactionType *f = theWorld.getFaction(i)->getType();
		for ( int j=0; j < f->getUnitTypeCount(); ++j ) {
			const UnitType *ut = f->getUnitType(j);
			funcNames.insert("unitCreatedOfType_" + ut->getName());
		}
	}
	for ( set<string>::iterator it = funcNames.begin(); it != funcNames.end(); ++it ) {
		if ( luaScript.isDefined(*it) ) {
			definedEvents.insert(*it);
		}
	}

	triggerManager.reset(world);

	//call startup function
	if ( definedEvents.find("startup") != definedEvents.end() ) {
		luaScript.luaCall("startup");
	} else {
		addErrorMessage("Warning, no startup script defined", true);
	}
}

// ========================== events ===============================================

void ScriptManager::onMessageBoxOk() {
	Lang &lang= Lang::getInstance();
	
	if(!messageQueue.empty()){
		messageQueue.pop();
		if(!messageQueue.empty()){
			messageBox.setText(wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount));
			messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
		}
	}

	//close the messageBox now all messages have been shown
	if ( messageQueue.empty() ) {
		messageBox.setEnabled(false);
		theGame.resume();
	}
}

void ScriptManager::onResourceHarvested(){
	if ( definedEvents.find( "resourceHarvested" ) != definedEvents.end() ) {
		if ( !luaScript.luaCall("resourceHarvested") ) {
			addErrorMessage();
		}
	}
}

void ScriptManager::onUnitCreated(const Unit* unit){
	latestCreated.name = unit->getType()->getName();
	latestCreated.id = unit->getId();
	if ( definedEvents.find( "unitCreated" ) != definedEvents.end() ) {
		if ( !luaScript.luaCall("unitCreated") ) {
			addErrorMessage();
		}
	}
	if ( definedEvents.find( "unitCreatedOfType_"+latestCreated.name ) != definedEvents.end() ) {
		if ( !luaScript.luaCall("unitCreatedOfType_"+latestCreated.name) ) {
			addErrorMessage();
		}
	}
}

void ScriptManager::onUnitDied(const Unit* unit){
	latestCasualty.name = unit->getType()->getName();
	latestCasualty.id = unit->getId();
	if ( definedEvents.find( "unitDied" ) != definedEvents.end() ) {
		if ( !luaScript.luaCall("unitDied") ) {
			addErrorMessage();
		}
	}
	triggerManager.unitDied(unit);
}

void ScriptManager::onTrigger(const string &name, int unitId, int userData) {
	if ( !luaScript.luaCallback("unitEvent_" + name, unitId, userData) ) {
		addErrorMessage("unitEvent_" + name + "(id:" + intToStr(unitId) + ", userData:"
			+ intToStr(userData) +"): call failed.");
		addErrorMessage();
	}
}

void ScriptManager::onTimer() {
	// when a timer is ready, call the corresponding lua function
	// and remove the timer, or reset to repeat.

	vector<ScriptTimer>::iterator timer;

	for (timer = timers.begin(); timer != timers.end();) {
		if ( timer->isReady() ) {
			if ( timer->isAlive() ) {
				if ( ! luaScript.luaCall("timer_" + timer->getName()) ) {
					timer->kill();
					addErrorMessage();
				}
			}
			if ( timer->isPeriodic() && timer->isAlive() ) {
				timer->reset();
			} 
			else {
				timer = timers.erase(timer); //returns next element
				continue;
			}
		}
		++timer;
	}
	for ( vector<ScriptTimer>::iterator it = newTimerQueue.begin(); it != newTimerQueue.end(); ++it ) {
		timers.push_back (*it);
	}
	newTimerQueue.clear();
}

// =============== util ===============

string ScriptManager::wrapString(const string &str, int wrapCount){

	string returnString;

	int letterCount= 0;
	for(int i= 0; i<str.size(); ++i){
		if(letterCount>wrapCount && str[i]==' '){
			returnString+= '\n';
			letterCount= 0;
		}
		else {
			returnString+= str[i];
		}
		++letterCount;
	}

	return returnString;
}

void ScriptManager::doSomeLua(string &code) {
	if ( !luaScript.luaDoLine(code) ) {
		addErrorMessage();
	}
}


// =============== Error handling bits ===============

void ScriptManager::addErrorMessage(const char *txt, bool quietly) {
	theGame.pause();
	
	string err = txt ? txt : luaScript.getLastError();
	theLogger.getErrorLog().add(err);
	theConsole.addLine(err);
	
	if ( !quietly ) {
		ScriptManagerMessage msg(err, "Error");
		messageQueue.push(msg);
		if ( !messageBox.getEnabled() ) {
			messageBox.setEnabled(true);
			messageBox.setText(wrapString(messageQueue.front().getText(), messageWrapCount));
			messageBox.setHeader(messageQueue.front().getHeader());
		}
	}
}

/** Extracts arguments for Lua callbacks.
  * <p>uses a string description of the expected arguments in arg_desc, then pointers the 
  * corresponding variables to populate the results.</p>
  * @param luaArgs the LuaArguments object
  * @param caller name of the lua callback, in case of errors
  * @param arg_desc argument descriptor, a string of the form "[xxx[,xxx[,etc]]]" where xxx can be any of
  * <ul><li>'int' for an integer</li>
  *		<li>'str' for a string</li>
  *		<li>'bln' for a boolean</li>
  *		<li>'v2i' for a Vec2i</li>
  *		<li>'v4i' for a Vec4i</li></ul>
  * @param ... a pointer to the appropriate type for each argument specified in arg_desc
  * @return true if arguments were extracted successfully, false if there was an error
  */
bool ScriptManager::extractArgs(LuaArguments &luaArgs, const char *caller, const char *arg_desc, ...) {
	if ( !*arg_desc ) {
		if ( luaArgs.getArgumentCount() ) {
			addErrorMessage(string(caller) + "(): expected 0 arguments, got " 
					+ intToStr(luaArgs.getArgumentCount()) );
			return false;
		} else {
			return true;
		}
	}
	const char *ptr = arg_desc;
	int expected = 1;
	while ( *ptr ) {
		if ( *ptr++ == ',' ) expected++;
	}
	if ( expected != luaArgs.getArgumentCount() ) {
		addErrorMessage(string(caller) + "(): expected " + intToStr(expected) 
			+ " arguments, got " + intToStr(luaArgs.getArgumentCount()));
		return false;
	}
	va_list vArgs;
	va_start(vArgs, arg_desc);
	char *tmp = strcpy (new char[strlen(arg_desc)], arg_desc);
	char *tok = strtok(tmp, ",");
	try {
		while ( tok ) {
			if ( strcmp(tok,"int") == 0 ) {
				*va_arg(vArgs,int*) = luaArgs.getInt(-expected);
			} else if ( strcmp(tok,"str") == 0 ) {
				*va_arg(vArgs,string*) = luaArgs.getString(-expected);
			} else if ( strcmp(tok,"bln") == 0 ) {
				*va_arg(vArgs,bool*) = luaArgs.getBoolean(-expected);
			} else if ( strcmp(tok,"v2i") == 0 ) {
				*va_arg(vArgs,Vec2i*) = luaArgs.getVec2i(-expected);
			} else if ( strcmp(tok,"v4i") == 0 ) {
				*va_arg(vArgs,Vec4i*) = luaArgs.getVec4i(-expected);
			} else {
				throw runtime_error("ScriptManager::extractArgs() passed bad arg_desc string");
			}
			--expected;
			tok = strtok(NULL, ",");
		}
	} catch ( LuaError e ) {
		addErrorMessage("Error: " + string(caller) + "() " + e.desc());
		va_end(vArgs);
		return false;
	}
	return true;
}

int ScriptManager::panicFunc(LuaHandle *luaHandle) {
	Logger::getErrorLog().add("Fatal Error: Lua panic.");
	return 0;
}

// ========================== lua callbacks ===============================================

int ScriptManager::debugLog(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	string msg;
	if ( extractArgs(args, "debugLog", "str", &msg) ) {
		Logger::getErrorLog().add(msg);
	}
	return args.getReturnCount();
}

int ScriptManager::consoleMsg (LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	string msg;
	if ( extractArgs(args, "consoleMsg", "str", &msg) ) {
		theConsole.addLine(msg);
	}
	return args.getReturnCount();
}

int ScriptManager::setTimer(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name, type;
	int period;
	bool repeat;
	if ( extractArgs(args, "setTimer", "str,str,int,bln", &name, &type, &period, &repeat) ) {
		if ( type == "real" ) {
			newTimerQueue.push_back(ScriptTimer(name, true, period, repeat));
		} else if ( type == "game" ) {
			newTimerQueue.push_back(ScriptTimer(name, false, period, repeat));
		} else {
			addErrorMessage("setTimer(): invalid type '" + type + "'");
		}
	}
	return args.getReturnCount(); // == 0
}

int ScriptManager::stopTimer(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string name;
	if ( extractArgs(args, "stopTimer", "str", &name) ) {
		vector<ScriptTimer>::iterator i;
		bool killed = false;
		for ( i = timers.begin(); i != timers.end(); ++i ) {
			if ( i->getName() == name ) {
				i->kill ();
				killed = true;
				break;
			}
		}
		if ( !killed ) {
			addErrorMessage("stopTimer(): timer '" + name + "' not found.");
		}
	} 
	return args.getReturnCount();
}

int ScriptManager::registerRegion(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	string name;
	Vec4i rect;
	if ( extractArgs(args, "registerRegion", "str,v4i", &name, &rect) ) {
		if ( !triggerManager.registerRegion(name, rect) ) {
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
	if ( extractArgs(args, "registerEvent", "str", &name) ) {
		if ( triggerManager.registerEvent(name) == -1 ) {
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
	if ( extractArgs(args, "setUnitTriggerX", "int,str,str,int", &id, &cond, &evnt, &ud) ) {
		doUnitTrigger(id, cond, evnt, ud);
	}
	return args.getReturnCount();
}

int ScriptManager::setUnitTrigger(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string cond, evnt;
	if ( extractArgs(args, "setUnitTrigger", "int,str,str", &id, &cond, &evnt) ) {
		doUnitTrigger(id, cond, evnt, 0);
	}
	return args.getReturnCount();
}

void ScriptManager::doUnitTrigger(int id, string &cond, string &evnt, int ud) {
	bool did_something = false;
	if ( cond == "attacked" ) { // nop
	} else if ( cond == "death" ) { // nop
		did_something = true;
	} else if ( cond == "enemy_sighted" ) { // nop
	} else if ( cond == "command_callback") {
		if ( triggerManager.addCommandCallback(id,evnt) == -1 ) {
			addErrorMessage("setUnitTrigger(): unit id invalid " + intToStr(id));
		}
		did_something = true;
	} else { // 'complex' conditions
		size_t ePos = cond.find('=');
		if ( ePos != string::npos ) {
			string key = cond.substr(0, ePos);
			string val = cond.substr(ePos+1);
			if ( key == "hp_below" ) {
				int threshold = atoi(val.c_str());
				if ( threshold < 1 ) {
					addErrorMessage("setUnitTrigger(): invalid hp_below condition = '" + val + "'");
				} else {
					int res = triggerManager.addHPBelowTrigger(id, threshold, evnt);
					if ( res == -1 ) {
						addErrorMessage("setUnitTrigger(): unit id invalid " + intToStr(id));
					} else if ( res == -2 ) {
						addErrorMessage("setUnitTrigger(): hp_below=" + intToStr(threshold) + ", unit doesn't have that many hp");
					}
				}
				did_something = true;
			} else if ( key == "hp_above" ) {
				int threshold = atoi(val.c_str());
				if ( threshold < 1 ) {
					addErrorMessage("setUnitTrigger(): invalid hp_above condition = '" + val + "'");
				} else {
					int res = triggerManager.addHPAboveTrigger(id, threshold, evnt);
					if ( res == -1 ) {
						addErrorMessage("setUnitTrigger(): unit id invalid " + intToStr(id));
					} else if ( res == -2 ) {
						addErrorMessage("setUnitTrigger(): hp_above=" + intToStr(threshold) + ", unit already has that many hp");
					}
				}
				did_something = true;
			} else if ( key == "region" ) {
				int res = triggerManager.addUnitPosTrigger(id,val,evnt,ud);
				if ( res == -1 ) {
					addErrorMessage("setUnitTrigger(): unit id invalid " + intToStr(id));
				} else if ( res == -2 ) {
					addErrorMessage("setUnitTrigger(): unkown event  '" + evnt + "'");
				} else if ( res == -3 ) {
					addErrorMessage("setUnitTrigger(): unkown region  '" + val + "'");
				} else if ( res == -4 ) {
					addErrorMessage("setUnitTrigger(): unit " + intToStr(id) 
						+ " already has a trigger for this region,event pair "
						+ "'" + val + ", " + evnt + "'");
				}
				did_something = true;
			}
		}
	}
	if ( !did_something ) {
		addErrorMessage("setUnitTrigger(): invalid condition = '" + cond + "'");
	}
}

int ScriptManager::setFactionTrigger(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int ndx, ud;
	string cond, evnt;
	if ( extractArgs(args, "setFactionTrigger", "int,str,str,int", &ndx, &cond, &evnt, &ud) ) {
		size_t ePos = cond.find('=');
		bool did_something = false;
		if ( ePos != string::npos ) {
			string key = cond.substr(0, ePos);
			string val = cond.substr(ePos+1);
			if ( key == "region" ) {
				int res = triggerManager.addFactionPosTrigger(ndx,val,evnt, ud);
				if ( res == -1 ) {
					addErrorMessage("setFactionTrigger(): invalid factio index" + intToStr(ndx));
				} else if ( res == -2 ) {
					addErrorMessage("setFactionTrigger(): unkown event  '" + evnt + "'");
				} else if ( res == -3 ) {
					addErrorMessage("setFactionTrigger(): unkown region  '" + val + "'");
				} else if ( res == -4 ) {
					addErrorMessage("setFactionTrigger(): faction " + intToStr(ndx) 
						+ " already has a trigger for this region,event pair "
						+ "'" + val + ", " + evnt + "'");
				}
				did_something = true;
			}
		}
		if ( !did_something ) {
			addErrorMessage("setFactionTrigger(): invalid condition = '" + cond + "'");
		}
	}
	return args.getReturnCount();
}

int ScriptManager::showMessage(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	Lang &lang= Lang::getInstance();
	string txt, hdr;
	if ( extractArgs(args, "showMessage", "str,str", &txt, &hdr) ) {
		theGame.pause ();
		ScriptManagerMessage msg(txt, hdr);
		messageQueue.push ( msg );
		if ( !messageBox.getEnabled() ) {
			messageBox.setEnabled ( true );
			messageBox.setText ( wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount) );
			messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::setDisplayText(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string txt;
	if ( extractArgs(args,"setDisplayText", "str", &txt) ) {
		displayText= wrapString(Lang::getInstance().getScenarioString(txt), displayTextWrapCount);
	}
	return args.getReturnCount();
}

int ScriptManager::clearDisplayText(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	displayText= "";
	return args.getReturnCount();
}

int ScriptManager::lockInput(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	theGame.lockInput();
	return args.getReturnCount();
}

int ScriptManager::unlockInput(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	theGame.unlockInput();
	return args.getReturnCount();
}

int ScriptManager::unfogMap(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	Vec4i area;
	int time;
	if ( extractArgs(args, "unfogMap", "int,v4i", &time, &area) ) {
		theWorld.unfogMap(area,time);
	}
	return args.getReturnCount();
}

int ScriptManager::setCameraPosition(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	Vec2i pos;
	if ( extractArgs(args, "setCameraPosition", "v2i", &pos) ) {
		theCamera.centerXZ((float)pos.x, (float)pos.y);
	}
	return args.getReturnCount();
}

int ScriptManager::createUnit(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string type;
	int fNdx;
	Vec2i pos;
	if ( extractArgs(args, "createUnit", "str,int,v2i", &type, &fNdx, &pos) ) {
		int id = theWorld.createUnit(type, fNdx, pos);
		if ( id < 0 ) {
			stringstream ss;
			switch (id) {
				case -1: ss << "createUnit(): invalid faction index " << fNdx; break;
				case -2: ss << "createUnit(): invalid unit type '" << type << "' for faction " << fNdx; break;
				case -3: ss << "createUnit(): unit could not be placed near " << pos; break;
				case -4: ss << "createUnit(): invalid positon " << pos; break;
				default: throw runtime_error("In ScriptManager::createUnit(), World::createUnit() returned unkown error code");
			}
			addErrorMessage(ss.str());
		}
	}
	return args.getReturnCount(); // == 0  Why not return ID ?
}

int ScriptManager::giveResource(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string resource;
	int fNdx, amount;
	if ( extractArgs(args, "giveResource", "str,int,int", &resource, &fNdx, &amount) ) {
		int err = theWorld.giveResource(resource, fNdx, amount);
		if ( err == -1 ) {
			addErrorMessage("giveResource(): invalid faction index " + intToStr(fNdx));
		} else if ( err == -2 ) {
			addErrorMessage("giveResource(): invalid resource '" + resource + "'");			
		}
	}
	return args.getReturnCount();
}

int ScriptManager::givePositionCommand(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int id;
	string cmd;
	Vec2i pos;
	if ( extractArgs(args, "givePositionCommand", "int,str,v2i", &id, &cmd, &pos) ) {
		int res = theWorld.givePositionCommand(id, cmd, pos);
		args.returnBool( (res == 0 ? true : false) );
		if ( res < 0 ) {
			stringstream ss;
			switch ( res ) {
				case -1: ss << "givePositionCommand(): invalid unit id " << id; break;
				case -2: ss << "givePositionCommand(): unit " << id << " has no '" << cmd << "' command"; break;
				case -3: ss << "givePositionCommand(): invalid command '" << cmd; break;
				default: throw runtime_error("In ScriptManager::givePositionCommand, World::givePositionCommand() returned unknown error code.");
			}
			addErrorMessage(ss.str());
		}	
	}
	return args.getReturnCount();
}

int ScriptManager::giveTargetCommand ( LuaHandle * luaHandle ) {
	LuaArguments args(luaHandle);
	int id, id2;
	string cmd;
	if ( extractArgs(args, "giveTargetCommand", "int,str,int", &id, &cmd, &id2) ) {
		int res = theWorld.giveTargetCommand(id, cmd, id2);
		args.returnBool( (res == 0 ? true : false) );
		if ( res < 0 ) {
			stringstream ss;
			switch ( res ) {
				case -1: ss << "giveTargetCommand(): invalid unit id " << id; break;
				case -2: ss << "giveTargetCommand(): unit " << id << " can not attack unit " << id2 << " no appropriate attack command found"; break;
				case -3: ss << "giveTargetCommand(): invalid command '" << cmd; break;
				default: throw runtime_error("In ScriptManager::giveTargetCommand, World::giveTargetCommand() returned unknown error code.");
			}
			addErrorMessage(ss.str());
		}	
	}
	return args.getReturnCount();
}

int ScriptManager::giveStopCommand ( LuaHandle * luaHandle ) {
	LuaArguments args(luaHandle);
	int id;
	string cmd;
	if ( extractArgs(args, "giveStopCommand", "int,str", &id, &cmd) ) {
		int res = theWorld.giveStopCommand(id, cmd);
		args.returnBool( (res == 0 ? true : false) );
		if ( res < 0 ) {
			stringstream ss;
			switch ( res ) {
				case -1: ss << "giveStopCommand(): invalid unit id " << id; break;
				case -2: ss << "giveStopCommand(): unit " << id << " has no '" << cmd << "' command"; break;
				case -3: ss << "giveStopCommand(): invalid command '" << cmd; break;
				default: throw runtime_error("In ScriptManager::giveStopCommand, World::giveStopCommand() returned unknown error code.");
			}
			addErrorMessage(ss.str());
		}
	}
	return args.getReturnCount();	
}

int ScriptManager::giveProductionCommand(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string prod;
	if ( extractArgs(args, "giveProductionCommand", "int,str", &id, &prod) ) {
		int res = theWorld.giveProductionCommand(id, prod);
		args.returnBool( (res == 0 ? true : false) );
		if ( res < 0 ) {
			stringstream ss;
			switch ( res ) {
				case -1: ss << "giveProductionCommand(): invalid unit id " << id; break;
				case -2: ss << "giveProductionCommand(): unit " << id << " can not produce unit '" << prod << "'"; break;
				case -3: ss << "giveProductionCommand(): unit '" << prod << " not found."; break;
				default: throw runtime_error("In ScriptManager::giveProductionCommand, World::giveProductionCommand() returned unknown error code.");
			}
			addErrorMessage(ss.str());
		}
	}
	return args.getReturnCount();
}

int ScriptManager::giveUpgradeCommand(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int id;
	string prod;
	if ( extractArgs(args, "giveUpgradeCommand", "int,str", &id, &prod) ) {
		int res = theWorld.giveUpgradeCommand(id, prod);
		args.returnBool( (res == 0 ? true : false) );
		if ( res < 0 ) {
			stringstream ss;
			switch ( res ) {
				case -1: ss << "giveUpgradeCommand(): invalid unit id " << id; break;
				case -2: ss << "giveUpgradeCommand(): unit " << id << " can not produce upgrade '" << prod << "'"; break;
				case -3: ss << "giveUpgradeCommand(): upgrade '" << prod << " not found."; break;
				default: throw runtime_error("In ScriptManager::giveUpgradeCommand, World::giveUpgradeCommand() returned unknown error code.");
			}
			addErrorMessage(ss.str());
		}
	}
	return args.getReturnCount();
}

int ScriptManager::disableAi(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "disableAi", "int", &fNdx) ) {
		if ( fNdx >= 0 && fNdx < theGameSettings.getFactionCount() ) {
			playerModifiers[fNdx].disableAi();
		} else {
			addErrorMessage("disableAi(): invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::setPlayerAsWinner(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "setPlayerAsWinner", "int", &fNdx) ) {
		if ( fNdx >= 0 && fNdx < theGameSettings.getFactionCount() ) {
			playerModifiers[fNdx].setAsWinner();
		} else {
			addErrorMessage("setPlayerAsWinner(): invalid faction index " + intToStr(fNdx));
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
	if ( extractArgs(args, "playerName", "int", &fNdx) ) {
		if ( fNdx >= 0 && fNdx < theGameSettings.getFactionCount() ) {
			string playerName= theGameSettings.getPlayerName(fNdx);
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
	if ( extractArgs(args, "factionTypeName", "int", &fNdx) ) {
		if ( fNdx >= 0 && fNdx < theGameSettings.getFactionCount() ) {
			string factionTypeName = theGameSettings.getFactionTypeName(fNdx);
			args.returnString(factionTypeName);
		} else {
			addErrorMessage("factionTypeName(): invalid faction index " + intToStr(fNdx));
		}
	}
	return args.getReturnCount();
}

int ScriptManager::scenarioDir(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	args.returnString(theGameSettings.getScenarioPath());
	return args.getReturnCount();
}

int ScriptManager::startLocation(LuaHandle* luaHandle) {
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "startLocation", "int", &fNdx) ) {
		Vec2i pos= theWorld.getStartLocation(fNdx);
		if ( pos == Vec2i(-1) ) {
			addErrorMessage("startLocation(): invalid faction index " + intToStr(fNdx));
		}
		args.returnVec2i(pos);
	}
	return args.getReturnCount();
}

int ScriptManager::unitPosition(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int id;
	if ( extractArgs(args, "unitPosition", "int", &id) ) {
		Vec2i pos= theWorld.getUnitPosition(id);
		if ( pos == Vec2i(-1) ) {
			addErrorMessage("unitPosition(): Can not find unit=" + intToStr(id) + " to get position");
		}
		args.returnVec2i(pos);
	}
	return args.getReturnCount();
}

int ScriptManager::unitFaction(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int id;
	if ( extractArgs(args, "unitFaction", "int", &id) ) {
		int factionIndex = theWorld.getUnitFactionIndex(id);
		if ( factionIndex == -1 ) {
			addErrorMessage("unitFaction(): Can not find unit=" + intToStr(id) + " to get faction index");
		}
		args.returnInt(factionIndex);
	}
	return args.getReturnCount();
}

int ScriptManager::resourceAmount(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	string resource;
	int fNdx;
	if ( extractArgs(args, "resourceAmount", "str,int", &resource, &fNdx) ) {
		int amount = theWorld.getResourceAmount(resource, fNdx);
		if ( amount == -1 ) {
			addErrorMessage("resourceAmount(): invalid faction index " + intToStr(fNdx));
		} else if ( amount == -2 ) {
			addErrorMessage("resourceAmount(): invalid resource '" + resource + "'");
		}
		args.returnInt(amount);
	}
	return args.getReturnCount();
}

int ScriptManager::lastCreatedUnitName(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	if ( latestCreated.id == -1 ) {
		addErrorMessage("lastCreatedUnitName(): called before any units created.");
	}
	args.returnString(latestCreated.name);
	return args.getReturnCount();
}

int ScriptManager::lastCreatedUnit(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	if ( latestCreated.id == -1 ) {
		addErrorMessage("lastCreatedUnit(): called before any units created.");
	}
	args.returnInt(latestCreated.id);
	return args.getReturnCount();
}

int ScriptManager::lastDeadUnitName(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	if ( latestCasualty.id == -1 ) {
		addErrorMessage("lastDeadUnitName(): called before any units died.");
	}
	args.returnString(latestCasualty.name);
	return args.getReturnCount();
}

int ScriptManager::lastDeadUnit(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	if ( latestCasualty.id == -1 ) {
		addErrorMessage("lastDeadUnit(): called before any units died.");
	}
	args.returnInt(latestCasualty.id);
	return args.getReturnCount();
}

int ScriptManager::unitCount(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int fNdx;
	if ( extractArgs(args, "unitCount", "int", &fNdx) ) {
		int amount = theWorld.getUnitCount(fNdx);
		if ( amount == -1 ) {
			addErrorMessage("unitCount() invalid faction index " + intToStr(fNdx));
		}
		args.returnInt(amount);
	}
	return args.getReturnCount();
}

int ScriptManager::unitCountOfType(LuaHandle* luaHandle){
	LuaArguments args(luaHandle);
	int fNdx;
	string type;
	if ( extractArgs(args, "unitCountOfType", "int,str", &fNdx, &type) ) {
		int amount = theWorld.getUnitCountOfType(fNdx, type);
		if ( amount == -1 ) {
			addErrorMessage("unitCountOfType(): invalid faction index " + intToStr(fNdx));
		} else if ( amount == -2 ) {
			addErrorMessage("unitCountOfType(): invalid unit type '" + type + "' for faction "
				+ intToStr(fNdx));
		}
		args.returnInt(amount);
	}
	return args.getReturnCount();
}

#if DEBUG_RENDERING_ENABLED

int ScriptManager::hilightRegion(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	string region;
	if ( extractArgs(args, "hilightRegion", "str", &region) ) {
		const Region *r = triggerManager.getRegion(region);
		if ( r ) {
			const Rect *rect = static_cast<const Rect*>(r);
			for ( int y = rect->y; y < rect->y + rect->h; ++y ) {
				for ( int x = rect->x; x < rect->x + rect->w; ++x ) {
					RegionHilightCallback::cells.insert(Vec2i(x,y));
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
	if ( extractArgs(args, "hilightCell", "v2i", &cell) ) {
		RegionHilightCallback::cells.insert(cell);
	}
	return args.getReturnCount();
}

int ScriptManager::clearHilights(LuaHandle *luaHandle) {
	LuaArguments args(luaHandle);
	RegionHilightCallback::cells.clear();
	return args.getReturnCount();
}


#endif

}}//end namespace

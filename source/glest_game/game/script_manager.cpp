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

#include "script_manager.h"

#include "world.h"
#include "lang.h"
#include "game_camera.h"
#include "game.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Lua;

namespace Glest{ namespace Game{

// =====================================================
//	class ScriptTimer
// =====================================================

bool ScriptTimer::ready() {
	if ( real ) {
		return Chrono::getCurMillis () >= targetTime;
	}
	return theWorld.getFrameCount() >= targetTime;
}

void ScriptTimer::reset() {
	if ( real ) {
		targetTime = Chrono::getCurMillis () + interval * 1000;
	}
	else {
		targetTime = theWorld.getFrameCount() + interval;
	}
}

// =====================================================
//	class PlayerModifiers
// =====================================================

PlayerModifiers::PlayerModifiers(){
	winner= false;
	aiEnabled= true;
}

// =====================================================
//	class ScriptManager
// =====================================================

// ========== statics ==========

//ScriptManager* ScriptManager::thisScriptManager= NULL;
const int ScriptManager::messageWrapCount= 30;
const int ScriptManager::displayTextWrapCount= 64;

string				ScriptManager::code;
LuaScript			ScriptManager::luaScript;
GraphicMessageBox	ScriptManager::messageBox;
string				ScriptManager::displayText;
string				ScriptManager::lastCreatedUnitName;
int					ScriptManager::lastCreatedUnitId;
string				ScriptManager::lastDeadUnitName;
int					ScriptManager::lastDeadUnitId;
bool				ScriptManager::gameOver;
PlayerModifiers		ScriptManager::playerModifiers[GameConstants::maxPlayers];
vector<ScriptTimer> ScriptManager::timers;
vector<ScriptTimer> ScriptManager::newTimerQueue;
set<string>			ScriptManager::definedEvents;
ScriptManager::MessageQueue ScriptManager::messageQueue;


void ScriptManager::init () {
	const Scenario*	scenario = theWorld.getScenario();

	luaScript.startUp();
	assert ( ! luaScript.isDefined ( "startup" ) );

	//register functions
	luaScript.registerFunction(setTimer, "setTimer");
	luaScript.registerFunction(stopTimer, "stopTimer");
	luaScript.registerFunction(showMessage, "showMessage");
	luaScript.registerFunction(setDisplayText, "setDisplayText");
	luaScript.registerFunction(clearDisplayText, "clearDisplayText");
	luaScript.registerFunction(setCameraPosition, "setCameraPosition");
	luaScript.registerFunction(createUnit, "createUnit");
	luaScript.registerFunction(giveResource, "giveResource");
	luaScript.registerFunction(givePositionCommand, "givePositionCommand");
	luaScript.registerFunction(giveProductionCommand, "giveProductionCommand");
	luaScript.registerFunction(giveStopCommand, "giveStopCommand");
	luaScript.registerFunction(giveTargetCommand, "giveTargetCommand");
	luaScript.registerFunction(giveUpgradeCommand, "giveUpgradeCommand");
	luaScript.registerFunction(disableAi, "disableAi");
	luaScript.registerFunction(setPlayerAsWinner, "setPlayerAsWinner");
	luaScript.registerFunction(endGame, "endGame");
	luaScript.registerFunction(debugLog, "debugLog");
	luaScript.registerFunction(consoleMsg, "consoleMsg");

	luaScript.registerFunction(getPlayerName, "playerName");
	luaScript.registerFunction(getFactionTypeName, "factionTypeName");
	luaScript.registerFunction(getScenarioDir, "scenarioDir");
	luaScript.registerFunction(getStartLocation, "startLocation");
	luaScript.registerFunction(getUnitPosition, "unitPosition");
	luaScript.registerFunction(getUnitFaction, "unitFaction");
	luaScript.registerFunction(getResourceAmount, "resourceAmount");
	luaScript.registerFunction(getLastCreatedUnitName, "lastCreatedUnitName");
	luaScript.registerFunction(getLastCreatedUnitId, "lastCreatedUnit");
	luaScript.registerFunction(getLastDeadUnitName, "lastDeadUnitName");
	luaScript.registerFunction(getLastDeadUnitId, "lastDeadUnit");
	luaScript.registerFunction(getUnitCount, "unitCount");
	luaScript.registerFunction(getUnitCountOfType, "unitCountOfType");
	

	//load code
	for(int i= 0; i<scenario->getScriptCount(); ++i){
		const Script* script= scenario->getScript(i);
		luaScript.loadCode("function " + script->getName() + "()" + script->getCode() + " end\n", script->getName());
	}
	
	// get globs, startup, unitDied, unitDiedOfType_xxxx (archer, worker, etc...) etc. 
	//  need unit names of all loaded factions
	// put defined function names in definedEvents, check membership before doing luaCall()
	set<string> funcNames;
	funcNames.insert ( "startup" );
	funcNames.insert ( "unitDied" );
	funcNames.insert ( "unitCreated" );
	funcNames.insert ( "resourceHarvested" );
	for ( int i=0; i < theWorld.getFactionCount(); ++i ) {
		const FactionType *f = theWorld.getFaction( i )->getType();
		for ( int j=0; j < f->getUnitTypeCount(); ++j ) {
			const UnitType *ut = f->getUnitType( j );
			funcNames.insert( "unitCreatedOfType_" + ut->getName() );
		}
	}
	for ( set<string>::iterator it = funcNames.begin(); it != funcNames.end(); ++it ) {
		if ( luaScript.isDefined( *it ) ) {
			definedEvents.insert( *it );
		}
	}

	//setup message box
	messageBox.init( "", Lang::getInstance().get("Ok") );
	messageBox.setEnabled(false);

	//last created unit
	lastCreatedUnitId= -1;
	lastDeadUnitId= -1;
	gameOver= false;

	//call startup function
	luaScript.luaCall("startup");
}

// ========================== events ===============================================

void ScriptManager::onMessageBoxOk(){
	Lang &lang= Lang::getInstance();
	
	if(!messageQueue.empty()){
		messageQueue.pop();
		if(!messageQueue.empty()){
			messageBox.setText(wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount));
			messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
		}
	}

	//close the messageBox now all messages have been shown
	if (messageQueue.empty()){
		messageBox.setEnabled(false);
		theGame.resume ();
	}
}

void ScriptManager::onResourceHarvested(){
	if ( definedEvents.find( "resourceHarvested" ) != definedEvents.end() ) {
		luaScript.luaCall("resourceHarvested");
	}
}

void ScriptManager::onUnitCreated(const Unit* unit){
	lastCreatedUnitName= unit->getType()->getName();
	lastCreatedUnitId= unit->getId();
	if ( definedEvents.find( "unitCreated" ) != definedEvents.end() ) {
		luaScript.luaCall("unitCreated");
	}
	if ( definedEvents.find( "unitCreatedOfType_"+unit->getType()->getName() ) != definedEvents.end() ) {
		luaScript.luaCall("unitCreatedOfType_"+unit->getType()->getName());
	}
}

void ScriptManager::onUnitDied(const Unit* unit){
	lastDeadUnitName= unit->getType()->getName();
	lastDeadUnitId= unit->getId();
	if ( definedEvents.find( "unitDied" ) != definedEvents.end() ) {
		luaScript.luaCall("unitDied");
	}
}

void ScriptManager::onTimer() {
	// when a timer is ready, call the corresponding lua function
	// and remove the timer, or reset to repeat.

	vector<ScriptTimer>::iterator timer;

	for (timer = timers.begin(); timer != timers.end();) {
		if ( timer->ready() ) {
			if ( timer->isAlive() ) {
				if ( ! luaScript.luaCall("timer_" + timer->getName()) ) {
					timer->kill();
					theConsole.addLine( "Error: timer_" + timer->getName() + " is not defined." );
				}
			}
			if ( timer->isPeriodic() && timer->isAlive() ) {
				timer->reset();
			} 
			else {
				timer = timers.erase( timer ); //returns next element
				continue;
			}
		}
		++timer;
	}
	for ( vector<ScriptTimer>::iterator it = newTimerQueue.begin(); it != newTimerQueue.end(); ++it ) {
		timers.push_back ( *it );
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

// =============== Error handling bits ===============

void ScriptManager::luaCppCallError ( const string &func, const string &expected, const string &received, const string extra ) {
	string msg = func + " called with wrong paramaters, Expected: ("+ expected + ")\n\tGot (" + received + ")";
	if ( extra.size() ) {
		msg += "\n\t" + extra;
	}
	Logger::getErrorLog().add ( msg );
}

string ScriptManager::describeLuaStack ( LuaArguments &args ) {
	if ( args.getArgumentCount () == 0 ) {
		return "";
	}
	string desc = args.getType ( 1 );
	for ( int i=2; i <= args.getArgumentCount(); ++i ) {
		desc += ", "; 
		desc += args.getType ( i );
	}
	return desc;
}


// ========================== lua callbacks ===============================================

int ScriptManager::debugLog(Shared::Lua::LuaHandle *luaHandle)  {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "debugLog", "String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try { 
		string &msg = luaArguments.getString(-1);
		Logger::getErrorLog().add ( msg );
	}
	catch ( LuaError e ) {
		luaCppCallError ( "debugLog", "String", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();

}

int ScriptManager::consoleMsg (Shared::Lua::LuaHandle *luaHandle)  {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "consoleMsg", "String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try { 
		string &msg = luaArguments.getString(-1);
		theConsole.addLine ( msg );
	}
	catch ( LuaError e ) {
		luaCppCallError ( "consoleMsg", "String", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();

}

int ScriptManager::setTimer(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount () != 4 ) {
		luaCppCallError ( "setTimer", "String, String, Number, Boolean", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		const string &name = luaArguments.getString(-4), &type = luaArguments.getString(-3);
		int interval = luaArguments.getInt(-2);
		bool periodic = luaArguments.getBoolean(-1);
		if ( type == "real" ) {
			newTimerQueue.push_back (ScriptTimer ( name, true, interval, periodic ));
		}
		else if ( type == "game" ) {
			newTimerQueue.push_back (ScriptTimer ( name, false, interval, periodic ));
		}
	}
	catch ( LuaError e ) {
		luaCppCallError ( "setTimer", "String, String, Number, Boolean", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount(); // == 0
}

int ScriptManager::stopTimer(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "stopTimer", "String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try { 
		// find timer with the name and remove it
		const string &name =  luaArguments.getString(-1);
		vector<ScriptTimer>::iterator i;
		for (i = timers.begin(); i != timers.end(); ++i) {
			if (i->getName() == name) {
				i->kill ();
				break;
			}
		}
	}
	catch ( LuaError e ) {
		luaCppCallError ( "stopTimer", "String", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::showMessage(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	Lang &lang= Lang::getInstance();
	if ( luaArguments.getArgumentCount() != 2 ) {
		luaCppCallError ( "showMessage", "String, String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		theGame.pause ();
		ScriptManagerMessage msg ( luaArguments.getString(-2), luaArguments.getString(-1) );
		messageQueue.push ( msg );
		messageBox.setEnabled ( true );
		messageBox.setText ( wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount) );
		messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
		//thisScriptManager->showMessage();
	}
	catch ( LuaError e ) {
		luaCppCallError ( "showMessage", "String, String", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::setDisplayText(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "setDisplayText", "String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		displayText= wrapString(Lang::getInstance().getScenarioString(luaArguments.getString(-1)), displayTextWrapCount);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "setDisplayText", "String", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::clearDisplayText(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 0 ) {
		luaCppCallError ( "clearDisplayText", "", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	displayText= "";
	return luaArguments.getReturnCount();
}

int ScriptManager::setCameraPosition(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "setCameraPosition", "Table", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		Vec2i pos ( luaArguments.getVec2i(-1) );
		theCamera.centerXZ((float)pos.x, (float)pos.y);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "setCameraPosition", "Table", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::createUnit(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 3 ) {
		luaCppCallError ( "createUnit", "String, Number, Table", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		theWorld.createUnit ( luaArguments.getString(-3),luaArguments.getInt(-2),luaArguments.getVec2i(-1) );
	}
	catch ( LuaError e ) {
		luaCppCallError ( "createUnit", "String, Number, Table", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::giveResource(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 3 ) {
		luaCppCallError ( "giveResource", "String, Number, Number", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		theWorld.giveResource(luaArguments.getString(-3), luaArguments.getInt(-2), luaArguments.getInt(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "giveResource", "String, Number, Number", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::givePositionCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount () != 3 ) {
		luaCppCallError ( "givePositionCommand", "Number, String, Table", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		theWorld.givePositionCommand(luaArguments.getInt(-3),luaArguments.getString(-2),luaArguments.getVec2i(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "givePositionCommand", "Number, String, Table", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::giveTargetCommand ( LuaHandle * luaHandle ) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount () != 3 ) {
		luaCppCallError ( "giveTargetCommand", "Number, String, Number", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		theWorld.giveTargetCommand(luaArguments.getInt(-3),luaArguments.getString(-2),luaArguments.getInt(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "giveTargetCommand", "Number, String, Number", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();	
}

int ScriptManager::giveStopCommand ( LuaHandle * luaHandle ) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount () != 2 ) {
		luaCppCallError ( "giveStopCommand", "Number, String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		theWorld.giveStopCommand( luaArguments.getInt(-2),luaArguments.getString(-1) );
	}
	catch ( LuaError e ) {
		luaCppCallError ( "giveStopCommand", "Number, String", describeLuaStack ( luaArguments ), e.desc() );
	}	
	return luaArguments.getReturnCount();	
}

int ScriptManager::giveProductionCommand(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount () != 2 ) {
		luaCppCallError ( "giveProductionCommand", "Number, String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		theWorld.giveProductionCommand(luaArguments.getInt(-2),luaArguments.getString(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "giveProductionCommand", "Number, String", describeLuaStack ( luaArguments ), e.desc() );
	}	
	return luaArguments.getReturnCount();
}

int ScriptManager::giveUpgradeCommand(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount () != 2 ) {
		luaCppCallError ( "giveUpgradeCommand", "Number, String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		theWorld.giveUpgradeCommand(luaArguments.getInt(-2), luaArguments.getString(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "giveUpgradeCommand", "Number, String", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::disableAi(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "disableAi", "Number", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try { 
		int factionIndex = luaArguments.getInt(-1);
		if(factionIndex<GameConstants::maxPlayers){
			playerModifiers[factionIndex].disableAi();
		}
	}
	catch ( LuaError e ) {
		luaCppCallError ( "disableAi", "Number", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::setPlayerAsWinner(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "setPlayerAsWinner", "Number", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try { 
		int factionIndex = luaArguments.getInt(-1);
		if(factionIndex<GameConstants::maxPlayers){
			playerModifiers[factionIndex].setAsWinner();
		}
	}
	catch ( LuaError e ) {
		luaCppCallError ( "setPlayerAsWinner", "Number", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::endGame(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 0 ) {
		luaCppCallError ( "endGame", "", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	gameOver = true;
	return luaArguments.getReturnCount();
}

// Queries
int ScriptManager::getPlayerName(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "getPlayerName", "Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnString("ERROR");
		return luaArguments.getReturnCount();
	}
	try { 
		string playerName= theGameSettings.getPlayerName(luaArguments.getInt(-1));
		luaArguments.returnString(playerName);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getPlayerName", "Number", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnString("ERROR");
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::getFactionTypeName(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "getFactionTypeName", "Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnInt(-1);
		return luaArguments.getReturnCount();
	}
	try { 
		string factionTypeName= theGameSettings.getFactionTypeName(luaArguments.getInt(-1));
		luaArguments.returnString(factionTypeName);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getFactionTypeName", "Number", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnString("ERROR");
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::getScenarioDir(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(theGameSettings.getScenarioDir());
	return luaArguments.getReturnCount();
}

int ScriptManager::getStartLocation(LuaHandle* luaHandle) {
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "getStartLocation", "Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnVec2i(Vec2i(-1));
		return luaArguments.getReturnCount();
	}
	try { 
		Vec2i pos= theWorld.getStartLocation(luaArguments.getInt(-1));
		luaArguments.returnVec2i(pos);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getStartLocation", "Number", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnVec2i(Vec2i(-1));
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitPosition(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "getUnitPosition", "Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnVec2i(Vec2i(-1));
		return luaArguments.getReturnCount();
	}
	try { 
		Vec2i pos= theWorld.getUnitPosition(luaArguments.getInt(-1));
		luaArguments.returnVec2i(pos);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getUnitPosition", "Number", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnVec2i(Vec2i(-1));
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitFaction(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "getUnitFaction", "Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnInt(-1);
		return luaArguments.getReturnCount();
	}
	try { 
		int factionIndex= theWorld.getUnitFactionIndex(luaArguments.getInt(-1));
		luaArguments.returnInt(factionIndex);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getUnitFaction", "Number", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnInt(-1);
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::getResourceAmount(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 2 ) {
		luaCppCallError ( "getResourceAmount", "String, Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnInt(-1);
		return luaArguments.getReturnCount();
	}
	try {
		int amount = theWorld.getResourceAmount(luaArguments.getString(-2), luaArguments.getInt(-1));
		luaArguments.returnInt(amount);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getResourceAmount", "String, Number", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnInt(-1);
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastCreatedUnitName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString( lastCreatedUnitName );
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastCreatedUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(lastCreatedUnitId);
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(lastDeadUnitName);
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(lastDeadUnitId);
	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitCount(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "getUnitCount", "Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnInt(-1);
		return luaArguments.getReturnCount();
	}
	try {
		int amount = theWorld.getUnitCount(luaArguments.getInt(-1));
		luaArguments.returnInt(amount);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getUnitCount", "Number", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnInt(-1);
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::getUnitCountOfType(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 2 ) {
		luaCppCallError ( "getUnitCount", "Number, String", describeLuaStack ( luaArguments ) );
		luaArguments.returnInt(-1);
		return luaArguments.getReturnCount();
	}
	try {
		int amount = theWorld.getUnitCountOfType(luaArguments.getInt(-2), luaArguments.getString(-1));
		luaArguments.returnInt(amount);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getUnitCount", "Number, String", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnInt(-1);
	}
	return luaArguments.getReturnCount();
}

}}//end namespace

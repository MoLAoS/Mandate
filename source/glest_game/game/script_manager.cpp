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
//	class PlayerModifiers
// =====================================================

PlayerModifiers::PlayerModifiers(){
	winner= false;
	aiEnabled= true;
}

// =====================================================
//	class ScriptManager
// =====================================================

ScriptManager* ScriptManager::thisScriptManager= NULL;
const int ScriptManager::messageWrapCount= 30;
const int ScriptManager::displayTextWrapCount= 64;

void ScriptManager::init(Game *game, World* world, GameCamera *gameCamera, GameSettings *gameSettings){
	const Scenario*	scenario= world->getScenario();
	
	this->game= game;
	this->world= world;
	this->gameCamera= gameCamera;
	this->gameSettings= gameSettings;

	//set static instance
	thisScriptManager= this;

	//register functions
	//NEW
	luaScript.registerFunction(setTimer, "setTimer");
	//luaScript.registerFunction(setInterval, "setInterval");
	luaScript.registerFunction(stopTimer, "stopTimer");
	//NEW END
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

	//NEW
	luaScript.registerFunction(getPlayerName, "playerName");
	luaScript.registerFunction(getFactionTypeName, "factionTypeName");
	luaScript.registerFunction(getScenarioDir, "scenarioDir");

	luaScript.registerFunction(debugLog, "debugLog");
	//NEW END
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
		game->resume();
	}
}

void ScriptManager::onResourceHarvested(){
	luaScript.luaCall("resourceHarvested");
}

void ScriptManager::onUnitCreated(const Unit* unit){
	lastCreatedUnitName= unit->getType()->getName();
	lastCreatedUnitId= unit->getId();
	luaScript.luaCall("unitCreated");
	luaScript.luaCall("unitCreatedOfType_"+unit->getType()->getName());
}

void ScriptManager::onUnitDied(const Unit* unit){
	lastDeadUnitName= unit->getType()->getName();
	lastDeadUnitId= unit->getId();
	luaScript.luaCall("unitDied");
}


bool ScriptTimer::ready() {
	if ( real ) {
		return Chrono::getCurMillis () >= targetTime;
	}
	return Game::getInstance()->getWorld()->getFrameCount() >= targetTime;
}

void ScriptTimer::reset() {
	if ( real ) {
		targetTime = Chrono::getCurMillis () + interval * 1000;
	}
	else {
		targetTime = Game::getInstance()->getWorld()->getFrameCount() + interval;
	}
}


void ScriptManager::setTimer( const string &name, const string &type, int interval, bool periodic ) {
	if ( type == "real" ) {
		newTimerQueue.push_back (ScriptTimer ( name, true, interval, periodic ));
	}
	else if ( type == "game" ) {
		newTimerQueue.push_back (ScriptTimer ( name, false, interval, periodic ));
	}
}

void ScriptManager::stopTimer(const string &name) {
	// find timer with the name and remove it
	vector<ScriptTimer>::iterator i;
	for (i = timers.begin(); i != timers.end(); ++i) {
		if (i->getName() == name) {
			i->kill ();
			break;
		}
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
					Logger::getErrorLog ().add ( "Error: function timer_" + timer->getName() + " not defined." );
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


// ========================== lua wrappers ===============================================


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

void ScriptManager::showMessage(const string &text, const string &header){
	Lang &lang= Lang::getInstance();
	
	game->pause();

	messageQueue.push(ScriptManagerMessage(text, header));
	messageBox.setEnabled(true);
	messageBox.setText(wrapString(lang.getScenarioString(messageQueue.front().getText()), messageWrapCount));
	messageBox.setHeader(lang.getScenarioString(messageQueue.front().getHeader()));
}

void ScriptManager::clearDisplayText(){
	displayText= "";
}

void ScriptManager::setDisplayText(const string &text){
	displayText= wrapString(Lang::getInstance().getScenarioString(text), displayTextWrapCount);
}

void ScriptManager::setCameraPosition(const Vec2i &pos){
	gameCamera->centerXZ((float)pos.x, (float)pos.y);
}

void ScriptManager::createUnit(const string &unitName, int factionIndex, Vec2i pos){
	world->createUnit(unitName, factionIndex, pos);
}

void ScriptManager::giveResource(const string &resourceName, int factionIndex, int amount){
	world->giveResource(resourceName, factionIndex, amount);
}

void ScriptManager::givePositionCommand(int unitId, const string &commandName, const Vec2i &pos){
	world->givePositionCommand(unitId, commandName, pos);
}

void ScriptManager::giveProductionCommand(int unitId, const string &producedName){
	world->giveProductionCommand(unitId, producedName);
}

void ScriptManager::giveUpgradeCommand(int unitId, const string &producedName){
	world->giveUpgradeCommand(unitId, producedName);
}

void ScriptManager::giveTargetCommand ( int unitId, const string &cmdName, int targetId ) {
	world->giveTargetCommand ( unitId, cmdName, targetId );
}

void ScriptManager::giveStopCommand ( int unitId, const string &cmdName ) {
	world->giveStopCommand ( unitId, cmdName );
}

void ScriptManager::disableAi(int factionIndex){
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].disableAi();
	}
}

void ScriptManager::setPlayerAsWinner(int factionIndex){
	if(factionIndex<GameConstants::maxPlayers){
		playerModifiers[factionIndex].setAsWinner();
	}
}

void ScriptManager::endGame(){
	gameOver= true;
}

// Queries
const string &ScriptManager::getPlayerName(int factionIndex){
	return gameSettings->getPlayerName(factionIndex);
}

const string &ScriptManager::getFactionTypeName(int factionIndex){
	return gameSettings->getFactionTypeName(factionIndex);
}

const string &ScriptManager::getScenarioDir(){
	return gameSettings->getScenarioDir();
}

Vec2i ScriptManager::getStartLocation(int factionIndex){
	return world->getStartLocation(factionIndex);
}


Vec2i ScriptManager::getUnitPosition(int unitId){
	return world->getUnitPosition(unitId);
}

int ScriptManager::getUnitFaction(int unitId){
	return world->getUnitFactionIndex(unitId);
}

int ScriptManager::getResourceAmount(const string &resourceName, int factionIndex){
	return world->getResourceAmount(resourceName, factionIndex);
}

const string &ScriptManager::getLastCreatedUnitName(){
	return lastCreatedUnitName;
}

int ScriptManager::getLastCreatedUnitId(){
	return lastCreatedUnitId;
}

const string &ScriptManager::getLastDeadUnitName(){
	return lastDeadUnitName;
}

int ScriptManager::getLastDeadUnitId(){
	return lastDeadUnitId;
}

int ScriptManager::getUnitCount(int factionIndex){
	return world->getUnitCount(factionIndex);
}

int ScriptManager::getUnitCountOfType(int factionIndex, const string &typeName){
	return world->getUnitCountOfType(factionIndex, typeName);
}

// ========================== lua callbacks ===============================================

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

int ScriptManager::setTimer(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount () != 4 ) {
		luaCppCallError ( "setTimer", "String, String, Number, Boolean", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		thisScriptManager->setTimer( luaArguments.getString(-4), luaArguments.getString(-3),
			luaArguments.getInt(-2), luaArguments.getBoolean(-1));
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
		thisScriptManager->stopTimer(luaArguments.getString(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "stopTimer", "String", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::showMessage(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 2 ) {
		luaCppCallError ( "showMessage", "String, String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		thisScriptManager->showMessage(luaArguments.getString(-2), luaArguments.getString(-1));
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
		thisScriptManager->setDisplayText(luaArguments.getString(-1));
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
	thisScriptManager->clearDisplayText();
	return luaArguments.getReturnCount();
}

int ScriptManager::setCameraPosition(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "setCameraPosition", "Table", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		thisScriptManager->setCameraPosition(Vec2i(luaArguments.getVec2i(-1)));
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
		thisScriptManager->createUnit(luaArguments.getString(-3),luaArguments.getInt(-2),luaArguments.getVec2i(-1));
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
		thisScriptManager->giveResource(luaArguments.getString(-3), luaArguments.getInt(-2), luaArguments.getInt(-1));
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
		thisScriptManager->givePositionCommand(luaArguments.getInt(-3),luaArguments.getString(-2),luaArguments.getVec2i(-1));
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
		thisScriptManager->giveTargetCommand(luaArguments.getInt(-3),luaArguments.getString(-2),luaArguments.getInt(-1));
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
		thisScriptManager->giveStopCommand( luaArguments.getInt(-2),luaArguments.getString(-1) );
	}
	catch ( LuaError e ) {
		luaCppCallError ( "giveStopCommand", "Number, String", describeLuaStack ( luaArguments ), e.desc() );
	}	
	return luaArguments.getReturnCount();	
}

int ScriptManager::giveProductionCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount () != 2 ) {
		luaCppCallError ( "giveProductionCommand", "Number, String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		thisScriptManager->giveProductionCommand(luaArguments.getInt(-2),luaArguments.getString(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "giveProductionCommand", "Number, String", describeLuaStack ( luaArguments ), e.desc() );
	}	
	return luaArguments.getReturnCount();
}

int ScriptManager::giveUpgradeCommand(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount () != 2 ) {
		luaCppCallError ( "giveUpgradeCommand", "Number, String", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try {
		thisScriptManager->giveUpgradeCommand(luaArguments.getInt(-2), luaArguments.getString(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "giveUpgradeCommand", "Number, String", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::disableAi(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "disableAi", "Number", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try { 
		thisScriptManager->disableAi(luaArguments.getInt(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "disableAi", "Number", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::setPlayerAsWinner(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "setPlayerAsWinner", "Number", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	try { 
		thisScriptManager->setPlayerAsWinner(luaArguments.getInt(-1));
	}
	catch ( LuaError e ) {
		luaCppCallError ( "setPlayerAsWinner", "Number", describeLuaStack ( luaArguments ), e.desc() );
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::endGame(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 0 ) {
		luaCppCallError ( "endGame", "", describeLuaStack ( luaArguments ) );
		return luaArguments.getReturnCount();
	}
	thisScriptManager->endGame();
	return luaArguments.getReturnCount();
}

// Queries
int ScriptManager::getPlayerName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "getPlayerName", "Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnString("ERROR");
		return luaArguments.getReturnCount();
	}
	try { 
		string playerName= thisScriptManager->getPlayerName(luaArguments.getInt(-1));
		luaArguments.returnString(playerName);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getPlayerName", "Number", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnString("ERROR");
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::getFactionTypeName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "getFactionTypeName", "Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnInt(-1);
		return luaArguments.getReturnCount();
	}
	try { 
		string factionTypeName= thisScriptManager->getFactionTypeName(luaArguments.getInt(-1));
		luaArguments.returnString(factionTypeName);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getFactionTypeName", "Number", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnString("ERROR");
	}
	return luaArguments.getReturnCount();
}

int ScriptManager::getScenarioDir(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getScenarioDir());
	return luaArguments.getReturnCount();
}

int ScriptManager::getStartLocation(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	if ( luaArguments.getArgumentCount() != 1 ) {
		luaCppCallError ( "getStartLocation", "Number", describeLuaStack ( luaArguments ) );
		luaArguments.returnVec2i(Vec2i(-1));
		return luaArguments.getReturnCount();
	}
	try { 
		Vec2i pos= thisScriptManager->getStartLocation(luaArguments.getInt(-1));
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
		Vec2i pos= thisScriptManager->getUnitPosition(luaArguments.getInt(-1));
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
		int factionIndex= thisScriptManager->getUnitFaction(luaArguments.getInt(-1));
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
		int amount = thisScriptManager->getResourceAmount(luaArguments.getString(-2), luaArguments.getInt(-1));
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
	luaArguments.returnString(thisScriptManager->getLastCreatedUnitName());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastCreatedUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getLastCreatedUnitId());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitName(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnString(thisScriptManager->getLastDeadUnitName());
	return luaArguments.getReturnCount();
}

int ScriptManager::getLastDeadUnitId(LuaHandle* luaHandle){
	LuaArguments luaArguments(luaHandle);
	luaArguments.returnInt(thisScriptManager->getLastDeadUnitId());
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
		int amount = thisScriptManager->getUnitCount(luaArguments.getInt(-1));
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
		int amount = thisScriptManager->getUnitCountOfType(luaArguments.getInt(-2), luaArguments.getString(-1));
		luaArguments.returnInt(amount);
	}
	catch ( LuaError e ) {
		luaCppCallError ( "getUnitCount", "Number, String", describeLuaStack ( luaArguments ), e.desc() );
		luaArguments.returnInt(-1);
	}
	return luaArguments.getReturnCount();
}

}}//end namespace

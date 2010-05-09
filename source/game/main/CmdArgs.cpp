// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2010 Frank Tetzel <tetzank@users.sourceforge.net>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "CmdArgs.h"

#include <iostream>

using std::cout;
using std::endl;


namespace Glest { namespace Main {

CmdArgs::CmdArgs(){
	this->server = false;
	this->clientIP = "";
	this->configDir = "";
	this->dataDir = "";
}

CmdArgs::~CmdArgs(){
	
}

/** parse command line arguments @return true if program should not proceed. */
bool CmdArgs::parse(int argc, char **argv){
	for(int i=1; i<argc; ++i){
		string arg(argv[i]);
		if(arg=="-server"){
			this->server = true;
		}else if(arg=="-client" && (i+1)<argc){
			this->clientIP = argv[++i];
		}else if(arg=="-configdir" && (i+1)<argc){
			this->configDir = argv[++i];
		}else if(arg=="-datadir" && (i+1)<argc){
			this->dataDir = argv[++i];
		}else if(arg=="-loadmap" && (i+2)<argc){
			this->map = argv[++i];
			this->tileset = argv[++i];
		}else if(arg=="-scenario" && (i+2)<argc){
			//TODO: reuse MenuStateScenario::loadGameSettings and MenuStateScenario::loadScenarioInfo
			this->category = argv[++i];
			this->scenario = argv[++i];
		}else if(arg=="--help" || arg=="-h"){
			cout << "usage: " << argv[0] << " [options]\n"
				<< "  -server                  startup and immediately host a game\n"
				<< "  -client IP               startup and immediately connect to server IP\n"
				<< "  -configdir path          set location of configs and logs to path\n"
				<< "  -datadir path            set location of data\n"
				<< "  -loadmap map tileset     load maps/map.gbm with tilesets/tileset for map preview\n"
				<< "  -scenario category name  load immediately scenario/category/name\n"; //TODO
			return true;
		}else{
			cout << "unknown argument: " << arg << endl;
			return true;
		}
	}
	return false;
}

}} //namespaces

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

#include "projectConfig.h"
#include "FSFactory.hpp"

using Shared::PhysFS::FSFactory;

using std::cout;
using std::endl;


namespace Glest { namespace Main {

CmdArgs::CmdArgs(){
	this->server = false;
	this->clientIP = "";
	this->configDir = DEFAULT_CONFIG_DIR;
	this->dataDir = DEFAULT_DATA_DIR;
	test = false;
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
			this->category = argv[++i];
			this->scenario = argv[++i];
		} else if (arg == "-test" && (i+1) < argc) {
			test = true;
			testType = argv[++i];
		} else if(arg=="--help" || arg=="-h") {
			cout << "usage: " << argv[0] << " [options]\n"
				<< "  -server                  startup and immediately host a game\n"
				<< "  -client IP               startup and immediately connect to server IP\n"
				<< "  -configdir path          set location of configs and logs to path\n"
				<< "  -datadir path            set location of data\n"
				<< "  -loadmap map tileset     load maps/map.gbm with tilesets/tileset for map preview\n"
				<< "  -scenario category name  load immediately scenario/category/name\n";
			return true;
		}else if(arg=="-list-tilesets"){  //FIXME: only works with physfs
				cout << "config: " << configDir << "\ndata: " << dataDir << endl;
				try{
					// init physfs
					if (configDir.empty()) configDir=".";  // fake configDir, won't get used anyway
					if (dataDir.empty()) dataDir = ".";
					FSFactory *fsfac = FSFactory::getInstance();
					fsfac->initPhysFS(argv[0], configDir.c_str(), dataDir.c_str());
					fsfac->usePhysFS = true;
					vector<string> results = FSFactory::findAll("tilesets/*", false);
					for(vector<string>::iterator it=results.begin(); it!=results.end(); ++it){
						cout << "~" << *it << endl;  // use ~ as prefix to easily filter other output
					}
					delete fsfac;
				}catch(...){
					cout << "Error: couldn't find tilesets.\n";
					exit(2);
				}
				return true;
		} else {
			cout << "unknown argument: " << arg << endl;
			return true;
		}
	}
	return false;
}

}} //namespaces

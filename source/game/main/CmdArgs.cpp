
#include "pch.h"
#include "CmdArgs.h"

#include <iostream>

using std::cout;
using std::endl;


namespace Glest{ namespace Game{

CmdArgs::CmdArgs(){
	this->server = false;
	this->clientIP = "";
	this->configDir = "";
	this->dataDir = "";
}

CmdArgs::~CmdArgs(){
	
}

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
		}else if(arg=="--help" || arg=="-h"){
			cout << "usage: " << argv[0] << " [options]\n"
				<< "  -server               startup and immediately host a game\n"
				<< "  -client IP            startup and immediately connect to server IP\n"
				<< "  -configdir path       set location of configs and logs to path\n"
				<< "  -datadir path         set location of data\n"
				<< "  -loadmap map tileset  load maps/map.gbm with tilesets/tileset for map preview\n";
			return true;
		}else{
			cout << "unknown argument: " << argv[i] << endl;
		}
	}
	return false;
}

}} //namespaces

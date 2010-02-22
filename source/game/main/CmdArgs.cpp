
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
		}else if(arg=="--help" || arg=="-h"){
			cout << "usage: " << argv[0] << " [-server] [-client IP] [-configdir path] [-datadir path]\n"
				<< "  -server           startup and immediately host a game\n"
				<< "  -client IP        startup and immediately connect to server IP\n"
				<< "  -configdir path   set location of configs and logs to path\n"
				<< "  -datadir path     set location \n";
			return true;
		}else{
			cout << "unknown argument: " << argv[i] << endl;
		}
	}
	return false;
}

}} //namespaces

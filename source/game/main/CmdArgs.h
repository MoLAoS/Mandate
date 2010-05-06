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

#ifndef _CMDARGS_H_
#define _CMDARGS_H_

#include <string>

using std::string;

namespace Glest { namespace Main {

/**
 * "parser" for commandline arguments
 */
class CmdArgs{

private:
	/// true if -server
	bool server;
	/// not empty if -client, contains following argument as IP
	string clientIP;
	/// not empty if -configdir, contains following argument as path
	string configDir;
	/// not empty if -datadir, contains following argument as path
	string dataDir;
	///
	string map;
	string tileset;

public:
	CmdArgs();
	~CmdArgs();

	/**
	 * returns true for quick exit, like help
	 */
	bool parse(int argc, char **argv);
	
	// getters
	bool isServer()         { return server; }
	string getClientIP()    { return clientIP; }
	string getConfigDir()   { return configDir; }
	string getDataDir()     { return dataDir; }
	string getLoadmap()     { return map; }
	string getLoadTileset() { return tileset; }

};

}} //namespaces

#endif

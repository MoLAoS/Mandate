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
	///
	string category;
	string scenario;

	bool test;
	string testType;

	bool m_redirStreams; // redirect stdout and stderr

public:
	CmdArgs();
	~CmdArgs();

	/**
	 * returns true for quick exit, like help
	 */
	bool parse(int argc, char **argv);
	
	// getters
	bool isServer() const	 { return server; }
	string &getClientIP()    { return clientIP; }
	string &getConfigDir()   { return configDir; }
	string &getDataDir()     { return dataDir; }
	string &getLoadmap()     { return map; }
	string &getLoadTileset() { return tileset; }
	string &getCategory()    { return category; }
	string &getScenario()    { return scenario; }

	bool isTest(const string &type) const { return (test && testType == type); }
	bool redirStreams() const { return m_redirStreams; }
};

}} //namespaces

#endif

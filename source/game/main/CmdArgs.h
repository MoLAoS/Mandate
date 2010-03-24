#ifndef _CMDARGS_H_
#define _CMDARGS_H_

#include <string>

using std::string;


namespace Glest{ namespace Game{

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

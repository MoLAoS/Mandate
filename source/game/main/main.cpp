// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "main.h"

#include <string>
#include <cstdlib>
#ifdef WIN32
#	include <direct.h>  // for mkdir, chdir
#endif

#include "game.h"
#include "main_menu.h"
#include "program.h"
#include "config.h"
#include "metrics.h"
#include "game_util.h"
#include "platform_util.h"
#include "platform_main.h"
#include "FSFactory.hpp"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;


string configDir = DEFAULT_CONFIG_DIR;
const string dataDir = DEFAULT_DATA_DIR;


namespace Glest{ namespace Game{

// =====================================================
// 	class ExceptionHandler
// =====================================================

class ExceptionHandler: public PlatformExceptionHandler {
public:
	virtual void log(const char *description, void *address, const char **backtrace, size_t count, const exception *e) {
		ostream *ofs = FSFactory::getInstance()->getOStream("gae-crash.txt");

		time_t t= time(NULL);
		char *timeString = asctime(localtime(&t));

		*ofs << "Crash\n"
			<< "Version: Advanced Engine " << gaeVersionString << endl
			<< "Time: " << timeString;
		if(description) {
			*ofs << "Description: " << description << endl;
		}
		if(e) {
			*ofs << "Exception: " << e->what() << endl;
		}
		*ofs << "Address: " << address << endl;
		if(backtrace) {
			*ofs << "Backtrace:\n";
			for(size_t i = 0 ; i < count; ++i) {
				*ofs << backtrace[i] << endl;
			}
		}
		*ofs << "\n=======================\n";

		delete ofs;
	}

	virtual void notifyUser(bool pretty) {
		if(pretty) {
			Program *program = Program::getInstance();
			if(program) {
				program->crash(NULL);
				return;
			}
		}

		Shared::Platform::message(
				"An error ocurred and Glest will close.\n"
				"Crash info has been saved in the crash.txt file\n"
				"Please report this bug to " + gaeMailString);
	}
};

// =====================================================
// Main
// =====================================================

int glestMain(int argc, char** argv) {
	if(configDir.empty()){
#ifdef WIN32
		configDir = getenv("UserProfile");
#else
		configDir = getenv("HOME");
#endif
		configDir += "/.glestadv/";
	}
	mkdir(configDir, true);
	mkdir(configDir+"addons/", true);
	//::mkdir(configDir.c_str(), 0750);
	//::mkdir((configDir+"addons/").c_str(), 0750);
	//::chdir(dataDir.c_str());
	//cout << "configDir:" << configDir << endl;
	
	FSFactory *fsfac = FSFactory::getInstance();
	fsfac->initPhysFS(argv[0], configDir.c_str());
	fsfac->usePhysFS(true);

	//char* buffer;
	//// Get the current working directory: 
	//if( (buffer = _getcwd( NULL, 0 )) == NULL ){  // getcwd is POSIX, deprecated in VC, use _getcwd
	//	perror( "_getcwd error" );
	//}else{
	//	printf( "cwd:%s \n", buffer );
	//	free(buffer);
	//}

	Config &config = Config::getInstance();

	if(config.getMiscCatchExceptions()) {
		ExceptionHandler exceptionHandler;

		try {
			exceptionHandler.install();
			Program program(config, argc, argv);
			showCursor(false/*config.getDisplayWindowed()*/);

			try {
				//main loop
				program.loop();
			} catch(const exception &e) {
				// friendlier error handling
				program.crash(&e);
				restoreVideoMode();
			}
		} catch(const exception &e) {
			restoreVideoMode();
			exceptionMessage(e);
		}
	} else {
		Program program(config, argc, argv);
		showCursor(false/*config.getDisplayWindowed()*/);
		program.loop();
	}

	fsfac->deinitPhysFS();
	return 0;
}

}}//end namespace

MAIN_FUNCTION(Glest::Game::glestMain)

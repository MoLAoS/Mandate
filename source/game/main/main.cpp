// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
		bool closeme = true;
		FILE *f = fopen("gae-crash.txt", "at");
		if(!f) {
			f = stderr;
			closeme = false;
		}
		time_t t= time(NULL);
		char *timeString = asctime(localtime(&t));

		fprintf(f, "Crash\n");
		fprintf(f, "Version: Advanced Engine %s\n", gaeVersionString.c_str());
		fprintf(f, "Time: %s", timeString);
		if(description) {
			fprintf(f, "Description: %s\n", description);
		}
		if(e) {
			fprintf(f, "Exception: %s\n", e->what());
		}
		fprintf(f, "Address: %p\n", address);
		if(backtrace) {
			fprintf(f, "Backtrace:\n");
			for(size_t i = 0 ; i < count; ++i) {
				fprintf(f, "%s\n", backtrace[i]);
			}
		}
		fprintf(f, "\n=======================\n");

		if(closeme) {
			fclose(f);
		}
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
	mkdir(configDir.c_str(), 0750);
	chdir(dataDir.c_str());
	//cout << "configDir:" << configDir << endl;

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

	return 0;
}

}}//end namespace

MAIN_FUNCTION(Glest::Game::glestMain)

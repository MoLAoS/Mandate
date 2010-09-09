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

#include "game.h"
#include "main_menu.h"
#include "program.h"
#include "config.h"
#include "metrics.h"
#include "game_util.h"
#include "platform_util.h"
#include "platform_main.h"
#include "renderer.h"
#include "FSFactory.hpp"
#include "CmdArgs.h"
#include "core_data.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Main {

// =====================================================
// 	class ExceptionHandler
// =====================================================

class ExceptionHandler: public PlatformExceptionHandler {
public:
	virtual void log(const char *description, void *address, const char **backtrace, size_t count, const exception *e) {
		try {
			Renderer::getInstance().saveScreen("glestadv-crash.tga");
		} catch(runtime_error &e) {
			printf("%s", e.what());
		}

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
	string configDir = DEFAULT_CONFIG_DIR;
	string dataDir = DEFAULT_DATA_DIR;
	CmdArgs args;
	if(args.parse(argc, argv)){
		// quick exit
		return 0;
	}
	if(!args.getConfigDir().empty()){
		configDir = args.getConfigDir();
	}
	if(!args.getDataDir().empty()){
		dataDir = args.getDataDir();
	}

	if (configDir.empty()) {
#		ifdef WIN32
			configDir = getenv("UserProfile");
			configDir += "/glestadv/";
#		else
			configDir = getenv("HOME");
			configDir += "/.glestadv/";
#		endif
	}
	//FIXME: debug
	//cout << "config: " << configDir << "\ndata: " << dataDir << endl;

	mkdir(configDir, true);
	mkdir(configDir + "/addons/", true);
	mkdir(configDir + "/screens/", true);

#	if USE_PHYSFS
		g_fileFactory.initPhysFS(argv[0], configDir.c_str(), dataDir.c_str());
		g_fileFactory.usePhysFS = true;
#	endif

	if (g_config.getMiscCatchExceptions()) {
		ExceptionHandler exceptionHandler;

		try {
			exceptionHandler.install();
			Program program(args, configDir);
			showCursor(false);

			try {
				//main loop
				program.loop();
			} catch(const exception &e) {
				// friendlier error handling
				program.crash(&e);
				if (!g_config.getDisplayWindowed()) {
					restoreVideoMode();
				}
			}
		} catch(const exception &e) {
			if (!g_config.getDisplayWindowed()) {
				restoreVideoMode();
			}
			exceptionMessage(e);
		}
	} else {
		Program program(args, configDir);
		showCursor(false);
		program.loop();
	}

	Profile::profileEnd();  // to write profiler data out
	g_coreData.closeSounds(); // close audio stuff with ogg files

	return 0;
}

}}//end namespace

MAIN_FUNCTION(Glest::Main::glestMain)

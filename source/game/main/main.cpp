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

#include "leak_dumper.h"
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
#include "version.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest { namespace Main {

#ifdef WIN32
#	define IF_WINDOZE(x) x
#	define IF_GNU_OS(x)
#else
#	define IF_WINDOZE(x)
#	define IF_GNU_OS(x) x
#endif

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
			<< "Version: Advanced Engine " << gaeVersionString << " " << string(build_git_sha) << endl
			<< getAboutString(1) << endl // Build time
			<< "Time: " << timeString;
		if(description) {
			*ofs << "Description: " << description << endl;
		}
		if(e) {
			*ofs << "Exception: " << e->what() << endl;
		}
		IF_GNU_OS(
			*ofs << "Address: " << address << endl;
			if(backtrace) {
				*ofs << "Backtrace:\n";
				for(size_t i = 0 ; i < count; ++i) {
					*ofs << backtrace[i] << endl;
				}
			}
		)
		*ofs << "\n=======================\n";

		delete ofs;

		try {
			Renderer::getInstance().saveScreen("glestadv-crash_" + Logger::fileTimestamp() + ".png");
		} catch(runtime_error &e) {
			printf("%s", e.what());
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
	IF_WINDOZE(
		// Enable run-time checks (is macro, auto-magically skipped on release builds)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

		// check for SSE2
		if (!IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE)) {
			std::exception e("Error: No SSE2 support detected. GAE requires Streaming SIMD Extensions 2");
			exceptionMessage(e);
			return 0;
		}
	)

	CmdArgs args;
	if (args.parse(argc, argv)) {
		// quick exit
		return 0;
	}

	string configDir = args.getConfigDir();
	string dataDir = args.getDataDir();

	if (configDir.empty()) {
		IF_WINDOZE(
			configDir = getenv("USERPROFILE");
			configDir += "/glestadv/";
		)
		IF_GNU_OS(
			configDir = getenv("HOME");
			configDir += "/.glestadv/";
		)
	}
	if (dataDir.empty()) {
		dataDir = ".";
	}
	IF_WINDOZE (
		if (args.redirStreams()) {
			string path = configDir + "/stdout.txt";
			freopen(path.c_str(), "w", stdout);
		}
	)

	mkdir(configDir, true);
	mkdir(configDir + "/addons/", true);
	mkdir(configDir + "/screens/", true);
	mkdir(configDir + "/savegames/", true);

	try {
		g_fileFactory.initPhysFS(argv[0], configDir, dataDir);
		g_fileFactory.usePhysFS = true;
	} catch (runtime_error &e) {
		exceptionMessage(e);
		return 0;
	}

	cout << "config-dir: " << configDir << "\ndata-dir: " << dataDir << endl << endl;

	if (g_config.getMiscCatchExceptions()) {
		ExceptionHandler exceptionHandler;

#	if _GAE_DEBUG_EDITION_
		exceptionHandler.install();
		Program program(args);
		if(!program.init()){
			program.exit();
			return 0;
		}
		//main loop
		program.loop();
#	else
		try {
			exceptionHandler.install();
			Program program(args);
			if(!program.init()){
				program.exit();
				return 0;
			}

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
			g_logger.logError(e.what());
			exceptionMessage(e);
		}
#	endif
	} else {
		Program program(args);
		if(!program.init()){
			program.exit();
			return 0;
		}
		program.loop();
	}

	Profile::profileEnd();  // to write profiler data out
	g_coreData.closeSounds(); // close audio stuff with ogg files
	// FSFactory is deleted atexit(), see FSFactory::getInstance()
	return 0;
}

}}//end namespace

MAIN_FUNCTION(Glest::Main::glestMain)

// ==============================================================
// This file is part of Glest (www.glest.org)
//
// Copyright (C) 2001-2008 Martiño Figueroa
//               2009-2011 James McCulloch
//
// You can redistribute this code and/or modify it under
// the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_LOGGER_H_
#define _SHARED_UTIL_LOGGER_H_

#include <string>
#include <deque>
#include <time.h>
#include <iomanip>
#include <sstream>

#include "FSFactory.hpp"
#include "timer.h"
#include "texture.h"

using std::deque;
using std::string;
using std::stringstream;

#define LOG_WIDGET_EVENTS 1
#if LOG_WIDGET_EVENTS
#	define WIDGET_LOG(x) {stringstream ss; ss << x; g_logger.logWidgetEvent(ss.str()); }
#else
#	define WIDGET_LOG(x)
#endif

#define AI_LOGGING 1
#if AI_LOGGING
#	define LOG_AI(faction, cmpnt, lvl, message) {             \
		stringstream _ss;                                     \
		_ss <<  message;                                      \
		g_logger.logAiEvent(faction, cmpnt, lvl, _ss.str()); }
#else
#	define LOG_AI(factionIndex, component, level, message)
#endif

namespace Glest {

namespace Graphics {
	class GraphicProgressBar;
}
using Graphics::GraphicProgressBar;

namespace Util {

using namespace Shared::PhysFS;

WRAPPED_ENUM( TimeStampType,
	NONE,
	SECONDS,
	MILLIS
);

// =====================================================
// class LogFile
//
/// Interface to a single log file
// =====================================================

class LogFile { // log file wrapper
protected:
	string         m_fileName;
	FileOps       *m_fileOps;
	TimeStampType  m_timeStampType;

public:
	LogFile(const string &filename, const string &type, TimeStampType timeType);
	virtual ~LogFile();

	virtual void add(const string &str);

	///@todo class ErrorLogFile
	void logXmlError(const string &path, const char *error);
	void logMediaError(const string &xmlPath, const string &mediaPath, const char *error);
	///@todo class NetLogFile
	void addNetworkMsg(const string &msg);
};

const int logLineCount = 15;

// =====================================================
// class ProgramLog
//
/// Interface to the main log file, includes support for rendering
/// to screen (15 line history) and a progress bar.
// =====================================================

class ProgramLog : public LogFile {
private:
	typedef deque<string> Strings;

private:
	string state;
	Strings logLines;
	string subtitle;
	string current;
	bool loadingGame;
	static char errorBuf[];
	int totalUnits, unitsLoaded;
	bool m_progressBar;
	int m_progress;
	Shared::Graphics::Texture2D *m_backgroundTexture;

public:
	ProgramLog();

	void add(const string &str,  bool renderScreen);

	void setState(const string &state);
	void resetState(const string &s)	{state= s;}
	void setSubtitle(const string &v)	{subtitle = v;}
	void setLoading(bool v);
	void setProgressBar(bool v)			{m_progressBar = v; m_progress = 0;}

	void useLoadingScreenDefaults();
	bool setupLoadingScreen(const string &xmlpath);
	void renderLoadingScreen();

	void setUnitCount(int count) { totalUnits = count; unitsLoaded = 0; }
	void addUnitCount(int val) { totalUnits += val; }
	void unitLoaded();
};

STRINGY_ENUM( AiComponent,
	ECONOMY,
	PRODUCTION,
	GENERAL,
	MILITARY,
	RESEARCH
);

struct AiLogFlags {
	bool  m_enabled;
	int   m_level;
	bool  m_components[AiComponent::COUNT];
};

class AiLogFile : public LogFile {
private:
	AiLogFlags  m_flags[GameConstants::maxPlayers];

public:
	AiLogFile();

	void add(int faction, AiComponent component, int level, const string &msg);

	// get
	bool isEnabled(int faction) const {
		ASSERT_RANGE(faction, GameConstants::maxPlayers);
		return m_flags[faction].m_enabled; 
	}

	int getLevel(int faction) const {
		ASSERT_RANGE(faction, GameConstants::maxPlayers);
		return m_flags[faction].m_level;
	}

	bool isEnabled(int faction, AiComponent component) const {
		ASSERT_RANGE(faction, GameConstants::maxPlayers);
		ASSERT_RANGE(component, AiComponent::COUNT);
		return m_flags[faction].m_enabled && m_flags[faction].m_components[component]; 
	}

	// set
	void setEnabled(int faction, bool val) {
		ASSERT_RANGE(faction, GameConstants::maxPlayers);
		m_flags[faction].m_enabled = val; 
	}

	void setLevel(int faction, int level) { 
		ASSERT_RANGE(faction, GameConstants::maxPlayers);
		m_flags[faction].m_level = level;
	}

	void setEnabled(int faction, AiComponent component, bool val) {
		ASSERT_RANGE(faction, GameConstants::maxPlayers);
		ASSERT_RANGE(component, AiComponent::COUNT);
		m_flags[faction].m_components[component] = val;
	}
};

// =====================================================
// class Logger
//
/// Interface to write log files
// =====================================================

class Logger {
private:
	ProgramLog	*m_programLog;  // Always enabled
	LogFile     *m_errorLog;    // Always enabled
	AiLogFile   *m_aiLog;       // Always enabled
	LogFile     *m_networkLog;  // Always enabled

	LogFile     *m_widgetLog;   // Pre-processor controlled
	LogFile     *m_worldLog;    // Pre-processor controlled

private:
	Logger();
	~Logger();

	static Logger *instance;

	static void deleteInstance() {
		delete instance;
		instance = 0;
	}

public:
	static Logger& getInstance() {
		if (!instance) {
			instance = new Logger();
			atexit(&Logger::deleteInstance);
		}
		return *instance;
	}

	ProgramLog& getProgramLog() { return *m_programLog; }

	/** A timestamp with filename safe characters. (ie no \/:*?"<>| chars)
	* @returns a string in the format:
	* [Day of the month (01-31)][Abbreviated month name (eg Feb)][Hour in 24h format (00-23)]-[Minute (00-59)]
	*/
	static string fileTimestamp();

	void logProgramEvent(const string &str, bool renderScreen = false) {
		m_programLog->add(str, renderScreen);
	}

	// funnel to Error log
	void logXmlError(const string &path, const char *error) {
		m_errorLog->logXmlError(path, error);
	}
	void logMediaError(const string &xmlPath, const string &mediaPath, const char *error) {
		m_errorLog->logMediaError(xmlPath, mediaPath, error);
	}
	void logError(const string &dir, const string &msg) {
		m_errorLog->add(dir + ": " + msg);
	}	
	void logError(const string &msg) {
		m_errorLog->add(msg);
	}
	
	// funnel to Network log
	void logNetworkEvent(const string &msg) {
		m_networkLog->addNetworkMsg(msg);
	}

	// AI
	bool shouldLogAiEvent(int faction, AiComponent component, int level) const {
		return m_aiLog->isEnabled(faction, component) && level <= m_aiLog->getLevel(faction);
	}
	void logAiEvent(int faction, AiComponent component, int level, const string &msg) {
		m_aiLog->add(faction, component, level, msg);
	}

#	if LOG_WORLD_EVENTS
		void logWorldEvent(const string &msg) {
			m_worldLog->add(msg);
		}
#	endif

#	if LOG_WIDGET_EVENTS
		void logWidgetEvent(const string &msg) {
			m_widgetLog->add(msg);
		}
#	endif

};

#define LOG_STUFF 0

#if defined(LOG_STUFF) && LOG_STUFF
#	define LOG(x) g_logger.logProgramEvent(x)
#	define STREAM_LOG(x) {stringstream _ss; _ss << x; g_logger.logProgramEvent(_ss.str()); }
#	define GAME_LOG(x) STREAM_LOG( "Frame " << g_world.getFrameCount() << ": " << x )
#else
#	define LOG(x)
#	define STREAM_LOG(x)
#	define GAME_LOG(x)
#endif

#define LOG_NETWORKING 1
#if LOG_NETWORKING
#	define LOG_NETWORK(x) g_logger.logNetworkEvent(x)
#	define NETWORK_LOG(x) {stringstream _ss; _ss << x; g_logger.logNetworkEvent(_ss.str()); }
#else
#	define LOG_NETWORK(x)
#	define NETWORK_LOG(x)
#endif

#define _LOG(x) {										\
	stringstream _ss;									\
	_ss << "\t" << x;									\
	g_logger.logWorldEvent(_ss.str());					\
}

#define _UNIT_LOG(u, x) {								\
	stringstream _ss;									\
	_ss << "Frame "	<< g_world.getFrameCount() <<		\
		", Unit " << u->getId() << ": " << x;			\
	g_logger.addWorldLogMsg(_ss.str());					\
}

#define _PATH_LOG(u) {									\
	stringstream _ss;									\
	_ss << "\tLow-Level Path: " << *u->getPath();		\
	g_logger.addWorldLogMsg(_ss.str());					\
	if (!u->getWaypointPath()->empty()) {				\
		stringstream _ss; _ss << "\tWaypoint Path: "	\
			<< *u->getWaypointPath();					\
		g_logger.addWorldLogMsg(_ss.str());				\
	}													\
}

// master switch, 'world' logging
#define LOG_WORLD_EVENTS 0

// Log pathfinding results
#define LOG_PATHFINDER 0

#if LOG_WORLD_EVENTS && LOG_PATHFINDER
#	define PF_LOG(x) _LOG(x)
#	define PF_UNIT_LOG(u, x) _UNIT_LOG(u, x)
#	define PF_PATH_LOG(u) _PATH_LOG(u)
#else
#	define PF_LOG(x)
#	define PF_UNIT_LOG(u, x)
#	define PF_PATH_LOG(u)
#endif

// log command issue / cancel / etc
#define LOG_COMMAND_ISSUE 0

#if LOG_WORLD_EVENTS && LOG_COMMAND_ISSUE
#	define CMD_LOG(x) _LOG(x)
#	define CMD_UNIT_LOG(u, x) _UNIT_LOG(u, x)
#else
#	define CMD_LOG(x)
#	define CMD_UNIT_LOG(u, x)
#endif

// log unit life-cycle (created, born, died, deleted)
#define LOG_UNIT_LIFECYCLE 0

#if LOG_WORLD_EVENTS && LOG_UNIT_LIFECYCLE
#	define ULC_LOG(x) _LOG(x)
#	define ULC_UNIT_LOG(u, x) _UNIT_LOG(u, x)
#else
#	define ULC_LOG(x)
#	define ULC_UNIT_LOG(u, x)
#endif


struct FunctionTimer {
	const char *m_name;
	int64 m_start;

	FunctionTimer(const char *name) : m_name(name) {
		m_start = Chrono::getCurMicros();
	}

	~FunctionTimer() {
		int64 time = Chrono::getCurMicros() - m_start;
		std::stringstream ss;
		ss << m_name << " ";
		if (time > 1000) {
			int64 millis = time / 1000;
			ss << millis << " ms, " << (time % 1000) << " us.";
		} else {
			ss << time << " us.";
		}
		g_logger.logProgramEvent(ss.str());
	}
};

#define SCOPE_TIMER(name) \
	FunctionTimer _function_timer(name)

#define TIME_FUNCTION() /*SCOPE_TIMER(__FUNCTION__)*/


}} // namespace Glest::Util

#endif

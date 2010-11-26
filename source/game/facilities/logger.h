// ==============================================================
// This file is part of Glest (www.glest.org)
//
// Copyright (C) 2001-2008 Martiño Figueroa
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

#include "FSFactory.hpp"
#include "timer.h"

using std::deque;
using std::string;

namespace Glest {

namespace Graphics {
	class GraphicProgressBar;
}
using Graphics::GraphicProgressBar;

namespace Util {

using namespace Shared::PhysFS;

// =====================================================
// class Logger
//
/// Interface to write log files
// =====================================================

WRAPPED_ENUM( TimeStampType, NONE, SECONDS, MILLIS );

class Logger {
private:
	static const int logLineCount;

private:
	typedef deque<string> Strings;

private:
	string fileName;
	string header;
	FileOps *fileOps;

	string state;
	Strings logLines;
	string subtitle;
	string current;
	bool loadingGame;
	static char errorBuf[];
	int totalUnits, unitsLoaded;

	TimeStampType timeStampType;

	bool m_progressBar;
	int m_progress;

private:
	Logger(const char *fileName, const string &type, TimeStampType timeType = TimeStampType::NONE);
	~Logger();

public:
	static Logger &getInstance();
	static Logger &getServerLog();
	static Logger &getClientLog();
	static Logger &getErrorLog();
	static Logger &getWidgetLog();
	static Logger &getWorldLog();
	static Logger &getAiLog();

	void setState(const string &state);
	void resetState(const string &s)	{state= s;}
	void setSubtitle(const string &v)	{subtitle = v;}
	void setLoading(bool v)				{loadingGame = v;}
	void setProgressBar(bool v)			{m_progressBar = v; m_progress = 0;}

	void add(const string &str, bool renderScreen = false);
	void addXmlError(const string &path, const char *error);
	void addMediaError(const string &xmlPth, const string &mediaPath, const char *error);
	void addNetworkMsg(const string &msg);

	void renderLoadingScreen();
	void clear();

	void setUnitCount(int count) { totalUnits = count; unitsLoaded = 0; }
	void addUnitCount(int val) { totalUnits += val; }
	void unitLoaded();
};

void logNetwork(const string &msg);

inline void logNetwork(const char *msg) { 
	logNetwork(string(msg)); 
}

#define LOG_STUFF 0

#if defined(LOG_STUFF) && LOG_STUFF
#	define LOG(x) g_logger.add(x)
#	define STREAM_LOG(x) {stringstream _ss; _ss << x; g_logger.add(_ss.str()); }
#	define GAME_LOG(x) STREAM_LOG( "Frame " << g_world.getFrameCount() << ": " << x )
#else
#	define LOG(x)
#	define STREAM_LOG(x)
#	define GAME_LOG(x)
#endif

#define AI_LOGGING 0

#if AI_LOGGING
#	define LOG_AI(x) {stringstream _ss; _ss << g_world.getFrameCount() << " : " << x; Logger::getAiLog().add(_ss.str()); }
#else
#	define LOG_AI(x)
#endif

#define LOG_NETWORKING 1

#if LOG_NETWORKING
#	define LOG_NETWORK(x) logNetwork(x)
#	define NETWORK_LOG(x) {stringstream _ss; _ss << x; logNetwork(_ss.str()); }
#else
#	define LOG_NETWORK(x)
#	define NETWORK_LOG(x)
#endif

#define _LOG(x) {										\
	stringstream _ss;									\
	_ss << "\t" << x;									\
	Logger::getWorldLog().add(_ss.str());			\
}

#define _UNIT_LOG(u, x) {								\
	stringstream _ss;									\
	_ss << "Frame "	<< g_world.getFrameCount() <<		\
		", Unit " << u->getId() << ": " << x;			\
	Logger::getWorldLog().add(_ss.str());			\
}

#define _PATH_LOG(u) {									\
	stringstream _ss;									\
	_ss << "\tLow-Level Path: " << *u->getPath();		\
	Logger::getWorldLog().add(_ss.str());			\
	if (!u->getWaypointPath()->empty()) {				\
		stringstream _ss; _ss << "\tWaypoint Path: "	\
			<< *u->getWaypointPath();					\
		Logger::getWorldLog().add(_ss.str());		\
	}													\
}

// master switch, 'world' logging
#define WORLD_LOGGING 0

// Log pathfinding results
#define LOG_PATHFINDER 1

#if WORLD_LOGGING && LOG_PATHFINDER
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

#if WORLD_LOGGING && LOG_COMMAND_ISSUE
#	define CMD_LOG(x) _LOG(x)
#	define CMD_UNIT_LOG(u, x) _UNIT_LOG(u, x)
#else
#	define CMD_LOG(x)
#	define CMD_UNIT_LOG(u, x)
#endif

// log unit life-cycle (created, born, died, deleted)
#define LOG_UNIT_LIFECYCLE 0

#if WORLD_LOGGING && LOG_UNIT_LIFECYCLE
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
		g_logger.add(ss.str());
	}
};

#define SCOPE_TIMER(name) \
	FunctionTimer _function_timer(name)

#define TIME_FUNCTION() /*SCOPE_TIMER(__FUNCTION__)*/


}} // namespace Glest::Util

#endif

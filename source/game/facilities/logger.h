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

	void setState(const string &state);
	void resetState(const string &s)	{state= s;}
	void setSubtitle(const string &v)	{subtitle = v;}
	void setLoading(bool v)				{loadingGame = v;}
	void setProgressBar(bool v) { m_progressBar = v; }

	void add(const string &str, bool renderScreen = false);
	void addXmlError(const string &path, const char *error);
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
#	define STREAM_LOG(x) {stringstream ss; ss << x; g_logger.add(ss.str()); }
#	define GAME_LOG(x) STREAM_LOG( "Frame " << g_world.getFrameCount() << ": " << x )
#else
#	define LOG(x)
#	define STREAM_LOG(x)
#	define GAME_LOG(x)
#endif


#define LOG_NETWORKING 1

#if LOG_NETWORKING
#	define LOG_NETWORK(x) logNetwork(x)
#	define NETWORK_LOG(x) {stringstream _ss; _ss << x; logNetwork(_ss.str()); }
#else
#	define LOG_NETWORK(x)
#	define NETWORK_LOG(x)
#endif

}} // namespace Glest::Util

#endif

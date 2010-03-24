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

#include "timer.h"

using namespace std;

namespace Glest { namespace Game {

class GraphicProgressBar;

// =====================================================
// class Logger
//
/// Interface to write log files
// =====================================================

class Logger {
private:
	static const int logLineCount;

private:
	typedef deque<string> Strings;

private:
	string fileName;
	//string sectionName;
	string state;
	Strings logLines;
	string subtitle;
	string current;
	bool loadingGame;
	static char errorBuf[];
	int totalUnits, unitsLoaded;
	int totalClusters, clustersInit;

	GraphicProgressBar *progressBar;

private:
	Logger( const char *fileName )
		: fileName( fileName )
		, loadingGame(true)
		, progressBar(NULL) {
	}

public:
	static Logger &getInstance() {
		static Logger logger( "glestadv.log" );
		return logger;
	}

	static Logger &getServerLog() {
		static Logger logger( "glestadv-server.log" );
		return logger;
	}

	static Logger &getClientLog() {
		static Logger logger( "glestadv-client.log" );
		return logger;
	}

	static Logger &getErrorLog() {
		static Logger logger( "glestadv-error.log" );
		return logger;
	}

	//void setFile(const string &fileName) {this->fileName= fileName;}
	void setState(const string &state);
	void resetState(const string &s)	{state= s;}
	void setSubtitle(const string &v)	{subtitle = v;}
	void setLoading(bool v)				{loadingGame = v;}
	void setProgressBar(GraphicProgressBar *v) { progressBar = v; }

	void add(const string &str, bool renderScreen = false);
	void addXmlError(const string &path, const char *error);
	void addNetworkMsg(const string &msg);

	void renderLoadingScreen();
	void clear();

	void setUnitCount(int count) { totalUnits = count; unitsLoaded = 0; }
	void addUnitCount(int val) { totalUnits += val; }
	void unitLoaded();

	void setClusterCount(int count) { totalClusters = count; clustersInit = 0; }
	void clusterInit();
};

void logNetwork(const string &msg);

inline void logNetwork(const char *msg) { 
	logNetwork(string(msg)); 
}

#define LOG_NETWORKING 1

#if LOG_NETWORKING
#	define LOG_NETWORK(x) logNetwork(x)
#else
#	define LOG_NETWORK(x) 
#endif

#if defined(WIN32) | defined(WIN64)

class Timer {
public:
	Timer( int threshold, const char* msg ) {}
	~Timer() {}

	void print( const char* msg ) {}

	struct timeval getDiff() {}
};

#else

class Timer {
	struct timeval start;
	struct timezone tz;
	unsigned int threshold;
	const char* msg;
	FILE *outfile;

public:
	Timer( int threshold, const char* msg, FILE *outfile = stderr ) :
			threshold( threshold ), msg( msg ), outfile( outfile ) {
		tz.tz_minuteswest = 0;
		tz.tz_dsttime = 0;
		gettimeofday( &start, &tz );
	}

	~Timer() {
		struct timeval diff = getDiff();
		unsigned int diffusec = diff.tv_sec * 1000000 + diff.tv_usec;
		if ( diffusec > threshold ) {
			fprintf( outfile, "%s: %d\n", msg, diffusec );
		}
	}

	void print( const char* msg ) {
		struct timeval diff = getDiff();
		unsigned int diffusec = diff.tv_sec * 1000000 + diff.tv_usec;
		fprintf( outfile, "%s -> %s: %d\n", this->msg, msg, diffusec );
	}

	struct timeval getDiff() {
		struct timeval cur, diff;
		gettimeofday( &cur, &tz );
		diff.tv_sec = cur.tv_sec - start.tv_sec;
		diff.tv_usec = cur.tv_usec - start.tv_usec;
		return diff;
	}
};
#endif

}}//end namespace

#endif

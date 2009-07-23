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

#ifndef _SHARED_UTIL_LOGGER_H_
#define _SHARED_UTIL_LOGGER_H_

#include <string>
#include <deque>
#include <time.h>

#include "timer.h"

using std::string;
using std::deque;

namespace Glest{ namespace Game{

// =====================================================
//	class Logger
//
/// Interface to write log files
// =====================================================

class Logger{
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

private:
	Logger(const char *fileName) : fileName(fileName) { loadingGame = true; }

public:
	static Logger &getInstance(){
		static Logger logger("glestadv.log");
		return logger;
	}

	static Logger &getServerLog(){
		static Logger logger("glestadv-server.log");
		return logger;
	}

	static Logger &getClientLog(){
		static Logger logger("glestadv-client.log");
		return logger;
	}

   static Logger &getErrorLog(){
		static Logger logger("glestadv-error.log");
		return logger;
	}

   void addXmlError ( const string &path, const char *error );

	//void setFile(const string &fileName)	{this->fileName= fileName;}
	void setState(const string &state);
   void setSubtitle(const string &subtitle)	{this->subtitle= subtitle;}

	void add(const string &str, bool renderScreen= false);
	void renderLoadingScreen();
   void setLoading ( bool loading ) { loadingGame = loading; }

	void clear();
};

#if defined(WIN32) | defined(WIN64)

class Timer {
public:
	Timer(int threshold, const char* msg) {}
	~Timer() {}

	void print(const char* msg) {}

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
	Timer(int threshold, const char* msg, FILE *outfile = stderr) :
			threshold(threshold), msg(msg), outfile(outfile) {
		tz.tz_minuteswest = 0;
		tz.tz_dsttime = 0;
		gettimeofday(&start, &tz);
	}

	~Timer() {
		struct timeval diff = getDiff();
		unsigned int diffusec = diff.tv_sec * 1000000 + diff.tv_usec;
		if(diffusec > threshold) {
			fprintf(outfile, "%s: %d\n", msg, diffusec);
		}
	}

	void print(const char* msg) {
		struct timeval diff = getDiff();
		unsigned int diffusec = diff.tv_sec * 1000000 + diff.tv_usec;
		fprintf(outfile, "%s -> %s: %d\n", this->msg, msg, diffusec);
	}

	struct timeval getDiff() {
		struct timeval cur, diff;
		gettimeofday(&cur, &tz);
		diff.tv_sec = cur.tv_sec - start.tv_sec;
		diff.tv_usec = cur.tv_usec - start.tv_usec;
		return diff;
	}
};
#endif

}}//end namespace

#endif

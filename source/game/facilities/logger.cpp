// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//                2009-2011 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "logger.h"

#include "util.h"
#include "renderer.h"
#include "core_data.h"
#include "metrics.h"
#include "lang.h"
#include "components.h"

#include "leak_dumper.h"
#include "world.h"
#include "sim_interface.h"

using namespace Shared::Graphics;
using namespace Glest::Net;

namespace Glest { namespace Util {

#ifdef WIN32
	string newLine = "\r\n";
#else
	string newLine = "\n";
#endif

// =====================================================
//  class LogFile
// =====================================================

LogFile::LogFile(const string &fileName, const string &type, TimeStampType timeType)
		: m_fileName(fileName)
		, m_fileOps(0)
		, m_timeStampType(timeType) {
	m_fileOps = g_fileFactory.getFileOps();
	m_fileOps->openWrite(m_fileName.c_str());
	string header = "Glest Advanced Engine: " + type + " log file. "
		+ Logger::fileTimestamp() + "\n\n";
	m_fileOps->write(header.c_str(), sizeof(char), header.size());
}

LogFile::~LogFile() {
	delete m_fileOps;
}

void LogFile::add(const string &str){
	if (m_timeStampType == TimeStampType::SECONDS) {
		string myTime = intToStr(int(clock() / 1000)) + ": ";
		m_fileOps->write(myTime.c_str(), sizeof(char), myTime.size());
	} else if (m_timeStampType == TimeStampType::MILLIS) {
		string myTime = intToStr(int(clock())) + ": ";
		m_fileOps->write(myTime.c_str(), sizeof(char), myTime.size());
	}
	m_fileOps->write(str.c_str(), sizeof(char), str.size());
	m_fileOps->write(newLine.c_str(), sizeof(char), newLine.size());
}

void LogFile::logXmlError(const string &path, const char *error) {
	stringstream ss;
	ss << "XML Error in " << path << ":\n\t" << error;
	add(ss.str());
}

void LogFile::logMediaError(const string &xmlPath, const string &mediaPath, const char *error) {
	stringstream ss;
	if (xmlPath != "") {
		ss << "Error loading " << mediaPath << ":\n\treferenced in " << xmlPath << "\n\t" << error;
	} else {
		ss << "Error loading " << mediaPath << ":\n\t" << error;
	}
	add(ss.str());
}

void LogFile::addNetworkMsg(const string &msg) {
	stringstream ss;
	if (World::isConstructed()) {
		ss << "Frame: " << g_world.getFrameCount(); 
	} else {
		ss << "Frame: 0";
	}
	ss << " :: " << msg;
	add(ss.str());
}

// =====================================================
//	class ProgramLog
// =====================================================

const int ProgramLog::logLineCount= 15;

ProgramLog::ProgramLog()
		: LogFile("glestadv.log", "Program", TimeStampType::SECONDS)
		, loadingGame(true)
		, m_progressBar(false)
		, m_progress(0)
		, m_backgroundTexture(0) {
}


void ProgramLog::add(const string &str,  bool renderScreen) {
	LogFile::add(str);
	if (loadingGame && renderScreen) {
		logLines.push_back(str);
		if(logLines.size() > logLineCount) {
			logLines.pop_front();
		}
	} else {
		current = str;
	}
	if (renderScreen) {
		renderLoadingScreen();
	}
}

void ProgramLog::setState(const string &state){
	this->state= state;
	logLines.clear();
}

void ProgramLog::unitLoaded() {
	++unitsLoaded;
	float pcnt = ((float)unitsLoaded) / ((float)totalUnits) * 100.f;
	m_progress = int(pcnt);
}

void ProgramLog::useLoadingScreenDefaults() {
	m_backgroundTexture = g_coreData.getBackgroundTexture();
}

/** Load the loading screen settings from xml
  * @return true if loaded successfully
  */
bool ProgramLog::setupLoadingScreen(const string &dir) {
	string path = dir + "/" + basename(dir) + ".xml";

	//open xml file
	XmlTree xmlTree;
	try { 
		xmlTree.load(path); 
	} catch (runtime_error e) { 
		g_logger.logXmlError(path, "File missing or wrongly named.");
		return false; // bail
	}
	const XmlNode *rootNode;
	try { 
		rootNode = xmlTree.getRootNode(); 
	} catch (runtime_error e) { 
		g_logger.logXmlError(path, "File appears to lack contents.");
		return false; // bail
	}

	const XmlNode *loadingScreenNode = rootNode->getChild("loading-screen", 0, false);

	if (loadingScreenNode) {
		// could randomly choose from multiple or choose 
		// based on resolution - hailstone 21Jan2011

		// background texture
		const XmlNode *backgroundImageNode = loadingScreenNode->getChild("background-image", 0, true);

		if (backgroundImageNode) {
			// load background image from faction.xml
			m_backgroundTexture = g_renderer.newTexture2D(ResourceScope::GLOBAL);
			m_backgroundTexture->setMipmap(false);
			m_backgroundTexture->getPixmap()->load(dir + "/" + 
				backgroundImageNode->getAttribute("path")->getValue());
			m_backgroundTexture->init(Texture::fBilinear);
		}

		// faction tips

		return true; // successfully using settings
	}
	return false;
}

void ProgramLog::renderLoadingScreen(){
	g_renderer.reset2d(true);
	g_renderer.clearBuffers();

	Font *normFont = g_coreData.getFTMenuFontSmall();
	Font *bigFont = g_coreData.getFTMenuFontNormal();

	if (m_backgroundTexture) {
		g_renderer.renderBackground(m_backgroundTexture);
	}
	
	Vec2i headerPos(g_metrics.getScreenW() / 4, 75 * g_metrics.getScreenH() / 100);
	g_renderer.renderText(state, bigFont, Vec3f(1.f), headerPos.x, headerPos.y, false);

	if (loadingGame) {
		int offset = 0;
		int step = int(normFont->getMetrics()->getHeight()) + 4;
		for (Strings::reverse_iterator it = logLines.rbegin(); it != logLines.rend(); ++it) {
			float alpha = offset == 0 ? 1.0f : 0.8f - 0.8f * static_cast<float>(offset) / logLines.size();
			g_renderer.renderText(*it, normFont, alpha, g_metrics.getScreenW() / 4,
				70 * g_metrics.getScreenH() / 100 - offset * step, false);
			++offset;
		}
		if (m_progressBar) {
			Vec2i progBarPos = headerPos;
			progBarPos.x += int(bigFont->getMetrics()->getTextDiminsions(state).x) + 20;
			int w = g_metrics.getScreenW() / 4 * 3 - progBarPos.x;
			int h = int(normFont->getMetrics()->getHeight() + 2.f);
			g_renderer.renderProgressBar(m_progress, progBarPos.x, progBarPos.y, w, h, normFont);
		}
	} else {
		g_renderer.renderText(current, normFont, 1.0f, g_metrics.getScreenW() / 4,
			62 * g_metrics.getScreenH() / 100, false);
	}
	g_renderer.swapBuffers();
}

// =====================================================
//	class AiLogFile
// =====================================================

AiLogFile::AiLogFile()
		: LogFile("glestadv-ai.log", "AI", TimeStampType::NONE) {
	for (int i=0; i < GameConstants::maxPlayers; ++i) {
		m_flags[i].m_level = 2;
		m_flags[i].m_enabled = true;
		for (int j=0; j < AiComponent::COUNT; ++j) {
			m_flags[i].m_components[j] = true;
		}
	}
}

void AiLogFile::add(int f, AiComponent c, int level, const string &msg) {
	ASSERT_RANGE(f, GameConstants::maxPlayers);
	if (m_flags[f].m_enabled && level <= m_flags[f].m_level
	&& (c == AiComponent::INVALID || m_flags[f].m_components[c])) {
		stringstream ss;
		ss << "AI: " << f << " [" << AiComponentNames[c] << "] Frame: " 
			<< g_world.getFrameCount() << ", " << ": " << msg;
		LogFile::add(ss.str());
	}
}

// =====================================================
//	class Logger
// =====================================================

Logger* Logger::instance = 0;

Logger::Logger()
		: m_programLog(0)
		, m_errorLog(0)
		, m_aiLog(0)
		, m_networkLog(0)
		, m_widgetLog(0)
		, m_worldLog(0) {
	
	// always enabled
	m_programLog = new ProgramLog();
	m_errorLog = new LogFile("glestadv-error.log", "Error", TimeStampType::NONE);
	m_aiLog  = new AiLogFile();
	m_networkLog  = new LogFile("glestadv-network.log", "Network", TimeStampType::MILLIS);

	// enabled by preprocessor symbol
#	if LOG_WIDGET_EVENTS
		m_widgetLog  = new LogFile("glestadv-widget.log", "Widget", TimeStampType::MILLIS);
#	endif

#	if LOG_WORLD_EVENTS
		m_worldLog  = new LogFile("glestadv-world.log", "World", TimeStampType::NONE);
#	endif
}

Logger::~Logger() {
	// collect lang look-up errors
	vector<string> &errors = Lang::getLookUpErrors();
	if (!errors.empty()) {
		m_errorLog->add("\nLang look-up errors:");
	}
	foreach_const (vector<string>, it, errors) {
		m_errorLog->add(*it);
	}
	// close everything
	delete m_programLog;
	delete m_errorLog;
	delete m_aiLog;
	delete m_widgetLog;
	delete m_worldLog;
	delete m_networkLog;
}

string Logger::fileTimestamp() {
	time_t rawtime;
	struct tm *timeinfo;
	char formatted[30];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	//Day of the month (01-31), Abbreviated month name (eg Feb), Year,
	// Hour in 24h format (00-23), Minute (00-59), Seconds (00-59)
	strftime(formatted, 30, "%d-%b-%Y_%H-%M-%S", timeinfo);

	return string(formatted);
}

}} // namespace Glest::Util

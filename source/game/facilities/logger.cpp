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

// =====================================================
//	class Logger
// =====================================================

const int Logger::logLineCount= 15;

Logger::Logger(const char *fileName, const string &type, TimeStampType timeType)
		: fileName(fileName)
		, fileOps(0)
		, loadingGame(true)
		, timeStampType(timeType)
		, m_progressBar(false)
		, m_progress(0) {
	header = "Glest Advanced Engine: " + type + " log file.\n\n";
}

Logger::~Logger() {
	delete fileOps;
}

// ===================== PUBLIC ========================

Logger& Logger::getInstance() {
	static Logger logger("glestadv.log", "Game", TimeStampType::SECONDS);
	return logger;
}

Logger& Logger::getServerLog() {
	static Logger logger("glestadv-server.log", "Server", TimeStampType::NONE);
	return logger;
}

Logger& Logger::getClientLog() {
	static Logger logger("glestadv-client.log", "Client", TimeStampType::NONE);
	return logger;
}

Logger& Logger::getErrorLog() {
	static Logger logger("glestadv-error.log", "Error", TimeStampType::NONE);
	return logger;
}

Logger& Logger::getWidgetLog() {
	static Logger logger("glestadv-widget.log", "Widget", TimeStampType::MILLIS);
	return logger;
}

void Logger::setState(const string &state){
	this->state= state;
	logLines.clear();
}

void Logger::unitLoaded() {
	++unitsLoaded;
	float pcnt = ((float)unitsLoaded) / ((float)totalUnits) * 100.f;
	m_progress = int(pcnt);
}

string newLine = "\n";

void Logger::add(const string &str,  bool renderScreen){
	if (timeStampType == TimeStampType::SECONDS) {
		string myTime = intToStr(int(clock() / 1000)) + ": ";
		fileOps->write(myTime.c_str(), sizeof(char), myTime.size());
	} else if (timeStampType == TimeStampType::MILLIS) {
		string myTime = intToStr(int(clock())) + ": ";
		fileOps->write(myTime.c_str(), sizeof(char), myTime.size());
	}
	fileOps->write(str.c_str(), sizeof(char), str.size());
	fileOps->write(newLine.c_str(), sizeof(char), newLine.size());
	
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

void Logger::addXmlError(const string &path, const char *error) {
	static char buffer[2048];
	sprintf(buffer, "XML Error in %s:\n %s", path.c_str(), error);
	add(buffer);
}

void Logger::addNetworkMsg(const string &msg) {
	stringstream ss;
	if (World::isConstructed()) {
		ss << "Frame: " << g_world.getFrameCount(); 
	} else {
		ss << "Frame: 0";
	}
	ss << " :: " << msg;
	add(ss.str());
}

void Logger::clear() {
	delete fileOps;
	fileOps = g_fileFactory.getFileOps();
	fileOps->openWrite(fileName.c_str());
	fileOps->write(header.c_str(), sizeof(char), header.size());
}

// ==================== PRIVATE ====================

void Logger::renderLoadingScreen(){
	g_renderer.reset2d(true);
	g_renderer.clearBuffers();

	Font *normFont = g_coreData.getFTMenuFontSmall();
	Font *bigFont = g_coreData.getFTMenuFontNormal();

	g_renderer.renderBackground(g_coreData.getBackgroundTexture());
	
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
			progBarPos.x += bigFont->getMetrics()->getTextDiminsions(state).x + 20;
			int w = g_metrics.getScreenW() / 4 * 3 - progBarPos.x;
			int h = normFont->getMetrics()->getHeight() + 6;
			g_renderer.renderProgressBar(m_progress, progBarPos.x, progBarPos.y, w, h, normFont);
		}
	} else {
		g_renderer.renderText(current, normFont, 1.0f, g_metrics.getScreenW() / 4,
			62 * g_metrics.getScreenH() / 100, false);
	}
	g_renderer.swapBuffers();
}

void logNetwork(const string &msg) {
	GameRole role = g_simInterface->getNetworkRole();
	if (role == GameRole::SERVER) {
		Logger::getServerLog().addNetworkMsg(msg);
	} else if (role == GameRole::CLIENT) {
		Logger::getClientLog().addNetworkMsg(msg);
	} else {
		// what the ...
		g_errorLog.addNetworkMsg(msg);
	}
}

}} // namespace Glest::Util

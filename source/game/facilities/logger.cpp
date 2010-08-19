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

Logger::Logger(const char *fileName, const string &type, bool prependTime)
		: fileName(fileName)
		, fileOps(0)
		, loadingGame(true)
		, progressBar(NULL)
		, prependTime(prependTime) {
	header = "Glest Advanced Enginge: " + type + " log file.\n\n";
}

Logger::~Logger() {
	delete fileOps;
}

// ===================== PUBLIC ========================

Logger& Logger::getInstance() {
	static Logger logger("glestadv.log", "Game");
	return logger;
}

Logger& Logger::getServerLog() {
	static Logger logger("glestadv-server.log", "Server", false);
	return logger;
}

Logger& Logger::getClientLog() {
	static Logger logger("glestadv-client.log", "Client", false);
	return logger;
}

Logger& Logger::getErrorLog() {
	static Logger logger("glestadv-error.log", "Error", false);
	return logger;
}

void Logger::setState(const string &state){
	this->state= state;
	logLines.clear();
}

void Logger::unitLoaded() {
	++unitsLoaded;
	float pcnt = ((float)unitsLoaded) / ((float)totalUnits) * 100.f;
	progressBar->setProgress(int(pcnt));
}

string newLine = "\n";

void Logger::add(const string &str,  bool renderScreen){
	if (prependTime) {
		string myTime = intToStr(int(clock() / 1000)) + ": ";
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
	Renderer &renderer= Renderer::getInstance();
	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();

	renderer.reset2d();
	renderer.clearBuffers();

	renderer.renderBackground(CoreData::getInstance().getBackgroundTexture());

	renderer.renderText(
		state, coreData.getMenuFontBig(), Vec3f(1.f),
		metrics.getVirtualW()/4, 75*metrics.getVirtualH()/100, false
	);
	if ( loadingGame ) {
		int offset= 0;
		Font *font= coreData.getMenuFontNormal();
		for(Strings::reverse_iterator it= logLines.rbegin(); it!=logLines.rend(); ++it){
			float alpha= offset==0? 1.0f: 0.8f-0.8f*static_cast<float>(offset)/logLines.size();
			renderer.renderText(
				*it, font, alpha ,
				metrics.getVirtualW()/4,
				70*metrics.getVirtualH()/100 - offset*(font->getSize()+4),
				false
			);
			++offset;
		}
		if (progressBar) {
			progressBar->render();
		}
	}
	else
	{
		renderer.renderText(
			current, coreData.getMenuFontNormal(), 1.0f, 
			metrics.getVirtualW()/4, 
			62*metrics.getVirtualH()/100, false);
	}
	renderer.swapBuffers();
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

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
#include "FSFactory.hpp"

#include "leak_dumper.h"
#include "world.h"
#include "network_util.h"

using namespace std;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
//	class Logger
// =====================================================

const int Logger::logLineCount= 15;

// ===================== PUBLIC ========================

void Logger::setState(const string &state){
	this->state= state;
	logLines.clear();
}

void Logger::unitLoaded() {
	++unitsLoaded;
	float pcnt = ((float)unitsLoaded) / ((float)totalUnits) * 100.f;
	progressBar->setProgress(int(pcnt));
}

void Logger::clusterInit() {
	++clustersInit;
	float pcnt = ((float)clustersInit) / ((float)totalClusters) * 100.f;
	progressBar->setProgress(int(pcnt));
	renderLoadingScreen();
}

void Logger::add(const string &str,  bool renderScreen){
/*	FILE *f=fopen(fileName.c_str(), "at+");
	if (f) {
		fprintf(f, "%d: %s\n", (int)(clock() / 1000), str.c_str());
		fclose(f);
	}*/
	FileOps *f = FSFactory::getInstance()->getFileOps();
	f->openAppend(fileName.c_str());
	//FIXME: ugly
	stringstream sstream;
	sstream << (int)(clock() / 1000) << ": " << str << endl;
	string s = sstream.str();
	f->write((void*)s.c_str(), sizeof(char), s.size());
	delete f;
	
	if (loadingGame && renderScreen) {
		if (f == NULL) {
			throw runtime_error("Error opening log file" + fileName);
		}

		logLines.push_back(str);
		if(logLines.size() > logLineCount) {
			logLines.pop_front();
		}
	} else {
		current = str;
	}
	if(renderScreen) {
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
		ss << "Frame: " << theWorld.getFrameCount(); 
	} else {
		ss << "Frame: 0";
	}
	ss << " timestamp: " << Chrono::getCurMillis() << " :: " << msg;
	add(ss.str());
}

void Logger::clear() {
	string s="Log file\n";

	FILE *f= fopen(fileName.c_str(), "wt+");
	if (!f) {
		throw runtime_error("Error opening log file" + fileName);
	}

	fprintf(f, "%s", s.c_str());
	fprintf(f, "\n");

	fclose(f);
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
		Font2D *font= coreData.getMenuFontNormal();
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
	if (isNetworkServer()) {
		Logger::getServerLog().addNetworkMsg(msg);
	} else if (isNetworkClient()) {
		Logger::getClientLog().addNetworkMsg(msg);
	} else {
		// what the ...
		Logger::getErrorLog().addNetworkMsg(msg);
	}
}

}}//end namespace

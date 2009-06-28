// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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

#include "leak_dumper.h"


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
   //MERGE DELETE
   //logLines.clear();
}

void Logger::add(const string &str,  bool renderScreen){
   FILE *f=fopen(fileName.c_str(), "at+");
	//MERGE ADD START
   if ( f )
   {
	   fprintf(f, "%d: %s\n", (int)(clock() / 1000), str.c_str());
      fclose(f);
   }
   //MERGE ADD END
   //MERGE DELETE START
   /*
   if(f==NULL){
      throw runtime_error("Error opening log file"+ fileName);
   }

   logLines.push_back(str);
	if(logLines.size() > logLineCount){
		logLines.pop_front();
	}
   */
	//MERGE ADD START
   current = str;
   //MERGE ADD END
   //MERGE DELETE END
	if(renderScreen){
		renderLoadingScreen();
	}
}

void Logger::clear() {
    string s="Log file\n";

	FILE *f= fopen(fileName.c_str(), "wt+");
	if(!f){
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
		metrics.getVirtualW()/4, 65*metrics.getVirtualH()/100, false);
   //MERGE ADD START
   renderer.renderText(
      current, coreData.getMenuFontNormal(), 1.0f, 
      metrics.getVirtualW()/4, 
      62*metrics.getVirtualH()/100, false);
   //MERGE ADD END
   //MERGE DELETE START
   // This is a 'better looking' method... we should keep it
   /* 
   int offset= 0;
	Font2D *font= coreData.getMenuFontNormal();
	for(Strings::reverse_iterator it= logLines.rbegin(); it!=logLines.rend(); ++it){
		float alpha= offset==0? 1.0f: 0.8f-0.8f*static_cast<float>(offset)/logLines.size();
		renderer.renderText(
			*it, font, alpha ,
			metrics.getVirtualW()/4,
			65*metrics.getVirtualH()/100 - offset*(font->getSize()+4),
			false);
		++offset;
	}
   */
   //MERGE DELETE END
	renderer.swapBuffers();
}

}}//end namespace

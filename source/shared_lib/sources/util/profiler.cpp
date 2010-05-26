// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "profiler.h"


#include "platform_util.h"
#include "timer.h"
#include "FSFactory.hpp"

#include <map>

namespace Shared { namespace Util { 

#ifdef SL_PROFILE

using Shared::Platform::Chrono;

using Shared::Platform::Chrono;

namespace Profile {

// =====================================================
//	class Section
// =====================================================

class Section {
public:
	typedef map<string, Section*> SectionContainer;
private:
	string name;
	Chrono chrono;
	int64 microsElapsed;
	int64 lastStart;
	unsigned int calls;
	Section *parent;
	SectionContainer children;

public:
	Section(const string &name);

	Section *getParent()				{return parent;}
	const string &getName() const		{return name;}

	void setParent(Section *parent)	{this->parent= parent;}

	void start()	{ lastStart = Chrono::getCurMicros();}
	void stop()		{ microsElapsed += Chrono::getCurMicros() - lastStart; } 

	void incCalls () { calls++; }

	void addChild(Section *child)	{children[child->name] = child;}
	Section *getChild(const string &name);

	void print(ostream *outSream, int tabLevel=0);
};

// =====================================================
//	class Section
// =====================================================

Section::Section(const string &name){
	this->name= name;
	microsElapsed= 0;
	calls = 0;
	parent= NULL;
}

Section *Section::getChild(const string &name){
	SectionContainer::iterator it = children.find(name);
	if (it == children.end()) {
		return NULL;
	}
	return it->second;
}

void Section::print(ostream *outStream, int tabLevel){
	float percent = ( parent == NULL || parent->microsElapsed == 0 )
					? 100.0f : 100.0f * microsElapsed / parent->microsElapsed;
	string pname= parent==NULL? "": parent->getName();

	for(int i=0; i<tabLevel; ++i)
		*outStream << "\t";

	*outStream << name << ": ";

	if ( microsElapsed ) {
		*outStream << int(microsElapsed) << " us";
		unsigned int milliseconds = microsElapsed / 1000;
		unsigned int seconds = milliseconds / 1000;
		unsigned int minutes = seconds / 60;
		if ( minutes ) {
			*outStream << " (" << minutes << "min " << seconds % 60 << "sec)";
		}
		else if ( seconds ) {
			*outStream << " (" << seconds << "sec " << milliseconds % 1000 << "ms)";
		}
		else if ( milliseconds ) {
			*outStream << " (" << milliseconds << "ms)";
		}
		outStream->precision(1);
		*outStream << std::fixed << ", " << percent << "%";
	}
	if ( calls ) {
		*outStream << ", " << calls << " calls";
	}
	*outStream << "\n";

	SectionContainer::iterator it;
	for(it= children.begin(); it!=children.end(); ++it){
		it->second->print(outStream, tabLevel+1);
	}
}


// =====================================================
//	class Profiler
// =====================================================

class Profiler {
	Section *rootSection;
	Section *currSection;

public:
	Profiler();
	~Profiler();

	void close();
	void sectionBegin(const string &name);
	void sectionEnd(const string &name);
};

Profiler::Profiler(){
	rootSection= new Section("Root");
	currSection= rootSection;
	rootSection->start();
}

Profiler::~Profiler(){
}

void Profiler::close(){
	rootSection->stop();

	ostream *ofs = FSFactory::getInstance()->getOStream("profiler.log");
	*ofs << "Profiler Results\n\n";
	rootSection->print(ofs);
	delete ofs;
}

void Profiler::sectionBegin(const string &name ){
	Section *childSection= currSection->getChild(name);
	if(childSection==NULL){
		childSection= new Section(name);
		currSection->addChild(childSection);
		childSection->setParent(currSection);
	}
	currSection= childSection;
	childSection->start();
}

void Profiler::sectionEnd(const string &name){
	if(name==currSection->getName()){
		currSection->stop();
		currSection= currSection->getParent();
	}
	else{
		throw runtime_error("Profile: Leaving section is not current section: "+name);
	}
}

Profiler& getProfiler() {
	static Profiler profiler;
	return profiler;
}

void profileEnd(){
	getProfiler().close();
}

void sectionBegin(const string &name) {
	getProfiler().sectionBegin(name);
}

void sectionEnd(const string &name) {
	getProfiler().sectionEnd(name);
}

} // namespace Profile

#endif

#ifdef SL_TRACE

namespace Trace {
	
	static int currDepth = 0;
	
	const char *trace_filename = "gae_trace.txt";
	char space20[] = "                    ";

	FileOps *traceFile;

	struct TraceLogFileContainer {
		TraceLogFileContainer() {
			traceFile = FSFactory::getInstance()->getFileOps();
			traceFile->openWrite(trace_filename);
		}
		~TraceLogFileContainer() {
			traceFile->close();
		}
	};

	FunctionTrace::FunctionTrace(const char *name) : name(name), callDepth(currDepth) {
		static TraceLogFileContainer startLog;
		int space = callDepth;
		while (space >= 20) {
			traceFile->write(space20, 20, 1);
			space -= 20;
		}
		if (space) {
			traceFile->write(space20, space, 1);
		}
		static char buf[1024];
		int len = sprintf(buf, "+ %s()\n", name);
		traceFile->write(buf, len, 1);
		++currDepth;
	}

	FunctionTrace::~FunctionTrace() {
		int space = callDepth;
		while (space >= 20) {
			traceFile->write(space20, 20, 1);
			space -= 20;
		}
		if (space) {
			traceFile->write(space20, space, 1);
		}
		static char buf[1024];
		int len = sprintf(buf, "- %s()\n", name);
		traceFile->write(buf, len, 1);
		--currDepth;
	}

} // namespace Trace


#endif

}} //end namespace Shared::Util

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

#ifdef SL_PROFILE

#include "platform_util.h"
#include "timer.h"

using Shared::Platform::Chrono;

namespace Shared { namespace Util { namespace Profile {

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

	void print(FILE *outSream, int tabLevel=0);
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

void Section::print(FILE *outStream, int tabLevel){
	float percent = ( parent == NULL || parent->microsElapsed == 0 )
					? 100.0f : 100.0f * microsElapsed / parent->microsElapsed;
	string pname= parent==NULL? "": parent->getName();

	for(int i=0; i<tabLevel; ++i)
		fprintf(outStream, "\t");

	fprintf(outStream, "%s: ", name.c_str());

	if ( microsElapsed ) {
		fprintf(outStream, "%d us", int(microsElapsed));
		unsigned int milliseconds = microsElapsed / 1000;
		unsigned int seconds = milliseconds / 1000;
		unsigned int minutes = seconds / 60;
		if ( minutes ) {
			fprintf ( outStream, " (%dmin %dsec)", minutes, seconds % 60 );
		}
		else if ( seconds ) {
			fprintf ( outStream, " (%dsec %dms)", seconds, milliseconds % 1000 );
		}
		else if ( milliseconds ) {
			fprintf ( outStream, " (%dms)", milliseconds );
		}
		fprintf(outStream, ", %.1f%%", percent );
	}
	if ( calls ) {
		fprintf(outStream, ", %u calls", calls );
	}
	fprintf( outStream, "\n" );

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

	void sectionBegin(const string &name);
	void sectionEnd(const string &name);
};

// =====================================================
//	class Profiler
// =====================================================

Profiler::Profiler(){
	rootSection= new Section("Root");
	currSection= rootSection;
	rootSection->start();
}

Profiler::~Profiler(){
	rootSection->stop();

	FILE *f= fopen("profiler.log", "w");
	if ( f ) {
		fprintf(f, "Profiler Results\n\n");
		rootSection->print(f);
		fclose(f);
	}
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

void sectionBegin(const string &name) {
	getProfiler().sectionBegin(name);
}

void sectionEnd(const string &name) {
	getProfiler().sectionEnd(name);
}

}}} //end namespace Shared::Util::Profile

#endif

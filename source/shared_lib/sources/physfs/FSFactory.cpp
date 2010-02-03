// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2010 Frank Tetzel <tetzank@users.sourceforge.net>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "FSFactory.hpp"

#include <stdexcept>
#include <cstring>
#include <iostream>
//#include <physfs.h> //already in PhysFileOps.hpp

#include "ifile_stream.hpp"
#include "ofile_stream.hpp"

#include "PhysFileOps.hpp"
#include "StdFileOps.hpp"

#include "projectConfig.h"

#include "util.h"

using namespace Shared::Util;


FSFactory *FSFactory::instance = NULL;

FSFactory::FSFactory(){
	this->physFS = false;
}

FSFactory::~FSFactory(){
	if(PHYSFS_isInit()){
		PHYSFS_deinit();
	}
	instance = NULL;
}

FSFactory *FSFactory::getInstance(){
	if(!instance){
		instance = new FSFactory();
	}
	return instance;
}

void FSFactory::initPhysFS(const char *argv0, const char *configDir){
	PHYSFS_init(argv0);
	PHYSFS_permitSymbolicLinks(1);
	
	if(!PHYSFS_mount(DEFAULT_DATA_DIR, NULL, 1)){
		throw runtime_error(string("Couldn't mount dataDir: ") + DEFAULT_DATA_DIR);
	}
	PHYSFS_setWriteDir(configDir);
	if(!PHYSFS_mount(PHYSFS_getWriteDir(), NULL, 1)){
		throw runtime_error(string("Couldn't mount configDir: ") + configDir);
	}
	// check for addons
	char **list = PHYSFS_enumerateFiles("addons");
	for(char **i=list; *i; i++){
		// because we use physfs functions, we need path in physfs
		string str("addons/");
		str += *i;
		// check if useful
		if(PHYSFS_isDirectory(str.c_str()) || ext(str)=="zip"){// || ext(str)=="7z"){
			// get full real name
			str = PHYSFS_getRealDir(str.c_str()) + str;
			if(!PHYSFS_mount(str.c_str(), NULL, 0)){  // last 0 -> overwrites all other files
				throw runtime_error("Couldn't mount addon: " + str);
			}
		}
	}
	PHYSFS_freeList(list);

	//FIXME: debug
	for(char **i=PHYSFS_getSearchPath(); *i; i++){
		cout << "[" << *i << "] is in the search path.\n";
	}
}

void FSFactory::deinitPhysFS(){
	PHYSFS_deinit();
}

void FSFactory::usePhysFS(bool enable){
	this->physFS = enable;
}

istream *FSFactory::getIStream(const char *fname){
	if(this->physFS){
		string str(fname);
		str = cleanPath(str);  // get rid of .. and .
		return new IFileStream(str);
	}else{
		return new ifstream(fname, ios::in | ios::binary);
	}
}

ostream *FSFactory::getOStream(const char *fname){
	if(this->physFS){
		string str(fname);
		str = cleanPath(str);  // get rid of ../ and ./
		return new OFileStream(str);
	}else{
		return new ofstream(fname, ios::out | ios::binary);
	}
}

FileOps *FSFactory::getFileOps(){
	if(this->physFS){
		return new PhysFileOps();
	}else{
		return new StdFileOps();
	}
}

// FIXME: quick & dirty
vector<string> FSFactory::findAll(const string &path, bool cutExtension){
	vector<string> res;
	
	// FIXME: currently assumes there's always a dir before wildcard
	//int pos = path.find_last_of('/');
	//const string dir = path.substr(0, pos);
	//const string basestr = path.substr(pos+1);  // +1 to skip /
	const string dir = dirname(path);
	const string basestr = basename(path);
	const char *base = basestr.c_str();  // otherwise something is not correctly allocated
	char **list = PHYSFS_enumerateFiles(dir.c_str());
	const char *b, *f;
	for(char **i=list; *i; i++){
		if(**i=='.'){  //skip files/folders with leading .
			continue;
		}
		b = base + strlen(base) -1;
		f = *i + strlen(*i) -1;
		while(*b==*f && (*b!='*' || *b!='.')){  // base=="*." -> wants all
			b--;
			f--;
		}
		if(*b=='*' || *b=='.'){
			if(cutExtension){
				string str = *i;
				str = cutLastExt(str);
				res.push_back(str);
			}else{
				res.push_back(*i);
			}
		}
	}
	PHYSFS_freeList(list);
	return res;
}

bool FSFactory::fileExists(const string &path){
	return PHYSFS_exists(path.c_str());
}


//Ogg callbacks
size_t FSFactory::cb_read(void *ptr, size_t size, size_t nmemb, void *source){
	return ((FileOps*)source)->read(ptr, size, nmemb);
}

int FSFactory::cb_close(void *source){
	int res = ((FileOps*)source)->close();
	delete (FileOps*)source;
	return res;
}

int FSFactory::cb_seek(void *source, ogg_int64_t offset, int whence){
	return ((FileOps*)source)->seek(offset, whence);
}

long int FSFactory::cb_tell(void *source){
	return ((FileOps*)source)->tell();
}

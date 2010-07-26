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

#include "projectConfig.h"
#if USE_PHYSFS
#	include "ifile_stream.hpp"
#	include "ofile_stream.hpp"

#	include "PhysFileOps.hpp"
#endif
#include "StdFileOps.hpp"
#include "util.h"

namespace Shared { namespace PhysFS {
using namespace Util;

using std::ios;
using std::istream;
using std::ifstream;
using std::ostream;
using std::ofstream;

FSFactory *FSFactory::instance = NULL;

FSFactory::FSFactory(){
	this->usePhysFS = false;
}

FSFactory::~FSFactory(){
#if USE_PHYSFS
	if(PHYSFS_isInit()){
		PHYSFS_deinit();
	}
#endif
	instance = NULL;
}

FSFactory *FSFactory::getInstance(){
	if(!instance){
		instance = new FSFactory();
		atexit(&FSFactory::shutdown);
	}
	return instance;
}

void FSFactory::shutdown() {
	delete instance;
}

bool FSFactory::initPhysFS(const char *argv0, const char *configDir, const char *dataDir){
#if USE_PHYSFS
	if (!PHYSFS_init(argv0)) {
		return false;
	}
	PHYSFS_permitSymbolicLinks(1);
	
	PHYSFS_setWriteDir(configDir);
	if(!PHYSFS_mount(PHYSFS_getWriteDir(), NULL, 1)){
		throw runtime_error(string("Couldn't mount configDir: ") + configDir);
	}
	if(!PHYSFS_mount(dataDir, NULL, 1)){
		throw runtime_error(string("Couldn't mount dataDir: ") + dataDir);
	}
	// check for addons
	char **list = PHYSFS_enumerateFiles("addons");
	for(char **i=list; *i; i++){
		// because we use physfs functions, we need path in physfs
#		ifdef WIN32 // needs leading '/' on windoze
			string str("/addons/");
#		else
			string str("addons/");
#		endif
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
	list = PHYSFS_getSearchPath();
	for(char **i=list; *i; i++){
		std::cout << "[" << *i << "] is in the search path.\n";
	}
	PHYSFS_freeList(list);
#endif
	return true;
}

bool FSFactory::mountSystemDir(const string &systemPath, const string &mapToPath) {
#if USE_PHYSFS
	if (!PHYSFS_mount(systemPath.c_str(), mapToPath.c_str(), 1)) {
		return false;
	}
	return true;
#endif
	return false;
}


istream *FSFactory::getIStream(const char *fname){
#if USE_PHYSFS
	if(this->usePhysFS){
		string str(fname);
		str = cleanPath(str);  // get rid of .. and .
		return new IFileStream(str);
	}else
#endif
		return new ifstream(fname, ios::in | ios::binary);
}

ostream *FSFactory::getOStream(const char *fname){
#if USE_PHYSFS
	if(this->usePhysFS){
		string str(fname);
		str = cleanPath(str);  // get rid of ../ and ./
		return new OFileStream(str);
	}else
#endif
		return new ofstream(fname, ios::out | ios::binary);
}

FileOps *FSFactory::getFileOps(){
#if USE_PHYSFS
	if(this->usePhysFS){
		return new PhysFileOps();
	}else
#endif
		return new StdFileOps();
}

// FIXME: quick & dirty
vector<string> FSFactory::findAll(const string &path, bool cutExtension){
	vector<string> res;
	
#if USE_PHYSFS
	// FIXME: currently assumes there's always a dir before wildcard
	const string dir = dirname(path);
	const string basestr = basename(path);
	const char *base = basestr.c_str();  // otherwise something is not correctly allocated
	char **list = PHYSFS_enumerateFiles(dir.c_str());
	const char *b, *f;
	for(char **i=list; *i; i++){
		if(**i=='.'){  //skip files/folders with leading .
			continue;
		}
		b = base + strlen(base) -1;  // start at end
		f = *i + strlen(*i) -1;
		while(*b==*f && (*b!='*' || *b!='.')){  // base=="*." -> wants all
			b--;
			f--;
		}
		if(*b=='*' || *b=='.'){
			if(cutExtension){
				string str(*i);
				str = cutLastExt(str);
				res.push_back(str);
			}else{
				res.push_back(*i);
			}
		}
	}
	PHYSFS_freeList(list);
#endif
	return res;
}

bool FSFactory::fileExists(const string &path){
#if USE_PHYSFS
	return PHYSFS_exists(path.c_str());
#else
	return false;
#endif
}

bool FSFactory::dirExists(const string &path) {
#if USE_PHYSFS
	return PHYSFS_isDirectory(path.c_str());
#else
	return false;
#endif
}

bool FSFactory::removeFile(const string &path) {
#if USE_PHYSFS
	return PHYSFS_delete(path.c_str());
#else
	return remove(path.c_str());
#endif
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


//freetype
unsigned long FSFactory::stream_load(FT_Stream stream, unsigned long offset, unsigned char *buffer, unsigned long count) {
	FileOps *fd = (FileOps*)stream->descriptor.pointer;
	fd->seek(offset, SEEK_SET);
	return fd->read(buffer, 1, count);
}

void FSFactory::stream_close(FT_Stream stream) {
	FileOps *fd = (FileOps*)stream->descriptor.pointer;
	fd->close();
	delete fd;
}

int FSFactory::openFace(FT_Library lib, const char *fname, FT_Long indx, FT_Face *face){
	FileOps *fops = FSFactory::instance->getFileOps();
	fops->openRead(fname);
	
	FT_StreamDesc desc;
	desc.pointer = fops;
	
	FT_StreamRec *stream = new FT_StreamRec;
	memset(stream, 0, sizeof(FT_StreamRec));
	stream->base = 0;
	stream->size = fops->fileSize();
	stream->pos = 0;
	stream->descriptor = desc;
	stream->read = FSFactory::stream_load;
	stream->close = FSFactory::stream_close;
	
	FT_Open_Args args;// = new FT_Open_Args;
	memset(&args, 0, sizeof(FT_Open_Args));
	args.flags = FT_OPEN_STREAM;
	args.stream = stream;

	return FT_Open_Face(lib, &args, indx, face);
}

void FSFactory::doneFace(FT_Face face){
	FT_StreamRec *streamrec = face->stream;
	FT_Done_Face(face);
	delete streamrec;
}

}} // namespace Shared::PhysFS

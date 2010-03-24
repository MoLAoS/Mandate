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

#ifndef _FSFACTORY_HPP_
#define _FSFACTORY_HPP_

//#include <istream>
#include <fstream>
#include <string>
#include <vector>

//FIXME: Ogg callbacks
#include <vorbis/vorbisfile.h>

#include "FileOps.hpp"

using namespace std;


class FSFactory{
	private:
		static FSFactory *instance;
		
		
		FSFactory();

	public:
		// singleton
		static FSFactory *getInstance();
		~FSFactory();

		bool physFS;
		
		void initPhysFS(const char *argv0, const char *configDir, const char *dataDir);
		void deinitPhysFS();
		void usePhysFS(bool enable);
		istream *getIStream(const char *fname);
		ostream *getOStream(const char *fname);
		
		FileOps *getFileOps();

		static vector<string> findAll(const string &path, bool cutExtension);
		static bool fileExists(const string &path);
		
		//Ogg callbacks, FIXME: move to better location
		static size_t cb_read(void *ptr, size_t size, size_t nmemb, void *source);
		static int cb_seek(void *source, ogg_int64_t offset, int whence);
		static int cb_close(void *source);
		static long cb_tell(void *source);
};

#endif

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

#include <fstream>
#include <string>
#include <vector>

// Ogg, for callbacks
#include <vorbis/vorbisfile.h>

// freetype, for callbacks
#include <ft2build.h>
#include FT_FREETYPE_H

#include "FileOps.hpp"
#include "leak_dumper.h"

namespace Shared { namespace PhysFS { 

using std::vector;
using std::ostream;
using std::istream;
using std::string;

class FSFactory{
	private:
		static FSFactory *instance;
		string m_configDir, m_dataDir;
		
		FSFactory();
		static void shutdown();

	public:
		// singleton
		static FSFactory *getInstance();
		~FSFactory();

		bool usePhysFS;

		bool initPhysFS(const char *argv0, const string &i_configDir, string &io_dataDir);
		bool mountSystemDir(const string &systemPath, const string &mapToPath);

		istream *getIStream(const char *fname);
		ostream *getOStream(const char *fname);
		
		FileOps *getFileOps();

		string getConfigDir() { return m_configDir; }
		string getDataDir() { return m_dataDir; }

		static vector<string> findAll(const string &path, bool cutExtension);
		static bool fileExists(const string &path);
		static bool removeFile(const string &path);
		static bool dirExists(const string &path);

		//Ogg callbacks
		static size_t cb_read(void *ptr, size_t size, size_t nmemb, void *source);
		static int cb_seek(void *source, ogg_int64_t offset, int whence);
		static int cb_close(void *source);
		static long cb_tell(void *source);

		//freetype stuff
		static unsigned long stream_load(FT_Stream stream, unsigned long offset, unsigned char *buffer, unsigned long count);
		static void stream_close(FT_Stream stream);
		static int openFace(FT_Library lib, const char *fname, FT_Long indx, FT_Face *face);
		static void doneFace(FT_Face face);
};

#define g_fileFactory (*Shared::PhysFS::FSFactory::getInstance())

}}

#endif

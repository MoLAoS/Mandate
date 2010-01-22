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

#ifndef _PHYSFILEOPS_HPP_
#define _PHYSFILEOPS_HPP_

#include "FileOps.hpp"

#include <physfs.h>


class PhysFileOps : public FileOps{
private:
	PHYSFS_File *f;

public:
	PhysFileOps();
	~PhysFileOps();
	
	void openRead(const char *fname);
	void openWrite(const char *fname);
	void openAppend(const char *fname);
	int read(void *buf, int size, int num);
	int write(void *buf, int size, int num);
	int seek(long offset, int w);
	int tell();
	int close();
	int eof();
};

#endif

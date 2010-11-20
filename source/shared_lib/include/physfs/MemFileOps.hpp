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

#ifndef _MEMFILEOPS_HPP_
#define _MEMFILEOPS_HPP_

#include "FileOps.hpp"

namespace Shared { namespace PhysFS {

class MemFileOps : public FileOps{
private:
	const unsigned char *data;
	int length; 
	int pos;

public:
	MemFileOps();
	~MemFileOps();
	
	void openRead(const char *fname);
	void openWrite(const char *fname);
	void openAppend(const char *fname);
	int read(void *buf, int size, int num);
	int write(const void *buf, int size, int num);
	int seek(long offset, int w);
	int tell();
	int close();
	int eof();

	// not in FileOps
	void openFromArray(const unsigned char *data, int length);
};

}} // namespace Shared::PhysFS



#endif

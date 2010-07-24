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

#ifndef _FILEOPS_HPP_
#define _FILEOPS_HPP_

namespace Shared { namespace PhysFS { 

class FileOps{
public:
	FileOps();
	virtual ~FileOps();

	virtual void openRead(const char *fname)=0;
	virtual void openWrite(const char *fname)=0;
	virtual void openAppend(const char *fname)=0;
	virtual int read(void *buf, int size, int num)=0;
	virtual int write(const void *buf, int size, int num)=0;
	virtual int seek(long offset, int w)=0;
	virtual int tell()=0;
	virtual int close()=0;
	virtual int eof()=0;
	virtual const char * getLastError() { return 0; }
	
	void rewind();

	// these were added to make sanity checks easier, and are not really
	// intended to be called outside of an assert()
	int fileSize();
	int bytesRemaining();
};

}} // namespace Shared::PhysFS

#endif

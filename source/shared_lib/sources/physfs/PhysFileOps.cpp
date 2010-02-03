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

#include "PhysFileOps.hpp"

#include <stdexcept>

#include "util.h"

using namespace std;
using Shared::Util::cleanPath;


PhysFileOps::PhysFileOps(){
	this->f = NULL;
}

PhysFileOps::~PhysFileOps(){
	if(this->f){
		this->close();
	}
}

void PhysFileOps::openRead(const char *fname){
	string str(fname);
	str = cleanPath(str);  // get rid of ../ and ./
	if(!(this->f = PHYSFS_openRead(str.c_str()))){
		throw runtime_error("PHYSFS_openRead failed: " + str);
	}
}

void PhysFileOps::openWrite(const char *fname){
	string str(fname);
	str = cleanPath(str);
	if(!(this->f = PHYSFS_openWrite(str.c_str()))){
		throw runtime_error("PHYSFS_openWrite failed: " + str);
	}
}

void PhysFileOps::openAppend(const char *fname){
	string str(fname);
	str = cleanPath(str);
	if(!(this->f = PHYSFS_openAppend(str.c_str()))){
		throw runtime_error("PHYSFS_openAppend failed: " + str);
	}
}

int PhysFileOps::read(void *buf, int size, int num){
	return PHYSFS_read(this->f, buf, size, num);
}

int PhysFileOps::write(void *buf, int size, int num){
	return PHYSFS_write(this->f, buf, size, num);
}

int PhysFileOps::seek(long offset, int w){
	switch(w){
		case SEEK_SET:
			if(!PHYSFS_seek(this->f, offset)){
				return -1;
			}
		break;
		case SEEK_CUR:
			offset += PHYSFS_tell(this->f);
			if(!PHYSFS_seek(this->f, offset)){
				return -1;
			}
		break;
		case SEEK_END:
			offset += PHYSFS_fileLength(this->f);
			if(!PHYSFS_seek(this->f, offset)){
				return -1;
			}
		break;
		default:
			throw runtime_error("unknown seek_whence" + w);
	}
	return 0;
}


int PhysFileOps::tell(){
	return PHYSFS_tell(this->f);
}

int PhysFileOps::close(){
	int res = PHYSFS_close(this->f);
	this->f = NULL;
	return res;
}

int PhysFileOps::eof(){
	return PHYSFS_eof(this->f);
}

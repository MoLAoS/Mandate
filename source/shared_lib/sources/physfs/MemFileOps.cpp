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

#include "MemFileOps.hpp"

#include <stdexcept>
#include "leak_dumper.h"
#include <string.h>

namespace Shared { namespace PhysFS {

using std::runtime_error;

MemFileOps::MemFileOps(){
	this->data = NULL;
	this->pos = 0;
}

MemFileOps::~MemFileOps(){
	if(this->data){
		this->close();
	}
}


// could be changed to read the whole file into memory and operate on it in memory until closed
// no difference between the different open* functions
// not needed now
void MemFileOps::openRead(const char *fname){
	throw runtime_error("not supported by MemFileOps");
}

void MemFileOps::openWrite(const char *fname){
	throw runtime_error("not supported by MemFileOps");
}

void MemFileOps::openAppend(const char *fname){
	throw runtime_error("not supported by MemFileOps");
}

int MemFileOps::read(void *buf, int size, int num){
	int n=size*num;
	if(this->pos+n >= length){
		n = this->length - this->pos;
	}
	memcpy(buf, this->data+this->pos, n);
	this->pos += n;
	return n;
}

int MemFileOps::write(const void *buf, int size, int num){
	throw runtime_error("not supported by MemFileOps");
}

int MemFileOps::seek(long offset, int w){
	switch(w){
		case SEEK_SET:
			this->pos = offset;
		break;
		case SEEK_CUR:
			this->pos += offset;
		break;
		case SEEK_END:
			this->pos = this->length-1 + offset;
		break;
		default:
			throw runtime_error("unknown seek_whence" + w);
	}
	return 0;
}

int MemFileOps::tell(){
	return this->pos;
}

int MemFileOps::close(){
	this->data = NULL;
	this->pos = 0;
	return 0;
}

int MemFileOps::eof(){
	return this->pos == this->length-1;
}

void MemFileOps::openFromArray(const unsigned char *data, int length){
	this->data = data;
	this->length = length;
}

}} // namespace Shared::PhysFS

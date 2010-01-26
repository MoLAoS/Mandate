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

#include "StdFileOps.hpp"

#include <stdexcept>


StdFileOps::StdFileOps(){
	this->f = NULL;
}

StdFileOps::~StdFileOps(){
	if(this->f){
		this->close();
	}
}

void StdFileOps::openRead(const char *fname){
	this->f = fopen(fname, "r");
}

void StdFileOps::openWrite(const char *fname){
	this->f = fopen(fname, "w");
}

void StdFileOps::openAppend(const char *fname){
	this->f = fopen(fname, "a");
}

int StdFileOps::read(void *buf, int size, int num){
	return fread(buf, size, num, this->f);
}

int StdFileOps::write(void *buf, int size, int num){
	return fwrite(buf, size, num, this->f);
}

int StdFileOps::seek(long offset, int w){
	return fseek(this->f, offset, w);
}

int StdFileOps::tell(){
	return ftell(this->f);
}

int StdFileOps::close(){
	int res = fclose(this->f);
	this->f = NULL;
	return res;
}

int StdFileOps::eof(){
	return feof(this->f);
}

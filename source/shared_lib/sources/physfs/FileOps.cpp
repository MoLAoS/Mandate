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

#include "FileOps.hpp"

#include <cstdio>


FileOps::FileOps(){
}

FileOps::~FileOps(){
}

void FileOps::rewind(){
	(void)this->seek(0L, SEEK_SET);
}

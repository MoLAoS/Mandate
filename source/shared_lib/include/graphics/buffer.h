// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_BUFFER_H_
#define _SHARED_GRAPHICS_BUFFER_H_

#include <string>

using std::string;

namespace Shared{ namespace Graphics{

// =====================================================
//	class VertexBuffer
// =====================================================

class VertexBuffer{
private:
	static const int texCoordCount = 8;
	static const int attribCount = 8;

private:
	void *positionPointer;
	void *normalPointer;

	void *texCoordPointers[texCoordCount];
	int texCoordCoordCounts[texCoordCount];

	void *attribPointers[attribCount];
	int attribCoordCounts[attribCount];
	string attribNames[attribCount];

public:
	VertexBuffer();
	virtual ~VertexBuffer(){};

	virtual void init(int size)= 0;

	void setPositionPointer(void *pointer);
	void setNormalPointer(void *pointer);
	void setTexCoordPointer(void *pointer, int texCoordIndex, int coordCount);
	void setAttribPointer(void *pointer, int attribIndex, int coordCount, const string &name);
};

// =====================================================
//	class IndexBuffer
// =====================================================

class IndexBuffer{
private:
	void *indexPointer;

public:
	IndexBuffer();
	virtual ~IndexBuffer(){}

	virtual void init(int size)= 0;

	void setIndexPointer(void *pointer);
};

}}//end namespace

#endif

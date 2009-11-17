// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_CONTEXT_H_
#define _SHARED_GRAPHICS_CONTEXT_H_ 

#include "types.h"
#include "util.h"

namespace Shared{ namespace Graphics{

using Platform::uint32;

// =====================================================
//	class Context
// =====================================================

class Context{
protected:
	uint32 colorBits;
	uint32 depthBits;
	uint32 stencilBits;

public:
	Context();
	virtual ~Context(){}

	uint32 getColorBits() const		{return colorBits;}
	uint32 getDepthBits() const		{return depthBits;}
	uint32 getStencilBits() const	{return stencilBits;}

	void setColorBits(uint32 colorBits)		{this->colorBits= colorBits;}
	void setDepthBits(uint32 depthBits)		{this->depthBits= depthBits;}
	void setStencilBits(uint32 stencilBits)	{this->stencilBits= stencilBits;}

	virtual void init() PURE_VIRTUAL;
	virtual void end() PURE_VIRTUAL;
	virtual void reset() PURE_VIRTUAL;

	virtual void makeCurrent() PURE_VIRTUAL;
	virtual void swapBuffers() PURE_VIRTUAL;
};

}}//end namespace

#endif

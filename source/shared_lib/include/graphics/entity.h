// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos (daniel.santos@pobox.com)
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================


#ifndef _SHARED_GRAPHICS_ENTITY_H_
#define _SHARED_GRAPHICS_ENTITY_H_

#include "vec.h"

namespace Shared{ namespace Graphics{

class Entity {
public:
	virtual ~Entity(){}
//	virtual int getSize() const = 0;
	virtual Vec2i getPos() const = 0;
	virtual Vec3f getCurrVector() const = 0;
	virtual Vec3f getCurrVectorFlat() const = 0;
};

}}//end namespace

#endif

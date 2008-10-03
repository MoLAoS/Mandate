// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================


#ifndef _SHARED_PHYSICS_PHYSOBJECT_H_
#define _SHARED_PHYSICS_PHYSOBJECT_H_

#include "vec.h"

using namespace Shared::Graphics;

namespace Shared { namespace Physics {


// =====================================================
//	class Object
// =====================================================

class Object {
protected:
	const ObjectType &type;
	Vec3f vertex;
	Vec3f velocity;
	Vec3f angVelocity;	/** Angular Velocity */

public:
	Object(const ObjectType &type, Vec3f vertex, Vec3f velocity, Vec3f angVelocity);
	const ObjectType &getType() const	{return type;}
	const Vec3f &getVelocity() const	{return velocity;}
	const Vec3f &getAngVelocity() const	{return angVelocity;}
	const Vec3f &getVertex() const		{return vertex;}
	
};

}} //end namespace

#endif // _SHARED_PHYSICS_PHYSOBJECT_H_


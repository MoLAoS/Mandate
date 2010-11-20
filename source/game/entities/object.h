// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _GLEST_GAME_OBJECT_H_
#define _GLEST_GAME_OBJECT_H_

#include "model.h"
#include "vec.h"
#include "forward_decs.h"

using Shared::Graphics::Model;
using Shared::Math::Vec2i;
using Shared::Math::Vec3f;

using Glest::ProtoTypes::ObjectType;
using Glest::ProtoTypes::ResourceType;

namespace Glest { namespace Entities {

class Resource;
class ObjectFactory;

// =====================================================
// 	class Object
//
///	A map object: tree, stone...
// =====================================================

class Object {
	friend class ObjectFactory;
private:
	int id;
	ObjectType *objectType;
	Resource *resource;
	Vec3f pos;
	float rotation;
	int variation;

	Object(int id, ObjectType *objectType, const Vec3f &pos);

public:
	~Object();

//	void setHeight(float height)		{pos.y= height;}
	void setPos(const Vec3f &pos)		{this->pos = pos;}

	int getId() const 					{return id;}
	const ObjectType *getType() const	{return objectType;}
	Resource *getResource() const		{return resource;}
	Vec3f getPos() const				{return pos;}
	float getRotation()	const			{return rotation;}
	const Model *getModel() const;
	bool getWalkable() const;

	void setResource(const ResourceType *resourceType, const Vec2i &pos);
};

class ObjectFactory {
private:
	int idCounter;
	std::map<int, Object*> objects;

public:
	ObjectFactory() : idCounter(0) {}

	Object* newInstance(ObjectType *objectType, const Vec3f &pos) {
		Object *obj = new Object(idCounter, objectType, pos);
		objects[idCounter] = obj;
		++idCounter;
		return obj;
	}

	Object* getObject(int id) {
		assert(id >= 0 && id < idCounter);
		return objects[id];
	}
};

}}//end namespace

#endif

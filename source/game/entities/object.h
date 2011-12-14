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

namespace Glest { namespace Entities {

class MapResource;

using Shared::Graphics::Model;
using Shared::Math::Vec2i;
using Shared::Math::Vec3f;

using ProtoTypes::MapObjectType;
using ProtoTypes::ResourceType;
using Sim::EntityFactory;

// =====================================================
// 	class MapObject
//
///	A map object: tree, stone...
// =====================================================

class MapObject {
	friend class EntityFactory<MapObject>;

private:
	int id;
	MapObjectType *objectType;
	MapResource *resource;
	Vec2i tilePos;
	Vec3f pos;
	float rotation;
	int variation;


public:
	struct CreateParams {
		MapObjectType *objectType;
		Vec2i tilePos;
		const Vec3f pos;
		CreateParams(MapObjectType *objectType, const Vec2i &tilePos, const Vec3f &worldPos) 
			: objectType(objectType), tilePos(tilePos), pos(worldPos) {}
	};

private:
	MapObject(CreateParams params);
	~MapObject();

	void setId(int v) { id = v; }

public:
	void setPos(const Vec3f &pos)		{this->pos = pos;}

	int getId() const 					{return id;}
	const MapObjectType *getType() const{return objectType;}
	MapResource *getResource() const	{return resource;}
	Vec2i getTilePos() const            { return tilePos; }
	Vec3f getPos() const				{return pos;}
	float getRotation()	const			{return rotation;}
	const Model *getModel() const;
	bool getWalkable() const;

	void setResource(const ResourceType *resourceType, const Vec2i &pos);
};

typedef vector<MapObject*>          MapObjectVector;
typedef vector<const MapObject*>    ConstMapObjVector;

}}//end namespace

#endif

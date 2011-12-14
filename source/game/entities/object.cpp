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

#include "pch.h"
#include "object.h"

#include "faction_type.h"
#include "tech_tree.h"
#include "resource.h"
#include "upgrade.h"
#include "object_type.h"
#include "resource.h"
#include "util.h"
#include "random.h"
#include "user_interface.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Entities {

// =====================================================
// 	class MapObject
// =====================================================

MapObject::MapObject(CreateParams params)//MapObjectType *objType, const Vec3f &p)
		: id(-1)
		, objectType(params.objectType)
		, resource(0) {
	int seed = int(Chrono::getCurMicros());
	Random random(seed);
	const float max_offset = 0.2f;
	tilePos = params.tilePos;
	pos = params.pos + Vec3f(random.randRange(-max_offset, max_offset), 0.0f, random.randRange(-max_offset, max_offset));
	rotation = random.randRange(0.f, 360.f);
	if (objectType != NULL) {
		variation = random.randRange(0, objectType->getModelCount() - 1);
	}
}

MapObject::~MapObject(){
	delete resource;
}

const Model *MapObject::getModel() const{
	return objectType==NULL? resource->getType()->getModel(): objectType->getModel(variation);
}

bool MapObject::getWalkable() const{
	return objectType==NULL? false: objectType->getWalkable();
}

void MapObject::setResource(const ResourceType *resourceType, const Vec2i &pos){
    assert(!resource);
	resource= new MapResource();
	resource->init(resourceType, pos);
	resource->Depleted.connect(&g_userInterface, &Gui::UserInterface::onResourceDepleted);
}

}}//end namespace

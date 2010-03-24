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

#include "leak_dumper.h"


using namespace Shared::Util;

namespace Glest{ namespace Game{


// =====================================================
// 	class Object
// =====================================================

Object::Object(ObjectType *objType, const Vec3f &p){
	objectType = objType;
	resource = NULL;
	Random random(int(p.x * p.z));
	pos = p + Vec3f(random.randRange(-0.6f, 0.6f), 0.0f, random.randRange(-0.6f, 0.6f));
	rotation = random.randRange(0.f, 360.f);
	if (objectType != NULL) {
		variation = random.randRange(0, objectType->getModelCount() - 1);
	}
}

Object::~Object(){
	delete resource;
}

const Model *Object::getModel() const{
	return objectType==NULL? resource->getType()->getModel(): objectType->getModel(variation);
}

bool Object::getWalkable() const{
	return objectType==NULL? false: objectType->getWalkable();
}

void Object::setResource(const ResourceType *resourceType, const Vec2i &pos){
	resource= new Resource();
	resource->init(resourceType, pos);
}

}}//end namespace

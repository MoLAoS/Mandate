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
#include "object_type.h"

#include "renderer.h"

#include "world.h" // to get ModelFactory

#include "leak_dumper.h"

using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class MapObjectType
// =====================================================

void MapObjectType::init(int modelCount, int objectClass, bool walkable, string foundation, string name){
	models.reserve(modelCount);
	this->objectClass = objectClass;
	this->walkable = walkable;
	this->foundation = foundation;
	this->name = name;
}

void MapObjectType::loadModel(const string &path){
	Model *model = g_world.getModelFactory().getModel(path, GameConstants::cellScale, 2);
	color = Vec3f(0.f);
	if (model->getMeshCount() > 0 && model->getMesh(0)->getTexture(0) != NULL) {
		const Texture2D *tex = model->getMesh(0)->getTexture(0);
		Pixmap2D *p = new Pixmap2D();
		p->load(tex->getPath());
		color = p->getPixel3f(p->getW()/2, p->getH()/2);
		delete p;
	}
	models.push_back(model);
}

}}//end namespace

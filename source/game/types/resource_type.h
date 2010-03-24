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

#ifndef _GLEST_GAME_RESOURCETYPE_H_
#define _GLEST_GAME_RESOURCETYPE_H_

#include "element_type.h"
#include "model.h"
#include "checksum.h"
#include "game_constants.h"

namespace Glest{ namespace Game{

using Shared::Graphics::Model;
using Shared::Util::Checksum;

// =====================================================
// 	class ResourceType
//
///	A type of resource that can be harvested or not
// =====================================================

class ResourceType: public DisplayableType {
private:
	ResourceClass resourceClass;
	int tilesetObject;	// used only if class == ResourceClass::TILESET
	int resourceNumber;	// used only if class == ResourceClass::TECHTREE, resource number in the map
	int interval;		// used only if class == ResourceClass::CONSUMABLE
	int defResPerPatch;	// used only if class == ResourceClass::TILESET || class == ResourceClass::TECHTREE
	bool recoupCost;	// used only if class == ResourceClass::STATIC

	Model *model;
	/**
	 * Rather or not to display this resource at the top of the screen (defaults to true).
	 */
	bool display;

public:
	bool load(const string &dir, int id);
	virtual void doChecksum(Checksum &checksum) const;

	//get
	ResourceClass getClass() const	{return resourceClass;}
	int getTilesetObject() const	{return tilesetObject;}
	int getResourceNumber() const	{return resourceNumber;}
	int getInterval() const			{return interval;}
	int getDefResPerPatch() const	{return defResPerPatch;}
	bool getRecoupCost() const		{ return recoupCost; }
	const Model *getModel() const	{return model;}
	bool isDisplay() const			{return display;}

	static ResourceClass strToRc(const string &s);
};

}} //end namespace

#endif

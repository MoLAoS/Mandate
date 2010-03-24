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
#include "resource_type.h"

#include "util.h"
#include "element_type.h"
#include "logger.h"
#include "renderer.h"
#include "xml_parser.h"

#include "leak_dumper.h"


using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest { namespace Game {

// =====================================================
//  class ResourceType
// =====================================================

bool ResourceType::load(const string &dir, int id) {
	string path, str;
	Renderer &renderer = Renderer::getInstance();
	this->id = id;

	bool loadOk = true;
	Logger::getInstance().add("Resource type: " + dir, true);
	name = basename(dir);
	path = dir + "/" + name + ".xml";

	XmlTree xmlTree;
	const XmlNode *resourceNode;
	try { // tree
		xmlTree.load(path); 
		resourceNode = xmlTree.getRootNode();
		if (!resourceNode) {
			Logger::getErrorLog().addXmlError(path, "XML file appears to lack contents.");
			return false; // bail
		}
	}
	catch (runtime_error &e) {
		Logger::getErrorLog().addXmlError(path, "Missing or wrong name of XML file.");
		return false; // bail
	}
	try { // image
		const XmlNode *imageNode;
		imageNode = resourceNode->getChild("image");
		image = renderer.newTexture2D(rsGame);
		image->load( dir + "/" + imageNode->getAttribute("path")->getRestrictedValue() );
	}
	catch (runtime_error &e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		loadOk = false; // can continue, to catch other errors
	}
	const XmlNode *typeNode;
	try { // type
		typeNode = resourceNode->getChild("type");
		resourceClass = strToRc(typeNode->getAttribute("value")->getRestrictedValue());
	}
	catch (runtime_error &e) {
		Logger::getErrorLog().addXmlError(path, e.what());
		return false; // bail, can't continue without type
	}

	switch (resourceClass) {
		case ResourceClass::TECHTREE: 
			try { // model
				const XmlNode *modelNode = typeNode->getChild("model");
				string mPath = dir + "/" + modelNode->getAttribute("path")->getRestrictedValue();
				model = renderer.newModel(rsGame);
				model->load(mPath);
			}
			catch (runtime_error e) {
				Logger::getErrorLog().addXmlError(path, e.what());
				loadOk = false; // can continue, to catch other errors
			}
			try { // default resources
				const XmlNode *defaultAmountNode = typeNode->getChild("default-amount");
				defResPerPatch = defaultAmountNode->getAttribute("value")->getIntValue();
			}
			catch (runtime_error e) {
				Logger::getErrorLog().addXmlError(path, e.what());
				loadOk = false; // can continue, to catch other errors
			}
			try { // resource number
				const XmlNode *resourceNumberNode = typeNode->getChild("resource-number");
				resourceNumber = resourceNumberNode->getAttribute("value")->getIntValue();
			}
			catch (runtime_error e) {
				Logger::getErrorLog().addXmlError(path, e.what());
				loadOk = false;
			}
			break;
		case ResourceClass::TILESET: 
			try { // default resources
				const XmlNode *defaultAmountNode = typeNode->getChild("default-amount");
				defResPerPatch = defaultAmountNode->getAttribute("value")->getIntValue();
			}
			catch (runtime_error e) {
				Logger::getErrorLog().addXmlError(path, e.what());
				loadOk = false; // can continue, to catch other errors
			}
			try { // object number
				const XmlNode *tilesetObjectNode = typeNode->getChild("tileset-object");
				tilesetObject = tilesetObjectNode->getAttribute("value")->getIntValue();
			}
			catch (runtime_error e) {
				Logger::getErrorLog().addXmlError(path, e.what());
				loadOk = false;
			}
			break;
		case ResourceClass::CONSUMABLE: 
			try { // interval
				const XmlNode *intervalNode = typeNode->getChild("interval");
				interval = intervalNode->getAttribute("value")->getIntValue();
			}
			catch (runtime_error e) {
				Logger::getErrorLog().addXmlError(path, e.what());
				loadOk = false;
			}
			break;
		case ResourceClass::STATIC:
			try {
				const XmlNode *recoupCostNode= typeNode->getChild("recoup_cost", 0, false);
				if (recoupCostNode) {
					recoupCost = recoupCostNode->getAttribute("value")->getBoolValue();
				} else {
					recoupCost = true;
				}
			} catch (runtime_error e) {
				Logger::getErrorLog().addXmlError(path, e.what());
				loadOk = false;
			}
			break;
		default:
			break;
	}
	display = resourceNode->getOptionalBoolValue("display", true);
	return loadOk;
}

void ResourceType::doChecksum(Checksum &checksum) const {
	NameIdPair::doChecksum(checksum);
	checksum.add<ResourceClass>(resourceClass);
	if (resourceClass == ResourceClass::CONSUMABLE) {
		checksum.add<int>(interval);
	} else if (resourceClass != ResourceClass::STATIC) {
		if (resourceClass == ResourceClass::TILESET) {
			checksum.add<int>(tilesetObject);
		} else {
			assert(resourceClass == ResourceClass::TECHTREE);
			checksum.add<int>(resourceNumber);
		}
		checksum.add<int>(defResPerPatch);
	}
	checksum.add<bool>(display);
}

// ==================== misc ====================

ResourceClass ResourceType::strToRc(const string &s) {
	if (s == "tech") {
		return ResourceClass::TECHTREE;
	}
	if (s == "tileset") {
		return ResourceClass::TILESET;
	}
	if (s == "static") {
		return ResourceClass::STATIC;
	}
	if (s == "consumable") {
		return ResourceClass::CONSUMABLE;
	}
	throw runtime_error("Error converting from string ro resourceClass, found: " + s);
}

}}//end namespace

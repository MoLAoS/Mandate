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

using Glest::Util::Logger;
using namespace Shared::Util;
using namespace Shared::Xml;
using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes {

// =====================================================
//  class ResourceType
// =====================================================

bool ResourceType::load(const string &dir, int id) {
	string path, str;
	Renderer &renderer = Renderer::getInstance();
	m_id = id;

	bool loadOk = true;
	g_logger.logProgramEvent("Resource type: " + dir, true);
	m_name = basename(dir);
	path = dir + "/" + m_name + ".xml";

	XmlTree xmlTree;
	const XmlNode *resourceNode;
	try { // tree
		xmlTree.load(path);
		resourceNode = xmlTree.getRootNode();
		if (!resourceNode) {
			g_logger.logXmlError(path, "XML file appears to lack contents.");
			return false; // bail
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError(path, "Missing or wrong name of XML file.");
		g_logger.logError("Fatal Error: could not load " + path);
		return false; // bail
	}
	try { // image
		const XmlNode *imageNode;
		imageNode = resourceNode->getChild("image");
		string imgPath = dir + "/" + imageNode->getAttribute("path")->getRestrictedValue();
		image = renderer.getTexture2D(ResourceScope::GAME, imgPath);
	} catch (runtime_error &e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false; // can continue, to catch other errors
	}
	const XmlNode *typeNode;
	try { // type
		typeNode = resourceNode->getChild("type");
		resourceClass = strToRc(typeNode->getAttribute("value")->getRestrictedValue());
	} catch (runtime_error &e) {
		g_logger.logXmlError(path, e.what());
		return false; // bail, can't continue without type
	}

	switch (resourceClass) {
		case ResourceClass::TECHTREE:
            hasModel = typeNode->getAttribute("has-model")->getBoolValue();
            if (hasModel) {
                try { // model
                        const XmlNode *modelNode = typeNode->getChild("model");
                        string mPath = dir + "/" + modelNode->getAttribute("path")->getRestrictedValue();
                        model = renderer.newModel(ResourceScope::GAME);
                        model->load(mPath, GameConstants::cellScale, 2);
                } catch (runtime_error e) {
                    g_logger.logXmlError(path, e.what());
                }
                try { // default resources
                    const XmlNode *defaultAmountNode = typeNode->getChild("default-amount");
                    defResPerPatch = defaultAmountNode->getAttribute("value")->getIntValue();
                } catch (runtime_error e) {
                    g_logger.logXmlError(path, e.what());
                    loadOk = false; // can continue, to catch other errors
                }
                try { // resource number
                    const XmlNode *resourceNumberNode = typeNode->getChild("resource-number");
                    resourceNumber = resourceNumberNode->getAttribute("value")->getIntValue();
                } catch (runtime_error e) {
                    g_logger.logXmlError(path, e.what());
                    loadOk = false;
                }
            }
			break;
		case ResourceClass::TILESET:
			try { // default resources
				const XmlNode *defaultAmountNode = typeNode->getChild("default-amount");
				defResPerPatch = defaultAmountNode->getAttribute("value")->getIntValue();
			} catch (runtime_error e) {
				g_logger.logXmlError(path, e.what());
				loadOk = false; // can continue, to catch other errors
			}
			try { // object number
				const XmlNode *tilesetObjectNode = typeNode->getChild("tileset-object");
				tilesetObject = tilesetObjectNode->getAttribute("value")->getIntValue();
			} catch (runtime_error e) {
				g_logger.logXmlError(path, e.what());
				loadOk = false;
			}
			break;
		case ResourceClass::CONSUMABLE:
			try { // interval
				const XmlNode *intervalNode = typeNode->getChild("interval");
				interval = intervalNode->getAttribute("value")->getIntValue();
			} catch (runtime_error e) {
				g_logger.logXmlError(path, e.what());
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
				g_logger.logXmlError(path, e.what());
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

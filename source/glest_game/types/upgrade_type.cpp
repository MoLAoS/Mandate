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
#include "upgrade_type.h"

#include <algorithm>
#include <cassert>

#include "unit_type.h"
#include "util.h"
#include "logger.h"
#include "lang.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "resource.h"
#include "renderer.h"

#include "leak_dumper.h"


using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class UpgradeType
// =====================================================

// ==================== misc ====================

void UpgradeType::load(const string &dir, const TechTree *techTree, const FactionType *factionType, Checksum &checksum) {
	string path;

	Logger::getInstance().add("Upgrade type: "+ dir, true);

	path = dir + "/" + name + ".xml";

	try {
		checksum.addFile(path, true);

		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *upgradeNode= xmlTree.getRootNode();

		//image
		const XmlNode *imageNode= upgradeNode->getChild("image");
		image= Renderer::getInstance().newTexture2D(rsGame);
		image->load(dir+"/"+imageNode->getAttribute("path")->getRestrictedValue());

		//image cancel
		const XmlNode *imageCancelNode= upgradeNode->getChild("image-cancel");
		cancelImage= Renderer::getInstance().newTexture2D(rsGame);
		cancelImage->load(dir+"/"+imageCancelNode->getAttribute("path")->getRestrictedValue());

		//upgrade time
		productionTime= upgradeNode->getChildIntValue("time");

		//ProducibleType parameters
		ProducibleType::load(upgradeNode, dir, techTree, factionType);

		//effects
		const XmlNode *effectsNode= upgradeNode->getChild("effects", 0, false);
		if(effectsNode) {
			for(int i=0; i<effectsNode->getChildCount(); ++i){
				const XmlNode *unitNode= effectsNode->getChild("unit", i);
				string name= unitNode->getAttribute("name")->getRestrictedValue();
				effects.push_back(factionType->getUnitType(name));
			}
		}

		//values
		//maintain backward compatibility using legacy format
		maxHp = upgradeNode->getOptionalIntValue("max-hp");
		maxEp = upgradeNode->getOptionalIntValue("max-ep");
		sight = upgradeNode->getOptionalIntValue("sight");
		attackStrength = upgradeNode->getOptionalIntValue("attack-strength");
		attackRange = upgradeNode->getOptionalIntValue("attack-range");
		armor = upgradeNode->getOptionalIntValue("armor");
		moveSpeed = upgradeNode->getOptionalIntValue("move-speed");
		prodSpeed = upgradeNode->getOptionalIntValue("production-speed");

		//initialize values using new format if nodes are present
		if(upgradeNode->getChild("static-modifiers", 0, false)
				 || upgradeNode->getChild("multipliers", 0, false)){
			EnhancementTypeBase::load(upgradeNode, dir, techTree, factionType);
		}
	} catch (const exception &e) {
		throw runtime_error("Error loading UpgradeType: " + dir + "\n" + e.what());
	}
}

string UpgradeType::getDesc() const {
	Lang &lang = Lang::getInstance();
	string str = getReqDesc();

	if(getEffectCount() > 0) {
		str += "\n" + lang.get("Upgrades") + ":";

		for(int i = 0; i < getEffectCount(); ++i) {
			str += "\n" + getEffect(i)->getName();
		}
	}

	EnhancementTypeBase::getDesc(str, "\n");
	return str;
}

}}//end namespace

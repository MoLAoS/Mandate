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
#include "unit_type.h"

#include <cassert>

#include "util.h"
#include "upgrade_type.h"
#include "resource_type.h"
#include "sound.h"
#include "logger.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "renderer.h"
#include "world.h"

#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace ProtoTypes {

// ===============================
// 	class Level
// ===============================

bool Level::load(const XmlNode *levelNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = true;
	try { name = levelNode->getAttribute("name")->getRestrictedValue(); }
	catch (runtime_error e) {
		g_errorLog.addXmlError ( dir, e.what() );
		loadOk = false;
	}

	try { 
		kills = levelNode->getAttribute("kills")->getIntValue(); 
		const XmlAttribute *defaultsAtt = levelNode->getAttribute("defaults", 0);
		if(defaultsAtt && !defaultsAtt->getBoolValue()) {
			maxHpMult = 1;
			maxEpMult = 1;
			sightMult = 1;
			armorMult = 1;
			effectStrength = 0;
		}
	}
	catch (runtime_error e) {
		g_errorLog.addXmlError ( dir, e.what() );
		loadOk = false;
	}

	if ( ! EnhancementType::load(levelNode, dir, tt, ft) )
		loadOk = false;
	return loadOk;
}

// ===============================
// 	class PetRule
// ===============================
/*
void PetRule::load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft){
	string unitTypeName = prn->getAttribute("type")->getRestrictedValue();
	type = ft->getUnitType(unitTypeName);
	count = prn->getAttribute("count")->getIntValue();
}
*/

// =====================================================
// 	class UnitType
// =====================================================

// ===================== PUBLIC ========================


// ==================== creation and loading ====================

UnitType::UnitType()
		: multiBuild(false), multiSelect(false)
		, armourType(0)
		, size(0), height(0)
		, light(false), lightColour(0.f)
		, meetingPoint(false), meetingPointImage(0)
		, startSkill(0)
		, halfSize(0), halfHeight(0)
		, m_cellMap(0)
		, m_colourMap(0) {
	reset();
}

UnitType::~UnitType(){
	deleteValues(emanations.begin(), emanations.end()); ///@todo EffectTypeFactory
	deleteValues(selectionSounds.getSounds().begin(), selectionSounds.getSounds().end());
	deleteValues(commandSounds.getSounds().begin(), commandSounds.getSounds().end());
	delete m_cellMap;
	delete m_colourMap;
}

void UnitType::preLoad(const string &dir){
	name= basename(dir);
}

bool UnitType::load(const string &dir, const TechTree *techTree, const FactionType *factionType, bool glestimal) {
	string path;

	Logger::getInstance().add("Unit type: " + dir, true);
	bool loadOk = true;
	//file load
	path= dir+"/"+name+".xml";

	XmlTree xmlTree;
	try { xmlTree.load(path); }
	catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		return false; // bail
	}
	const XmlNode *unitNode;
	try { unitNode = xmlTree.getRootNode(); }
	catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		return false;
	}
	const XmlNode *parametersNode;
	try { parametersNode = unitNode->getChild("parameters"); }
	catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		return false; // bail out
	}
	if (!UnitStats::load(parametersNode, dir, techTree, factionType))
		loadOk = false;
	//armor type string
	try {
		string armorTypeName = parametersNode->getChildRestrictedValue("armor-type");
		armourType = techTree->getArmourType(armorTypeName);
	}
	catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	//size
	try { size = parametersNode->getChildIntValue("size"); }
	catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	//height
	try { height = parametersNode->getChildIntValue("height"); }
	catch (runtime_error e) {
		g_errorLog.addXmlError(dir, e.what());
		loadOk = false;
	}
	halfSize = size / fixed(2);
	halfHeight = height / fixed(2);

	//image
	try { DisplayableType::load(parametersNode, dir); }
	catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}

	if (!glestimal) {
		//prod time
		try { productionTime= parametersNode->getChildIntValue("time"); }
		catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//multi-build
		try { multiBuild = parametersNode->getOptionalBoolValue("multi-build"); }
		catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//multi selection
		try { multiSelect= parametersNode->getChildBoolValue("multi-selection"); }
		catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//light & lightColour
		try {
			const XmlNode *lightNode = parametersNode->getChild("light");
			light = lightNode->getAttribute("enabled")->getBoolValue();
			if (light)
				lightColour = lightNode->getColor3Value();
		} catch (runtime_error e) {
			g_errorLog.addXmlError(dir, e.what());
			loadOk = false;
		}
		//cellmap
		try {
			const XmlNode *cellMapNode= parametersNode->getChild("cellmap", 0, false);
			if(cellMapNode && cellMapNode->getAttribute("value")->getBoolValue()){
				//cellMap= new bool[size*size];
				m_cellMap = new PatchMap<1>(Rectangle(0,0, size, size), 0);
				for(int i=0; i<size; ++i){
					try {
						const XmlNode *rowNode= cellMapNode->getChild("row", i);
						string row= rowNode->getAttribute("value")->getRestrictedValue();
						if(row.size()!=size){
							throw runtime_error("Cellmap row is not the same as unit size");
						}
						for(int j=0; j<row.size(); ++j){
							m_cellMap->setInfluence(Vec2i(j, i), row[j]=='0'? 0: 1);
//							cellMap[i*size+j]= row[j]=='0'? false: true;
						}
					} catch (runtime_error e) {
						g_errorLog.addXmlError(path, e.what());
						loadOk = false;
					}
				}
			}
		} catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//levels
		try {
			const XmlNode *levelsNode= parametersNode->getChild("levels", 0, false);
			if(levelsNode) {
				levels.resize(levelsNode->getChildCount());
				for(int i=0; i<levels.size(); ++i){
					const XmlNode *levelNode= levelsNode->getChild("level", i);
					levels[i].load(levelNode, dir, techTree, factionType);
				}
			}
		} catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
	} // !glestimal

	// fields: begin clumsy... multiple fields are 'allowed' here, but we only want one...
	Fields fields;
	try {
		fields.load(parametersNode->getChild("fields"), dir, techTree, factionType);

		// extract ONE, making sure to chose Land over Air (for magitech compatability)
		field = Field::INVALID;
		if (fields.get(Field::AMPHIBIOUS))		field = Field::AMPHIBIOUS;
		else if (fields.get(Field::ANY_WATER))	field = Field::ANY_WATER;
		else if (fields.get(Field::DEEP_WATER))	field = Field::DEEP_WATER;
		else if(fields.get(Field::LAND))		field = Field::LAND;
		else if(fields.get(Field::AIR))			field = Field::AIR;
		else throw runtime_error("unit prototypes must specify a field");
		zone = field == Field::AIR ? Zone::AIR : Zone::LAND;
	} catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}
	if (!glestimal) {
		//properties
		try { properties.load(parametersNode->getChild("properties"), dir, techTree, factionType); }
		catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//ProducibleType parameters
		try { ProducibleType::load(parametersNode, dir, techTree, factionType); }
		catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//resources stored
		try {
			const XmlNode *resourcesStoredNode= parametersNode->getChild("resources-stored", 0, false);
			if (resourcesStoredNode) {
				storedResources.resize(resourcesStoredNode->getChildCount());
				for(int i=0; i<storedResources.size(); ++i){
					const XmlNode *resourceNode= resourcesStoredNode->getChild("resource", i);
					string name= resourceNode->getAttribute("name")->getRestrictedValue();
					int amount= resourceNode->getAttribute("amount")->getIntValue();
					storedResources[i].init(techTree->getResourceType(name), amount);
				}
			}
		} catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//image cancel
		try {
			const XmlNode *imageCancelNode= parametersNode->getChild("image-cancel");
			cancelImage= Renderer::getInstance().newTexture2D(ResourceScope::GAME);
			cancelImage->load(dir+"/"+imageCancelNode->getAttribute("path")->getRestrictedValue());
		} catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//meeting point
		try {
			const XmlNode *meetingPointNode= parametersNode->getChild("meeting-point");
			meetingPoint= meetingPointNode->getAttribute("value")->getBoolValue();
			if (meetingPoint) {
				meetingPointImage= Renderer::getInstance().newTexture2D(ResourceScope::GAME);
				meetingPointImage->load(dir+"/"+meetingPointNode->getAttribute("image-path")->getRestrictedValue());
			}
		} catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//selection sounds
		try {
			const XmlNode *selectionSoundNode= parametersNode->getChild("selection-sounds");
			if(selectionSoundNode->getAttribute("enabled")->getBoolValue()){
				selectionSounds.resize(selectionSoundNode->getChildCount());
				for(int i=0; i<selectionSounds.getSounds().size(); ++i){
					const XmlNode *soundNode= selectionSoundNode->getChild("sound", i);
					string path= soundNode->getAttribute("path")->getRestrictedValue();
					StaticSound *sound= new StaticSound();
					sound->load(dir + "/" + path);
					selectionSounds[i]= sound;
				}
			}
		} catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
		//command sounds
		try {
			const XmlNode *commandSoundNode= parametersNode->getChild("command-sounds");
			if(commandSoundNode->getAttribute("enabled")->getBoolValue()){
				commandSounds.resize(commandSoundNode->getChildCount());
				for(int i=0; i<commandSoundNode->getChildCount(); ++i){
					const XmlNode *soundNode= commandSoundNode->getChild("sound", i);
					string path= soundNode->getAttribute("path")->getRestrictedValue();
					StaticSound *sound= new StaticSound();
					sound->load(dir + "/" + path);
					commandSounds[i]= sound;
				}
			}
		} catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
	} // !glestimal

	//skills
	try {
		const XmlNode *skillsNode= unitNode->getChild("skills");
		//skillTypes.resize(skillsNode->getChildCount());
		for (int i=0; i < skillsNode->getChildCount(); ++i) {
			const XmlNode *sn = skillsNode->getChild(i);
			if (sn->getName() != "skill") continue;
			const XmlNode *typeNode = sn->getChild("type");
			string classId = typeNode->getAttribute("value")->getRestrictedValue();
			SkillType *skillType = g_world.getSkillTypeFactory().newInstance(classId);
			skillType->load(sn, dir, techTree, factionType);
			skillTypes.push_back(skillType);
			Checksum checksum;
			skillType->doChecksum(checksum);
			g_world.getSkillTypeFactory().setChecksum(skillType, checksum.getSum());
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		return false; // if skills are screwy, stop
	}

	sortSkillTypes();

	//commands
	try {
		const XmlNode *commandsNode = unitNode->getChild("commands");
		for (int i = 0; i < commandsNode->getChildCount(); ++i) {
			const XmlNode *commandNode = commandsNode->getChild(i);
			if (commandNode->getName() != "command") continue;
			string classId = commandNode->getChildRestrictedValue("type");
			CommandType *commandType = g_world.getCommandTypeFactory().newInstance(classId, this);
			commandType->load(commandNode, dir, techTree, factionType);
			commandTypes.push_back(commandType);
			Checksum checksum;
			commandType->doChecksum(checksum);
			g_world.getCommandTypeFactory().setChecksum(commandType, checksum.getSum());
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}
	if (!loadOk) return false; // unsafe to keep going...

	// if type has a meeting point, add a SetMeetingPoint command
	if (meetingPoint) {
		CommandType *smpct = g_world.getCommandTypeFactory().newInstance("set-meeting-point", this);
		commandTypes.push_back(smpct);
		Checksum checksum;
		smpct->doChecksum(checksum);
		g_world.getCommandTypeFactory().setChecksum(smpct, checksum.getSum());
	}

	sortCommandTypes();

	try { // Logger::addXmlError() expects a char*, so it's easier just to throw & catch ;)
		if(!getFirstStOfClass(SkillClass::STOP)) {
			throw runtime_error("Every unit must have at least one stop skill: "+ path);
		}
		if(!getFirstStOfClass(SkillClass::DIE)) {
			throw runtime_error("Every unit must have at least one die skill: "+ path);
		}
	} catch (runtime_error e) {
		g_errorLog.addXmlError(path, e.what());
		loadOk = false;
	}

	if (!glestimal) {
		//emanations
		try {
			const XmlNode *emanationsNode = parametersNode->getChild("emanations", 0, false);
			if (emanationsNode) {
				emanations.resize(emanationsNode->getChildCount());
				for (int i = 0; i < emanationsNode->getChildCount(); ++i) {
					try {
						const XmlNode *emanationNode = emanationsNode->getChild("emanation", i);
						Emanation *emanation = new Emanation();
						emanation->load(emanationNode, dir, techTree, factionType);
						emanations[i] = emanation;
					} catch (runtime_error e) {
						g_errorLog.addXmlError(path, e.what());
						loadOk = false;
					}
				}
			}
		} catch (runtime_error e) {
			g_errorLog.addXmlError(path, e.what());
			loadOk = false;
		}
	}
	m_colourMap = new PatchMap<1>(Rectangle(0,0, size, size), 0);
	RectIterator iter(Rect2i(0, 0, size - 1, size - 1));
	while (iter.more()) {
		Vec2i pos = iter.next();
		if (!hasCellMap() || m_cellMap->getInfluence(pos)) {
			m_colourMap->setInfluence(pos, 1);
		} else {
			int ncount = 0;
			PerimeterIterator pIter(Vec2i(pos.x - 1, pos.y - 1), Vec2i(pos.x + 1, pos.y + 1));
			while (pIter.more()) {
				Vec2i nPos = pIter.next();
				if (m_cellMap->getInfluence(nPos)) {
					++ncount;
				}
			}
			if (ncount >= 2) {
				m_colourMap->setInfluence(pos, 1);
			}
		}
	}
	return loadOk;   
}

void UnitType::doChecksum(Checksum &checksum) const {
	ProducibleType::doChecksum(checksum);
	UnitStats::doChecksum(checksum);

	if (armourType) checksum.add(armourType->getName());
	checksum.add(light);
	checksum.add(lightColour);
	checksum.add(size);
	checksum.add(height);

	checksum.add(multiBuild);
	checksum.add(multiSelect);
	foreach_const (SkillTypes, it, skillTypes) {
		(*it)->doChecksum(checksum);
	}
	foreach_const (CommandTypes, it, commandTypes) {
		(*it)->doChecksum(checksum);
	}
	foreach_const (StoredResources, it, storedResources) {
		checksum.add(it->getType()->getName());
		checksum.add(it->getAmount());
	}
	foreach_const (Levels, it, levels) {
		it->doChecksum(checksum);
	}
	foreach_const (Emanations, it, emanations) {
		(*it)->doChecksum(checksum);
	}
	//meeting point
	checksum.add(meetingPoint);
	checksum.add(halfSize);
	checksum.add(halfHeight);

}

// ==================== get ====================

const CommandType *UnitType::getCommandType(const string &name) const {
	for (CommandTypes::const_iterator i = commandTypes.begin(); i != commandTypes.end(); ++i) {
		if ((*i)->getName() == name) {
			return (*i);
		}
	}
	return NULL;
}

const HarvestCommandType *UnitType::getHarvestCommand(const ResourceType *rt) const {
	foreach_const (CommandTypes, it, commandTypesByClass[CommandClass::HARVEST]) {
		const HarvestCommandType *hct = static_cast<const HarvestCommandType*>(*it);
		if (hct->canHarvest(rt)) {
			return hct;
		}
	}
	return 0;
}

const AttackCommandType *UnitType::getAttackCommand(Zone zone) const {
	foreach_const (CommandTypes, it, commandTypesByClass[CommandClass::ATTACK]) {
		const AttackCommandType *act = static_cast<const AttackCommandType*>(*it);
		if (act->getAttackSkillTypes()->getZone(zone)) {
			return act;
		}
	}
	return 0;
}

const RepairCommandType *UnitType::getRepairCommand(const UnitType *repaired) const {
	foreach_const (CommandTypes, it, commandTypesByClass[CommandClass::REPAIR]) {
		const RepairCommandType *rct = static_cast<const RepairCommandType*>(*it);
		if (rct->canRepair(repaired)) {
			return rct;
		}
	}
	return 0;
}

int UnitType::getStore(const ResourceType *rt) const {
	foreach_const (StoredResources, it, storedResources) {
		if (it->getType() == rt) {
			return it->getAmount();
		}
	}
	return 0;
}

// only used for matching while loading commands
const SkillType *UnitType::getSkillType(const string &skillName, SkillClass skillClass) const{
	for (int i=0; i < skillTypes.size(); ++i) {
		if (skillTypes[i]->getName() == skillName) {
			if (skillTypes[i]->getClass() == skillClass || skillClass == SkillClass::COUNT) {
				return skillTypes[i];
			} else {
				throw runtime_error("Skill '" + skillName + "' is not of class " + SkillClassNames[skillClass]);
			}
		}
	}
	throw runtime_error("No skill named '" + skillName + "'");
}


// ==================== has ====================

bool UnitType::hasSkillClass(SkillClass skillClass) const {
	return !skillTypesByClass[skillClass].empty();
}

bool UnitType::hasCommandType(const CommandType *ct) const {
	assert(ct);
	foreach_const (CommandTypes, it, commandTypesByClass[ct->getClass()]) {
		if (*it == ct) {
			return true;
		}
	}
	return false;
}

bool UnitType::hasSkillType(const SkillType *st) const {
	assert(st);
	foreach_const (SkillTypes, it, skillTypesByClass[st->getClass()]) {
		if (*it == st) {
			return true;
		}
	}
	return false;
}

bool UnitType::isOfClass(UnitClass uc) const{
	switch (uc) {
		case UnitClass::WARRIOR:
			return hasSkillClass(SkillClass::ATTACK) 
				&& !hasSkillClass(SkillClass::HARVEST);
		case UnitClass::WORKER:
			return hasSkillClass(SkillClass::BUILD) 
				|| hasSkillClass(SkillClass::REPAIR)
				|| hasSkillClass(SkillClass::HARVEST);
		case UnitClass::BUILDING:
			return hasSkillClass(SkillClass::BE_BUILT)
				&& !hasSkillClass(SkillClass::MOVE);
		case UnitClass::CARRIER:
			return hasSkillClass(SkillClass::LOAD)
				&& hasSkillClass(SkillClass::UNLOAD);
		default:
			throw runtime_error("Unknown UnitClass passed to UnitType::isOfClass()");
	}
	return false;
}

// ==================== PRIVATE ====================

void UnitType::sortSkillTypes() {
	foreach_enum (SkillClass, sc) {
		foreach (SkillTypes, it, skillTypes) {
			if ((*it)->getClass() == sc) {
				skillTypesByClass[sc].push_back(*it);
			}
		}
	}
	if (!skillTypesByClass[SkillClass::BE_BUILT].empty()) {
		startSkill = skillTypesByClass[SkillClass::BE_BUILT].front();
	} else {
		startSkill = skillTypesByClass[SkillClass::STOP].front();
	}
}

void UnitType::sortCommandTypes() {
	foreach_enum (CommandClass, cc) {
		foreach (CommandTypes, it, commandTypes) {
			if ((*it)->getClass() == cc) {
				commandTypesByClass[cc].push_back(*it);
			}
		}
	}
//	foreach (CommandTypes, it, commandTypes) {
//		commandTypeMap[(*it)->getId()] = *it;
//	}
}

UnitTypeFactory::~UnitTypeFactory() {
	foreach (vector<UnitType*>, it, m_types) {
//		assert(m_typeSet.find(*it) != m_typeSet.end());
//		m_typeSet.erase(m_typeSet.find(*it));
//		assert(m_checksumTable.find(*it) != m_checksumTable.end());
//		m_checksumTable.erase(m_checksumTable.find(*it));
		delete *it;
	}
//	assert(m_typeSet.empty());
//	assert(m_checksumTable.empty());
	m_types.clear();
}

/*
void UnitTypeFactory::assertTypes() {
	foreach (vector<UnitType*>, it, m_types) {
		assert(m_typeSet.find(*it) != m_typeSet.end());
		assert(m_checksumTable.find(*it) != m_checksumTable.end());
		Checksum checksum;
		(*it)->doChecksum(checksum);
		assert(m_checksumTable[*it] == checksum.getSum());
	}
}
*/

UnitType* UnitTypeFactory::newInstance() {
	UnitType *ut = SingleTypeFactory<UnitType>::newInstance();
	ut->setId(m_idCounter++);
	m_types.push_back(ut);
//	m_typeSet.insert(ut);
	return ut;
}

/*
void UnitTypeFactory::setChecksum(UnitType *ut, int32 cs) {
	assert(m_typeSet.find(ut) != m_typeSet.end());
	assert(m_checksumTable.find(ut) == m_checksumTable.end());
	m_checksumTable[ut] = cs;
}
*/

UnitType* UnitTypeFactory::getType(int id) {
	if (id < 0 || id >= m_types.size()) {
		throw runtime_error("Error: Unknown unit type id: " + intToStr(id));
	}
	return m_types[id];
}

}}//end namespace

// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "faction_type.h"

#include "logger.h"
#include "util.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "platform_util.h"
#include "world.h"
#include "program.h"
#include "sim_interface.h"
#include "events.h"

//#include "core.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest { namespace ProtoTypes {

using Main::Program;

// ======================================================
//          Class FactionType
// ======================================================
FactionType::FactionType()
		: music(0)
		, attackNotice(0)
		, enemyNotice(0)
		, attackNoticeDelay(0)
		, enemyNoticeDelay(0)
		, m_logoTeamColour(0)
		, m_logoRgba(0) {
}

bool FactionType::preLoad(const string &dir, const TechTree *techTree) {
	m_name = basename(dir);
    bool loadOk = true;
    // a1.75) preload modifications
	string modificationsPath = dir + "/modifications/*.";
	vector<string> modificationsFilenames;
	try {
		findAll(modificationsPath, modificationsFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	for (int i = 0; i < modificationsFilenames.size(); ++i) {
		string path = dir + "/modifications/" + modificationsFilenames[i];
		Modification newMod;
		modifications.push_back(newMod);
		modifications.back().preLoad(path);
	}
	// a1) preload units
	string unitsPath = dir + "/units/*.";
	vector<string> unitFilenames;
	try {
		findAll(unitsPath, unitFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
		loadOk = false;
	}
	for (int i = 0; i < unitFilenames.size(); ++i) {
		string path = dir + "/units/" + unitFilenames[i];
		UnitType *ut = g_prototypeFactory.newUnitType();
		unitTypes.push_back(ut);
		unitTypes.back()->preLoad(path);
	}
    // a1.5) preload items
	string itemsPath = dir + "/items/*.";
	vector<string> itemFilenames;
	try {
		findAll(itemsPath, itemFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	for (int i = 0; i < itemFilenames.size(); ++i) {
		string path = dir + "/items/" + itemFilenames[i];
		ItemType *it = g_prototypeFactory.newItemType();
		itemTypes.push_back(it);
		itemTypes.back()->preLoad(path);
	}
	// a2) preload upgrades
	string upgradesPath= dir + "/upgrades/*.";
	vector<string> upgradeFilenames;
	try {
		findAll(upgradesPath, upgradeFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	for (int i = 0; i < upgradeFilenames.size(); ++i) {
		string path = dir + "/upgrades/" + upgradeFilenames[i];
		UpgradeType *ut = g_prototypeFactory.newUpgradeType();
		upgradeTypes.push_back(ut);
		upgradeTypes.back()->preLoad(path);
	}
    // a3) preload events
	string eventsPath = dir + "/events/*.";
	vector<string> eventFilenames;
	try {
		findAll(eventsPath, eventFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	for (int i = 0; i < eventFilenames.size(); ++i) {
		string path = dir + "/events/" + eventFilenames[i];
		EventType *it = g_prototypeFactory.newEventType();
		eventTypes.push_back(it);
		eventTypes.back()->preLoad(path);
		//throw runtime_error(path);
	}

	string tpath = dir + "/traits/*.";
	vector<string> tpaths;
	try {
		findAll(tpath, tpaths);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	traitsList.resize(tpaths.size());
	for (int i = 0; i < tpaths.size(); ++i) {
        string tstr = dir + "/traits/" + tpaths[i] + "/" + tpaths[i];
        traitsList[i].preLoad(tstr, i);
	}

    string spath = dir + "/specializations/*.";
	vector<string> paths;
	try {
		findAll(spath, paths);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	specializations.resize(paths.size());
	for (int i = 0; i < paths.size(); ++i) {
        string sstr = dir + "/specializations/" + paths[i] + "/" + paths[i];
        specializations[i].preLoad(sstr);
	}

	return loadOk;
}

bool FactionType::guiPreLoad(const string &dir, const TechTree *techTree) {
    bool loadOk = true;
    string guiPath = dir + "/gui/*.";
    vector<string> guiFilenames;
	try {
		findAll(guiPath, guiFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	for (int i = 0; i < guiFilenames.size(); ++i) {
		string path = dir + "/gui/" + guiFilenames[i];
        guiFileNames.push_back(path);
	}
	return loadOk;
}

bool FactionType::preLoadGlestimals(const string &dir, const TechTree *techTree) {
	m_name = basename(dir);

	// a1) preload units
	string unitsPath = dir + "/glestimals/*.";
	vector<string> unitFilenames;
	bool loadOk = true;
	try {
		findAll(unitsPath, unitFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
		loadOk = false;
	}
	for (int i = 0; i < unitFilenames.size(); ++i) {
		string path = dir + "/glestimals/" + unitFilenames[i];
		UnitType *ut = g_prototypeFactory.newUnitType();
		unitTypes.push_back(ut);
		unitTypes.back()->preLoad(path);
	}
	return loadOk;
}

/// load a faction, given a directory @param ndx faction index, and hence id
/// @param dir path to faction directory @param techTree pointer to TechTree
bool FactionType::load(int ndx, const string &dir, const TechTree *techTree) {
	m_id = ndx;
	m_name = basename(dir);
	Logger &logger = g_logger;
	logger.logProgramEvent("Loading faction type: " + m_name, true);
	bool loadOk = true;

	// 1. Open xml file
	string path = dir + "/" + m_name + ".xml";
	XmlTree xmlTree;
	try {
		xmlTree.load(path);
	} catch (runtime_error e) {
		g_logger.logXmlError(path, "File missing or wrongly named.");
		return false; // bail
	}
	const XmlNode *factionNode;
	try {
		factionNode = xmlTree.getRootNode();
	} catch (runtime_error e) {
		g_logger.logXmlError(path, "File appears to lack contents.");
		return false; // bail
	}

    try {
        const XmlNode *factionTypeNamesNode = factionNode->getChild("faction-type-names", 0, false);
        if (factionTypeNamesNode) {
            for (int i = 0; i < factionTypeNamesNode->getChildCount(); ++i) {
                try {
                    const XmlNode *factionTypeNameNode = factionTypeNamesNode->getChild("faction-type", i, false);
                    if (factionTypeNameNode) {
                        string factionTypeName = factionTypeNameNode->getAttribute("name")->getRestrictedValue();
                        factionTypeNames.push_back(factionTypeName);
                    }
                } catch (runtime_error e) {
                g_logger.logXmlError(path, e.what());
                loadOk = false;
                }
            }
        }
	} catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}

    personalityDirectory = dir + "/ai/" + "personalities.xml";
    XmlTree personalityXmlTree;
	try { personalityXmlTree.load(personalityDirectory); }
	catch (runtime_error e) {
		g_logger.logXmlError(personalityDirectory, e.what());
		g_logger.logError("Fatal Error: could not load " + personalityDirectory);
		return false; // bail
	}
	const XmlNode *behaviorNode;
	try {
	    behaviorNode = personalityXmlTree.getRootNode();
    } catch (runtime_error e) {
		g_logger.logXmlError(personalityDirectory, e.what());
		return false;
	}
	const XmlNode *personalitiesNode = behaviorNode->getChild("personalities", 0, false);
	if (personalitiesNode) {
	    personalities.resize(personalitiesNode->getChildCount());
		for (int i = 0; i < personalitiesNode->getChildCount(); ++i) {
		    const XmlNode *personalityNode = personalitiesNode->getChild("personality", i);
		    personalities[i].load(personalityNode);

		}
	}
	try {
        const XmlNode *needsNode = factionNode->getChild("needs", 0, false);
        if (needsNode) {
            citizenNeeds.load(needsNode);
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	// 3.75. Load modifications
	for (int i = 0; i < modifications.size(); ++i) {
		string str = dir + "/modifications/" + modifications[i].getName();
		if (modifications[i].load(str)) {
		} else {
			loadOk = false;
		}
	}
	// 3. Load units
	for (int i = 0; i < unitTypes.size(); ++i) {
		string str = dir + "/units/" + unitTypes[i]->getName();
		if (unitTypes[i]->load(str, techTree, this)) {
			g_prototypeFactory.setChecksum(unitTypes[i]);
		} else {
			loadOk = false;
		}
		logger.getProgramLog().unitLoaded();
	}
	// 3.5. Load items
	for (int i = 0; i < itemTypes.size(); ++i) {
		string str = dir + "/items/" + itemTypes[i]->getName();
		if (itemTypes[i]->load(str, techTree, this)) {
			g_prototypeFactory.setChecksum(itemTypes[i]);
		} else {
			loadOk = false;
		}
		logger.getProgramLog().itemLoaded();
	}
	// 3.6. Load events
	for (int i = 0; i < eventTypes.size(); ++i) {
		string str = dir + "/events/" + eventTypes[i]->getName();
		if (eventTypes[i]->load(str, techTree, this)) {
			//g_prototypeFactory.setChecksum(eventTypes[i]);
		} else {
			loadOk = false;
		}
		//logger.getProgramLog().eventLoaded();
	}
	// 4. Add BeLoadedCommandType to units that need them

	// 4a. Discover which mobile unit types can be loaded(/housed) in other units
	foreach_const (UnitTypes, uit, unitTypes) {
		const UnitType *ut = *uit;
		for (int i=0; i < ut->getActions()->getCommandTypeCount<LoadCommandType>(); ++i) {
			const LoadCommandType *lct = ut->getActions()->getCommandType<LoadCommandType>(i);
			foreach (UnitTypes, luit, unitTypes) {
				UnitType *lut = *luit;
				if (lct->canCarry(lut) && lut->getActions()->getFirstCtOfClass(CmdClass::MOVE)) {
					loadableUnitTypes.insert(lut);
				}
			}
		}

	}

	foreach_const (UnitTypes, uit, unitTypes) {
		const UnitType *ut = *uit;
		for (int i=0; i < ut->getActions()->getCommandTypeCount<FactionLoadCommandType>(); ++i) {
			const FactionLoadCommandType *lct = ut->getActions()->getCommandType<FactionLoadCommandType>(i);
			foreach (UnitTypes, luit, unitTypes) {
				UnitType *lut = *luit;
				if (lct->canCarry(lut) && lut->getActions()->getFirstCtOfClass(CmdClass::MOVE)) {
					loadableUnitTypes.insert(lut);
				}
			}
		}

	}

	foreach_const (UnitTypes, uit, unitTypes) {
		const UnitType *ut = *uit;
		for (int i=0; i < ut->getActions()->getCommandTypeCount<GarrisonCommandType>(); ++i) {
			const GarrisonCommandType *lct = ut->getActions()->getCommandType<GarrisonCommandType>(i);
			foreach (UnitTypes, luit, unitTypes) {
				UnitType *lut = *luit;
				if (lct->canCarry(lut) && lut->getActions()->getFirstCtOfClass(CmdClass::MOVE)) {
					loadableUnitTypes.insert(lut);
				}
			}
		}

	}

	// 4b. Give mobile housable unit types a be-loaded command type
	foreach (UnitTypeSet, it, loadableUnitTypes) {
		(*it)->addBeLoadedCommand();
	}

	// 5. Load upgrades
	for (int i = 0; i < upgradeTypes.size(); ++i) {
		string str = dir + "/upgrades/" + upgradeTypes[i]->getName();
		if (upgradeTypes[i]->load(str)) {
			g_prototypeFactory.setChecksum(upgradeTypes[i]);
		} else {
			loadOk = false;
		}
	}
	try {
        const XmlNode *resourceTradesNode = factionNode->getChild("resource-trades", 0, false);
        if (resourceTradesNode) {
            resourceTrades.resize(resourceTradesNode->getChildCount());
            for (int i = 0; i < resourceTradesNode->getChildCount(); ++i) {
                const XmlNode *resourceTradeNode = resourceTradesNode->getChild("resource", i);
                string resType = resourceTradeNode->getAttribute("type")->getRestrictedValue();
                int required = resourceTradeNode->getAttribute("value")->getIntValue();
                resourceTrades[i].init(techTree->getResourceType(resType),required,0,0);
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	// 6. Read starting resources
	try {
		const XmlNode *startingResourcesNode = factionNode->getChild("starting-resources");
		startingResources.resize(startingResourcesNode->getChildCount());
		for (int i = 0; i < startingResources.size(); ++i) {
			try {
				const XmlNode *resourceNode = startingResourcesNode->getChild("resource", i);
				string name = resourceNode->getAttribute("name")->getRestrictedValue();
				int amount = resourceNode->getAttribute("amount")->getIntValue();
				startingResources[i].init(techTree->getResourceType(name), amount);
			} catch (runtime_error e) {
				g_logger.logXmlError(path, e.what());
				loadOk = false;
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}

    try {
        const XmlNode *itemImagesNode = factionNode->getChild("item-images");
        for (int i = 0; i < itemImagesNode->getChildCount(); ++i) {
            try {
                const XmlNode *itemImageNode = itemImagesNode->getChild("item-image", i);
                string itemImgPath = dir + "/" + itemImageNode->getAttribute("image-path")->getRestrictedValue();
				Texture2D *itemImage = g_renderer.getTexture2D(ResourceScope::GAME, itemImgPath);
				itemImages.push_back(itemImage);
            } catch (runtime_error e) {
            g_logger.logXmlError(path, e.what());
            loadOk = false;
            }
        }
	} catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}

	// 7. Read starting units
	try {
		const XmlNode *startingUnitsNode = factionNode->getChild("starting-units");
		for (int i = 0; i < startingUnitsNode->getChildCount(); ++i) {
			try {
				const XmlNode *unitNode = startingUnitsNode->getChild("unit", i);
				string name = unitNode->getAttribute("name")->getRestrictedValue();
				int amount = unitNode->getAttribute("amount")->getIntValue();
				startingUnits.push_back(PairPUnitTypeInt(getUnitType(name), amount));
			} catch (runtime_error e) {
				g_logger.logXmlError(path, e.what());
				loadOk = false;
			}
		}
		usesSovereign = false;
        const XmlNode *usesSovereignNode = factionNode->getChild("uses-sovereign", 0, false);
		if (usesSovereignNode) {
            bool needsSovereign = usesSovereignNode->getAttribute("check")->getBoolValue();
		    if (needsSovereign) {
                string sovName = g_simInterface.getGameSettings().getSovereignType();
		        if (sovName == "") {
                    sovName= "ruler";
		        }
                startingUnits.push_back(PairPUnitTypeInt(getUnitType(sovName), 1));
		    }
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}

    try {
        const XmlNode *expTypeNode = factionNode->getChild("exp-type");
        string expType = expTypeNode->getAttribute("type")->getRestrictedValue();
        if (expType == "hit") {
            onHitExp = true;
        } else if (expType == "kill") {
            onHitExp = false;
        }
	} catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}

	// 8. Read music
	try {
		const XmlNode *musicNode= factionNode->getChild("music");
		bool value = musicNode->getAttribute("value")->getBoolValue();
		if (value) {
			XmlAttribute *playListAttr = musicNode->getAttribute("play-list", false);
			if (playListAttr) {
				if (playListAttr->getBoolValue()) {
					const XmlAttribute *shuffleAttrib = musicNode->getAttribute("shuffle", false);
					bool shuffle = (shuffleAttrib && shuffleAttrib->getBoolValue() ? true : false);

					vector<StrSound*> tracks;
					for (int i=0; i < musicNode->getChildCount(); ++i) {
						StrSound *sound = new StrSound();
						sound->open(dir+"/"+musicNode->getChild("music-file", i)->getAttribute("path")->getRestrictedValue());
						tracks.push_back(sound);
					}
					if (tracks.empty()) {
						throw runtime_error("No tracks in play-list!");
					}
					if (shuffle) {
						int seed = int(Chrono::getCurTicks());
						Random random(seed);
						Shared::Util::jumble(tracks, random);
					}
					vector<StrSound*>::iterator it = tracks.begin();
					vector<StrSound*>::iterator it2 = it + 1;
					while (it2 != tracks.end()) {
						(*it)->setNext(*it2);
						++it; ++it2;
					}
					music = tracks[0];
				}
			} else {
				XmlAttribute *pathAttr = musicNode->getAttribute("path", false);
				if (pathAttr) {
					music = new StrSound();
					music->open(dir + "/" + pathAttr->getRestrictedValue());
				} else {
					g_logger.logXmlError(path, "'music' node must have either a 'path' or 'play-list' attribute");
					loadOk = false;
				}
			}
		}
	} catch (runtime_error e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}

	// 9. Load faction logo pixmaps
	try {
		const XmlNode *logoNode = factionNode->getOptionalChild("logo");
		if (logoNode && logoNode->getBoolValue()) {
			const XmlNode *n = logoNode->getOptionalChild("team-colour");
			if (n) {
				string logoPath = dir + "/" + n->getRestrictedAttribute("path");
				m_logoTeamColour = new Pixmap2D();
				m_logoTeamColour->load(logoPath);
			} else {
				m_logoTeamColour = 0;
		}
			n = logoNode->getOptionalChild("rgba-colour");
			if (n) {
				string logoPath = dir + "/" + n->getRestrictedAttribute("path");
				m_logoRgba = new Pixmap2D();
				m_logoRgba->load(logoPath);
			} else {
				m_logoRgba = 0;
			}
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError(path, e.what());
		delete m_logoTeamColour;
		delete m_logoRgba;
		m_logoTeamColour = m_logoRgba = 0;
	}

	// 10. Notification sounds

	// 10a. Notification of being attacked off screen
	try {
		const XmlNode *attackNoticeNode= factionNode->getChild("attack-notice", 0, false);
		if (attackNoticeNode && attackNoticeNode->getAttribute("enabled")->getBoolValue()) {
			attackNoticeDelay = attackNoticeNode->getAttribute("min-delay")->getIntValue();
			attackNotice = new SoundContainer();
			attackNotice->resize(attackNoticeNode->getChildCount());
			for (int i=0; i < attackNoticeNode->getChildCount(); ++i) {
				string path = attackNoticeNode->getChild("sound-file", i)->getAttribute("path")->getRestrictedValue();
				StaticSound *sound = new StaticSound();
				sound->load(dir + "/" + path);
				(*attackNotice)[i] = sound;
			}
			if (attackNotice->getSounds().size() == 0) {
				g_logger.logXmlError(path, "An enabled attack-notice must contain at least one sound-file.");
				loadOk = false;
			}
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}
	// 10b. Notification of visual contact with enemy off screen
	try {
		const XmlNode *enemyNoticeNode= factionNode->getChild("enemy-notice", 0, false);
		if (enemyNoticeNode && enemyNoticeNode->getAttribute("enabled")->getRestrictedValue() == "true") {
			enemyNoticeDelay = enemyNoticeNode->getAttribute("min-delay")->getIntValue();
			enemyNotice = new SoundContainer();
			enemyNotice->resize(enemyNoticeNode->getChildCount());
			for (int i = 0; i < enemyNoticeNode->getChildCount(); ++i) {
				string path= enemyNoticeNode->getChild("sound-file", i)->getAttribute("path")->getRestrictedValue();
				StaticSound *sound= new StaticSound();
				sound->load(dir + "/" + path);
				(*enemyNotice)[i]= sound;
			}
			if (enemyNotice->getSounds().size() == 0) {
				g_logger.logXmlError(path, "An enabled enemy-notice must contain at least one sound-file.");
				loadOk = false;
			}
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError(path, e.what());
		loadOk = false;
	}
	return loadOk;
}

bool FactionType::loadSpecTraitSkills(int ndx, const string &dir, const TechTree *techTree) {
    bool loadOk = true;
    string spath = dir + "/specializations/*.";
	vector<string> paths;
	try {
		findAll(spath, paths);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	for (int i = 0; i < paths.size(); ++i) {
        string sstr = dir + "/specializations/" + paths[i] + "/" + paths[i];
        specializations[i].reset();
        if(!specializations[i].load(sstr)){
            loadOk = false;
        }
	}
    string apath = dir + "/skills/*.";
	vector<string> apaths;
	try {
		findAll(apath, apaths);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	listActions.resize(apaths.size());
	for (int i = 0; i < apaths.size(); ++i) {
        string sstr = dir + "/skills/" + apaths[i] + "/" + apaths[i] + ".xml";
        XmlTree	xmlTree;
        try {
            xmlTree.load(sstr);
        }
        catch (runtime_error &e) {
            g_logger.logXmlError ( apaths[i], "File missing or wrongly named." );
            return false;
        }
        const XmlNode *listActionsNode;
        try { listActionsNode = xmlTree.getRootNode(); }
        catch (runtime_error &e) {
            g_logger.logXmlError ( apath, "File appears to lack contents." );
            return false;
        }
        const XmlNode *actionsNode = listActionsNode->getChild("actions", 0, false);
	    if (actionsNode) {
	        const CreatableType *ct = 0;
	        const FactionType *ft = 0;
            if(!listActions[i].load(actionsNode, dir, true, ft, ct)){
                loadOk = false;
            }
	    }
	}
	for (int i = 0; i < listActions.size(); ++i) {
	    for (int j = 0; j < listActions[i].getSkillTypeCount(); ++j) {
            SkillType *skillType = listActions[i].getSkillType(j);
            actions.addSkillType(skillType);
	    }
	    for (int j = 0; j < listActions[i].getCommandTypeCount(); ++j) {
            CommandType *commandType = listActions[i].getCommandType(j);
            actions.addCommand(commandType);
	    }
	}
	actions.sortSkillTypes();
	actions.sortCommandTypes();
	string tpath = dir + "/traits/*.";
	vector<string> tpaths;
	try {
		findAll(tpath, tpaths);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	for (int i = 0; i < tpaths.size(); ++i) {
        string tstr = dir + "/traits/" + tpaths[i] + "/" + tpaths[i];
        if (!traitsList[i].load(tstr)) {
            loadOk = false;
        }
	}
    return loadOk;
}

bool FactionType::loadGlestimals(const string &dir, const TechTree *techTree) {
	Logger &logger = Logger::getInstance();
	logger.logProgramEvent("Glestimal Faction: " + dir, true);
	m_id = -1;
	m_name = basename(dir);
	bool loadOk = true;

	// load glestimals
	for (int i = 0; i < unitTypes.size(); ++i) {
		string str = dir + "/glestimals/" + unitTypes[i]->getName();
		if (unitTypes[i]->load(str, techTree, this)) {
			Checksum checksum;
			unitTypes[i]->doChecksum(checksum);
			g_prototypeFactory.setChecksum(unitTypes[i]);
		} else {
			loadOk = false;
		}
		///@todo count glestimals
		//logger.unitLoaded();
	}
	return loadOk;
}

void FactionType::doChecksum(Checksum &checksum) const {
	checksum.add(m_name);
	foreach_const (UnitTypes, it, unitTypes) {
		(*it)->doChecksum(checksum);
	}
	/*foreach_const (ItemTypes, it, itemTypes) {
		(*it)->doChecksum(checksum);
	}*/
	foreach_const (UpgradeTypes, it, upgradeTypes) {
		(*it)->doChecksum(checksum);
	}
	foreach_const (StartingUnits, it, startingUnits) {
		checksum.add(it->first->getName());
		checksum.add<int>(it->second);
	}
	foreach_const (SResources, it, startingResources) {
		checksum.add(it->getType()->getName());
		checksum.add<int>(it->getAmount());
	}
}

FactionType::~FactionType() {
	while (music) {
		StrSound *delMusic = music;
		music = music->getNext();
		delete delMusic;
	}
	if (attackNotice) {
		deleteValues(attackNotice->getSounds().begin(), attackNotice->getSounds().end());
		delete attackNotice;
	}
	if (enemyNotice) {
		deleteValues(enemyNotice->getSounds().begin(), enemyNotice->getSounds().end());
		delete enemyNotice;
	}
	delete m_logoTeamColour;
	delete m_logoRgba;
}

// ==================== get ====================

Trait *FactionType::getTraitById(int findId) {
    //if (findId == -1) {
        //assert(false);
    //}
    for (int i = 0; i < traitsList.size(); ++i) {
        if (traitsList[i].getTraitId() == findId) {
            return &traitsList[i];
        }
    }
    throw runtime_error("Trait not found:" + intToStr(findId));
}

const UnitType *FactionType::getUnitType(const string &m_name) const{
    for (int i = 0; i < unitTypes.size(); ++i) {
		if (unitTypes[i]->getName() == m_name) {
            return unitTypes[i];
		}
    }
	throw runtime_error("Unit not found: " + m_name);
}

const ItemType *FactionType::getItemType(const string &m_name) const{
    for (int i = 0; i < itemTypes.size(); ++i) {
		if (itemTypes[i]->getName() == m_name) {
            return itemTypes[i];
		}
    }
	throw runtime_error("Item not found: " + m_name);
}

const UpgradeType *FactionType::getUpgradeType(const string &m_name) const{
    for (int i = 0; i < upgradeTypes.size(); ++i) {
		if (upgradeTypes[i]->getName() == m_name) {
            return upgradeTypes[i];
		}
    }
	throw runtime_error("Upgrade not found: " + m_name);
}

int FactionType::getStartingResourceAmount(const ResourceType *resourceType) const{
	for (int i = 0; i < startingResources.size(); ++i) {
		if (startingResources[i].getType() == resourceType) {
			return startingResources[i].getAmount();
		}
	}
	return 0;
}

}}//end namespace

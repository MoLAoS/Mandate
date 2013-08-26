// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "character.h"
#include "tech_tree.h"

namespace Glest { namespace ProtoTypes {
// ===============================
// 	class CharDatum
// ===============================
void CharDatum::init(int amount, string title) {
    name = title;
    value = amount;
}

void CharDatum::save(XmlNode *node) const {
    node->addAttribute("name", name);
    node->addAttribute("value", value);
}

void CharDatum::getDesc(string &str, const char *pre) {
    str += name;
    str += " |Proficiency: ";
    str += intToStr(value);
}

void CharDatum::getDesc(string &str, const char *pre) const {
    str += name;
    str += " |Proficiency: ";
    str += intToStr(value);
}

// ===============================
// 	class Knowledge
// ===============================
void Knowledge::sum(const Knowledge *knowledge) {
    bool knownKnowledge = false;
    for (int i = 0; i < knowledge->getLanguageCount(); ++i) {
        for (int j = 0; j < getLanguageCount(); ++j) {
            if (languages[j].getName() == knowledge->getLanguage(i)->getName()) {
                languages[j].incValue(knowledge->getLanguage(i)->getValue());
                knownKnowledge = true;
            }
        }
        if (knownKnowledge == false) {
            CharDatum language;
            language.init(knowledge->getLanguage(i)->getValue(), knowledge->getLanguage(i)->getName());
            languages.push_back(language);
        }
        knownKnowledge = false;
    }
    for (int i = 0; i < knowledge->getHistoryCount(); ++i) {
        for (int j = 0; j < getHistoryCount(); ++j) {
            if (histories[j].getName() == knowledge->getHistory(i)->getName()) {
                histories[j].incValue(knowledge->getHistory(i)->getValue());
                knownKnowledge = true;
            }
        }
        if (knownKnowledge == false) {
            CharDatum history;
            history.init(knowledge->getHistory(i)->getValue(), knowledge->getHistory(i)->getName());
            histories.push_back(history);
        }
        knownKnowledge = false;
    }
    for (int i = 0; i < knowledge->getCustomCount(); ++i) {
        for (int j = 0; j < getCustomCount(); ++j) {
            if (customs[j].getName() == knowledge->getCustom(i)->getName()) {
                customs[j].incValue(knowledge->getCustom(i)->getValue());
                knownKnowledge = true;
            }
        }
        if (knownKnowledge == false) {
            CharDatum custom;
            custom.init(knowledge->getCustom(i)->getValue(), knowledge->getCustom(i)->getName());
            customs.push_back(custom);
        }
        knownKnowledge = false;
    }
}

bool Knowledge::load(const XmlNode *baseNode) {
    bool loadOk = true;

	const XmlNode *creatorCostNode = baseNode->getChild("creator-cost", 0, false);
	if (creatorCostNode) {
        creatorCost.load(creatorCostNode);
	}

    const XmlNode *languagesNode = baseNode->getChild("languages", 0, false);
    if (languagesNode) {
        languages.resize(languagesNode->getChildCount());
        for (int i = 0; i < languagesNode->getChildCount(); ++i) {
            const XmlNode *languageNode = languagesNode->getChild("language");
            string name = languageNode->getAttribute("name")->getRestrictedValue();
            int value = languageNode->getAttribute("value")->getIntValue();
            languages[i].init(value, name);
        }
    }
    const XmlNode *historiesNode = baseNode->getChild("histories", 0, false);
    if (historiesNode) {
        histories.resize(historiesNode->getChildCount());
        for (int i = 0; i < historiesNode->getChildCount(); ++i) {
            const XmlNode *historyNode = historiesNode->getChild("history");
            string name = historyNode->getAttribute("name")->getRestrictedValue();
            int value = historyNode->getAttribute("value")->getIntValue();
            histories[i].init(value, name);
        }
    }
    const XmlNode *customsNode = baseNode->getChild("customs", 0, false);
    if (customsNode) {
        customs.resize(customsNode->getChildCount());
        for (int i = 0; i < customsNode->getChildCount(); ++i) {
            const XmlNode *customNode = customsNode->getChild("custom");
            string name = customNode->getAttribute("name")->getRestrictedValue();
            int value = customNode->getAttribute("value")->getIntValue();
            customs[i].init(value, name);
        }
    }
    return loadOk;
}

bool Knowledge::isEmpty() const {
    bool empty = true;
    if (languages.size() > 0 || histories.size() > 0 || customs.size() > 0) empty = false;
    return empty;
}

void Knowledge::save(XmlNode *node) const {
    XmlNode *n;
    if (languages.size() > 0) {
        n = node->addChild("languages");
        for (int i = 0; i < languages.size(); ++i) {
            languages[i].save(n->addChild("language"));
        }
    }
    if (histories.size() > 0) {
        n = node->addChild("histories");
        for (int i = 0; i < histories.size(); ++i) {
            histories[i].save(n->addChild("history"));
        }
    }
    if (customs.size() > 0) {
        n = node->addChild("customs");
        for (int i = 0; i < customs.size(); ++i) {
            customs[i].save(n->addChild("custom"));
        }
    }
}

void Knowledge::getDesc(string &str, const char *pre) {
    str += pre;
    str += "Knowledge:";
    for (int i = 0; i < languages.size(); ++i) {
        str += pre;
        str += "Language: ";
        languages[i].getDesc(str, pre);
    }
    for (int i = 0; i < histories.size(); ++i) {
        str += pre;
        str += "History: ";
        histories[i].getDesc(str, pre);
    }
    for (int i = 0; i < customs.size(); ++i) {
        str += pre;
        str += "Custom: ";
        customs[i].getDesc(str, pre);
    }
}

void Knowledge::getDesc(string &str, const char *pre) const {
    str += pre;
    str += "Knowledge:";
    for (int i = 0; i < languages.size(); ++i) {
        str += pre;
        str += "Language: ";
        languages[i].getDesc(str, pre);
    }
    for (int i = 0; i < histories.size(); ++i) {
        str += pre;
        str += "History: ";
        histories[i].getDesc(str, pre);
    }
    for (int i = 0; i < customs.size(); ++i) {
        str += pre;
        str += "Custom: ";
        customs[i].getDesc(str, pre);
    }
}

// ===============================
// 	class CharacterStats
// ===============================
void CharacterStats::reset() {
	conception.reset();
	might.reset();
	potency.reset();
	spirit.reset();
	awareness.reset();
	acumen.reset();
	authority.reset();
	finesse.reset();
	mettle.reset();
	fortitude.reset();
}

bool CharacterStats::isEmpty() const {
    bool empty = true;
	if (!conception.isEmpty()) empty = false;
	if (!might.isEmpty()) empty = false;
	if (!potency.isEmpty()) empty = false;
	if (!spirit.isEmpty()) empty = false;
	if (!awareness.isEmpty()) empty = false;
	if (!acumen.isEmpty()) empty = false;
	if (!authority.isEmpty()) empty = false;
	if (!finesse.isEmpty()) empty = false;
	if (!mettle.isEmpty()) empty = false;
	if (!fortitude.isEmpty()) empty = false;
	return empty;
}

void CharacterStats::clampMultipliers() {
	conception.clampMultipliers();
	might.clampMultipliers();
	potency.clampMultipliers();
	spirit.clampMultipliers();
	awareness.clampMultipliers();
	acumen.clampMultipliers();
	authority.clampMultipliers();
	finesse.clampMultipliers();
	mettle.clampMultipliers();
	fortitude.clampMultipliers();
}

void CharacterStats::doChecksum(Checksum &checksum) const {
	checksum.add(conception);
	checksum.add(might);
	checksum.add(potency);
	checksum.add(spirit);
	checksum.add(awareness);
	checksum.add(acumen);
	checksum.add(authority);
	checksum.add(finesse);
	checksum.add(mettle);
	checksum.add(fortitude);
}

bool CharacterStats::load(const XmlNode *baseNode, const string &dir) {
	bool loadOk = true;
	reset();

	const XmlNode *creatorCostNode = baseNode->getChild("creator-cost", 0, false);
	if (creatorCostNode) {
        creatorCost.load(creatorCostNode);
	}

	try {
        const XmlNode *conceptionNode = baseNode->getChild("conception", 0, false);
        if (conceptionNode) {
            if (!conception.load(conceptionNode)) {
                loadOk = false;
            }
        }
        const XmlNode *mightNode = baseNode->getChild("might", 0, false);
        if (mightNode) {
            if (!might.load(mightNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *potencyNode = baseNode->getChild("potency", 0, false);
        if (potencyNode) {
            if (!potency.load(potencyNode)) {
                loadOk = false;
            }
        }
        const XmlNode *spiritNode = baseNode->getChild("spirit", 0, false);
        if (spiritNode) {
            if (!spirit.load(spiritNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *awarenessNode = baseNode->getChild("awareness", 0, false);
        if (awarenessNode) {
            if (!awareness.load(awarenessNode)) {
                loadOk = false;
            }
        }
        const XmlNode *acumenNode = baseNode->getChild("acumen", 0, false);
        if (acumenNode) {
            if (!acumen.load(acumenNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *authorityNode = baseNode->getChild("authority", 0, false);
        if (authorityNode) {
            if (!authority.load(authorityNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *finesseNode = baseNode->getChild("finesse", 0, false);
        if (finesseNode) {
            if (!finesse.load(finesseNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *mettleNode = baseNode->getChild("mettle", 0, false);
        if (mettleNode) {
            if (!mettle.load(mettleNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	try {
        const XmlNode *fortitudeNode = baseNode->getChild("fortitude", 0, false);
        if (fortitudeNode) {
            if (!fortitude.load(fortitudeNode)) {
                loadOk = false;
            }
        }
    }
    catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

void CharacterStats::save(XmlNode *node) const {
    if (!conception.isEmpty()) {
        conception.save(node->addChild("conception"));
    }
    if (!might.isEmpty()) {
        might.save(node->addChild("might"));
    }
    if (!potency.isEmpty()) {
        potency.save(node->addChild("potency"));
    }
    if (!spirit.isEmpty()) {
        spirit.save(node->addChild("spirit"));
    }
    if (!awareness.isEmpty()) {
        awareness.save(node->addChild("awareness"));
    }
    if (!acumen.isEmpty()) {
        acumen.save(node->addChild("acumen"));
    }
    if (!authority.isEmpty()) {
        authority.save(node->addChild("authority"));
    }
    if (!finesse.isEmpty()) {
        finesse.save(node->addChild("finesse"));
    }
    if (!mettle.isEmpty()) {
        mettle.save(node->addChild("mettle"));
    }
    if (!fortitude.isEmpty()) {
        fortitude.save(node->addChild("fortitude"));
    }
}

void CharacterStats::addStatic(const CharacterStats *cs, fixed strength) {
    int newConception = (cs->getConception()->getValue() * strength).intp();
	conception.incValue(newConception);
	int newMight = (cs->getMight()->getValue() * strength).intp();
	might.incValue(newMight);
	int newPotency = (cs->getPotency()->getValue() * strength).intp();
	potency.incValue(newPotency);
	int newSpirit = (cs->getSpirit()->getValue() * strength).intp();
	spirit.incValue(newSpirit);
	int newAwareness = (cs->getAwareness()->getValue() * strength).intp();
	awareness.incValue(newAwareness);
	int newAcumen = (cs->getAcumen()->getValue() * strength).intp();
	acumen.incValue(newAcumen);
	int newAuthority = (cs->getAuthority()->getValue() * strength).intp();
	authority.incValue(newAuthority);
	int newFinesse = (cs->getFinesse()->getValue() * strength).intp();
	finesse.incValue(newFinesse);
	int newMettle = (cs->getMettle()->getValue() * strength).intp();
	mettle.incValue(newMettle);
	int newFortitude = (cs->getFortitude()->getValue() * strength).intp();
	fortitude.incValue(newFortitude);
}

void CharacterStats::addMultipliers(const CharacterStats *cs, fixed strength) {
	conception.incValueMult((cs->getConception()->getValueMult() - 1) * strength);
	might.incValueMult((cs->getMight()->getValueMult() - 1) * strength);
	potency.incValueMult((cs->getPotency()->getValueMult() - 1) * strength);
	spirit.incValueMult((cs->getSpirit()->getValueMult() - 1) * strength);
	awareness.incValueMult((cs->getAwareness()->getValueMult() - 1) * strength);
	acumen.incValueMult((cs->getAcumen()->getValueMult() - 1) * strength);
	authority.incValueMult((cs->getAuthority()->getValueMult() - 1) * strength);
	finesse.incValueMult((cs->getFinesse()->getValueMult() - 1) * strength);
	mettle.incValueMult((cs->getMettle()->getValueMult() - 1) * strength);
	fortitude.incValueMult((cs->getFortitude()->getValueMult() - 1) * strength);
}

void CharacterStats::sanitiseCharacterStats() {
	conception.sanitiseStat(0);
	might.sanitiseStat(0);
	potency.sanitiseStat(0);
    spirit.sanitiseStat(-1);
	awareness.sanitiseStat(0);
	acumen.sanitiseStat(0);
	authority.sanitiseStat(0);
	finesse.sanitiseStat(0);
	mettle.sanitiseStat(0);
	fortitude.sanitiseStat(0);
}

void CharacterStats::applyMultipliers(const CharacterStats *cs) {
    int newConception = (conception.getValue() * cs->getConception()->getValueMult()).intp();
	conception.setValue(newConception);
	int newMight = (might.getValue() * cs->getMight()->getValueMult()).intp();
	might.setValue(newMight);
    int newPotency = (potency.getValue() * cs->getPotency()->getValueMult()).intp();
	potency.setValue(newPotency);
	int newSpirit = (spirit.getValue() * cs->getSpirit()->getValueMult()).intp();
	spirit.setValue(newSpirit);
    int newAwareness = (awareness.getValue() * cs->getAwareness()->getValueMult()).intp();
	awareness.setValue(newAwareness);
	int newAcumen = (acumen.getValue() * cs->getAcumen()->getValueMult()).intp();
	acumen.setValue(newAcumen);
    int newAuthority = (authority.getValue() * cs->getAuthority()->getValueMult()).intp();
	authority.setValue(newAuthority);
    int newFinesse = (finesse.getValue() * cs->getFinesse()->getValueMult()).intp();
	finesse.setValue(newFinesse);
	int newMettle = (mettle.getValue() * cs->getMettle()->getValueMult()).intp();
	mettle.setValue(newMettle);
    int newFortitude = (fortitude.getValue() * cs->getFortitude()->getValueMult()).intp();
	fortitude.setValue(newFortitude);
}

void CharacterStats::getDesc(string &str, const char *pre) {
	conception.getDesc(str, "", "Conception");
	might.getDesc(str, pre, "Might");
	potency.getDesc(str, pre, "Potency");
	spirit.getDesc(str, pre, "Spirit");
	awareness.getDesc(str, pre, "Awareness");
	acumen.getDesc(str, pre, "Acumen");
	authority.getDesc(str, pre, "Authority");
	finesse.getDesc(str, pre, "Finesse");
	mettle.getDesc(str, pre, "Mettle");
	fortitude.getDesc(str, pre, "Fortitude");
}

void CharacterStats::getDesc(string &str, const char *pre) const {
	conception.getDesc(str, "", "Conception");
	might.getDesc(str, pre, "Might");
	potency.getDesc(str, pre, "Potency");
	spirit.getDesc(str, pre, "Spirit");
	awareness.getDesc(str, pre, "Awareness");
	acumen.getDesc(str, pre, "Acumen");
	authority.getDesc(str, pre, "Authority");
	finesse.getDesc(str, pre, "Finesse");
	mettle.getDesc(str, pre, "Mettle");
	fortitude.getDesc(str, pre, "Fortitude");
}

}}//end namespace

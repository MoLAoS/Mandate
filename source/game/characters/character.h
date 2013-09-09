// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_CHARACTER_H_
#define _GLEST_GAME_CHARACTER_H_

#include "pch.h"
#include "crafting.h"
#include "statistics.h"
#include "abilities.h"
#include "actions.h"
#include "effect_type.h"

namespace Glest{ namespace ProtoTypes {
// =====================================================
// 	class CharacterStats
// =====================================================
class CharacterStats {
private:
    Stat conception;
    Stat might;
    Stat potency;
    Stat spirit;
    Stat awareness;
    Stat acumen;
    Stat authority;
    Stat finesse;
    Stat mettle;
    Stat fortitude;
    Stat creatorCost;
public:
    const Stat *getCreatorCost() const {return &creatorCost;}

	CharacterStats() { reset(); }
	virtual ~CharacterStats() {}

	virtual void doChecksum(Checksum &checksum) const;

	const Stat *getConception() const		{return &conception;}
	const Stat *getMight() const			{return &might;}
	const Stat *getPotency() const			{return &potency;}
	const Stat *getSpirit() const			{return &spirit;}
	const Stat *getAwareness() const		{return &awareness;}
	const Stat *getAcumen() const			{return &acumen;}
	const Stat *getAuthority() const		{return &authority;}
	const Stat *getFinesse() const			{return &finesse;}
	const Stat *getMettle() const			{return &mettle;}
	const Stat *getFortitude() const		{return &fortitude;}

    void setConception(int v)	{conception.setValue(v);}
	void setMight(int v)		{might.setValue(v);}
	void setPotency(int v)		{potency.setValue(v);}
	void setSpirit(int v)		{spirit.setValue(v);}
	void setAwareness(int v)	{awareness.setValue(v);}
	void setAcumen(int v)		{acumen.setValue(v);}
	void setAuthority(int v)	{authority.setValue(v);}
	void setFinesse(int v)		{finesse.setValue(v);}
	void setMettle(int v)		{mettle.setValue(v);}
	void setFortitude(int v)	{fortitude.setValue(v);}

    void getDesc(string &str, const char *pre);
    void getDesc(string &str, const char *pre) const;

	void reset();
	void modify();
	bool isEmpty() const;
	bool load(const XmlNode *baseNode, const string &dir);
	void save(XmlNode *node) const;
	void addStatic(const CharacterStats *cs, fixed strength = 1);
	void addMultipliers(const CharacterStats *cs, fixed strength = 1);
	void applyMultipliers(const CharacterStats *cs);
	void clampMultipliers();
	void sanitiseCharacterStats();
	void sum(const CharacterStats *cStats) {
		addStatic(cStats);
		addMultipliers(cStats);
	}
};

typedef vector<CraftRes> CraftResources;
typedef vector<CraftItem> CraftItems;
typedef vector<Equipment> Equipments;

class CraftingStats {
private:
    CraftResources craftResources;
    CraftItems craftItems;
public:
    int getCraftResourceCount() {return craftResources.size();}
    CraftRes *getCraftResource(int i) {return &craftResources[i];}
    int getCraftItemCount() {return craftItems.size();}
    CraftItem *getCraftItem(int i) {return &craftItems[i];}
};

class CharDatum {
private:
    string name;
    int value;
    Stat creatorCost;
public:
    const Stat *getCreatorCost() const {return &creatorCost;}
    int getValue() {return value;}
    string getName() {return name;}
    int getValue() const {return value;}
    string getName() const {return name;}
    void init(int amount, string title);
    void incValue(int i) {value += i;}
    void save(XmlNode *node) const;
    void getDesc(string &str, const char *pre);
    void getDesc(string &str, const char *pre) const;
    bool load(const XmlNode *baseNode);
};

typedef vector<CharDatum> CharData;

class Knowledge {
private:
    CharData languages;
    CharData histories;
    CharData customs;
    Stat creatorCost;
public:
    const Stat *getCreatorCost() const {return &creatorCost;}
    int getLanguageCount() {return languages.size();}
    CharDatum *getLanguage(int i) {return &languages[i];}
    int getHistoryCount() {return histories.size();}
    CharDatum *getHistory(int i) {return &histories[i];}
    int getCustomCount() {return customs.size();}
    CharDatum *getCustom(int i) {return &customs[i];}
    bool load(const XmlNode *baseNode);
    void save(XmlNode *node) const;
    void reset();
    bool isEmpty() const;
    void sum(const Knowledge *knowledge);
    void getDesc(string &str, const char *pre);
    void getDesc(string &str, const char *pre) const;

    int getLanguageCount() const {return languages.size();}
    const CharDatum *getLanguage(int i) const {return &languages[i];}
    int getHistoryCount() const {return histories.size();}
    const CharDatum *getHistory(int i) const {return &histories[i];}
    int getCustomCount() const {return customs.size();}
    const CharDatum *getCustom(int i) const {return &customs[i];}
};

class CitizenNeeds {
private:
    CharData foods;
    CharData goods;
    CharData services;
public:
    int getFoodCount() {return foods.size();}
    CharDatum *getFood(int i) {return &foods[i];}
    int getGoodCount() {return goods.size();}
    CharDatum *getGood(int i) {return &goods[i];}
    int getServiceCount() {return services.size();}
    CharDatum *getService(int i) {return &services[i];}

    int getFoodCount() const {return foods.size();}
    const CharDatum *getFood(int i) const {return &foods[i];}
    int getGoodCount() const {return goods.size();}
    const CharDatum *getGood(int i) const {return &goods[i];}
    int getServiceCount() const {return services.size();}
    const CharDatum *getService(int i) const {return &services[i];}

    bool load(const XmlNode *baseNode);
    void save(XmlNode *node) const;
    void reset();
    bool isEmpty() const;
    void sum(const CitizenNeeds *citizenNeeds);
    void getDesc(string &str, const char *pre);
};

}}//end namespace

#endif

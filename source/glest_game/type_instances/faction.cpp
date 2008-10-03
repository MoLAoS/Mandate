// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "faction.h"

#include <algorithm>
#include <cassert>
#include <time.h>

#include "resource_type.h"
#include "unit.h"
#include "util.h"
#include "sound_renderer.h"
#include "renderer.h"
#include "tech_tree.h"
#include "world.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Game {

// =====================================================
// 	class Faction
// =====================================================
Faction::ResourceTypes Faction::neededResources;

void Faction::init(
	const FactionType *factionType, ControlType control, TechTree *techTree,
	int factionIndex, int teamIndex, int startLocationIndex, bool thisFaction)
{
	this->control= control;
	this->factionType= factionType;
	this->startLocationIndex= startLocationIndex;
	this->index= factionIndex;
	this->teamIndex= teamIndex;
	this->thisFaction= thisFaction;
	this->subfaction= 0;
	this->lastAttackNotice= 0;
	this->lastEnemyNotice= 0;
	lastEventLoc.x = -1.0f;		// -1 x indicates uninitialized, no last event

	resources.resize(techTree->getResourceTypeCount());
	store.resize(techTree->getResourceTypeCount());
	for(int i=0; i<techTree->getResourceTypeCount(); ++i){
		const ResourceType *rt= techTree->getResourceType(i);
		resources[i].init(rt, factionType->getStartingResourceAmount(rt));
		store[i].init(rt, 0);
	}

	texture= Renderer::getInstance().newTexture2D(rsGame);
	texture->load("data/core/faction_textures/faction"+intToStr(index)+".tga");
}

void Faction::load(const XmlNode *node, World *world, const FactionType *ft, ControlType control, TechTree *tt) {
	XmlNode *n;
	Map * map = world->getMap();

	this->factionType = ft;
	this->control = control;
	this->lastAttackNotice= 0;
	this->lastEnemyNotice= 0;

	index = node->getChildIntValue("index");
	teamIndex = node->getChildIntValue("teamIndex");
	startLocationIndex = node->getChildIntValue("startLocationIndex");
	thisFaction = node->getChildBoolValue("thisFaction");
	subfaction = node->getChildIntValue("subfaction");
	time_t lastAttackNotice = 0;
	time_t lastEnemyNotice = 0;
	lastEventLoc = node->getChildVec3fValue("lastEventLoc");

	upgradeManager.load(node->getChild("upgrades"), factionType);

	n = node->getChild("resources");
	resources.resize(n->getChildCount());
	store.resize(n->getChildCount());
	for(int i = 0; i < n->getChildCount(); ++i) {
		XmlNode *resourceNode = n->getChild("resource", i);
		const ResourceType *rt = tt->getResourceType(resourceNode->getChildStringValue("type"));
		resources[i].init(rt, resourceNode->getChildIntValue("amount"));
		store[i].init(rt, resourceNode->getChildIntValue("store"));
	}

	n = node->getChild("units");
	units.reserve(n->getChildCount());
	assert(units.empty() && unitMap.empty());
	for(int i = 0; i < n->getChildCount(); ++i) {
		new Unit(n->getChild("unit", i), this, map, tt);
//		addUnit(new Unit(n->getChild("unit", i), world, this, map, tt));
	}

	subfaction = node->getChildIntValue("subfaction"); //reset in case unit construction changed it
	texture= Renderer::getInstance().newTexture2D(rsGame);
	texture->load("data/core/faction_textures/faction"+intToStr(index)+".tga");
	assert(units.size() == unitMap.size());
}

void Faction::save(XmlNode *node) const {
	XmlNode *n;

	node->addChild("index", index);
	node->addChild("teamIndex", teamIndex);
	node->addChild("startLocationIndex", startLocationIndex);
	node->addChild("thisFaction", thisFaction);
	node->addChild("subfaction", subfaction);
	node->addChild("lastEventLoc", lastEventLoc);
	upgradeManager.save(node->addChild("upgrades"));

	n = node->addChild("resources");
	for(int i = 0; i < resources.size(); ++i) {
		assert(resources[i].getType() == store[i].getType());

		XmlNode *resourceNode = n->addChild("resource");
		resourceNode->addChild("type", resources[i].getType()->getName());
		resourceNode->addChild("amount", resources[i].getAmount());
		resourceNode->addChild("store", store[i].getAmount());
	}

	n = node->addChild("units");
	for(Units::const_iterator i = units.begin(); i != units.end(); i++) {
		(*i)->save(n->addChild("unit"));
	}
}

void Faction::writeUpdate(XmlNode *node) const {
	XmlNode *n;

	node->addChild("subfaction", subfaction);
	node->addChild("lastEventLoc", lastEventLoc);
	upgradeManager.save(node->addChild("upgrades"));

	n = node->addChild("resources");
	for(int i = 0; i < resources.size(); ++i) {
		assert(resources[i].getType() == store[i].getType());

		XmlNode *resourceNode = n->addChild("resource");
		resourceNode->addAttribute("type", resources[i].getType()->getName());
		resourceNode->addAttribute("amount", intToStr(resources[i].getAmount()));
		resourceNode->addAttribute("store", intToStr(store[i].getAmount()));
	}
}

void Faction::update(const XmlNode *node) {
	XmlNode *n;
	const TechTree *tt = World::getCurrWorld()->getTechTree();

	subfaction = node->getChildIntValue("subfaction");
	lastEventLoc = node->getChildVec3fValue("lastEventLoc");
	upgradeManager.load(node->getChild("upgrades"), factionType);

	n = node->getChild("resources");
	resources.resize(n->getChildCount());
	store.resize(n->getChildCount());
	for(int i = 0; i < n->getChildCount(); ++i) {
		XmlNode *resourceNode = n->getChild("resource", i);
		const ResourceType *rt = tt->getResourceType(resourceNode->getAttribute("type")->getValue());
		resources[i].init(rt, resourceNode->getAttribute("amount")->getIntValue());
		store[i].init(rt, resourceNode->getAttribute("store")->getIntValue());
	}
}

void Faction::end(){
	deleteValues(units.begin(), units.end());
}

// ================== get ==================

const Resource *Faction::getResource(const ResourceType *rt) const{
	for(int i=0; i<resources.size(); ++i){
		if(rt==resources[i].getType()){
			return &resources[i];
		}
	}
	assert(false);
	return NULL;
}

int Faction::getStoreAmount(const ResourceType *rt) const{
	for(int i=0; i<store.size(); ++i){
		if(rt==store[i].getType()){
			return store[i].getAmount();
		}
	}
	assert(false);
	return 0;
}

bool Faction::getCpuControl() const{
	return control==ctCpu || control==ctCpuUltra;
}


// ==================== upgrade manager ====================

void Faction::startUpgrade(const UpgradeType *ut){
	upgradeManager.startUpgrade(ut, index);
}

void Faction::cancelUpgrade(const UpgradeType *ut){
	upgradeManager.cancelUpgrade(ut);
}

void Faction::finishUpgrade(const UpgradeType *ut){
	upgradeManager.finishUpgrade(ut);

	for(int i = 0; i < getUnitCount(); ++i) {
		getUnit(i)->applyUpgrade(ut);
	}
}

// ==================== reqs ====================

//checks if all required units and upgrades are present
bool Faction::reqsOk(const RequirableType *rt) const {

	//required units
	for(int i = 0; i < rt->getUnitReqCount(); ++i) {
		bool found = false;

		for(int j = 0; j < getUnitCount(); ++j) {
			Unit *unit = getUnit(j);
			const UnitType *ut = unit->getType();

			if(rt->getUnitReq(i) == ut && unit->isOperative()) {
				found = true;
				break;
			}
		}

		if(!found) {
			return false;
		}
	}

	//required upgrades
	for(int i = 0; i < rt->getUpgradeReqCount(); ++i) {
		if(!upgradeManager.isUpgraded(rt->getUpgradeReq(i))) {
			return false;
		}
	}

	//available in subfaction
	return rt->isAvailableInSubfaction(subfaction);
}

bool Faction::reqsOk(const CommandType *ct) const{

	if(ct->getProduced()!=NULL && !reqsOk(ct->getProduced()) || !isAvailable(ct)){
		return false;
	}

	if(ct->getClass()==ccUpgrade){
		const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
		if(upgradeManager.isUpgradingOrUpgraded(uct->getProducedUpgrade())){
			return false;
		}
	}

	return reqsOk(static_cast<const RequirableType*>(ct));
}

//checks if the command should be displayed or not
bool Faction::isAvailable(const CommandType *ct) const {
	if(!ct->isAvailableInSubfaction(subfaction)) {
		return false;
	}

	//If this command is producing or building anything, we need to make sure
	//that producable is also available.
	switch(ct->getClass()) {
		case ccBuild:
			// we can display as long as one of these is available
			for(int i = 0; i < ((BuildCommandType*)ct)->getBuildingCount(); i++) {
				if(((BuildCommandType*)ct)->getBuilding(i)->isAvailableInSubfaction(subfaction)) {
					return true;
				}
			}
			return false;

		case ccProduce:
			return ((ProduceCommandType*)ct)->getProduced()->isAvailableInSubfaction(subfaction);

		case ccUpgrade:
			return ((UpgradeCommandType*)ct)->getProduced()->isAvailableInSubfaction(subfaction);

		case ccMorph:
			return ((MorphCommandType*)ct)->getProduced()->isAvailableInSubfaction(subfaction);

		default:
			return true;
	}
}

// ================== cost application ==================

//apply costs except static production (start building/production)
bool Faction::applyCosts(const ProducibleType *p){

	if(!checkCosts(p)){
		return false;
	}

	//for each unit cost spend it
    //pass 2, decrease resources, except negative static costs (ie: farms)
	for(int i=0; i<p->getCostCount(); ++i){
        const ResourceType *rt= p->getCost(i)->getType();
		int cost= p->getCost(i)->getAmount();
		if((cost>0 || rt->getClass()!=rcStatic) && rt->getClass()!=rcConsumable){
            incResourceAmount(rt, -(cost));
		}

    }
    return true;
}

//apply discount (when a morph ends)
void Faction::applyDiscount(const ProducibleType *p, int discount){
	//increase resources
	for(int i=0; i<p->getCostCount(); ++i){
		const ResourceType *rt= p->getCost(i)->getType();
        int cost= p->getCost(i)->getAmount();
		if((cost>0 || rt->getClass()!=rcStatic) && rt->getClass()!=rcConsumable){
            incResourceAmount(rt, cost*discount/100);
		}
    }
}

//apply static production (for starting units)
void Faction::applyStaticCosts(const ProducibleType *p){

	//decrease static resources
    for(int i=0; i<p->getCostCount(); ++i){
		const ResourceType *rt= p->getCost(i)->getType();
        if(rt->getClass()==rcStatic){
            int cost= p->getCost(i)->getAmount();
			if(cost>0){
				incResourceAmount(rt, -cost);
			}
        }
    }
}

//apply static production (when a mana source is done)
void Faction::applyStaticProduction(const ProducibleType *p){

	//decrease static resources
    for(int i=0; i<p->getCostCount(); ++i){
		const ResourceType *rt= p->getCost(i)->getType();
        if(rt->getClass()==rcStatic){
            int cost= p->getCost(i)->getAmount();
			if(cost<0){
				incResourceAmount(rt, -cost);
			}
        }
    }
}

//deapply all costs except static production (usually when a building is cancelled)
void Faction::deApplyCosts(const ProducibleType *p){

	//increase resources
	for(int i=0; i<p->getCostCount(); ++i){
		const ResourceType *rt= p->getCost(i)->getType();
        int cost= p->getCost(i)->getAmount();
		if((cost>0 || rt->getClass()!=rcStatic) && rt->getClass()!=rcConsumable){
            incResourceAmount(rt, cost);
		}

    }
}

//deapply static costs (usually when a unit dies)
void Faction::deApplyStaticCosts(const ProducibleType *p){

    //decrease resources
	for(int i=0; i<p->getCostCount(); ++i){
		const ResourceType *rt= p->getCost(i)->getType();
		if(rt->getClass()==rcStatic){
            int cost= p->getCost(i)->getAmount();
            incResourceAmount(rt, cost);
        }
    }
}

//apply resource on interval (cosumable resouces)
void Faction::applyCostsOnInterval(){

	//increment consumables
	for(int j=0; j<getUnitCount(); ++j){
		Unit *unit= getUnit(j);
		if(unit->isOperative()){
			for(int k=0; k<unit->getType()->getCostCount(); ++k){
				const Resource *resource= unit->getType()->getCost(k);
				if(resource->getType()->getClass()==rcConsumable && resource->getAmount()<0){
					incResourceAmount(resource->getType(), -resource->getAmount());
				}
			}
		}
	}

	//decrement consumables
	for(int j=0; j<getUnitCount(); ++j){
		Unit *unit= getUnit(j);
		if(unit->isOperative()){
			for(int k=0; k<unit->getType()->getCostCount(); ++k){
				const Resource *resource= unit->getType()->getCost(k);
				if(resource->getType()->getClass()==rcConsumable && resource->getAmount()>0){
					incResourceAmount(resource->getType(), -resource->getAmount());

					//decrease unit hp
					if(getResource(resource->getType())->getAmount()<0){
						resetResourceAmount(resource->getType());
						//FIXME unreleased pets may occur here
						unit->decHp(unit->getType()->getMaxHp()/3);
						StaticSound *sound= unit->getType()->getFirstStOfClass(scDie)->getSound();
						if(sound!=NULL && thisFaction){
							SoundRenderer::getInstance().playFx(sound);
						}
					}
				}
			}
		}
	}
}

bool Faction::checkCosts(const ProducibleType *pt) {
	bool ok = true;
	neededResources.clear();

	//for each unit cost check if enough resources
	for(int i = 0; i < pt->getCostCount(); ++i) {
		const ResourceType *rt = pt->getCost(i)->getType();
		int cost = pt->getCost(i)->getAmount();

		if(cost > 0) {
			int available = getResource(rt)->getAmount();

			if(cost > available) {
				ok = false;
				neededResources.push_back(rt);
			}
		}
	}

	return ok;
}

// ================== diplomacy ==================

bool Faction::isAlly(const Faction *faction){
	return teamIndex == faction->getTeam();
}

// ================== misc ==================

void Faction::incResourceAmount(const ResourceType *rt, int amount){
	for(int i=0; i<resources.size(); ++i){
		Resource *r= &resources[i];
		if(r->getType()==rt){
			r->setAmount(r->getAmount()+amount);
			if(r->getType()->getClass()!=rcStatic && r->getAmount()>getStoreAmount(rt)){
				r->setAmount(getStoreAmount(rt));
			}
			return;
		}
	}
	assert(false);
}

void Faction::setResourceBalance(const ResourceType *rt, int balance){
	for(int i=0; i<resources.size(); ++i){
		Resource *r= &resources[i];
		if(r->getType()==rt){
			r->setBalance(balance);
			return;
		}
	}
	assert(false);
}

void Faction::addUnit(Unit *unit){
	units.push_back(unit);
	unitMap.insert(make_pair(unit->getId(), unit));
}

void Faction::removeUnit(Unit *unit){
	for(int i=0; i<units.size(); ++i){
		if(units[i]==unit){
			units.erase(units.begin()+i);
			unitMap.erase(unit->getId());
			assert(units.size()==unitMap.size());
			return;
		}
	}
	assert(false);
}

void Faction::addStore(const UnitType *unitType){
	for(int i=0; i<unitType->getStoredResourceCount(); ++i){
		const Resource *r= unitType->getStoredResource(i);
		for(int j=0; j<store.size(); ++j){
			Resource *storedResource= &store[j];
			if(storedResource->getType() == r->getType()){
				storedResource->setAmount(storedResource->getAmount() + r->getAmount());
			}
		}
	}
}

void Faction::removeStore(const UnitType *unitType){
	for(int i=0; i<unitType->getStoredResourceCount(); ++i){
		const Resource *r= unitType->getStoredResource(i);
		for(int j=0; j<store.size(); ++j){
			Resource *storedResource= &store[j];
			if(storedResource->getType() == r->getType()){
				storedResource->setAmount(storedResource->getAmount() - r->getAmount());
			}
		}
	}
	limitResourcesToStore();
}

void Faction::limitResourcesToStore(){
	for(int i=0; i<resources.size(); ++i){
		Resource *r= &resources[i];
		Resource *s= &store[i];
		if(r->getType()->getClass()!=rcStatic && r->getAmount()>s->getAmount()){
			r->setAmount(s->getAmount());
		}
	}
}

void Faction::resetResourceAmount(const ResourceType *rt){
	for(int i=0; i<resources.size(); ++i){
		if(resources[i].getType()==rt){
			resources[i].setAmount(0);
			return;
		}
	}
	assert(false);
}

/**
 * Plays an auditory notification of being attacked, if it hasn't been done
 * recently
 */
void Faction::attackNotice(const Unit *u) {
	if(thisFaction && factionType->getAttackNotice()) {
		time_t curTime;
		time(&curTime);
		if(curTime >= lastAttackNotice + factionType->getAttackNoticeDelay()) {
			lastAttackNotice = curTime;
			lastEventLoc = u->getCurrVector();
			StaticSound *sound= factionType->getAttackNotice()->getRandSound();
			if(sound){
				SoundRenderer::getInstance().playFx(sound);
			}
		}
	}
}
void Faction::advanceSubfaction(int subfaction) {
	this->subfaction = subfaction;
	//FIXME: We should probably play a sound, display a banner-type notice or
	//something.
}

}}//end namespace

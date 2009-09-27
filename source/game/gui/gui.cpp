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
#include "gui.h"

#include <cassert>
#include <algorithm>

#include "world.h"
#include "renderer.h"
#include "game.h"
#include "upgrade.h"
#include "unit.h"
#include "metrics.h"
#include "display.h"
#include "platform_util.h"
#include "sound_renderer.h"
#include "util.h"
#include "faction.h"
#include "xml_parser.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;
using namespace Shared::Util;
using Shared::Xml::XmlNode;

namespace Glest{ namespace Game{

// =====================================================
// 	class Mouse3d
// =====================================================

const float Mouse3d::fadeSpeed= 1.f/50.f;

Mouse3d::Mouse3d(){
	enabled = false;
	rot = 0;
	fade = 0.f;
}

void Mouse3d::show(Vec2i pos) {
	enabled = true;
	fade = 0.f;
	this->pos = pos;
}

void Mouse3d::update() {
	if (enabled) {
		rot = (rot + 3) % 360;
		fade += fadeSpeed;
		if (fade > 1.f) {
			fade = 1.f;
			enabled = false;
		}
	}
}

// ===============================
// 	class SelectionQuad
// ===============================

SelectionQuad::SelectionQuad(){
	enabled= false;
	posDown= Vec2i(0);
	posUp= Vec2i(0);
}

void SelectionQuad::setPosDown(const Vec2i &posDown){
	enabled= true;
	this->posDown= posDown;
	this->posUp= posDown;
}

void SelectionQuad::setPosUp(const Vec2i &posUp){
	this->posUp= posUp;
}

void SelectionQuad::disable(){
	enabled= false;
}

// =====================================================
// 	class Gui
// =====================================================

Gui* Gui::currentGui = NULL;

//constructor
Gui::Gui(Game &game) : game(game), input(game.getInput()) {
	posObjWorld= Vec2i(54, 14);
	dragStartPos= Vec2i(0, 0);
	computeSelection= false;
	validPosObjWorld= false;
	activeCommandType= NULL;
	activeCommandClass= ccStop;
	selectingBuilding= false;
	selectingPos= false;
	selectingMeetingPoint= false;
	activePos= invalidPos;
	dragging = false;
	draggingMinimap = false;
	needSelectionUpdate = false;
	currentGui = this;
	currentGroup= invalidGroupIndex;
}

void Gui::init(){
	this->commander= game.getCommander();
	this->gameCamera= game.getGameCamera();
	this->console= game.getConsole();
	this->world= game.getWorld();
	buildPositions.reserve(max(world->getMap()->getH(), world->getMap()->getW()));
	selection.init(this, world->getThisFactionIndex());
}

void Gui::end(){
	selection.clear();
}

// ==================== get ====================

const UnitType *Gui::getBuilding() const{
	assert(selectingBuilding);
	return choosenBuildingType;
}

// ==================== is ====================

bool Gui::isPlacingBuilding() const{
	return isSelectingPos() && activeCommandType!=NULL && activeCommandType->getClass()==ccBuild;
}

// ==================== reset state ====================

void Gui::resetState(){
	selectingBuilding= false;
	selectingPos= false;
	selectingMeetingPoint= false;
	activePos= invalidPos;
	activeCommandClass= ccStop;
	activeCommandType= NULL;
	dragging = false;
	draggingMinimap = false;
	needSelectionUpdate = false;
	buildPositions.clear();
}

/** return true if the position is valid, false otherwise */
bool Gui::getMinimapCell(int x, int y, Vec2i &cell) {
	const Map *map= world->getMap();
	const Metrics &metrics= Metrics::getInstance();

	int xm= x - metrics.getMinimapX();
	int ym= y - metrics.getMinimapY();
	int xCell= static_cast<int>(xm * (static_cast<float>(map->getW()) / metrics.getMinimapW()));
	int yCell= static_cast<int>(map->getH() - ym * (static_cast<float>(map->getH()) / metrics.getMinimapH()));
	if(map->isInside(xCell, yCell)) {
		cell.x = xCell;
		cell.y = yCell;
		return true;
	} else {
		return false;
	}
}

static void calculateNearest(Selection::UnitContainer &units, const Vec3f &pos) {
	if(units.size() > 1) {
		float minDist = 100000.f;
		Unit *nearest = NULL;
		for(Selection::UnitContainer::const_iterator i = units.begin(); i != units.end(); ++i) {
			float dist = pos.dist((*i)->getCurrVector());
			if(minDist > dist) {
				minDist = dist;
				nearest = *i;
			}
		}
		units.clear();
		units.push_back(nearest);
	}
}

// ==================== events ====================
void Gui::mouseDownLeft(int x, int y) {
	const Metrics &metrics= Metrics::getInstance();
	Selection::UnitContainer units;
	const Unit *targetUnit= NULL;
	Vec2i worldPos;
	bool validWorldPos;

	//display panel
	if(metrics.isInDisplay(x, y)) {
		int posDisplay = computePosDisplay(x - metrics.getDisplayX(), y - metrics.getDisplayY());
		if(posDisplay != invalidPos){
			if(isSelectingPos()) {
				resetState();
			} else {
				mouseDownLeftDisplay(posDisplay);
			}
			return;
		}
	}

	// handle minimap click
	if(getMinimapCell(x, y, worldPos)) {
		if(isSelectingPos() && Config::getInstance().getUiEnableCommandMinimap()) {
			targetUnit = NULL;
			validWorldPos = true;
		} else {
			game.setCameraCell(worldPos.x, worldPos.y);
			draggingMinimap = true;
			return;
		}
	} else {
		validWorldPos = computeTarget(Vec2i(x, y), worldPos, units);
	}

	//graphics panel
	if(!validWorldPos){
		console->addStdMessage("InvalidPosition");
		return;
	}

	//remaining options will prefer the actual target unit's position
	if(units.size()) {
		targetUnit = units.front();
		worldPos = targetUnit->getPos();
	}

	if(selectingPos) {
		//give standard orders
		giveTwoClickOrders(worldPos, (Unit *)targetUnit);

	//set meeting point
	} else if(selectingMeetingPoint) {
		if(selection.isComandable()) {
			commander->tryGiveCommand(selection, CommandFlags(cpQueue, input.isShiftDown()), NULL,
					ccSetMeetingPoint, worldPos);
		}
		resetState();

	//begin drag-drop selection & update selection for single-click
	} else {
		selectionQuad.setPosDown(Vec2i(x, y));
		calculateNearest(units, gameCamera->getPos());
		updateSelection(false, units);
	}

	computeDisplay();
}

void Gui::mouseDownRight(int x, int y) {
	const Metrics &metrics= Metrics::getInstance();
	Vec2i worldPos;

	if(selectingPos || selectingMeetingPoint){
		resetState();
		computeDisplay();
		return;
	}

	//display panel
	if(metrics.isInDisplay(x, y)) {
		int posDisplay = computePosDisplay(x - metrics.getDisplayX(), y - metrics.getDisplayY());
		//ignore right click to display buttons, but do not pass on to graphics
		if(posDisplay != invalidPos) {
			return;
		}
	}

	// handle minimap click
	if(getMinimapCell(x, y, worldPos) && Config::getInstance().getUiEnableCommandMinimap()) {
		giveDefaultOrders(worldPos, NULL);
	} else if(selection.isComandable()) {
		Selection::UnitContainer units;
		if(computeTarget(Vec2i(x, y), worldPos, units)) {
			Unit *targetUnit = units.size() ? units.front() : NULL;
			giveDefaultOrders(targetUnit ? targetUnit->getPos() : worldPos, targetUnit);
		} else {
			console->addStdMessage("InvalidPosition");
		}
	}

	computeDisplay();
}

void Gui::mouseUpLeft(int x, int y) {
	mouseUpLeftGraphics(x, y);
	draggingMinimap = false;
}

void Gui::mouseUpRight(int x, int y) {
}

void Gui::mouseDoubleClickLeft(int x, int y) {
	const Metrics &metrics= Metrics::getInstance();

	//display panel
	if(metrics.isInDisplay(x, y)){
		int posDisplay = computePosDisplay(x - metrics.getDisplayX(), y - metrics.getDisplayY());
		if(posDisplay != invalidPos){
			return;
		}
	}

    //graphics panel
	mouseDoubleClickLeftGraphics(x, y);
}

void Gui::mouseDownLeftDisplay(int posDisplay){
	if(!selectingPos && !selectingMeetingPoint){
		if(posDisplay!= invalidPos){
			if(selection.isComandable()){
				if(selectingBuilding){
					mouseDownDisplayUnitBuild(posDisplay);
				}
				else{
					mouseDownDisplayUnitSkills(posDisplay);
				}
			}
			else{
				resetState();
			}
		}
		computeDisplay();
	} else {
		// if they clicked on a button again, they must have changed their mind
		resetState();
	}
}

void Gui::mouseMoveDisplay(int x, int y){
	computeInfoString(computePosDisplay(x, y));
}

void Gui::mouseUpLeftGraphics(int x, int y){
	if(!selectingPos && !selectingMeetingPoint && selectionQuad.isEnabled()){
		selectionQuad.setPosUp(Vec2i(x, y));
		if(selection.isComandable() && random.randRange(0, 1)){
			SoundRenderer::getInstance().playFx(
				selection.getFrontUnit()->getType()->getSelectionSound(),
				selection.getFrontUnit()->getCurrVector(),
				gameCamera->getPos());
		}
		selectionQuad.disable();
	} else if (isPlacingBuilding() && dragging) {
		Vec2i worldPos;
		Selection::UnitContainer units;

		//FIXME: we don't have to do the expensivce calculations of this computeTarget for this.
		if(computeTarget(Vec2i(x, y), worldPos, units)) {
			giveTwoClickOrders(worldPos, NULL);
		} else {
			console->addStdMessage("InvalidPosition");
		}
	}
}

void Gui::mouseMoveGraphics(int x, int y){
	Vec2i mmCell;

	if(draggingMinimap && getMinimapCell(x, y, mmCell)) {
		game.setCameraCell(mmCell.x, mmCell.y);
		return;
	}

	//compute selection
	if(selectionQuad.isEnabled()) {
		selectionQuad.setPosUp(Vec2i(x, y));
		Selection::UnitContainer units;
		if(computeSelection) {
			Renderer::getInstance().computeSelected(units, selectionQuad.getPosDown(), selectionQuad.getPosUp());
			computeSelection = false;
			updateSelection(false, units);
		}
	}

	//compute position for building
	if(isPlacingBuilding()){
		validPosObjWorld= Renderer::getInstance().computePosition(Vec2i(x,y), posObjWorld);

		if(!validPosObjWorld) {
			buildPositions.clear();
		} else {
			computeBuildPositions(posObjWorld);
		}
	}

	display.setInfoText("");
}

void Gui::mouseDoubleClickLeftGraphics(int x, int y){
	if(!selectingPos && !selectingMeetingPoint){
		Selection::UnitContainer units;
		Vec2i pos(x, y);

		selectionQuad.setPosDown(pos);
		Renderer::getInstance().computeSelected(units, pos, pos);
		calculateNearest(units, gameCamera->getPos());
		updateSelection(true, units);
		computeDisplay();
	}
}

void Gui::groupKey(int groupIndex){
	if (input.isCtrlDown()) {
		selection.assignGroup(groupIndex);
	} else {
		if(currentGroup == groupIndex){
			centerCameraOnSelection();
		}

		selection.recallGroup(groupIndex);

		currentGroup= groupIndex;
	}
}

void Gui::hotKey(UserCommand cmd) {
	int f = 0;
	switch(cmd) {
 	// goto selection
	case ucCameraGotoSelection:
		centerCameraOnSelection();
		break;

	// goto last event
	case ucCameraGotoLastEvent:
		centerCameraOnLastEvent();
		break;

 	// select idle harvester
	case ucSelectNextIdleHarvester:
		selectInterestingUnit(iutIdleHarvester);
		break;

 	// select idle builder
	case ucSelectNextIdleBuilder:
		selectInterestingUnit(iutIdleBuilder);
		break;

 	// select idle repairing/healing unit
	case ucSelectNextIdleRepairer:
		selectInterestingUnit(iutIdleRepairer);
		break;

 	// select idle worker (can either build, repair, or harvest)
	case ucSelectNextIdleWorker:
		selectInterestingUnit(iutIdleWorker);
		break;

 	// select idle non-hp restoration-skilled unit
	case ucSelectNextIdleRestorer:
		selectInterestingUnit(iutIdleRestorer);
		break;

 	// select idle producer
	case ucSelectNextIdleProducer:
		selectInterestingUnit(iutIdleProducer);
		break;

 	// select idle or non-idle producer
	case ucSelectNextProducer:
		selectInterestingUnit(iutProducer);
		break;

 	// select damaged unit
	case ucSelectNextDamaged:
		selectInterestingUnit(iutDamaged);
		break;

 	// select building (completed)
	case ucSelectNextBuiltBuilding:
		selectInterestingUnit(iutBuiltBuilding);
		break;

 	// select storeage unit
	case ucSelectNextStore:
		selectInterestingUnit(iutStore);
		break;

 	// Attack
	case ucAttack:
		clickCommonCommand(ccAttack);
		break;

	// Stop
	case ucStop:
		clickCommonCommand(ccStop);

 	// Move
	case ucMove:
		clickCommonCommand(ccMove);
		break;

 	// Repair / Heal / Replenish
	case ucReplenish:
		clickCommonCommand(ccRepair);
		break;	

 	// Guard
	case ucGuard:
		clickCommonCommand(ccGuard);
		break;

 	// Follow
	case ucFollow:
		//clickCommonCommand();
		break;

 	// Patrol
	case ucPatrol:
		//clickCommonCommand(ccPatrol);
		break;
#ifdef _GAE_DEBUG_EDITION_
	case ucSwitchDebugField:
		f = (int)Renderer::getInstance().getDebugField ();
		f ++; f %= FieldCount;
		Renderer::getInstance().setDebugField ( (Field)f );
		break;
#endif
	default:
		break;
	}
}

bool Gui::cancelPending() {
	if(selectingPos || selectingMeetingPoint){
		resetState();
		return true;
	} else if(!selection.isEmpty()) {
		selection.clear();
		return true;
	} else {
		return false;
	}
}

void Gui::load(const XmlNode *node) {
	selection.load(node->getChild("selection"));
	gameCamera->load(node->getChild("camera"));
}

void Gui::save(XmlNode *node) const {
	selection.save(node->addChild("selection"));
	gameCamera->save(node->addChild("camera"));
}


// ================= PRIVATE =================

void Gui::giveOneClickOrders() {
	CommandResult result;
	CommandFlags flags(cpQueue, input.isShiftDown());

	if (selection.isUniform()) {
		result = commander->tryGiveCommand(selection, flags, activeCommandType);
	} else {
		result = commander->tryGiveCommand(selection, flags, NULL, activeCommandClass);
	}

	addOrdersResultToConsole(activeCommandClass, result);

	activeCommandType = NULL;
	activeCommandClass = ccStop;
}

void Gui::giveDefaultOrders(const Vec2i &targetPos, Unit *targetUnit) {

	//give order
	CommandResult result = commander->tryGiveCommand(selection,
			CommandFlags(cpQueue, input.isShiftDown()), NULL, ccNull, targetPos, targetUnit);

	//graphical result
	addOrdersResultToConsole(activeCommandClass, result);
	if(result == crSuccess || result == crSomeFailed) {
		posObjWorld = targetPos;
		mouse3d.show(targetPos);

		if(random.randRange(0, 1)==0){
			SoundRenderer::getInstance().playFx(
				selection.getFrontUnit()->getType()->getCommandSound(),
				selection.getFrontUnit()->getCurrVector(),
				gameCamera->getPos());
		}
	}

	//reset
	resetState();
}

void Gui::giveTwoClickOrders(const Vec2i &targetPos, Unit *targetUnit) {

	CommandResult result;
	CommandFlags flags(cpQueue, input.isShiftDown());

	//give orders to the units of this faction

	if(!selectingBuilding) {

		if(selection.isUniform()) {
			result = commander->tryGiveCommand(selection, flags, activeCommandType, ccNull, targetPos, targetUnit);
		} else {
			result = commander->tryGiveCommand(selection, flags, NULL, activeCommandClass, targetPos, targetUnit);
		}
	} else {
		//selecting building
		assert(isPlacingBuilding());

		//if this is a drag&drop then start dragging and wait for mouse up

		if(choosenBuildingType->isMultiBuild() && !dragging) {
			dragging = true;
			dragStartPos = posObjWorld;
			return;
		}

		computeBuildPositions(posObjWorld);
		bool firstBuildPosition = true;

		BuildPositions::const_iterator i;
		for(i = buildPositions.begin(); i != buildPositions.end(); i++) {
			flags.set(cpQueue, input.isShiftDown() || !firstBuildPosition);
			flags.set(cpDontReserveResources, selection.getCount() > 1 || !firstBuildPosition);
			result = commander->tryGiveCommand(selection, flags, activeCommandType, ccNull, *i, NULL, choosenBuildingType);
		}
	}

	//graphical result
	addOrdersResultToConsole(activeCommandClass, result);

	if(result == crSuccess || result == crSomeFailed) {
		mouse3d.show(targetPos);

		if(random.randRange(0, 1) == 0) {
			SoundRenderer::getInstance().playFx(
				selection.getFrontUnit()->getType()->getCommandSound(),
				selection.getFrontUnit()->getCurrVector(),
				gameCamera->getPos());
		}
	}

	resetState();
}

void Gui::centerCameraOnSelection(){
	if(!selection.isEmpty()){
		Vec3f refPos= selection.getRefPos();
		gameCamera->centerXZ(refPos.x, refPos.z);
	}
}

void Gui::centerCameraOnLastEvent(){
	Vec3f lastEventLoc = world->getThisFaction()->getLastEventLoc();
	if(!(lastEventLoc.x == -1.0f)){
		gameCamera->centerXZ(lastEventLoc.x, lastEventLoc.z);
	}
}

void Gui::selectInterestingUnit(InterestingUnitType iut){
	const Faction* thisFaction= world->getThisFaction();
	const Unit* previousUnit= NULL;
	bool previousFound= true;

	//start at the next harvester
	if(selection.getCount()==1){
		const Unit* refUnit= selection.getFrontUnit();

		if(refUnit->isInteresting(iut)){
			previousUnit= refUnit;
			previousFound= false;
		}
	}

	//clear selection
	selection.clear();

	//search
	for(int i= 0; i<thisFaction->getUnitCount(); ++i){
		Unit* unit= thisFaction->getUnit(i);

		if(previousFound){
			if(unit->isInteresting(iut)){
				selection.select(unit);
				break;
			}
		}
		else{
			if(unit==previousUnit){
				previousFound= true;
			}
		}
	}

	//search again if we have a previous
	if(selection.isEmpty() && previousUnit!=NULL && previousFound==true) {
		for(int i= 0; i<thisFaction->getUnitCount(); ++i) {
			Unit* unit= thisFaction->getUnit(i);

			if(unit->isInteresting(iut)) {
				selection.select(unit);
				break;
			}
		}
	}
}

void Gui::clickCommonCommand(CommandClass commandClass) {
	for(int i= 0; i<Display::downCellCount; ++i) {
		const CommandType* ct = display.getCommandType(i);
		if(((ct && ct->getClass() == commandClass) || display.getCommandClass(i) == commandClass)
				&& display.getDownLighted(i)) {
			mouseDownDisplayUnitSkills(i);
			break;
		}
	}
}

void Gui::mouseDownDisplayUnitSkills(int posDisplay) {
	if(selection.isEmpty()) {
		return;
	}
	if(posDisplay == cancelPos) {
		if(isPlacingBuilding()) {
			resetState();
		} else if(selection.isCancelable()) {
			commander->tryCancelCommand(&selection);
		} else {
			// not our click
		}
	} else if(posDisplay == autoRepairPos) {
		bool newState = selection.getAutoRepairState() == arsOn ? false : true;
		commander->trySetAutoRepairEnabled(selection,
				CommandFlags(cpQueue, input.isShiftDown()), newState);
	} else if(posDisplay == meetingPointPos) {
		activePos= posDisplay;
		selectingMeetingPoint= true;
	} else {
		const Unit *unit= selection.getFrontUnit();

		//uniform selection
		if(selection.isUniform()) {
			if(unit->getFaction()->reqsOk(display.getCommandType(posDisplay))){
				activeCommandType = display.getCommandType(posDisplay);
				activeCommandClass = activeCommandType->getClass();
			}
			else{
				posDisplay = invalidPos;
				activeCommandType = NULL;
				activeCommandClass = ccStop;
				return;
			}
		}

		//non uniform selection
		else{
			activeCommandType= NULL;
			activeCommandClass= display.getCommandClass(posDisplay);
		}

		//give orders depending on command type
		if(!selection.isEmpty()){
			//selection.getUnit(0)->getType()->getFirstCtOfClass(activeCommandClass);
			const CommandType *ct= selection.getUnit(0)->getFirstAvailableCt(activeCommandClass);
			if(activeCommandType!=NULL && activeCommandType->getClass()==ccBuild){
				assert(selection.isUniform());
				selectingBuilding= true;
			}
			else if(ct->getClicks()==cOne) {
				invalidatePosObjWorld();
				giveOneClickOrders();
			}
			else{
				selectingPos= true;
				activePos= posDisplay;
			}
		}
	}
}

void Gui::mouseDownDisplayUnitBuild(int posDisplay){
	int factionIndex= world->getThisFactionIndex();

	if(posDisplay == cancelPos){
		resetState();
	}
	else{
		if(activeCommandType!=NULL && activeCommandType->getClass()==ccBuild){
			const BuildCommandType *bct= static_cast<const BuildCommandType*>(activeCommandType);
			const UnitType *ut= bct->getBuilding(display.getIndex(posDisplay));
			if(world->getFaction(factionIndex)->reqsOk(ut)){
				choosenBuildingType= ut;
				assert(choosenBuildingType!=NULL);
				selectingPos= true;
				activePos= posDisplay;
			}
		}
	}
}

void Gui::computeInfoString(int posDisplay) {
	Lang &lang = Lang::getInstance();

	display.setInfoText("");

	if(!selection.isComandable()) {
		return;
	}

	if(posDisplay == invalidPos) {
		return;
	}

	if(!selectingBuilding) {
		if(posDisplay == cancelPos) {
			display.setInfoText(lang.get("Cancel"));
		} else if(posDisplay == meetingPointPos) {
			display.setInfoText(lang.get("MeetingPoint"));
		} else if(posDisplay == autoRepairPos) {
			string str = lang.get("AutoRepair");

			switch (selection.getAutoRepairState()) {
			case arsOn:
				str += " " + lang.get("On");
				break;

			case arsOff:
				str += " " + lang.get("Off");
				break;

			case arsMixed:
				str += " " + lang.get("Mixed");
				break;
			}

			display.setInfoText(str);
		} else {
			//uniform selection
			if(selection.isUniform()) {
				const Unit *unit = selection.getFrontUnit();
				const CommandType *ct = display.getCommandType(posDisplay);

				if(ct != NULL) {
					if(unit->getFaction()->reqsOk(ct)) {
						display.setInfoText(ct->getDesc(unit));
					} else {
						if(ct->getClass() == ccUpgrade) {
							const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(ct);

							if(unit->getFaction()->getUpgradeManager()->isUpgrading(uct->getProducedUpgrade())) {
								display.setInfoText(lang.get("Upgrading") + "\n" + ct->getDesc(unit));
							} else if(unit->getFaction()->getUpgradeManager()->isUpgraded(uct->getProducedUpgrade())) {
								display.setInfoText(lang.get("AlreadyUpgraded") + "\n" + ct->getDesc(unit));
							} else {
								display.setInfoText(ct->getReqDesc());
							}
						} else {
							display.setInfoText(ct->getReqDesc());
						}
					}
				}
			}

			//non uniform selection
			else {
				const UnitType *ut = selection.getFrontUnit()->getType();
				CommandClass cc = display.getCommandClass(posDisplay);

				if(cc != ccNull) {
					display.setInfoText(lang.get("CommonCommand") + ": " + ut->getFirstCtOfClass(cc)->toString());
				}
			}
		}
	} else {
		if(posDisplay == cancelPos) {
			display.setInfoText(lang.get("Return"));
		} else {
			if(activeCommandType != NULL && activeCommandType->getClass() == ccBuild) {
				const BuildCommandType *bct = static_cast<const BuildCommandType*>(activeCommandType);
				display.setInfoText(bct->getBuilding(display.getIndex(posDisplay))->getReqDesc());
			}
		}
 	}
}

void Gui::computeDisplay() {

	//init
	display.clear();

	// ================ PART 1 ================

	int thisTeam = Game::getInstance()->getWorld()->getThisTeamIndex();
	//title, text and progress bar
	if (selection.getCount() == 1) {
		display.setTitle(selection.getFrontUnit()->getFullName());
		//FIXME maybe do a 'partial' getDesc() showing HP of enemy unit
		if ( selection.getFrontUnit()->getFaction()->getTeam() == thisTeam ) {
			display.setText(selection.getFrontUnit()->getDesc());
			display.setProgressBar(selection.getFrontUnit()->getProductionPercent());
		}
	}

	//portraits
	for (int i = 0; i < selection.getCount(); ++i) {
		display.setUpImage(i, selection.getUnit(i)->getType()->getImage());
	}

	// ================ PART 2 ================

	if (selectingPos || selectingMeetingPoint) {
		display.setDownSelectedPos(activePos);
	}

	if (selection.isComandable() 
	&& selection.getFrontUnit()->getFaction()->getTeam() == thisTeam ) {
		if (!selectingBuilding) {

			//cancel button
			const Unit *u = selection.getFrontUnit();
			const UnitType *ut = u->getType();
			if (selection.isCancelable()) {
				display.setDownImage(cancelPos, ut->getCancelImage());
				display.setDownLighted(cancelPos, true);
			}

			//meeting point
			if (selection.isMeetable()) {
				display.setDownImage(meetingPointPos, ut->getMeetingPointImage());
				display.setDownLighted(meetingPointPos, true);
			}

			if (selection.isCanRepair()) {
				if (selection.getAutoRepairState() == arsOn) {
					const CommandType *rct = ut->getFirstCtOfClass(ccRepair);
					assert(rct);
					display.setDownImage(autoRepairPos, rct->getImage());
				} else {
					display.setDownImage(autoRepairPos, ut->getCancelImage());
				}
				display.setDownLighted(autoRepairPos, true);
			}

			if (selection.isUniform()) {
				//uniform selection
				if (u->isBuilt()) {
					int morphPos = 8;
					for (int i = 0, j = 0; i < ut->getCommandTypeCount(); ++i) {
						const CommandType *ct = ut->getCommandType(i);
						int displayPos = ct->getClass() == ccMorph ? morphPos++ : j;
						if (u->getFaction()->isAvailable(ct) && ct->getClass() != ccSetMeetingPoint) {
							display.setDownImage(displayPos, ct->getImage());
							display.setCommandType(displayPos, ct);
							display.setDownLighted(displayPos, u->getFaction()->reqsOk(ct));
							++j;
						}
					}
				}
			} else {

				//non uniform selection
				int lastCommand = 0;
				for (int i = 0; i < ccCount; ++i) {
					CommandClass cc = static_cast<CommandClass>(i);
					if (isSharedCommandClass(cc) && cc != ccBuild) {
						display.setDownLighted(lastCommand, true);
						display.setDownImage(lastCommand, ut->getFirstCtOfClass(cc)->getImage());
						display.setCommandClass(lastCommand, cc);
						lastCommand++;
					}
				}
			}
		} else {

			//selecting building
			const Unit *unit = selection.getFrontUnit();
			if (activeCommandType != NULL && activeCommandType->getClass() == ccBuild) {
				const BuildCommandType* bct = static_cast<const BuildCommandType*>(activeCommandType);
				for (int i = 0, j = 0; i < bct->getBuildingCount(); ++i) {
					if (unit->getFaction()->isAvailable(bct->getBuilding(i))) {
						display.setDownImage(j, bct->getBuilding(i)->getImage());
						display.setDownLighted(j, unit->getFaction()->reqsOk(bct->getBuilding(i)));
						display.setIndex(j, i);
						++j;
					}
				}
				display.setDownImage(cancelPos, selection.getFrontUnit()->getType()->getCancelImage());
				display.setDownLighted(cancelPos, true);
			}
		}
	}
}

int Gui::computePosDisplay(int x, int y) {
	int posDisplay = display.computeDownIndex(x, y);

	if (posDisplay < 0 || posDisplay >= Display::downCellCount) {
		posDisplay = invalidPos;
	} else if (selection.isComandable()) {
		if (posDisplay == cancelPos) {
			//check cancel button
			if (!selection.isCancelable() && !selectingBuilding) {
				posDisplay = invalidPos;
			}
		} else if (posDisplay == meetingPointPos) {
			//check meeting point
			if (!selection.isMeetable()) {
				posDisplay = invalidPos;
			}
		} else if (posDisplay == autoRepairPos) {
			if (!selection.isCanRepair()) {
				posDisplay = invalidPos;
			}
		} else {
			if (!selectingBuilding) {
				//standard selection
				if (display.getCommandClass(posDisplay) == ccNull && display.getCommandType(posDisplay) == NULL) {
					posDisplay = invalidPos;
				}
			} else {
				//building selection
				if (activeCommandType != NULL && activeCommandType->getClass() == ccBuild) {
					const BuildCommandType *bct = static_cast<const BuildCommandType*>(activeCommandType);
					if (posDisplay >= bct->getBuildingCount()) {
						posDisplay = invalidPos;
					}
				}
			}
		}
	} else {
		posDisplay = invalidPos;
	}

	return posDisplay;
}

void Gui::addOrdersResultToConsole(CommandClass cc, CommandResult result) {

	switch(result){
	case crSuccess:
		break;
	case crFailReqs:
		switch(cc){
		case ccBuild:
			console->addStdMessage("BuildingNoReqs");
			break;
		case ccProduce:
			console->addStdMessage("UnitNoReqs");
			break;
		case ccUpgrade:
			console->addStdMessage("UpgradeNoReqs");
			break;
		default:
			break;
		}
		break;
	case crFailRes: {
   			const Faction::ResourceTypes &needed = Faction::getNeededResources();
			string a, b;
			for(int i = 0; i < needed.size(); ++i) {
				if(i != needed.size() - 1 || needed.size() == 1) {
					if(i) {
						a += ", ";
					}
					a += needed[i]->getName();
				} else {
					b = needed[i]->getName();
				}
			}

			if(needed.size() == 1) {
				switch(cc){
				case ccBuild:
					console->addStdMessage("BuildingNoRes1", a);
					break;
				case ccProduce:
					console->addStdMessage("UnitNoRes1", a);
					break;
				case ccUpgrade:
					console->addStdMessage("UpgradeNoRes1", a);
					break;
				case ccMorph:
					console->addStdMessage("MorphNoRes1", a);
					break;
				default:
					console->addStdMessage("GenericNoRes1", a);
					break;
				}
			} else {
				switch(cc){
				case ccBuild:
					console->addStdMessage("BuildingNoRes2", a, b);
					break;
				case ccProduce:
					console->addStdMessage("UnitNoRes2", a, b);
					break;
				case ccUpgrade:
					console->addStdMessage("UpgradeNoRes2", a, b);
					break;
				case ccMorph:
					console->addStdMessage("MorphNoRes2", a, b);
					break;
				default:
					console->addStdMessage("GenericNoRes2", a, b);
					break;
				}
			}
		}

		break;

	case crFailPetLimit:
		console->addStdMessage("PetLimitReached");
		break;

	case crFailUndefined:
		console->addStdMessage("InvalidOrder");
		break;

	case crSomeFailed:
		console->addStdMessage("SomeOrdersFailed");
		break;
	}
}

bool Gui::isSharedCommandClass(CommandClass commandClass){
	for(int i = 0; i < selection.getCount(); ++i){
		if(!selection.getUnit(i)->getFirstAvailableCt(commandClass)) {
			return false;
		}
	}
	return true;
}

void Gui::updateSelection(bool doubleClick, Selection::UnitContainer &units) {
	//Selection::UnitContainer units;
	//Renderer::getInstance().computeSelected(units, selectionQuad.getPosDown(), selectionQuad.getPosUp());
	selectingBuilding = false;
	activeCommandType = NULL;
	needSelectionUpdate = false;

	//select all units of the same type if double click
	if (doubleClick && units.size()) {
		const Unit *refUnit = units.front();
		int factionIndex = refUnit->getFactionIndex();

		for (int i = 0; i < world->getFaction(factionIndex)->getUnitCount(); ++i) {
			Unit *unit = world->getFaction(factionIndex)->getUnit(i);
			if (unit->getPos().dist(refUnit->getPos()) < doubleClickSelectionRadius &&
					unit->getType() == refUnit->getType()) {
				units.push_back(unit);
			}
		}
	}

	bool shiftDown = input.isShiftDown();
	bool controlDown = input.isCtrlDown();

	if (!shiftDown && !controlDown) {
		selection.clear();
	}

	if (!controlDown) {
		selection.select(units);
	} else {
		selection.unSelect(units);
	}
}

/**
 * @return true if the position is a valid world position, false if the position is outside of the
 * world.
 */
bool Gui::computeTarget(const Vec2i &screenPos, Vec2i &worldPos, Selection::UnitContainer &units) {
	units.clear();
	Renderer &renderer = Renderer::getInstance();
	validPosObjWorld = renderer.computePosition(screenPos, worldPos);
	renderer.computeSelected(units, screenPos, screenPos);

	if (!units.empty()) {
		return true;
	} else {
		if (validPosObjWorld) {
			posObjWorld = worldPos;
		}
		return validPosObjWorld;
	}
}

void Gui::computeBuildPositions(const Vec2i &end) {
	assert(isPlacingBuilding());

	if(!dragging) {
		buildPositions.resize(1);
		buildPositions[0] = end;
		return;
	}

	int size = choosenBuildingType->getSize();
	Vec2i offset = dragStartPos - end;
	Vec2i offsetAdjusted;
	int count;
	if(abs(offset.x) > abs(offset.y)) {
		count = abs(offset.x / size) + 1;
		offsetAdjusted.x = (offset.x / size) * size;
		float mulit = (float)offset.x / (float)offsetAdjusted.x;
		offsetAdjusted.y = (float)offset.y * mulit;
	} else {
		count = abs(offset.y / size) + 1;
		offsetAdjusted.y = (offset.y / size) * size;
		float mulit = (float)offset.y / (float)offsetAdjusted.y;
		offsetAdjusted.x = (float)offset.x * mulit;
	}

	buildPositions.resize(count);
	buildPositions[0] = dragStartPos;
	for(int i = 1; i < count; i++) {
		buildPositions[i].x = dragStartPos.x - offsetAdjusted.x * i / (count - 1);
		buildPositions[i].y = dragStartPos.y - offsetAdjusted.y * i / (count - 1);
	}
}

}}//end namespace

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
#include "user_interface.h"

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
#include "sim_interface.h"

using std::max;
using namespace Glest::Sim;
using namespace Shared::Graphics;
using namespace Shared::Util;
using Shared::Xml::XmlNode;

namespace Glest { namespace Gui {

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
// 	class UserInterface
// =====================================================

UserInterface* UserInterface::currentGui = NULL;

//constructor
UserInterface::UserInterface(GameState &game)
		: game(game)
		, input(game.getInput()) 
		, selectedObject(0) {
	posObjWorld= Vec2i(54, 14);
	dragStartPos= Vec2i(0, 0);
	computeSelection= false;
	validPosObjWorld= false;
	activeCommandType= NULL;
	activeCommandClass= CommandClass::STOP;
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

void UserInterface::init() {
	this->commander= g_simInterface->getCommander();
	this->gameCamera= game.getGameCamera();
	this->console= game.getConsole();
	this->world= &g_world;
	buildPositions.reserve(max(world->getMap()->getH(), world->getMap()->getW()));
	selection.init(this, world->getThisFactionIndex());
}

void UserInterface::end() {
	selection.clear();
}

// ==================== get ====================

const UnitType *UserInterface::getBuilding() const {
	assert(selectingBuilding);
	return choosenBuildingType;
}

// ==================== is ====================

bool UserInterface::isPlacingBuilding() const {
	return isSelectingPos() && activeCommandType!=NULL && activeCommandType->getClass()==CommandClass::BUILD;
}

// ==================== reset state ====================

void UserInterface::resetState() {
	selectingBuilding= false;
	selectingPos= false;
	selectingMeetingPoint= false;
	activePos= invalidPos;
	activeCommandClass= CommandClass::STOP;
	activeCommandType= NULL;
	dragging = false;
	draggingMinimap = false;
	needSelectionUpdate = false;
	buildPositions.clear();
}

/** return true if the position is valid, false otherwise */
bool UserInterface::getMinimapCell(int x, int y, Vec2i &cell) {
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
void UserInterface::mouseDownLeft(int x, int y) {
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
		validWorldPos = computeTarget(Vec2i(x, y), worldPos, units, true);
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
			commander->tryGiveCommand(selection, CommandFlags(CommandProperties::QUEUE, input.isShiftDown()), NULL,
					CommandClass::SET_MEETING_POINT, worldPos);
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

void UserInterface::mouseDownRight(int x, int y) {
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
		if(computeTarget(Vec2i(x, y), worldPos, units, false)) {
			Unit *targetUnit = units.size() ? units.front() : NULL;
			giveDefaultOrders(targetUnit ? targetUnit->getPos() : worldPos, targetUnit);
		} else {
			console->addStdMessage("InvalidPosition");
		}
	}

	computeDisplay();
}

void UserInterface::mouseUpLeft(int x, int y) {
	mouseUpLeftGraphics(x, y);
	draggingMinimap = false;
}

void UserInterface::mouseUpRight(int x, int y) {
}

void UserInterface::mouseDoubleClickLeft(int x, int y) {
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

void UserInterface::mouseDownLeftDisplay(int posDisplay){
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

void UserInterface::mouseMoveDisplay(int x, int y){
	computeInfoString(computePosDisplay(x, y));
}

void UserInterface::mouseUpLeftGraphics(int x, int y){
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
		if(computeTarget(Vec2i(x, y), worldPos, units, false)) {
			giveTwoClickOrders(worldPos, NULL);
		} else {
			console->addStdMessage("InvalidPosition");
		}
	}
}

void UserInterface::mouseMoveGraphics(int x, int y){
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
			Renderer::getInstance().computeSelected(units, selectedObject, selectionQuad.getPosDown(), selectionQuad.getPosUp());
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

void UserInterface::mouseDoubleClickLeftGraphics(int x, int y){
	if(!selectingPos && !selectingMeetingPoint){
		Selection::UnitContainer units;
		Vec2i pos(x, y);

		selectionQuad.setPosDown(pos);
		const Object *obj;
		Renderer::getInstance().computeSelected(units, obj, pos, pos);
		calculateNearest(units, gameCamera->getPos());
		updateSelection(true, units);
		computeDisplay();
	}
}

void UserInterface::groupKey(int groupIndex){
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

void UserInterface::hotKey(UserCommand cmd) {
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
		selectInterestingUnit(InterestingUnitType::IDLE_HARVESTER);
		break;

 	// select idle builder
	case ucSelectNextIdleBuilder:
		selectInterestingUnit(InterestingUnitType::IDLE_BUILDER);
		break;

 	// select idle repairing/healing unit
	case ucSelectNextIdleRepairer:
		selectInterestingUnit(InterestingUnitType::IDLE_REPAIRER);
		break;

 	// select idle worker (can either build, repair, or harvest)
	case ucSelectNextIdleWorker:
		selectInterestingUnit(InterestingUnitType::IDLE_WORKER);
		break;

 	// select idle non-hp restoration-skilled unit
	case ucSelectNextIdleRestorer:
		selectInterestingUnit(InterestingUnitType::IDLE_RESTORER);
		break;

 	// select idle producer
	case ucSelectNextIdleProducer:
		selectInterestingUnit(InterestingUnitType::IDLE_PRODUCER);
		break;

 	// select idle or non-idle producer
	case ucSelectNextProducer:
		selectInterestingUnit(InterestingUnitType::PRODUCER);
		break;

 	// select damaged unit
	case ucSelectNextDamaged:
		selectInterestingUnit(InterestingUnitType::DAMAGED);
		break;

 	// select building (completed)
	case ucSelectNextBuiltBuilding:
		selectInterestingUnit(InterestingUnitType::BUILT_BUILDING);
		break;

 	// select storeage unit
	case ucSelectNextStore:
		selectInterestingUnit(InterestingUnitType::STORE);
		break;

 	// Attack
	case ucAttack:
		clickCommonCommand(CommandClass::ATTACK);
		break;

	// Stop
	case ucStop:
		clickCommonCommand(CommandClass::STOP);

 	// Move
	case ucMove:
		clickCommonCommand(CommandClass::MOVE);
		break;

 	// Repair / Heal / Replenish
	case ucReplenish:
		clickCommonCommand(CommandClass::REPAIR);
		break;

 	// Guard
	case ucGuard:
		clickCommonCommand(CommandClass::GUARD);
		break;

 	// Follow
	case ucFollow:
		//clickCommonCommand();
		break;

 	// Patrol
	case ucPatrol:
		//clickCommonCommand(CommandClass::PATROL);
		break;
	default:
		break;
	}
}

bool UserInterface::cancelPending() {
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

void UserInterface::load(const XmlNode *node) {
	selection.load(node->getChild("selection"));
	gameCamera->load(node->getChild("camera"));
}

void UserInterface::save(XmlNode *node) const {
	selection.save(node->addChild("selection"));
	gameCamera->save(node->addChild("camera"));
}


// ================= PRIVATE =================

void UserInterface::giveOneClickOrders() {
	CommandResult result;
	CommandFlags flags(CommandProperties::QUEUE, input.isShiftDown());

	if (selection.isUniform()) {
		result = commander->tryGiveCommand(selection, flags, activeCommandType);
	} else {
		result = commander->tryGiveCommand(selection, flags, NULL, activeCommandClass);
	}

	addOrdersResultToConsole(activeCommandClass, result);

	activeCommandType = NULL;
	activeCommandClass = CommandClass::STOP;
}

void UserInterface::giveDefaultOrders(const Vec2i &targetPos, Unit *targetUnit) {

	//give order
	CommandResult result = commander->tryGiveCommand(selection,
			CommandFlags(CommandProperties::QUEUE, input.isShiftDown()), NULL, CommandClass::NULL_COMMAND, targetPos, targetUnit);

	//graphical result
	addOrdersResultToConsole(activeCommandClass, result);
	if(result == CommandResult::SUCCESS || result == CommandResult::SOME_FAILED) {
		posObjWorld = targetPos;
		if (!targetUnit) {
			mouse3d.show(targetPos);
		}

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

void UserInterface::giveTwoClickOrders(const Vec2i &targetPos, Unit *targetUnit) {

	CommandResult result;
	CommandFlags flags(CommandProperties::QUEUE, input.isShiftDown());

	//give orders to the units of this faction

	if(!selectingBuilding) {

		if(selection.isUniform()) {
			result = commander->tryGiveCommand(selection, flags, activeCommandType, CommandClass::NULL_COMMAND, targetPos, targetUnit);
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
		for(i = buildPositions.begin(); i != buildPositions.end(); ++i) {
			flags.set(CommandProperties::QUEUE, input.isShiftDown() || !firstBuildPosition);
			flags.set(CommandProperties::DONT_RESERVE_RESOURCES, selection.getCount() > 1 || !firstBuildPosition);
			result = commander->tryGiveCommand(selection, flags, activeCommandType, CommandClass::NULL_COMMAND, *i, NULL, choosenBuildingType);
		}
	}

	//graphical result
	addOrdersResultToConsole(activeCommandClass, result);

	if(result == CommandResult::SUCCESS || result == CommandResult::SOME_FAILED) {
		if (!targetUnit) {
			mouse3d.show(targetPos);
		}
		if(random.randRange(0, 1) == 0) {
			SoundRenderer::getInstance().playFx(
				selection.getFrontUnit()->getType()->getCommandSound(),
				selection.getFrontUnit()->getCurrVector(),
				gameCamera->getPos());
		}
		resetState();
	} else if (result == CommandResult::FAIL_BLOCKED) {
		// let the player choose again
	} else {
		resetState();
	}
}

void UserInterface::centerCameraOnSelection(){
	if(!selection.isEmpty()){
		Vec3f refPos= selection.getRefPos();
		gameCamera->centerXZ(refPos.x, refPos.z);
	}
}

void UserInterface::centerCameraOnLastEvent(){
	Vec3f lastEventLoc = world->getThisFaction()->getLastEventLoc();
	if(!(lastEventLoc.x == -1.0f)){
		gameCamera->centerXZ(lastEventLoc.x, lastEventLoc.z);
	}
}

void UserInterface::selectInterestingUnit(InterestingUnitType iut){
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

void UserInterface::clickCommonCommand(CommandClass commandClass) {
	for(int i= 0; i<Display::downCellCount; ++i) {
		const CommandType* ct = display.getCommandType(i);
		if(((ct && ct->getClass() == commandClass) || display.getCommandClass(i) == commandClass)
				&& display.getDownLighted(i)) {
			mouseDownDisplayUnitSkills(i);
			break;
		}
	}
}

void UserInterface::mouseDownDisplayUnitSkills(int posDisplay) {
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
				CommandFlags(CommandProperties::QUEUE, input.isShiftDown()), newState);
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
				activeCommandClass = CommandClass::STOP;
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
			if(activeCommandType!=NULL && activeCommandType->getClass()==CommandClass::BUILD){
				assert(selection.isUniform());
				selectingBuilding= true;
			}
			else if(ct->getClicks()==Clicks::ONE) {
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

void UserInterface::mouseDownDisplayUnitBuild(int posDisplay){
	int factionIndex= world->getThisFactionIndex();

	if(posDisplay == cancelPos){
		resetState();
	}
	else{
		if(activeCommandType!=NULL && activeCommandType->getClass()==CommandClass::BUILD){
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

void UserInterface::computeInfoString(int posDisplay) {
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
						if(ct->getClass() == CommandClass::UPGRADE) {
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

				if(cc != CommandClass::NULL_COMMAND) {
					display.setInfoText(lang.get("CommonCommand") + ": " + ut->getFirstCtOfClass(cc)->toString());
				}
			}
		}
	} else {
		if(posDisplay == cancelPos) {
			display.setInfoText(lang.get("Return"));
		} else {
			if(activeCommandType != NULL && activeCommandType->getClass() == CommandClass::BUILD) {
				const BuildCommandType *bct = static_cast<const BuildCommandType*>(activeCommandType);
				display.setInfoText(bct->getBuilding(display.getIndex(posDisplay))->getReqDesc());
			}
		}
 	}
}

void UserInterface::computeDisplay() {

	//init
	display.clear();

	// ================ PART 1 ================

	int thisTeam = g_world.getThisTeamIndex();
	//title, text and progress bar
	if (selection.getCount() == 1) {
		const Unit *unit = selection.getFrontUnit();
		bool friendly = unit->getFaction()->getTeam() == thisTeam;

		display.setTitle(unit->getFullName());
		display.setText(unit->getDesc(friendly));
		if (friendly) {
			display.setProgressBar(selection.getFrontUnit()->getProductionPercent());
		}
	}

	//portraits
	for (int i = 0; i < selection.getCount(); ++i) {
		display.setUpImage(i, selection.getUnit(i)->getType()->getImage());
	}

	if (selection.getCount() && selection.getFrontUnit()->getType()->isOfClass(UnitClass::CARRIER)) {
		const Unit *unit = selection.getFrontUnit();
		Unit::UnitContainer carriedUnits = unit->getCarriedUnits();
		for (int i = 0; i < carriedUnits.size(); ++i) {
			display.setCarryImage(i, carriedUnits[i]->getType()->getImage());
		}
	}

	// ================ PART 2 ================

	if (selectingPos || selectingMeetingPoint) {
		display.setDownSelectedPos(activePos);
	}

	if (selection.isComandable()
	&& selection.getFrontUnit()->getFaction()->getTeam() == thisTeam ) {
		if (!selectingBuilding) { 
			const Unit *u = selection.getFrontUnit();
			const UnitType *ut = u->getType();
			if (selection.isCancelable()) { //cancel button
				display.setDownImage(cancelPos, ut->getCancelImage());
				display.setDownLighted(cancelPos, true);
			}
			if (selection.isMeetable()) { //meeting point
				display.setDownImage(meetingPointPos, ut->getMeetingPointImage());
				display.setDownLighted(meetingPointPos, true);
			}

			if (selection.isCanRepair()) {
				if (selection.getAutoRepairState() == arsOn) {
					const CommandType *rct = ut->getFirstCtOfClass(CommandClass::REPAIR);
					assert(rct);
					display.setDownImage(autoRepairPos, rct->getImage());
				} else {
					display.setDownImage(autoRepairPos, ut->getCancelImage());
				}
				display.setDownLighted(autoRepairPos, true);
			}

			if (selection.isUniform()) { //uniform selection
				if (u->isBuilt()) {
					int morphPos = 8;
					for (int i = 0, j = 0; i < ut->getCommandTypeCount(); ++i) {
						const CommandType *ct = ut->getCommandType(i);
						int displayPos = ct->getClass() == CommandClass::MORPH ? morphPos++ : j;
						if (u->getFaction()->isAvailable(ct) && ct->getClass() != CommandClass::SET_MEETING_POINT) {
							display.setDownImage(displayPos, ct->getImage());
							display.setCommandType(displayPos, ct);
							display.setDownLighted(displayPos, u->getFaction()->reqsOk(ct));
							++j;
						}
					}
				}
			} else { //non uniform selection
				int lastCommand = 0;
				foreach_enum (CommandClass, cc) {
					if (isSharedCommandClass(cc) && cc != CommandClass::BUILD) {
						display.setDownLighted(lastCommand, true);
						display.setDownImage(lastCommand, ut->getFirstCtOfClass(cc)->getImage());
						display.setCommandClass(lastCommand, cc);
						lastCommand++;
					}
				}
			}
		} else { //selecting building
			const Unit *unit = selection.getFrontUnit();
			if (activeCommandType != NULL && activeCommandType->getClass() == CommandClass::BUILD) {
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

	if (selection.isEmpty() && selectedObject) {
		Resource *r = selectedObject->getResource();
		if (r) {
			display.setTitle(r->getType()->getName());
			display.setText(g_lang.get("amount") + ":" + intToStr(r->getAmount()));
			display.setUpImage(0, r->getType()->getImage());
		} ///@todo else
	}
}

int UserInterface::computePosDisplay(int x, int y) {
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
				if (display.getCommandClass(posDisplay) == CommandClass::NULL_COMMAND && display.getCommandType(posDisplay) == NULL) {
					posDisplay = invalidPos;
				}
			} else {
				//building selection
				if (activeCommandType != NULL && activeCommandType->getClass() == CommandClass::BUILD) {
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

void UserInterface::addOrdersResultToConsole(CommandClass cc, CommandResult result) {

	switch(result){
	case CommandResult::SUCCESS:
		break;
	case CommandResult::FAIL_BLOCKED:
		console->addStdMessage("BuildingNoPlace");
		break;
	case CommandResult::FAIL_REQUIREMENTS:
		switch(cc){
		case CommandClass::BUILD:
			console->addStdMessage("BuildingNoReqs");
			break;
		case CommandClass::PRODUCE:
			console->addStdMessage("UnitNoReqs");
			break;
		case CommandClass::UPGRADE:
			console->addStdMessage("UpgradeNoReqs");
			break;
		default:
			break;
		}
		break;
	case CommandResult::FAIL_RESOURCES: {
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
				case CommandClass::BUILD:
					console->addStdMessage("BuildingNoRes1", a);
					break;
				case CommandClass::PRODUCE:
					console->addStdMessage("UnitNoRes1", a);
					break;
				case CommandClass::UPGRADE:
					console->addStdMessage("UpgradeNoRes1", a);
					break;
				case CommandClass::MORPH:
					console->addStdMessage("MorphNoRes1", a);
					break;
				default:
					console->addStdMessage("GenericNoRes1", a);
					break;
				}
			} else {
				switch(cc){
				case CommandClass::BUILD:
					console->addStdMessage("BuildingNoRes2", a, b);
					break;
				case CommandClass::PRODUCE:
					console->addStdMessage("UnitNoRes2", a, b);
					break;
				case CommandClass::UPGRADE:
					console->addStdMessage("UpgradeNoRes2", a, b);
					break;
				case CommandClass::MORPH:
					console->addStdMessage("MorphNoRes2", a, b);
					break;
				default:
					console->addStdMessage("GenericNoRes2", a, b);
					break;
				}
			}
		}

		break;

	case CommandResult::FAIL_PET_LIMIT:
		console->addStdMessage("PetLimitReached");
		break;

	case CommandResult::FAIL_UNDEFINED:
		console->addStdMessage("InvalidOrder");
		break;

	case CommandResult::SOME_FAILED:
		console->addStdMessage("SomeOrdersFailed");
		break;
	default:
		throw runtime_error("unhandled CommandResult");
	}
}

bool UserInterface::isSharedCommandClass(CommandClass commandClass){
	for(int i = 0; i < selection.getCount(); ++i){
		if(!selection.getUnit(i)->getFirstAvailableCt(commandClass)) {
			return false;
		}
	}
	return true;
}

void UserInterface::updateSelection(bool doubleClick, Selection::UnitContainer &units) {
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
bool UserInterface::computeTarget(const Vec2i &screenPos, Vec2i &worldPos, Selection::UnitContainer &units, bool setObj) {
	units.clear();
	Renderer &renderer = Renderer::getInstance();
	validPosObjWorld = renderer.computePosition(screenPos, worldPos);
	const Object *junk;
	renderer.computeSelected(units, setObj ? selectedObject : junk, screenPos, screenPos);

	if (!units.empty()) {
		return true;
	} else {
		if (validPosObjWorld) {
			posObjWorld = worldPos;
		}
		return validPosObjWorld;
	}
}

void UserInterface::computeBuildPositions(const Vec2i &end) {
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
		float mulit = float(offset.x) / float(offsetAdjusted.x);
		offsetAdjusted.y = int(float(offset.y) * mulit);
	} else {
		count = abs(offset.y / size) + 1;
		offsetAdjusted.y = (offset.y / size) * size;
		float mulit = float(offset.y) / float(offsetAdjusted.y);
		offsetAdjusted.x = int(float(offset.x * mulit));
	}

	buildPositions.resize(count);
	buildPositions[0] = dragStartPos;
	for(int i = 1; i < count; i++) {
		buildPositions[i].x = dragStartPos.x - offsetAdjusted.x * i / (count - 1);
		buildPositions[i].y = dragStartPos.y - offsetAdjusted.y * i / (count - 1);
	}
}

}}//end namespace

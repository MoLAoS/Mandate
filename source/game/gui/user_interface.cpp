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
		, commander(0)
		, world(0)
		, gameCamera(0)
		, m_console(0)
		, m_dialogConsole(0)
		, m_minimap(0)
		, m_display(0)
		, m_resourceBar(0)
		, m_luaConsole(0)
		, m_selectingSecond(false)
		, m_selectedFacing(CardinalDir::NORTH) {
	posObjWorld= Vec2i(54, 14);
	dragStartPos= Vec2i(0, 0);
	computeSelection= false;
	validPosObjWorld= false;
	activeCommandType= NULL;
	activeCommandClass= CommandClass::STOP;
	selectingPos= false;
	selectedObject = 0;
	selectingMeetingPoint= false;
	activePos= invalidPos;
	dragging = false;
	needSelectionUpdate = false;
	currentGui = this;
	currentGroup= invalidGroupIndex;
}

UserInterface::~UserInterface() {
	currentGui = 0;
	delete m_minimap;
	delete m_display;
	delete m_resourceBar;
	delete m_console;
	delete m_dialogConsole;
}

void UserInterface::init() {
	this->commander= g_simInterface->getCommander();
	this->gameCamera= game.getGameCamera();
	m_console = new Console(&g_widgetWindow);
	m_dialogConsole = new Console(&g_widgetWindow, 10, 200, true);
	this->world= &g_world;
	buildPositions.reserve(max(world->getMap()->getH(), world->getMap()->getW()));
	selection.init(this, world->getThisFactionIndex());

	int x = g_metrics.getScreenW() - 20 - 195;
	int y = (g_metrics.getScreenH() - 500) / 2;

	m_display = new Display(this, Vec2i(x,y));

	// get 'this' FactionType, discover what resources need to be displayed
	const Faction *fac = g_world.getThisFaction();
	if (fac) {  //loadmap has no faction
		const FactionType *ft = fac->getType();
		set<const ResourceType*> displayResources;
		for (int i= 0; i < g_world.getTechTree()->getResourceTypeCount(); ++i) {
			const ResourceType *rt = g_world.getTechTree()->getResourceType(i);
			if (!rt->isDisplay()) {
				continue;
			}
			// if any UnitType needs the resource
			for (int j=0; j < ft->getUnitTypeCount(); ++j) {
				const UnitType *ut = ft->getUnitType(j);
				if (ut->getCost(rt)) {
					displayResources.insert(rt);
					break;
				}
			}
		}
		// create ResourceBar, connect thisFactions Resources to it...
		m_resourceBar = new ResourceBar(fac, displayResources);

		m_luaConsole = new LuaConsole(this, &g_program, Vec2i(200,200), Vec2i(500, 300));
		m_luaConsole->setVisible(false);
		m_luaConsole->Button1Clicked.connect(this, &UserInterface::onCloseLuaConsole);
	}
}

void UserInterface::initMinimap(bool fow, bool resuming) {
	const int &mapW = g_map.getW();
	const int &mapH = g_map.getH();

	fixed ratio = fixed(mapW) / mapH;
	//cout << "Map aspect ratio : " << ratio.toFloat();

	Vec2i size;
	if (ratio == 1) {
		size = Vec2i(128);
	} else if (ratio < 1) {
		size = Vec2i((ratio * 128).intp(), 128);
	} else { // (ratio > 1) {
		size = Vec2i(128, (128 / ratio).intp());
	}
	size += Vec2i(8, 16);

	int mx = 10;
	int my = g_metrics.getScreenH() - size.y - 30;
	m_minimap = new Minimap(fow, WidgetWindow::getInstance(), Vec2i(mx, my), size);
	m_minimap->init(g_map.getW(), g_map.getH(), &g_world, resuming);
	m_minimap->LeftClickOrder.connect(this, &UserInterface::onLeftClickOrder);
	m_minimap->RightClickOrder.connect(this, &UserInterface::onRightClickOrder);
}

// ==================== get ====================

const UnitType *UserInterface::getBuilding() const {
	assert(activeCommandType);
	assert(m_selectingSecond && activeCommandType->getClass() == CommandClass::BUILD);
	return choosenBuildingType;
}

// ==================== is ====================

bool UserInterface::isPlacingBuilding() const {
	return isSelectingPos() && activeCommandType
		&& activeCommandType->getClass() == CommandClass::BUILD;
}

// ==================== reset state ====================

void UserInterface::resetState() {
	m_selectingSecond = false;
	selectingPos = false;
	selectingMeetingPoint = false;
	activePos = invalidPos;
	activeCommandClass = CommandClass::STOP;
	activeCommandType = 0;
	dragging = false;
	needSelectionUpdate = false;
	buildPositions.clear();
	m_minimap->setLeftClickOrder(false);
	m_minimap->setRightClickOrder(!selection.isEmpty());
	g_program.setMouseCursorIcon();
}

static void calculateNearest(UnitVector &units, const Vec3f &pos) {
	if (units.size() > 1) {
		float minDist = numeric_limits<float>::infinity();
		Unit *nearest = 0;
		foreach_const (UnitVector, i, units) {
			RUNTIME_CHECK(!(*i)->isCarried());
			float dist = pos.dist((*i)->getCurrVector());
			if (dist < minDist) {
				minDist = dist;
				nearest = *i;
			}
		}
		units.clear();
		units.push_back(nearest);
	}
}

void UserInterface::onLeftClickOrder(Vec2i cellPos) {
	giveTwoClickOrders(cellPos, 0);
}

void UserInterface::onRightClickOrder(Vec2i cellPos) {
	giveDefaultOrders(cellPos, 0);
}

void UserInterface::onResourceDepleted(Vec2i cellPos) {
	Object *obj = g_map.getTile(Map::toTileCoords(cellPos))->getObject();
	RUNTIME_CHECK(obj != 0);
	if (obj == selectedObject) {
		selectedObject = 0;
		m_display->setSize();
	}
}

void UserInterface::commandButtonPressed(int posDisplay) {
	if (!selectingPos && !selectingMeetingPoint) {
		if (selection.isComandable()) {
			if (m_selectingSecond) {
				mouseDownSecondTier(posDisplay);
			} else {
				mouseDownDisplayUnitSkills(posDisplay);
			}
		} else {
			resetState();
		}
		activePos = posDisplay;
		computeDisplay();
		computeInfoString(activePos);
	} else { // m_selectingSecond
		// if they clicked on a button again, they must have changed their mind
		resetState();
	}
}

void UserInterface::unloadRequest(int carryIndex) {
	int i=0;
	Unit *transportUnit = 0, *unloadUnit = 0;
	for (int ndx = 0; !unloadUnit && ndx < selection.getCount(); ++ndx) {
		if (selection.getUnit(ndx)->getType()->isOfClass(UnitClass::CARRIER)) {
			const Unit *unit = selection.getUnit(ndx);
			UnitIdList carriedUnits = unit->getCarriedUnits();
			foreach (UnitIdList, it, carriedUnits) {
				if (carryIndex == i) {
					unloadUnit = g_simInterface->getUnitFactory().getUnit(*it);
					transportUnit = const_cast<Unit*>(unit);
					break;
				}
				++i;
			}
		}
	}
	assert(transportUnit && unloadUnit);
	CommandResult res = commander->tryUnloadCommand(transportUnit, CommandFlags(), Command::invalidPos, unloadUnit);
	if (res != CommandResult::SUCCESS) {
		addOrdersResultToConsole(CommandClass::UNLOAD, res);
	}
}

// ==================== events ====================

void UserInterface::mouseDownLeft(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "(" << x << ", " << y << ")");	
	const Metrics &metrics= Metrics::getInstance();

	UnitVector units;
	Vec2i worldPos;
	bool validWorldPos = computeTarget(Vec2i(x, y), worldPos, units, true);

	if (!validWorldPos) {
		m_console->addStdMessage("InvalidPosition");
		return;
	}

	const Unit *targetUnit= NULL;
	if (units.size()) {
		targetUnit = units.front();
		worldPos = targetUnit->getPos();
	}

	if (selectingPos) {
		// give standard orders
		giveTwoClickOrders(worldPos, (Unit *)targetUnit);

	// set meeting point
	} else if (selectingMeetingPoint) {
		if (selection.isComandable()) {
			commander->tryGiveCommand(selection, CommandFlags(), NULL,
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
	WIDGET_LOG( __FUNCTION__ << "(" << x << ", " << y << ")");	
	const Metrics &metrics= Metrics::getInstance();
	Vec2i worldPos;

	if (selectingPos || selectingMeetingPoint) {
		resetState();
		computeDisplay();
		return;
	}

	if (selection.isComandable()) {
		UnitVector units;
		if (computeTarget(Vec2i(x, y), worldPos, units, false)) {
			Unit *targetUnit = units.size() ? units.front() : NULL;
			giveDefaultOrders(targetUnit ? targetUnit->getPos() : worldPos, targetUnit);
		} else {
			m_console->addStdMessage("InvalidPosition");
		}
	}

	computeDisplay();
}

void UserInterface::mouseUpLeft(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "(" << x << ", " << y << ")");	
	if (!selectingPos && !selectingMeetingPoint && selectionQuad.isEnabled()) {
		selectionQuad.setPosUp(Vec2i(x, y));
		if (selection.isComandable() && random.randRange(0, 1)) {
			g_soundRenderer.playFx(selection.getFrontUnit()->getType()->getSelectionSound(),
				selection.getFrontUnit()->getCurrVector(), gameCamera->getPos());
		}
		selectionQuad.disable();
	} else if (isPlacingBuilding() && dragging) {
		Vec2i worldPos;
		if (g_renderer.computePosition(Vec2i(x, y), worldPos)) {
			giveTwoClickOrders(worldPos, NULL);
		} else {
			m_console->addStdMessage("InvalidPosition");
		}
	}
}

void UserInterface::mouseUpRight(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "(" << x << ", " << y << ")");	
}

void UserInterface::mouseDoubleClickLeft(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "(" << x << ", " << y << ")");	
	if (!selectingPos && !selectingMeetingPoint) {
		UnitVector units;
		Vec2i pos(x, y);

		const Object *obj;
		g_renderer.computeSelected(units, obj, pos, pos);
		//g_gameState.lastPick(units, obj);
		calculateNearest(units, gameCamera->getPos());
		updateSelection(true, units);
		computeDisplay();
	}
}

void UserInterface::mouseMove(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "(" << x << ", " << y << ")");	
	//compute selection
	if (selectionQuad.isEnabled()) {
		selectionQuad.setPosUp(Vec2i(x, y));
		UnitVector units;
		if (computeSelection) {
			g_renderer.computeSelected(units, selectedObject, 
				selectionQuad.getPosDown(), selectionQuad.getPosUp());
			//g_gameState.lastPick(units, selectedObject);
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

	//m_display->setInfoText("");
}

void UserInterface::groupKey(int groupIndex){
	if (input.isCtrlDown()) {
		selection.assignGroup(groupIndex);
	} else {
		if (currentGroup == groupIndex) {
			centerCameraOnSelection();
		}

		selection.recallGroup(groupIndex);
		currentGroup = groupIndex;
		computeDisplay();
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
		clickCommonCommand(CommandClass::PATROL);
		break;

	case ucRotate:
		m_selectedFacing = enum_cast<CardinalDir>((m_selectedFacing + 1) % CardinalDir::COUNT);
		break;

	case ucLuaConsole:
		if (g_simInterface->asNetworkInterface()) {
			g_console.addLine(g_lang.get("NotAvailable"));
		} else {
			m_luaConsole->setVisible(!m_luaConsole->isVisible());
		}
		break;

	default:
		break;
	}
}

bool UserInterface::cancelPending() {
	if (selectingPos || selectingMeetingPoint) {
		resetState();
		return true;
	} else if (!selection.isEmpty()) {
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
		if (activeCommandType->getProducedCount()) {
			result = commander->tryGiveCommand(selection, flags, activeCommandType,
				CommandClass::NULL_COMMAND, Command::invalidPos, 0, activeCommandType->getProduced(0));
		} else {
			result = commander->tryGiveCommand(selection, flags, activeCommandType);
		}
	} else {
		result = commander->tryGiveCommand(selection, flags, NULL, activeCommandClass);
	}
	addOrdersResultToConsole(activeCommandClass, result);
	activeCommandType = NULL;
	activeCommandClass = CommandClass::STOP;
}

void UserInterface::giveDefaultOrders(const Vec2i &targetPos, Unit *targetUnit) {
	// give order
	CommandResult result = commander->tryGiveCommand(selection,
			CommandFlags(CommandProperties::QUEUE, input.isShiftDown()), NULL, CommandClass::NULL_COMMAND, targetPos, targetUnit);

	// graphical result
	addOrdersResultToConsole(activeCommandClass, result);
	if (result == CommandResult::SUCCESS || result == CommandResult::SOME_FAILED) {
		posObjWorld = targetPos;
		if (!targetUnit) {
			mouse3d.show(targetPos);
		}
		if (random.randRange(0, 1) == 0) {
			SoundRenderer::getInstance().playFx(selection.getFrontUnit()->getType()->getCommandSound(),
				selection.getFrontUnit()->getCurrVector(), gameCamera->getPos());
		}
	}
	// reset
	resetState();
}

void UserInterface::giveTwoClickOrders(const Vec2i &targetPos, Unit *targetUnit) {
	CommandResult result;
	CommandFlags flags(CommandProperties::QUEUE, input.isShiftDown());

	// give orders to the units of this faction
	if (!m_selectingSecond) {
		if (selection.isUniform()) {
			result = commander->tryGiveCommand(selection, flags, activeCommandType, CommandClass::NULL_COMMAND, targetPos, targetUnit);
		} else {
			result = commander->tryGiveCommand(selection, flags, NULL, activeCommandClass, targetPos, targetUnit);
		}
	} else {
		if (activeCommandClass == CommandClass::BUILD) {
			// selecting building
			assert(isPlacingBuilding());

			// if this is a drag & drop then start dragging and wait for mouse up
			if (choosenBuildingType->isMultiBuild() && !dragging) {
				dragging = true;
				dragStartPos = posObjWorld;
				return;
			}

			computeBuildPositions(posObjWorld);
			bool firstBuildPosition = true;

			BuildPositions::const_iterator i;
			for (i = buildPositions.begin(); i != buildPositions.end(); ++i) {
				flags.set(CommandProperties::QUEUE, input.isShiftDown() || !firstBuildPosition);
				flags.set(CommandProperties::DONT_RESERVE_RESOURCES, selection.getCount() > 1 || !firstBuildPosition);
				result = commander->tryGiveCommand(selection, flags, activeCommandType, CommandClass::NULL_COMMAND, *i, NULL, choosenBuildingType, m_selectedFacing);
			}
		}
	}

	// graphical result
	addOrdersResultToConsole(activeCommandClass, result);

	if (result == CommandResult::SUCCESS || result == CommandResult::SOME_FAILED) {
		if (!targetUnit) {
			mouse3d.show(targetPos);
		}
		if (random.randRange(0, 1) == 0) {
			const Unit *unit = selection.getFrontUnit();
			g_soundRenderer.playFx(unit->getType()->getCommandSound(),
				unit->getCurrVector(), gameCamera->getPos());
		}
		resetState();
	} else if (result == CommandResult::FAIL_BLOCKED) {
		// let the player choose again
	} else {
		resetState();
	}
}

void UserInterface::centerCameraOnSelection(){
	if (!selection.isEmpty()) {
		Vec3f refPos = selection.getRefPos();
		gameCamera->centerXZ(refPos.x, refPos.z);
	}
}

void UserInterface::centerCameraOnLastEvent(){
	Vec3f lastEventLoc = world->getThisFaction()->getLastEventLoc();
	if (!(lastEventLoc.x == -1.0f)) {
		gameCamera->centerXZ(lastEventLoc.x, lastEventLoc.z);
	}
}

void UserInterface::selectInterestingUnit(InterestingUnitType iut){
	const Faction* thisFaction = world->getThisFaction();
	const Unit* previousUnit = NULL;
	bool previousFound = true;

	//start at the next harvester
	if (selection.getCount() == 1) {
		const Unit* refUnit = selection.getFrontUnit();
		if (refUnit->isInteresting(iut)) {
			previousUnit = refUnit;
			previousFound = false;
		}
	}

	//clear selection
	selection.clear();

	//search
	for(int i=0; i < thisFaction->getUnitCount(); ++i) {
		Unit* unit = thisFaction->getUnit(i);
		if (previousFound) {
			if (unit->isInteresting(iut)) {
				selection.select(unit);
				break;
			}
		} else {
			if (unit == previousUnit) {
				previousFound = true;
			}
		}
	}
	// search again if we have a previous
	if (selection.isEmpty() && previousUnit != NULL && previousFound == true) {
		for (int i=0; i < thisFaction->getUnitCount(); ++i) {
			Unit* unit = thisFaction->getUnit(i);
			if (unit->isInteresting(iut)) {
				selection.select(unit);
				break;
			}
		}
	}
}

void UserInterface::clickCommonCommand(CommandClass commandClass) {
	for(int i=0; i < Display::downCellCount; ++i) {
		const CommandType* ct = m_display->getCommandType(i);
		if (((ct && ct->getClass() == commandClass) || m_display->getCommandClass(i) == commandClass)
		&& m_display->getDownLighted(i)) {
			mouseDownDisplayUnitSkills(i);
			break;
		}
	}
}

void UserInterface::mouseDownDisplayUnitSkills(int posDisplay) {
	if (selection.isEmpty()) {
		return;
	}
	if (posDisplay == cancelPos) {
		if (isPlacingBuilding()) {
			resetState();
		} else if (selection.isCancelable()) {
			commander->tryCancelCommand(&selection);
		} else {
			// not our click
		}
	} else if (posDisplay == autoRepairPos) {
		AutoCmdState state = selection.getAutoRepairState();
		bool action = (state != AutoCmdState::ALL_ON);
		commander->trySetAutoRepairEnabled(selection,
				CommandFlags(CommandProperties::QUEUE, input.isShiftDown()), action);
	} else if (posDisplay == meetingPointPos) {
		activePos= posDisplay;
		selectingMeetingPoint= true;
	} else {
		const Unit *unit= selection.getFrontUnit();

		if (selection.isUniform()) { // uniform selection
			if (unit->getFaction()->reqsOk(m_display->getCommandType(posDisplay))) {
				activeCommandType = m_display->getCommandType(posDisplay);
				activeCommandClass = activeCommandType->getClass();
			} else {
				posDisplay = invalidPos;
				activeCommandType = NULL;
				activeCommandClass = CommandClass::STOP;
				return;
			}
		} else { // non uniform selection
			activeCommandType = NULL;
			activeCommandClass = m_display->getCommandClass(posDisplay);
		}

		if (!selection.isEmpty()) { // give orders depending on command type
			const CommandType *ct = selection.getUnit(0)->getFirstAvailableCt(activeCommandClass);
			if (activeCommandType && activeCommandType->getClass() == CommandClass::BUILD) {
				assert(selection.isUniform());
				m_selectedFacing = CardinalDir::NORTH;
				m_selectingSecond = true;
			} else if (ct->getClicks() == Clicks::ONE) {
				invalidatePosObjWorld();
				giveOneClickOrders();
			} else {
				if (ct->getClass() == CommandClass::MORPH || ct->getClass() == CommandClass::PRODUCE) {
					assert(selection.isUniform());
					m_selectingSecond = true;
				} else {
					selectingPos = true;
					g_program.setMouseCursorIcon(ct->getImage());
					m_minimap->setLeftClickOrder(true);
					activePos = posDisplay;
				}
			}
		}
	}
}

void UserInterface::mouseDownSecondTier(int posDisplay){
	int factionIndex = world->getThisFactionIndex();

	if (posDisplay == cancelPos) {
		resetState();
	} else {
		RUNTIME_CHECK(activeCommandType != 0);
		int ndx = m_display->getIndex(posDisplay);
		RUNTIME_CHECK(ndx >= 0 && ndx < activeCommandType->getProducedCount());
		const ProducibleType *pt = activeCommandType->getProduced(ndx);

		if (activeCommandType->getClass() == CommandClass::BUILD) {
			if (world->getFaction(factionIndex)->reqsOk(pt)) {
				choosenBuildingType = static_cast<const UnitType*>(pt);
				assert(choosenBuildingType != NULL);
				selectingPos = true;
				activePos = posDisplay;
				g_program.setMouseCursorIcon(choosenBuildingType->getImage());
			}
		} else {
			if (world->getFaction(factionIndex)->reqsOk(pt)) {
				CommandResult result = commander->tryGiveCommand(selection, CommandFlags(),
					activeCommandType, CommandClass::NULL_COMMAND, Command::invalidPos, 0, pt);
				addOrdersResultToConsole(activeCommandClass, result);
				resetState();
			}
		}
	}
}

void UserInterface::computeInfoString(int posDisplay) {
	m_display->setInfoText("");

	if (!selection.isComandable() || posDisplay == invalidPos) {
		return;
	}

	if (!m_selectingSecond) {
		if (posDisplay == cancelPos) {
			m_display->setInfoText(g_lang.get("Cancel"));
		} else if (posDisplay == meetingPointPos) {
			m_display->setInfoText(g_lang.get("SetMeetingPoint"));
		} else if (posDisplay == autoRepairPos) {
			string str = g_lang.get("AutoRepair");
			switch (selection.getAutoRepairState()) {
				case AutoCmdState::ALL_ON:
					str += " " + g_lang.get("On");
					break;
				case AutoCmdState::ALL_OFF:
					str += " " + g_lang.get("Off");
					break;
				case AutoCmdState::MIXED:
					str += " " + g_lang.get("Mixed");
					break;
			}
			m_display->setInfoText(str);
		} else { // uniform selection
			if (selection.isUniform()) {
				const Unit *unit = selection.getFrontUnit();
				const CommandType *ct = m_display->getCommandType(posDisplay);

				if (ct != NULL) {
					string factionName = unit->getFaction()->getType()->getName();
					string commandName = g_lang.getFactionString(factionName, ct->getName());
					if (commandName == ct->getName()) { // no custom command name
						commandName = g_lang.get(ct->getName()); // assume command class is name
						if (commandName.find("???") != string::npos) { // or just use name from xml
							commandName = formatString(ct->getName());
						}
					}
					string tip = g_lang.getFactionString(factionName, ct->getTipKey());
					string res = commandName + "\n\n";
					if (!tip.empty()) {
						res += tip + "\n\n";
					}

					if (unit->getFaction()->reqsOk(ct)) {
						res += formatString(ct->getDesc(unit));
						m_display->setInfoText(res);
					} else {
						if (ct->getClass() == CommandClass::UPGRADE) {
							const UpgradeCommandType *uct = static_cast<const UpgradeCommandType*>(ct);

							if (unit->getFaction()->getUpgradeManager()->isUpgrading(uct->getProducedUpgrade())) {
								m_display->setInfoText(g_lang.get("Upgrading")/* + "\n" + ct->getDesc(unit)*/);
							} else if (unit->getFaction()->getUpgradeManager()->isUpgraded(uct->getProducedUpgrade())) {
								m_display->setInfoText(g_lang.get("AlreadyUpgraded")/* + "\n" + ct->getDesc(unit)*/);
							} else {
								res += formatString(ct->getReqDesc());
								m_display->setInfoText(res);
							}
						} else {
							res += formatString(ct->getReqDesc());
							m_display->setInfoText(res);
						}
					}
				}			
			} else { // non uniform selection
				const UnitType *ut = selection.getFrontUnit()->getType();
				CommandClass cc = m_display->getCommandClass(posDisplay);

				if (cc != CommandClass::NULL_COMMAND) {
					m_display->setInfoText(g_lang.get("CommonCommand") + ": "
						+ g_lang.get(CommandClassNames[cc]));
				}
			}
		}
	} else { // m_selectingSecond
		if (posDisplay == cancelPos) {
			m_display->setInfoText(g_lang.get("Return"));
			return;
		}
		const Unit *unit = selection.getFrontUnit();
		const ProducibleType *pt = activeCommandType->getProduced(m_display->getIndex(posDisplay));
		string factionName = unit->getFaction()->getType()->getName();
		string commandName = g_lang.getFactionString(factionName, activeCommandType->getName());
		if (commandName == activeCommandType->getName()) { // no custom command name
			commandName = g_lang.get(activeCommandType->getName()); // assume command class is name
		}
		string tip = g_lang.getFactionString(factionName, activeCommandType->getTipKey(pt->getName()));
		string res = commandName + " : " + g_lang.getFactionString(factionName, pt->getName()) + "\n\n";
		if (!tip.empty()) {
			res += tip + "\n\n";
		}
		res += pt->getReqDesc();
		m_display->setInfoText(res);
	}
}

void UserInterface::computeDisplay() {
	if (selectedObject && !selection.isEmpty()) {
		selectedObject = 0;
	}
	// init
	m_display->clear();

	// ================ PART 1 ================

	int thisTeam = g_world.getThisTeamIndex();
	// title, text and progress bar
	if (selection.getCount() == 1) {
		const Unit *unit = selection.getFrontUnit();
		bool friendly = unit->getFaction()->getTeam() == thisTeam;

		m_display->setTitle(unit->getFullName());
		m_display->setText(unit->getDesc(true, friendly));
		if (friendly) {
			m_display->setProgressBar(unit->getProductionPercent());
		}
	}

	// portraits
	for (int i = 0; i < selection.getCount(); ++i) {
		m_display->setUpImage(i, selection.getUnit(i)->getType()->getImage());
	}

	bool transported = false;
	int i=0;
	for (int ndx = 0; ndx < selection.getCount(); ++ndx) {
		if (selection.getUnit(ndx)->getType()->isOfClass(UnitClass::CARRIER)) {
			const Unit *unit = selection.getUnit(ndx);
			UnitIdList carriedUnits = unit->getCarriedUnits();
			if (!carriedUnits.empty()) {
				transported = true;
				foreach (UnitIdList, it, carriedUnits) {
					if (i < Display::carryCellCount) {
						Unit *unit = g_simInterface->getUnitFactory().getUnit(*it);
						m_display->setCarryImage(i++, unit->getType()->getImage());
					}
				}
			}
		}
	}
	m_display->setTransportedLabel(transported);

	// ================ PART 2 ================

	if (selectingPos || selectingMeetingPoint) {
		m_display->setDownSelectedPos(activePos);
	}

	if (selection.isComandable()) {
		if (!m_selectingSecond) {
			const Unit *u = selection.getFrontUnit();
			const UnitType *ut = u->getType();
			if (selection.isCancelable()) { // cancel button
				m_display->setDownImage(cancelPos, ut->getCancelImage());
				m_display->setDownLighted(cancelPos, true);
			}
			if (selection.isMeetable()) { // meeting point
				m_display->setDownImage(meetingPointPos, ut->getMeetingPointImage());
				m_display->setDownLighted(meetingPointPos, true);
			}
			if (selection.isCanRepair()) {
				if (selection.getAutoRepairState() == AutoCmdState::ALL_ON) {
					const CommandType *rct = ut->getFirstCtOfClass(CommandClass::REPAIR);
					if (!rct) {
						RUNTIME_CHECK(selection.getCount() > 1);
						for (int i=1; i < selection.getCount(); ++i) {
							ut = selection.getUnit(i)->getType();
							rct = ut->getFirstCtOfClass(CommandClass::REPAIR);
							if (rct) {
								break;
							}
						}
					}
					RUNTIME_CHECK(rct != 0);
					m_display->setDownImage(autoRepairPos, rct->getImage());
				} else {
					m_display->setDownImage(autoRepairPos, ut->getCancelImage());
				}
				m_display->setDownLighted(autoRepairPos, true);
			}

			if (selection.isUniform()) { // uniform selection
				if (u->isBuilt()) {
					int morphPos = cellWidthCount * 2;
					for (int i = 0, j = 0; i < ut->getCommandTypeCount(); ++i) {
						const CommandType *ct = ut->getCommandType(i);
						int displayPos = ct->getClass() == CommandClass::MORPH ? morphPos++ : j;
						if (u->getFaction()->isAvailable(ct) 
						&& ct->getClass() != CommandClass::SET_MEETING_POINT) {
							m_display->setDownImage(displayPos, ct->getImage());
							m_display->setCommandType(displayPos, ct);
							m_display->setDownLighted(displayPos, u->getFaction()->reqsOk(ct));
							++j;
						}
					}
				}
			} else { // non uniform selection
				int lastCommand = 0;
				foreach_enum (CommandClass, cc) {
					if (isSharedCommandClass(cc) && cc != CommandClass::BUILD) {
						m_display->setDownLighted(lastCommand, true);
						m_display->setDownImage(lastCommand, ut->getFirstCtOfClass(cc)->getImage());
						m_display->setCommandClass(lastCommand, cc);
						lastCommand++;
					}
				}
			}
		} else { // two-tier select
			RUNTIME_CHECK(activeCommandType != 0 && activeCommandType->getProducedCount() > 0);
			const Unit *unit = selection.getFrontUnit();
			m_display->setDownImage(cancelPos, selection.getFrontUnit()->getType()->getCancelImage());
			m_display->setDownLighted(cancelPos, true);
			for (int i=0, j=0; i < activeCommandType->getProducedCount(); ++i) {
				const ProducibleType *pt = activeCommandType->getProduced(i);
				if (unit->getFaction()->isAvailable(pt)) {
					m_display->setDownImage(j, pt->getImage());
					m_display->setDownLighted(j, unit->getFaction()->reqsOk(pt));
					m_display->setIndex(j, i);
					++j;
				}
			}
			if (activePos >= activeCommandType->getProducedCount()) {
				activePos = invalidPos;
			}
		}
	} // end if (selection.isComandable())

	if (selection.isEmpty() && selectedObject) {
		Resource *r = selectedObject->getResource();
		if (r) {
			m_display->setTitle(r->getType()->getName());
			m_display->setText(g_lang.get("amount") + ": " + intToStr(r->getAmount()));
			m_display->setUpImage(0, r->getType()->getImage());
		} ///@todo else
	}
}

int UserInterface::computePosDisplay(int x, int y) {
	Vec2i pos(x, y);
	DisplayButton btn = m_display->computeIndex(pos, true);
	int posDisplay = (btn.m_section == DisplaySection::COMMANDS ? btn.m_index : invalidPos);

	if (selection.isComandable()) {
		if (posDisplay == cancelPos) {
			// check cancel button
			if (!selection.isCancelable() && !m_selectingSecond) {
				posDisplay = invalidPos;
			}
		} else if (posDisplay == meetingPointPos) {
			// check meeting point
			if (!selection.isMeetable()) {
				posDisplay = invalidPos;
			}
		} else if (posDisplay == autoRepairPos) {
			if (!selection.isCanRepair()) {
				posDisplay = invalidPos;
			}
		} else {
			if (!m_selectingSecond) {
				// standard selection
				if (m_display->getCommandClass(posDisplay) == CommandClass::NULL_COMMAND && m_display->getCommandType(posDisplay) == NULL) {
					posDisplay = invalidPos;
				}
			} else { // TODO: none of this is needed anymore Display::computeDownIndex() will cull these....
				//assert(activeCommandType);
				//if (activeCommandType->getClass() == CommandClass::BUILD) { // building selection
				//	const BuildCommandType *bct = static_cast<const BuildCommandType*>(activeCommandType);
				//	if (posDisplay >= bct->getBuildingCount()) {
				//		posDisplay = invalidPos;
				//	}
				//} else if (activeCommandType->getClass() == CommandClass::MORPH) { // morph selection
				//	const MorphCommandType *mct = static_cast<const MorphCommandType*>(activeCommandType);
				//	if (posDisplay >= mct->getMorphUnitCount()) {
				//		posDisplay = invalidPos;
				//	}
				//} else if (activeCommandType->getClass() == CommandClass::PRODUCE) { // prod. selection
				//	const ProduceCommandType *pct = static_cast<const ProduceCommandType*>(activeCommandType);
				//	if (posDisplay >= pct->getProducedUnitCount()) {
				//		posDisplay = invalidPos;
				//	}
				//}
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
		m_console->addStdMessage("BuildingNoPlace");
		break;
	case CommandResult::FAIL_REQUIREMENTS:
		switch(cc){
		case CommandClass::BUILD:
			m_console->addStdMessage("BuildingNoReqs");
			break;
		case CommandClass::PRODUCE:
			m_console->addStdMessage("UnitNoReqs");
			break;
		case CommandClass::UPGRADE:
			m_console->addStdMessage("UpgradeNoReqs");
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
					m_console->addStdMessage("BuildingNoRes1", a);
					break;
				case CommandClass::PRODUCE:
					m_console->addStdMessage("UnitNoRes1", a);
					break;
				case CommandClass::UPGRADE:
					m_console->addStdMessage("UpgradeNoRes1", a);
					break;
				case CommandClass::MORPH:
					m_console->addStdMessage("MorphNoRes1", a);
					break;
				default:
					m_console->addStdMessage("GenericNoRes1", a);
					break;
				}
			} else {
				switch(cc){
				case CommandClass::BUILD:
					m_console->addStdMessage("BuildingNoRes2", a, b);
					break;
				case CommandClass::PRODUCE:
					m_console->addStdMessage("UnitNoRes2", a, b);
					break;
				case CommandClass::UPGRADE:
					m_console->addStdMessage("UpgradeNoRes2", a, b);
					break;
				case CommandClass::MORPH:
					m_console->addStdMessage("MorphNoRes2", a, b);
					break;
				default:
					m_console->addStdMessage("GenericNoRes2", a, b);
					break;
				}
			}
		}

		break;

	case CommandResult::FAIL_PET_LIMIT:
		m_console->addStdMessage("PetLimitReached");
		break;

	case CommandResult::FAIL_LOAD_LIMIT:
		m_console->addStdMessage("LoadLimitReached");
		break;

	case CommandResult::FAIL_INVALID_LOAD:
		m_console->addStdMessage("CanNotLoadUnit");
		break;

	case CommandResult::FAIL_UNDEFINED:
		m_console->addStdMessage("InvalidOrder");
		break;

	case CommandResult::SOME_FAILED:
		m_console->addStdMessage("SomeOrdersFailed");
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

void UserInterface::updateSelection(bool doubleClick, UnitVector &units) {
	m_selectingSecond = false;
	activeCommandType = 0;
	needSelectionUpdate = false;

	bool wasEmpty = selection.isEmpty();

	// select all units of the same type if double click
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
	if (wasEmpty != selection.isEmpty()) {
		m_minimap->setRightClickOrder(!selection.isEmpty());
	}
	if (!selection.isEmpty() && selectedObject) {
		selectedObject = 0;
	}
	computeDisplay();
}

/**
 * @return true if the position is a valid world position, false if the position is outside of the
 * world.
 */
bool UserInterface::computeTarget(const Vec2i &screenPos, Vec2i &worldPos, UnitVector &units, bool setObj) {
	units.clear();
	validPosObjWorld = g_renderer.computePosition(screenPos, worldPos);
	const Object *junk;
	g_renderer.computeSelected(units, setObj ? selectedObject : junk, screenPos, screenPos);
	//g_gameState.lastPick(units, selectedObject);

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

void UserInterface::onCloseLuaConsole(BasicDialog*) {
	m_luaConsole->setVisible(false);
}

}}//end namespace

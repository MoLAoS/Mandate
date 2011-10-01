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
#include "resource_bar.h"
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

Mouse3d::Mouse3d() {
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

SelectionQuad::SelectionQuad() {
	enabled = false;
	posDown = Vec2i(0);
	posUp = Vec2i(0);
}

void SelectionQuad::setPosDown(const Vec2i &posDown) {
	enabled = true;
	this->posDown = posDown;
	this->posUp = posDown;
}

void SelectionQuad::setPosUp(const Vec2i &posUp) {
	this->posUp = posUp;
}

void SelectionQuad::disable() {
	enabled = false;
}

// =====================================================
// 	class UserInterface
// =====================================================

UserInterface* UserInterface::currentGui = 0;

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
		, selection(0)
		, m_selectingSecond(false)
		, m_selectedFacing(CardinalDir::NORTH)
		, m_selectionDirty(false) {
	posObjWorld = Vec2i(54, 14);
	dragStartPos = Vec2i(0, 0);
	computeSelection = false;
	validPosObjWorld = false;
	choosenBuildingType = 0;
	activeCommandType = 0;
	activeCommandClass = CmdClass::STOP;
	selectingPos = false;
	selectedObject = 0;
	selectingMeetingPoint = false;
	activePos = invalidPos;
	dragging = false;
	needSelectionUpdate = false;
	currentGui = this;
	currentGroup = invalidGroupIndex;
	m_teamCoulorMode = false;
}

UserInterface::~UserInterface() {
	delete selection;
	currentGui = 0;
	// widgets deleted by program.clear(), called in GameState::update()
}

void UserInterface::init() {
	// refs
	this->commander = g_simInterface.getCommander();
	this->gameCamera = game.getGameCamera();
	this->world = &g_world;

	buildPositions.reserve(max(world->getMap()->getH(), world->getMap()->getW()));
	selection = new Selection();
	selection->init(this, world->getThisFactionIndex());

	// Create Consoles...
	m_console = new Console(&g_widgetWindow);
	m_console->setPos(Vec2i(20, g_metrics.getScreenH() - (m_console->getReqHeight() + 20)));

	m_dialogConsole = new Console(&g_widgetWindow, 10, true);
	m_dialogConsole->setPos(Vec2i(20, 196));

	// get 'this' FactionType, discover what resources need to be displayed
	const Faction *fac = g_world.getThisFaction();
	if (fac) {  //loadmap has no faction
		const FactionType *ft = fac->getType();
		set<const ResourceType*> displayResources;
		for (int i = 0; i < g_world.getTechTree()->getResourceTypeCount(); ++i) {
			const ResourceType *rt = g_world.getTechTree()->getResourceType(i);
			if (!rt->isDisplay()) {
				continue;
			}
			// if any UnitType needs the resource
			for (int j = 0; j < ft->getUnitTypeCount(); ++j) {
				const UnitType *ut = ft->getUnitType(j);
				ResourceAmount r = ut->getCost(rt, fac);
				if (r.getType()) {
					displayResources.insert(rt);
					break;
				}
			}
		}
		// create ResourceBar, connect thisFactions Resources to it...
		ResourceBarFrame *barFrame = new ResourceBarFrame();
		m_resourceBar = barFrame->getResourceBar();
		m_resourceBar->init(fac, displayResources);

		m_luaConsole = new LuaConsole(this, &g_program);
		m_luaConsole->setPos(Vec2i(200,200));
		m_luaConsole->setSize(Vec2i(500, 300));
		m_luaConsole->setVisible(false);
		m_luaConsole->Button1Clicked.connect(this, &UserInterface::onCloseLuaConsole);
		m_luaConsole->Close.connect(this, &UserInterface::onCloseLuaConsole);
	}
	int y = m_resourceBar->getParent()->getPos().y + m_resourceBar->getParent()->getHeight() + 10;
	int x = g_metrics.getScreenW() - 20 - 195;

	// Display Panel
	DisplayFrame *displayFrame = new DisplayFrame(this, Vec2i(x,y));
	m_display = displayFrame->getDisplay();
	m_display->setSize();
	m_minimap->getParent()->setPos(Vec2i(20, y));
}

void UserInterface::initMinimap(bool fow, bool sod, bool resuming) {
	const int &mapW = g_map.getW();
	const int &mapH = g_map.getH();

	int mx = 10;
	int my = 50;

	MinimapFrame *frame = new MinimapFrame(WidgetWindow::getInstance(), Vec2i(mx, my), fow, sod);
	frame->initMinimp(g_map.getW(), g_map.getH(), &g_world, resuming);
	m_minimap = frame->getMinimap();
	m_minimap->LeftClickOrder.connect(this, &UserInterface::onLeftClickOrder);
	m_minimap->RightClickOrder.connect(this, &UserInterface::onRightClickOrder);
}

// ==================== get ====================

const UnitType *UserInterface::getBuilding() const {
	RUNTIME_CHECK(activeCommandType);
	RUNTIME_CHECK(activeCommandType->getClass() == CmdClass::BUILD
		|| activeCommandType->getClass() == CmdClass::TRANSFORM);
	return choosenBuildingType;
}

// ==================== is ====================

bool UserInterface::isPlacingBuilding() const {
	return isSelectingPos() && activeCommandType
		&& (activeCommandType->getClass() == CmdClass::BUILD 
		|| activeCommandType->getClass() == CmdClass::TRANSFORM);
}

// ==================== reset state ====================

void UserInterface::resetState(bool redoDisplay) {
	m_selectingSecond = false;
	selectingPos = false;
	selectingMeetingPoint = false;
	activePos = invalidPos;
	activeCommandClass = CmdClass::STOP;
	activeCommandType = 0;
	dragging = false;
	needSelectionUpdate = false;
	choosenBuildingType = 0;
	buildPositions.clear();
	m_minimap->setLeftClickOrder(false);
	m_minimap->setRightClickOrder(!selection->isEmpty());
	g_program.getMouseCursor().setAppearance(MouseAppearance::DEFAULT);
	if (redoDisplay) {
		computeDisplay();
	}
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
	MapObject *obj = g_map.getTile(Map::toTileCoords(cellPos))->getObject();
	RUNTIME_CHECK(obj != 0);
	if (obj == selectedObject) {
		selectedObject = 0;
		m_display->setSize();
	}
}

void UserInterface::update() {
	//WIDGET_LOG( __FUNCTION__ << "()");
	setComputeSelectionFlag();
	mouse3d.update();
	selection->clearDeadUnits();

	if (m_selectionDirty) {
		tick();
	}
}
///@todo wrap in Display
void UserInterface::tick() {
	computeDisplay();
	DisplayButton btn = m_display->getHoverButton();
	if (btn.m_section == DisplaySection::COMMANDS) {
		computeCommandInfo(btn.m_index);
	} else if (btn.m_section == DisplaySection::SELECTION) {
		if (selection->getCount() == 1) {
			computePortraitInfo(btn.m_index);
		}
	}
	m_selectionDirty = false;
}
///@todo move to Display
void UserInterface::commandButtonPressed(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
	if (!selectingPos && !selectingMeetingPoint) {
		if (selection->isComandable()) {
			if (m_selectingSecond) {
				onSecondTierSelect(posDisplay);
			} else {
				onFirstTierSelect(posDisplay);
			}
			computeDisplay();
		} else {
			resetState();
		}
		activePos = posDisplay;
		computeCommandInfo(activePos);
	} else { // m_selectingSecond || m_selectingPos
		// if they clicked on a button again, they must have changed their mind
		resetState();
	}
}

void UserInterface::unloadRequest(int carryIndex) {
	WIDGET_LOG( __FUNCTION__ << "( " << carryIndex << " )");
	int i=0;
	Unit *transportUnit = 0, *unloadUnit = 0;
	for (int ndx = 0; !unloadUnit && ndx < selection->getCount(); ++ndx) {
		if (selection->getUnit(ndx)->getType()->isOfClass(UnitClass::CARRIER)) {
			const Unit *unit = selection->getUnit(ndx);
			UnitIdList carriedUnits = unit->getCarriedUnits();
			foreach (UnitIdList, it, carriedUnits) {
				if (carryIndex == i) {
					unloadUnit = g_world.getUnit(*it);
					transportUnit = const_cast<Unit*>(unit);
					break;
				}
				++i;
			}
		}
	}
	assert(transportUnit && unloadUnit);
	CmdResult res = commander->tryUnloadCommand(transportUnit, CmdFlags(), Command::invalidPos, unloadUnit);
	if (res != CmdResult::SUCCESS) {
		addOrdersResultToConsole(CmdClass::UNLOAD, res);
	}
}

// ==================== events ====================

void UserInterface::mouseDownLeft(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");

	UnitVector units;
	Vec2i worldPos;
	bool validWorldPos = computeTarget(Vec2i(x, y), worldPos, units, selectedObject);

	if (!validWorldPos) {
		if (!selectingPos && !selectingMeetingPoint) {
			selectionQuad.setPosDown(Vec2i(x, y));
			calculateNearest(units, gameCamera->getPos());
			updateSelection(false, units);
			computeDisplay();
		} else {
			m_console->addStdMessage("InvalidPosition");
		}
		return;
	}

	const Unit *targetUnit = 0;
	if (units.size()) {
		targetUnit = units.front();
		worldPos = targetUnit->getPos();
	}

	if (selectingPos) { // give standard orders
		giveTwoClickOrders(worldPos, (Unit *)targetUnit);

	} else if (selectingMeetingPoint) { // set meeting point
		if (selection->isComandable()) {
			commander->tryGiveCommand(selection, CmdFlags(), 0, CmdClass::SET_MEETING_POINT, worldPos);
		}
		resetState(false);

	} else { // begin drag-drop selection & update selection for single-click
		selectionQuad.setPosDown(Vec2i(x, y));
		calculateNearest(units, gameCamera->getPos());
		updateSelection(false, units);
	}
	computeDisplay();
}

void UserInterface::mouseDownRight(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");	
}

void UserInterface::mouseUpLeft(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");	
	if (!selectingPos && !selectingMeetingPoint && selectionQuad.isEnabled()) {
		selectionQuad.setPosUp(Vec2i(x, y));
		if (selection->isComandable() && random.randRange(0, 1)) {
			g_soundRenderer.playFx(selection->getFrontUnit()->getType()->getSelectionSound(),
				selection->getFrontUnit()->getCurrVector(), gameCamera->getPos());
		}
		selectionQuad.disable();
	} else if (isPlacingBuilding() && dragging) {
		Vec2i worldPos;
		if (g_renderer.computePosition(Vec2i(x, y), worldPos)) {
			giveTwoClickOrders(worldPos, 0);
		} else {
			m_console->addStdMessage("InvalidPosition");
		}
	}
}

void UserInterface::mouseUpRight(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )" );

	if (!g_camera.isMoving()) {
		Vec2i worldPos;

		if (selectingPos || selectingMeetingPoint) {
			resetState();
			return;
		}

		if (selection->isComandable()) {
			UnitVector units;
			const MapObject *obj = 0;
			if (computeTarget(Vec2i(x, y), worldPos, units, obj)) {
				Unit *targetUnit = units.empty() ? 0 : units.front();
				Vec2i pos = targetUnit ? targetUnit->getPos() 
					: obj ? obj->getResource()->getPos() : worldPos;
				giveDefaultOrders(pos, targetUnit);
			} else {
				m_console->addStdMessage("InvalidPosition");
			}
		}
	}

	computeDisplay();
}

void UserInterface::mouseDoubleClickLeft(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");	
	if (!selectingPos && !selectingMeetingPoint) {
		UnitVector units;
		Vec2i pos(x, y);

		const MapObject *obj = 0;
		g_renderer.computeSelected(units, obj, pos, pos);
		//g_gameState.lastPick(units, obj);
		calculateNearest(units, gameCamera->getPos());
		updateSelection(true, units);
		computeDisplay();
	}
}

void UserInterface::mouseMove(int x, int y) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");	
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
	if (isPlacingBuilding()) {
		validPosObjWorld= Renderer::getInstance().computePosition(Vec2i(x,y), posObjWorld);

		if (!validPosObjWorld) {
			buildPositions.clear();
		} else {
			computeBuildPositions(posObjWorld);
		}
	}
}

void UserInterface::groupKey(int groupIndex) {
	if (input.isCtrlDown()) {
		selection->assignGroup(groupIndex);
	} else {
		if (currentGroup == groupIndex) {
			centerCameraOnSelection();
		}

		selection->recallGroup(groupIndex);
		currentGroup = groupIndex;
		resetState();
	}
}

void UserInterface::hotKey(UserCommand cmd) {
	WIDGET_LOG( __FUNCTION__ << "( " << UserCommandNames[cmd] << " )" );

	switch (cmd) {
		case UserCommand::TOGGLE_TEAM_TINT:
			m_teamCoulorMode = !m_teamCoulorMode;
			if (m_teamCoulorMode) {
				int mode = g_config.getUiTeamColourMode();
				g_renderer.setTeamColourMode(TeamColourMode(mode));
				m_console->addLine("Team-Colour-Mode On");
			} else {
				g_renderer.setTeamColourMode(TeamColourMode::DISABLED);
				m_console->addLine("Team-Colour-Mode Off");
			}
			break;
		case UserCommand::GOTO_SELECTION:
			centerCameraOnSelection();
			break;
		case UserCommand::GOTO_LAST_EVENT:
			centerCameraOnLastEvent();
			break;
		case UserCommand::SELECT_IDLE_HARVESTER:
			selectInterestingUnit(InterestingUnitType::IDLE_HARVESTER);
			break;
		case UserCommand::SELECT_IDLE_BUILDER:
			selectInterestingUnit(InterestingUnitType::IDLE_BUILDER);
			break;
		case UserCommand::SELECT_IDLE_REPAIRER:
			selectInterestingUnit(InterestingUnitType::IDLE_REPAIRER);
			break;
		case UserCommand::SELECT_IDLE_WORKER:
			selectInterestingUnit(InterestingUnitType::IDLE_WORKER);
			break;
		case UserCommand::SELECT_IDLE_RESTORER:
			selectInterestingUnit(InterestingUnitType::IDLE_RESTORER);
			break;
		case UserCommand::SELECT_IDLE_PRODUCER:
			selectInterestingUnit(InterestingUnitType::IDLE_PRODUCER);
			break;
		case UserCommand::SELECT_NEXT_PRODUCER:
			selectInterestingUnit(InterestingUnitType::PRODUCER);
			break;
		case UserCommand::SELECT_NEXT_DAMAGED:
			selectInterestingUnit(InterestingUnitType::DAMAGED);
			break;
		case UserCommand::SELECT_NEXT_BUILT_BUILDING:
			selectInterestingUnit(InterestingUnitType::BUILT_BUILDING);
			break;
		case UserCommand::SELECT_NEXT_STORE:
			selectInterestingUnit(InterestingUnitType::STORE);
			break;
		case UserCommand::ATTACK:
			clickCommonCommand(CmdClass::ATTACK);
			break;
		case UserCommand::STOP:
			clickCommonCommand(CmdClass::STOP);
			break;
		case UserCommand::MOVE:
			clickCommonCommand(CmdClass::MOVE);
			break;
		case UserCommand::REPAIR:
			clickCommonCommand(CmdClass::REPAIR);
			break;
		case UserCommand::GUARD:
			clickCommonCommand(CmdClass::GUARD);
			break;
		case UserCommand::FOLLOW:
			//clickCommonCommand();
			break;
		case UserCommand::PATROL:
			clickCommonCommand(CmdClass::PATROL);
			break;
		case UserCommand::ROTATE_BUILDING:
			if (isPlacingBuilding()) {
				m_selectedFacing = CardinalDir((m_selectedFacing + 1) % CardinalDir::COUNT);
			}
			break;
		case UserCommand::SHOW_LUA_CONSOLE:
			if (g_simInterface.asNetworkInterface()) {
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
	if (selectingPos || selectingMeetingPoint || m_selectingSecond) {
		resetState();
		return true;
	}
	return false;
}

void UserInterface::load(const XmlNode *node) {
	selection->load(node->getChild("selection"));
	gameCamera->load(node->getChild("camera"));
}

void UserInterface::save(XmlNode *node) const {
	selection->save(node->addChild("selection"));
	gameCamera->save(node->addChild("camera"));
}


// ================= PRIVATE =================

void UserInterface::giveOneClickOrders() {
	CmdResult result;
	CmdFlags flags(CmdProps::QUEUE, input.isShiftDown());
	if (selection->isUniform()) {
		if (activeCommandType->getProducedCount()) {
			result = commander->tryGiveCommand(selection, flags, activeCommandType,
				CmdClass::NULL_COMMAND, Command::invalidPos, 0, activeCommandType->getProduced(0));
		} else {
			result = commander->tryGiveCommand(selection, flags, activeCommandType);
		}
	} else {
		result = commander->tryGiveCommand(selection, flags, 0, activeCommandClass);
	}
	addOrdersResultToConsole(activeCommandClass, result);
	activeCommandType = 0;
	activeCommandClass = CmdClass::STOP;
}

void UserInterface::giveDefaultOrders(const Vec2i &targetPos, Unit *targetUnit) {
	// give order
	CmdResult result = commander->tryGiveCommand(selection, CmdFlags(CmdProps::QUEUE, input.isShiftDown()),
			0, CmdClass::NULL_COMMAND, targetPos, targetUnit);

	// graphical result
	addOrdersResultToConsole(activeCommandClass, result);
	if (result == CmdResult::SUCCESS || result == CmdResult::SOME_FAILED) {
		posObjWorld = targetPos;
		if (!targetUnit) {
			mouse3d.show(targetPos);
		}
		if (random.randRange(0, 1) == 0) {
			SoundRenderer::getInstance().playFx(selection->getFrontUnit()->getType()->getCommandSound(),
				selection->getFrontUnit()->getCurrVector(), gameCamera->getPos());
		}
	}
	// reset
	resetState();
}

void UserInterface::giveTwoClickOrders(const Vec2i &targetPos, Unit *targetUnit) {
	CmdResult result;
	CmdFlags flags(CmdProps::QUEUE, input.isShiftDown());

	// give orders to the units of this faction
	if (!m_selectingSecond) {
		if (selection->isUniform()) {
			if (choosenBuildingType) {
				result = commander->tryGiveCommand(selection, flags, activeCommandType,
					CmdClass::NULL_COMMAND, targetPos, 0, choosenBuildingType, m_selectedFacing);
			} else {
				result = commander->tryGiveCommand(selection, flags, activeCommandType,
					CmdClass::NULL_COMMAND, targetPos, targetUnit);
			}
		} else {
			result = commander->tryGiveCommand(selection, flags, 0, activeCommandClass, targetPos, targetUnit);
		}
	} else {
		if (activeCommandClass == CmdClass::BUILD || activeCommandClass == CmdClass::TRANSFORM) {
			// selecting pos for building
			assert(isPlacingBuilding());

			// if this is a multi-build (ie. walls) then start dragging and wait for mouse up
			if (activeCommandClass == CmdClass::BUILD && choosenBuildingType->isMultiBuild() && !dragging) {
				dragging = true;
				dragStartPos = posObjWorld;
				return;
			}

			computeBuildPositions(posObjWorld);

			// give command(s) (one for each building-pos if multi-build)
			foreach_const(BuildPositions, i, buildPositions) {
				bool first = i == buildPositions.begin();
				// queue if not shift down or first build pos
				flags.set(CmdProps::QUEUE, input.isShiftDown() || !first);
				// don't reserve any resources if multiple builders or if not first build pos
				flags.set(CmdProps::DONT_RESERVE_RESOURCES, selection->getCount() > 1 || !first);
				result = commander->tryGiveCommand(selection, flags, activeCommandType,
					CmdClass::NULL_COMMAND, *i, 0, choosenBuildingType, m_selectedFacing);
			}
		}
	}

	// graphical result
	addOrdersResultToConsole(activeCommandClass, result);

	if (result == CmdResult::SUCCESS || result == CmdResult::SOME_FAILED) {
		if (!targetUnit) {
			mouse3d.show(targetPos);
		}
		if (random.randRange(0, 1) == 0) {
			const Unit *unit = selection->getFrontUnit();
			g_soundRenderer.playFx(unit->getType()->getCommandSound(),
				unit->getCurrVector(), gameCamera->getPos());
		}
		resetState();
	} else if (result == CmdResult::FAIL_BLOCKED) {
		// let the player choose again
	} else {
		resetState();
	}
}

void UserInterface::centerCameraOnSelection() {
	if (!selection->isEmpty()) {
		Vec3f refPos = selection->getRefPos();
		gameCamera->centerXZ(refPos.x, refPos.z);
	}
}

void UserInterface::centerCameraOnLastEvent() {
	Vec3f lastEventLoc = world->getThisFaction()->getLastEventLoc();
	if (!(lastEventLoc.x == -1.0f)) {
		gameCamera->centerXZ(lastEventLoc.x, lastEventLoc.z);
	}
}

void UserInterface::selectInterestingUnit(InterestingUnitType iut) {
	const Faction* thisFaction = world->getThisFaction();
	const Unit* previousUnit = 0;
	bool previousFound = true;

	//start at the next harvester
	if (selection->getCount() == 1) {
		const Unit* refUnit = selection->getFrontUnit();
		if (refUnit->isInteresting(iut)) {
			previousUnit = refUnit;
			previousFound = false;
		}
	}

	//clear selection
	selection->clear();

	//search
	for(int i=0; i < thisFaction->getUnitCount(); ++i) {
		Unit* unit = thisFaction->getUnit(i);
		if (previousFound) {
			if (unit->isInteresting(iut)) {
				selection->select(unit);
				break;
			}
		} else {
			if (unit == previousUnit) {
				previousFound = true;
			}
		}
	}
	// search again if we have a previous
	if (selection->isEmpty() && previousUnit != 0 && previousFound == true) {
		for (int i=0; i < thisFaction->getUnitCount(); ++i) {
			Unit* unit = thisFaction->getUnit(i);
			if (unit->isInteresting(iut)) {
				selection->select(unit);
				break;
			}
		}
	}
}

void UserInterface::clickCommonCommand(CmdClass commandClass) {
	for(int i=0; i < Display::commandCellCount; ++i) {
		const CommandType* ct = m_display->getCommandType(i);
		if (((ct && ct->getClass() == commandClass) || m_display->getCommandClass(i) == commandClass)
		&& m_display->getDownLighted(i)) {
			onFirstTierSelect(i);
			break;
		}
	}
}

void UserInterface::onFirstTierSelect(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
	if (selection->isEmpty()) {
		return;
	}
	if (posDisplay == cancelPos) {
		if (isPlacingBuilding()) {
			resetState(false);
		} else if (selection->isCancelable()) {
			commander->tryCancelCommand(selection);
		} else {
			// not our click
		}

	// Auto-Command toggles, if selection is not all-on, turn on, else turn off
	} else if (posDisplay == autoRepairPos) {
		AutoCmdState state = selection->getAutoCmdState(AutoCmdFlag::REPAIR);
		bool action = (state != AutoCmdState::ALL_ON);
		commander->trySetAutoCommandEnabled(selection, AutoCmdFlag::REPAIR, action);
	} else if (posDisplay == autoAttackPos) {
		AutoCmdState state = selection->getAutoCmdState(AutoCmdFlag::ATTACK);
		bool action = (state != AutoCmdState::ALL_ON);
		commander->trySetAutoCommandEnabled(selection, AutoCmdFlag::ATTACK, action);
	} else if (posDisplay == autoFleePos) {
		AutoCmdState state = selection->getAutoCmdState(AutoCmdFlag::FLEE);
		bool action = (state != AutoCmdState::ALL_ON);
		commander->trySetAutoCommandEnabled(selection, AutoCmdFlag::FLEE, action);

	// Cloak toggle, if selection has any units cloaked, then de-cloak, else cloak
	} else if (posDisplay == cloakTogglePos) {
		bool action = !selection->anyCloaked();
		commander->trySetCloak(selection, action);

	} else if (posDisplay == meetingPointPos) {
		activePos= posDisplay;
		selectingMeetingPoint= true;
	} else {
		// posDisplay is for a real command...
		const Unit *unit= selection->getFrontUnit();

		if (selection->isUniform()) { // uniform selection, use activeCommandType
			if (unit->getFaction()->reqsOk(m_display->getCommandType(posDisplay))) {
				activeCommandType = m_display->getCommandType(posDisplay);
				activeCommandClass = activeCommandType->getClass();
			} else {
				posDisplay = invalidPos;
				activeCommandType = 0;
				activeCommandClass = CmdClass::STOP;
				return;
			}
		} else { // non uniform selection, use activeCommandClass
			activeCommandType = 0;
			activeCommandClass = m_display->getCommandClass(posDisplay);
		}

		// give orders depending on command type
		const CommandType *ct = selection->getUnit(0)->getFirstAvailableCt(activeCommandClass);
		if (activeCommandType && activeCommandType->getClass() == CmdClass::BUILD) {
			assert(selection->isUniform());
			m_selectedFacing = CardinalDir::NORTH;
			m_selectingSecond = true;
		} else if (activeCommandType && activeCommandType->getClass() == CmdClass::TRANSFORM
		&& activeCommandType->getProducedCount() == 1) {
			choosenBuildingType = static_cast<const UnitType*>(activeCommandType->getProduced(0));
			assert(choosenBuildingType);
			selectingPos = true;
			g_program.getMouseCursor().setAppearance(MouseAppearance::CMD_ICON, activeCommandType->getImage());
			m_minimap->setLeftClickOrder(true);
			activePos = posDisplay;
		} else if ((activeCommandType && activeCommandType->getClicks() == Clicks::ONE)
		|| (!activeCommandType && ct->getClicks() == Clicks::ONE)) {
			invalidatePosObjWorld();
			giveOneClickOrders();
		} else {
			if (activeCommandType && activeCommandType->getProducedCount() > 0) {
				assert(selection->isUniform());
				m_selectingSecond = true;
			} else {
				selectingPos = true;
				if (activeCommandType) {
					g_program.getMouseCursor().setAppearance(MouseAppearance::CMD_ICON, activeCommandType->getImage());
				} else {
					g_program.getMouseCursor().setAppearance(MouseAppearance::CMD_ICON, ct->getImage());
				}
				m_minimap->setLeftClickOrder(true);
				activePos = posDisplay;
			}
		}
	}
	computeDisplay();
}

///@todo move to Display
void UserInterface::onSecondTierSelect(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
	int factionIndex = world->getThisFactionIndex();

	if (posDisplay == cancelPos) {
		resetState(false);
	} else {
		RUNTIME_CHECK(activeCommandType != 0);
		int ndx = m_display->getIndex(posDisplay);
		RUNTIME_CHECK(ndx >= 0 && ndx < activeCommandType->getProducedCount());
		const ProducibleType *pt = activeCommandType->getProduced(ndx);

		if (activeCommandType->getClass() == CmdClass::BUILD
		|| activeCommandType->getClass() == CmdClass::TRANSFORM) {
			if (world->getFaction(factionIndex)->reqsOk(pt)) {
				choosenBuildingType = static_cast<const UnitType*>(pt);
				assert(choosenBuildingType != 0);
				selectingPos = true;
				activePos = posDisplay;
				g_program.getMouseCursor().setAppearance(MouseAppearance::CMD_ICON, choosenBuildingType->getImage());
			}
		} else {
			if (world->getFaction(factionIndex)->reqsOk(pt)) {
				CmdResult result = commander->tryGiveCommand(selection, CmdFlags(),
					activeCommandType, CmdClass::NULL_COMMAND, Command::invalidPos, 0, pt);
				addOrdersResultToConsole(activeCommandClass, result);
				resetState(false);
			}
		}
	}
	computeDisplay();
}

///@todo move to Display?
void UserInterface::computePortraitInfo(int posDisplay) {
	if (selection->getCount() < posDisplay) {
		m_display->setToolTipText2("", "");
	} else {
		if (selection->getCount() == 1 && !selection->isEnemy()) {
			const Unit *unit = selection->getFrontUnit();
			string name = g_lang.getTranslatedFactionName(unit->getFaction()->getType()->getName(), unit->getType()->getName());
			m_display->setToolTipText2(name, unit->getLongDesc(), DisplaySection::SELECTION);
		} else if (selection->isComandable()) {
			m_display->setToolTipText2("", g_lang.get("PotraitInfo"), DisplaySection::SELECTION);
		}
	}
}

inline string describeAutoCommandState(AutoCmdState state) {
	switch (state) {
		case AutoCmdState::ALL_ON:
			return g_lang.get("On");
		case AutoCmdState::ALL_OFF:
			return g_lang.get("Off");
		case AutoCmdState::MIXED:
			return g_lang.get("Mixed");
	}
	return "";
}

void UserInterface::computeCommandTip(const CommandType *ct, const ProducibleType *pt) {
	m_display->getCommandTip()->clearItems();
	ct->describe(selection->getFrontUnit(), m_display->getCommandTip(), pt);
	m_display->resetTipPos();
}

///@todo move to Display
void UserInterface::computeCommandInfo(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
	if (!selection->isComandable() || posDisplay == invalidPos
	|| (m_selectingSecond && posDisplay >= activeCommandType->getProducedCount())) {
		m_display->setToolTipText2("", "");
		return;
	}
	if (!m_selectingSecond) {
		if (posDisplay == cancelPos) {
			m_display->setToolTipText2("", g_lang.get("Cancel"));
		} else if (posDisplay == meetingPointPos) {
			m_display->setToolTipText2("", g_lang.get("SetMeetingPoint"));
		} else if (posDisplay == autoRepairPos) {
			string str = g_lang.get("AutoRepair") + " ";
			str += describeAutoCommandState(selection->getAutoRepairState());
			m_display->setToolTipText2("", str);
		} else if (posDisplay == autoAttackPos) {
			string str = g_lang.get("AutoAttack") + " ";
			str += describeAutoCommandState(selection->getAutoCmdState(AutoCmdFlag::ATTACK));
			m_display->setToolTipText2("", str);
		} else if (posDisplay == autoFleePos) {
			string str = g_lang.get("AutoFlee") + " ";
			str += describeAutoCommandState(selection->getAutoCmdState(AutoCmdFlag::FLEE));
			m_display->setToolTipText2("", str);
		} else if (posDisplay == cloakTogglePos) {
			m_display->setToolTipText2("", g_lang.get("ToggleCloak"));
		} else {
			if (selection->isUniform()) { // uniform selection
				const CommandType *ct = m_display->getCommandType(posDisplay);
				if (ct != 0) {
					computeCommandTip(ct);
				}
			} else { // non uniform selection
				CmdClass cc = m_display->getCommandClass(posDisplay);
				if (cc != CmdClass::NULL_COMMAND) {
					m_display->setToolTipText2("", g_lang.get("CommonCommand") + ": "
						+ g_lang.get(CmdClassNames[cc]));
				}
			}
		}
	} else { // m_selectingSecond
		if (posDisplay == cancelPos) {
			m_display->setToolTipText2("", g_lang.get("Return"));
			return;
		}
		RUNTIME_CHECK(activeCommandType != 0);
		RUNTIME_CHECK(posDisplay >= 0 && posDisplay < activeCommandType->getProducedCount());
		const ProducibleType *pt = activeCommandType->getProduced(m_display->getIndex(posDisplay));
		computeCommandTip(activeCommandType, pt);
	}
}

void UserInterface::computeSelectionPanel() {
	int thisTeam = g_world.getThisTeamIndex();

	// resource selected ?
	if (selection->isEmpty() && selectedObject) {
		MapResource *r = selectedObject->getResource();
		if (r) {
			string name = g_lang.getTranslatedTechName(r->getType()->getName());
			m_display->setPortraitTitle(name);
			m_display->setPortraitText(g_lang.get("amount") + ": " + intToStr(r->getAmount()));
			m_display->setUpImage(0, r->getType()->getImage());
		} ///@todo else (selectable tileset objects ?)
		return;
	}

	// title, text and progress bar (for single unit selected)
	if (selection->getCount() == 1) {
		const Unit *unit = selection->getFrontUnit();
		bool friendly = unit->getFaction()->getTeam() == thisTeam;

		string name = unit->getFullName(); ///@todo tricksy Lang use... will need a %s
		name = g_lang.getTranslatedFactionName(unit->getFaction()->getType()->getName(), name);

		IF_DEBUG_EDITION(
			name += ": " + intToStr(unit->getId());
		)
		m_display->setPortraitTitle(name);

		m_display->setPortraitText(unit->getShortDesc());
		if (friendly) {
			if (unit->getProductionPercent() == -1) {
				if (unit->getLoadCount()) {
					string resName = g_lang.getTechString(unit->getLoadType()->getName());
					if (resName == unit->getLoadType()->getName()) {
						resName = formatString(resName);
					}
					m_display->setLoadInfo(g_lang.get("Load") + ": " + intToStr(unit->getLoadCount()) 
						+  " " + resName);
				} else {
					m_display->setLoadInfo("");
				}
			} else {
				m_display->setProgressBar(unit->getProductionPercent());
			}
			int ordersQueued = unit->getQueuedOrderCount();
			if (ordersQueued) {
				m_display->setOrderQueueText(g_lang.get("OrdersOnQueue") + ": " + intToStr(ordersQueued));
			} else {
				m_display->setOrderQueueText("");
			}
		}
	}

	// selection portraits
	for (int i = 0; i < selection->getCount(); ++i) {
		m_display->setUpImage(i, selection->getUnit(i)->getType()->getImage());
	}
}

void UserInterface::computeHousedUnitsPanel() {
	bool transported = false;
	int i = 0;
	for (int ndx = 0; ndx < selection->getCount(); ++ndx) {
		if (selection->getUnit(ndx)->getType()->isOfClass(UnitClass::CARRIER)) {
			const Unit *unit = selection->getUnit(ndx);
			UnitIdList carriedUnits = unit->getCarriedUnits();
			if (!carriedUnits.empty()) {
				transported = true;
				foreach (UnitIdList, it, carriedUnits) {
					if (i < Display::transportCellCount) {
						Unit *unit = g_world.getUnit(*it);
						m_display->setCarryImage(i++, unit->getType()->getImage());
					}
				}
			}
		}
	}
	m_display->setTransportedLabel(transported);
}

void UserInterface::computeCommandPanel() {
	if (selectingPos || selectingMeetingPoint) {
		assert(!selection->isEmpty());
		m_display->setSelectedCommandPos(activePos);
	}

	if (selection->isComandable()) {
		if (!m_selectingSecond) {
			const Unit *u = selection->getFrontUnit();
			const UnitType *ut = u->getType();

			if (selection->canRepair()) { // auto-repair toggle
				m_display->setDownImage(autoRepairPos, selection->getCommandImage(CmdClass::REPAIR));
			}

			if (selection->canAttack()) {
				m_display->setDownImage(autoAttackPos, selection->getCommandImage(CmdClass::ATTACK));
				AutoCmdState attackState = selection->getAutoCmdState(AutoCmdFlag::ATTACK);
				if (attackState == AutoCmdState::NONE || attackState == AutoCmdState::ALL_OFF) {
					if (selection->canMove()) {
						m_display->setDownImage(autoFleePos, selection->getCommandImage(CmdClass::MOVE));
					}
				}
			} else {
				if (selection->canMove()) {
					m_display->setDownImage(autoFleePos, selection->getCommandImage(CmdClass::MOVE));
				}
			}

			if (selection->canCloak()) { // cloak toggle
				m_display->setDownImage(cloakTogglePos, selection->getCloakImage());
				if (selection->cloakAvailable()) {
					m_display->setDownLighted(cloakTogglePos, true);
				} else {
					m_display->setDownLighted(cloakTogglePos, false);
				}
			}

			if (selection->isMeetable()) { // meeting point
				m_display->setDownImage(meetingPointPos, ut->getMeetingPointImage());
				m_display->setDownLighted(meetingPointPos, true);
			}
			if (selection->isCancelable()) { // cancel button
				m_display->setDownImage(cancelPos, ut->getCancelImage());
				m_display->setDownLighted(cancelPos, true);
			}

			if (selection->isUniform()) { // uniform selection
				if (u->isBuilt()) {
					int morphPos = cellWidthCount * 2;
					for (int i = 0, j = 0; i < ut->getCommandTypeCount(); ++i) {
						const CommandType *ct = ut->getCommandType(i);
						int displayPos = ct->getClass() == CmdClass::MORPH ? morphPos++ : j;
						if (u->getFaction()->isAvailable(ct) && !ct->isInvisible()) {
							m_display->setDownImage(displayPos, ct->getImage());
							m_display->setCommandType(displayPos, ct);
							m_display->setDownLighted(displayPos, u->getFaction()->reqsOk(ct));
							++j;
						}
					}
				}
			} else { // non uniform selection
				int lastCommand = 0;
				foreach_enum (CmdClass, cc) {
					if (selection->isSharedCommandClass(cc) && !isProductionCmdClass(cc)) {
						m_display->setDownLighted(lastCommand, true);
						m_display->setDownImage(lastCommand, ut->getFirstCtOfClass(cc)->getImage());
						m_display->setCommandClass(lastCommand, cc);
						lastCommand++;
					}
				}
			}
		} else { // two-tier select
			RUNTIME_CHECK(activeCommandType != 0 && activeCommandType->getProducedCount() > 0);
			const Unit *unit = selection->getFrontUnit();
			m_display->setDownImage(cancelPos, selection->getFrontUnit()->getType()->getCancelImage());
			m_display->setDownLighted(cancelPos, true);
			for (int i=0, j=0; i < activeCommandType->getProducedCount(); ++i) {
				const ProducibleType *pt = activeCommandType->getProduced(i);
				if (unit->getFaction()->isAvailable(pt)) {
					m_display->setDownImage(j, pt->getImage());
					m_display->setDownLighted(j, unit->getFaction()->reqsOk(activeCommandType, pt));
						m_display->setIndex(j, i);
						++j;
					}
				}
				if (activePos >= activeCommandType->getProducedCount()) {
					activePos = invalidPos;
				}
			}
	} // end if (selection->isComandable())
}

void UserInterface::computeDisplay() {
	if (selectedObject && !selection->isEmpty()) {
		selectedObject = 0;
	}

	// init
	m_display->clear();

	// === Selection Panel ===
	computeSelectionPanel();

	// === Housed Units Panel ===
	computeHousedUnitsPanel();
	
	// === Command Panel ===
	computeCommandPanel();	
}

///@todo move parts to CommandType classes (hmmm... CommandTypes that need to know about Console ?!? Nope.)
void UserInterface::addOrdersResultToConsole(CmdClass cc, CmdResult result) {
	switch (result) {
		case CmdResult::SUCCESS:
			break;
		case CmdResult::FAIL_BLOCKED:
			m_console->addStdMessage("BuildingNoPlace");
			break;
		case CmdResult::FAIL_REQUIREMENTS:
				switch (cc) {
			case CmdClass::BUILD:
				m_console->addStdMessage("BuildingNoReqs");
				break;
			case CmdClass::PRODUCE:
				m_console->addStdMessage("UnitNoReqs");
				break;
			case CmdClass::UPGRADE:
				m_console->addStdMessage("UpgradeNoReqs");
				break;
			default:
				break;
			}
			break;
		case CmdResult::FAIL_RESOURCES: {
   			const Faction::ResourceTypes &needed = Faction::getNeededResources();
			string a, b;
				for (int i = 0; i < needed.size(); ++i) {
					if (i != needed.size() - 1 || needed.size() == 1) {
						if (i) {
							a += ", ";
						}
						a += needed[i]->getName();
					} else {
						b = needed[i]->getName();
					}
				}

				if (needed.size() == 1) {
					switch(cc) {
						case CmdClass::BUILD:
							m_console->addStdMessage("BuildingNoRes1", a);
							break;
						case CmdClass::PRODUCE:
							m_console->addStdMessage("UnitNoRes1", a);
							break;
						case CmdClass::UPGRADE:
							m_console->addStdMessage("UpgradeNoRes1", a);
							break;
						case CmdClass::MORPH:
							m_console->addStdMessage("MorphNoRes1", a);
							break;
						default:
							m_console->addStdMessage("GenericNoRes1", a);
							break;
					}
				} else {
					switch (cc) {
						case CmdClass::BUILD:
							m_console->addStdMessage("BuildingNoRes2", a, b);
							break;
						case CmdClass::PRODUCE:
							m_console->addStdMessage("UnitNoRes2", a, b);
							break;
						case CmdClass::UPGRADE:
							m_console->addStdMessage("UpgradeNoRes2", a, b);
							break;
						case CmdClass::MORPH:
							m_console->addStdMessage("MorphNoRes2", a, b);
							break;
						default:
							m_console->addStdMessage("GenericNoRes2", a, b);
							break;
					}
				}
			}
			break;
		case CmdResult::FAIL_PET_LIMIT:
			m_console->addStdMessage("PetLimitReached");
			break;

		case CmdResult::FAIL_LOAD_LIMIT:
			m_console->addStdMessage("LoadLimitReached");
			break;

		case CmdResult::FAIL_INVALID_LOAD:
			m_console->addStdMessage("CanNotLoadUnit");
			break;

		case CmdResult::FAIL_UNDEFINED:
			m_console->addStdMessage("InvalidOrder");
			break;

		case CmdResult::SOME_FAILED:
			m_console->addStdMessage("SomeOrdersFailed");
			break;
		default:
			throw runtime_error("unhandled CmdResult");
	}
}

void UserInterface::selectAllUnitsOfType(UnitVector &out_units, const Unit *refUnit, int radius) {
	int factionIndex = refUnit->getFactionIndex();

	for (int i = 0; i < world->getFaction(factionIndex)->getUnitCount(); ++i) {
		Unit *unit = world->getFaction(factionIndex)->getUnit(i);
		if (unit->getPos().dist(refUnit->getPos()) < radius
		&& unit->getType() == refUnit->getType()) {
			out_units.push_back(unit);
		}
	}
}

void UserInterface::updateSelection(bool doubleClick, UnitVector &units) {
	m_selectingSecond = false;
	activeCommandType = 0;
	needSelectionUpdate = false;

	bool wasEmpty = selection->isEmpty();

	// select all units of the same type if double click
	if (doubleClick && units.size()) {
		selectAllUnitsOfType(units, units.front(), doubleClickSelectionRadius);
	}

	bool shiftDown = input.isShiftDown();
	bool controlDown = input.isCtrlDown();

	if (!shiftDown && !controlDown) {
		selection->clear();
	}

	if (!controlDown) {
		selection->select(units);
	} else {
		selection->unSelect(units);
	}
	if (wasEmpty != selection->isEmpty()) {
		m_minimap->setRightClickOrder(!selection->isEmpty());
	}
	if (!selection->isEmpty() && selectedObject) {
		selectedObject = 0;
	}
	computeDisplay();
}

/**
 * @return true if the position is a valid world position, false if the position is outside of the
 * world.
 */
bool UserInterface::computeTarget(const Vec2i &screenPos, Vec2i &worldPos, UnitVector &units, const MapObject *&obj) {
	units.clear();
	validPosObjWorld = g_renderer.computePosition(screenPos, worldPos);
	g_renderer.computeSelected(units, obj, screenPos, screenPos);

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

	if (!dragging) {
		buildPositions.resize(1);
		buildPositions[0] = end;
		return;
	}

	int size = choosenBuildingType->getSize();
	Vec2i offset = dragStartPos - end;
	Vec2i offsetAdjusted;
	int count;
	if (abs(offset.x) > abs(offset.y)) {
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

void UserInterface::onCloseLuaConsole(Widget*) {
	m_luaConsole->setVisible(false);
}

}}//end namespace

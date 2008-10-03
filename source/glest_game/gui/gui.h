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

#ifndef _GLEST_GAME_GUI_H_
#define _GLEST_GAME_GUI_H_

#include "resource.h"
#include "command_type.h"
#include "display.h"
#include "commander.h"
#include "console.h"
#include "selection.h"
#include "random.h"
#include "game_camera.h"

using Shared::Util::Random;

namespace Glest{ namespace Game{

class Unit;
class World;
class CommandType;
class Game;

enum DisplayState{
	dsEmpty,
	dsUnitSkills,
	dsUnitBuild,
	dsEnemy
};

// =====================================================
//	class Mouse3d
// =====================================================

class Mouse3d{
public:
	static const float fadeSpeed;

private:
	bool enabled;
	int rot;
	float fade;

public:
	Mouse3d();

	void enable();
	void update();

	bool isEnabled() const	{return enabled;}
	float getFade() const	{return fade;}
	int getRot() const		{return rot;}
};

// =====================================================
//	class SelectionQuad
// =====================================================

class SelectionQuad{
private:
	Vec2i posDown;
	Vec2i posUp;
	bool enabled;

public:
	SelectionQuad();

	bool isEnabled() const		{return enabled;}
	Vec2i getPosDown() const	{return posDown;}
	Vec2i getPosUp() const		{return posUp;}

	void setPosDown(const Vec2i &posDown);
	void setPosUp(const Vec2i &posUp);
	void disable();
};

// =====================================================
// 	class Gui
//
///	In game GUI
// =====================================================

class Gui{
public:
	static const int maxSelBuff= 128*5;
	static const int upgradeDisplayIndex= 8;
	static const int cancelPos= 15;
	static const int meetingPointPos= 14;
	static const int autoRepairPos= 12;
	static const int imageCount= 16;
	static const int invalidPos= -1;
	static const int doubleClickSelectionRadius= 20;

	typedef vector<Vec2i> BuildPositions;

private:
	//External objects
	Random random;
	const Commander *commander;
	const World *world;
	GameCamera *gameCamera;
	Console *console;
	Game *game;

	//Positions
	Vec2i posObjWorld;		//world coords
	bool validPosObjWorld;
	bool computeSelection;

	//display
	const UnitType *choosenBuildingType;
	const CommandType *activeCommandType;
	CommandClass activeCommandClass;
	int activePos;

	//multi-build
	Vec2i dragStartPos;		//begining coord of multi-build command
	BuildPositions buildPositions;
	bool dragging;

	//composite
	Display display;
	Mouse3d mouse3d;
	Selection selection;
	SelectionQuad selectionQuad;

	//states
	bool selectingBuilding;
	bool selectingPos;
	bool selectingMeetingPoint;
	bool shiftDown;
	bool controlDown;

public:
	Gui();
	void init(Game *game);
	void end();

	//get
	Vec2i getPosObjWorld() const					{return posObjWorld;}
	const UnitType *getBuilding() const;
	const Vec2i &getDragStartPos() const			{return dragStartPos;}
	const BuildPositions &getBuildPositions() const	{return buildPositions;}

	const Mouse3d *getMouse3d() const				{return &mouse3d;}
	const Display *getDisplay()	const				{return &display;}
	const Selection *getSelection()	const			{return &selection;}
	const SelectionQuad *getSelectionQuad() const	{return &selectionQuad;}

	bool isValidPosObjWorld() const			{return validPosObjWorld;}
	bool isSelecting() const				{return selectionQuad.isEnabled();}
	bool isSelectingPos() const				{return selectingPos;}
	bool isSelectingBuilding() const		{return selectingBuilding;}
	bool isPlacingBuilding() const;
	bool isVisible(const Vec2i &pos) const	{return gameCamera->computeVisibleQuad().isInside(pos);}
	bool isDragging() const					{return dragging;}
	bool isShiftDown() const				{return shiftDown;}
	bool isControlDown() const				{return controlDown;}

	//set
	void invalidatePosObjWorld()			{validPosObjWorld= false;}
	void setComputeSelectionFlag()			{computeSelection= true;}
	void setShiftDown(bool shiftDown)		{this->shiftDown = shiftDown;}
	void setControlDown(bool controlDown)	{this->controlDown = controlDown;}

	//events
	void update(){
		setComputeSelectionFlag();
		mouse3d.update();
	}

	void tick(){
		computeDisplay();
	}

	void mouseDownLeft(int x, int y);
	void mouseDownRight(int x, int y);
	void mouseUpLeft(int x, int y);
	void mouseUpRight(int x, int y);
	void mouseDoubleClickLeft(int x, int y);

	void mouseMoveDisplay(int x, int y);
	void mouseDownLeftGraphics(int x, int y);
	void mouseDownRightGraphics(int x, int y);
	void mouseUpLeftGraphics(int x, int y);
	void mouseMoveGraphics(int x, int y);
	void mouseDoubleClickLeftGraphics(int x, int y);
	void groupKey(int groupIndex);
	void hotKey(char key);
	bool cancelPending();

	bool mouseValid(int x, int y){
		return computePosDisplay(x, y) != invalidPos; //in display coords
	}

	//misc
	void switchToNextDisplayColor(){
		display.switchColor();
	}

	void onSelectionChanged(){
		selection.update();
		resetState();
		computeDisplay();
	}

	void onSelectionStateChanged(){
		selection.update();
		computeDisplay();
	}
	bool getMinimapCell(int x, int y, Vec2i &cell);

private:
	void mouseDownLeftDisplay(int posDisplay);

	//orders
	void giveDefaultOrders(const Vec2i &targetPos, const Unit *targetUnit);
	void giveOneClickOrders();
	void giveTwoClickOrders(const Vec2i &targetPos, const Unit *targetUnit);

	//hotkeys
	void centerCameraOnSelection();
	void centerCameraOnLastEvent();
	void selectInterestingUnit(InterestingUnitType iut);
	void clickCommonCommand(CommandClass commandClass);

	//misc
	int computePosDisplay(int x, int y);
	void computeDisplay();
	void resetState();
	void mouseDownDisplayUnitSkills(int posDisplay);
	void mouseDownDisplayUnitBuild(int posDisplay);
	void computeInfoString(int posDisplay);
	void addOrdersResultToConsole(CommandClass cc, CommandResult rr);
	bool isSharedCommandClass(CommandClass commandClass);
	void computeSelected(bool doubleCkick);
	bool computeTarget(const Vec2i &screenPos, Vec2i &worldPos, const Unit *&targetUnit);
	void computeBuildPositions(const Vec2i &end);
};

}} //end namespace

#endif

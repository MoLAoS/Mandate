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

#ifndef _GLEST_GAME_GUI_H_
#define _GLEST_GAME_GUI_H_
#include "resource.h"
#include "command_type.h"
#include "display.h"
#include "item_display.h"
#include "production_display.h"
#include "carried_display.h"
#include "resources_display.h"
#include "stats_display.h"
#include "event_display.h"
#include "faction_display.h"
#include "map_display.h"
#include "commander.h"
#include "console.h"
#include "lua_console.h"
#include "selection.h"
#include "random.h"
#include "game_camera.h"
#include "keymap.h"
#include "xml_parser.h"

using Shared::Xml::XmlNode;
using Shared::Util::Random;

namespace Glest { namespace Gui {

using namespace Gui_Mandate;
class GameState;
class ResourceBar;
class UnitBar;
class TradeBar;

// =====================================================
//	class Mouse3d
// =====================================================

class Mouse3d {
public:
	static const float fadeSpeed;

private:
	bool enabled;
	int rot;
	float fade;
	Vec2i pos;

public:
	Mouse3d();

	void show(Vec2i pos);
	void update();

	bool isEnabled() const	{return enabled;}
	float getFade() const	{return fade;}
	int getRot() const		{return rot;}
	Vec2i getPos() const	{return pos;}
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

	bool isEnabled() const			{return enabled;}
	const Vec2i& getPosDown() const	{return posDown;}
	const Vec2i& getPosUp() const	{return posUp;}

	void setPosDown(const Vec2i &posDown);
	void setPosUp(const Vec2i &posUp);
	void disable();
};

// =====================================================
// 	class UserInterface
//
///	In game GUI
// =====================================================

class UserInterface : public sigslot::has_slots {
public:
	static const int cellWidthCount = Display::cellWidthCount;
	static const int cellHeightCount = Display::cellHeightCount;
	static const int maxSelBuff = 128 * 5;
	static const int upgradeDisplayIndex = cellWidthCount * 2;

	static const int autoRepairPos = cellWidthCount * cellHeightCount - 6;
	static const int autoAttackPos = cellWidthCount * cellHeightCount - 5;
	static const int autoFleePos = cellWidthCount * cellHeightCount - 4;
	static const int cloakTogglePos = cellWidthCount * cellHeightCount - 3;
	static const int meetingPointPos = cellWidthCount * cellHeightCount - 2;
	static const int cancelPos = cellWidthCount * cellHeightCount - 1;

	static const int imageCount = cellWidthCount * cellHeightCount;
	static const int invalidPos = -1;
	static const int doubleClickSelectionRadius = 20;
	static const int invalidGroupIndex = -1;

	typedef vector<Vec2i> BuildPositions;

private:
    vector <Event *> events;

	// External objects
	GameState &game;
	const Input &input;
	const Commander *commander;
	const World *world;
	GameCamera *gameCamera;

	// Widgets
	///@todo claim ChatDialog and GameMenu from GameState?
	Console *m_console;
	Console *m_dialogConsole;
	Minimap *m_minimap;
	Display *m_display;
	ItemWindow *m_itemWindow;
	CarriedWindow *m_carriedWindow;
	ProductionWindow *m_productionWindow;
	StatsWindow *m_statsWindow;
	EventWindow *m_eventWindow;
	ResourcesWindow *m_resourcesWindow;
	FactionDisplay *m_factionDisplay;
	MapDisplay *m_mapDisplay;
	ResourceBar *m_resourceBar;
	TradeBar *m_tradeBar;
	UnitBar *m_unitBar;
	LuaConsole *m_luaConsole;

	Random random;
	vector<int> taxes;

	// Positions
	Vec2i posObjWorld;		// world coords
	bool validPosObjWorld;
	bool computeSelection;

	// display
	const UnitType *choosenBuildingType;
	const CommandType *activeCommandType;
	CmdClass activeCommandClass;
	int activePos;

	// multi-build
	Vec2i dragStartPos;		// begining coord of multi-build command
	BuildPositions buildPositions;
	bool dragging;

	// composite
	Mouse3d mouse3d;
	Selection *selection;
	SelectionQuad selectionQuad;
	const MapObject *selectedObject;

	// states
	bool m_selectingSecond; // two tier command selection
	bool m_typeSelect;
	bool m_recruiting;
	CardinalDir m_selectedFacing; // facing for building
	bool selectingPos;
	bool selectingMeetingPoint;
	bool needSelectionUpdate;
	int currentGroup;
	const UnitType *m_assignmentType;

	bool	m_selectionDirty;	// selection state changed this frame, update it
	bool    m_teamCoulorMode;
	static UserInterface* currentGui;

public:
	UserInterface(GameState &game);
	~UserInterface();

	static UserInterface* getCurrentGui() {
		return currentGui;
	}

	void init();
	void initMinimap(bool fow, bool sod, bool resuming);

    void                    addEvent(Event *newEvent)    {events.push_back(newEvent);}
    void                    removeEvent()                {events.erase(events.begin());}
	void                    setActivePos(int i)          {activePos = i;}
	void                    setSelectingPos(bool b)      {selectingPos = b;}

	//get
	int                     getEventCount() const        { return events.size();      }
	Event*                  getEvent(int i) const        { return events[i];          }
	int                     getActivePos()               { return activePos;          }
	Minimap*                getMinimap()                 { return m_minimap;          }
	const Minimap*          getMinimap() const           { return m_minimap;          }
	ResourceBar*            getResourceBar()             { return m_resourceBar;      }
	UnitBar*                getUnitBar()                 { return m_unitBar;          }
	TradeBar*               getTradeBar()                { return m_tradeBar;         }
	const Input&            getInput() const             { return input;              }
	LuaConsole*             getLuaConsole()              { return m_luaConsole;       }
	Vec2i                   getPosObjWorld() const       { return posObjWorld;        }
	const UnitType*         getBuilding() const;
	const Vec2i&            getDragStartPos() const      { return dragStartPos;       }
	const BuildPositions&   getBuildPositions() const    { return buildPositions;     }
	CardinalDir             getBuildingFacing() const    { return m_selectedFacing;   }
	const Mouse3d*          getMouse3d() const           { return &mouse3d;           }
	Console*                getRegularConsole()          { return m_console;          }
	Console*                getDialogConsole()           { return m_dialogConsole;    }
	Display*                getDisplay()	             { return m_display;          }
	const Display*          getDisplay()	const        { return m_display;          }
	ItemWindow*             getItemWindow()	             { return m_itemWindow;       }
	const ItemWindow*       getItemWindow()	const        { return m_itemWindow;       }
	ProductionWindow*       getProductionWindow()	     { return m_productionWindow; }
	const ProductionWindow* getProductionWindow() const  { return m_productionWindow; }
	CarriedWindow*          getCarriedWindow()	         { return m_carriedWindow;    }
	const CarriedWindow*    getCarriedWindow() const     { return m_carriedWindow;    }
	StatsWindow*            getStatsWindow()	         { return m_statsWindow;      }
	const StatsWindow*      getStatsWindow() const       { return m_statsWindow;      }
	ResourcesWindow*        getStorageWindow()	         { return m_resourcesWindow;  }
	const ResourcesWindow*  getStorageWindow() const     { return m_resourcesWindow;  }
	EventWindow*            getEventWindow()	         { return m_eventWindow;      }
	const EventWindow*      getEventWindow() const       { return m_eventWindow;      }
	FactionDisplay*         getFactionDisplay()	         { return m_factionDisplay;   }
	const FactionDisplay*   getFactionDisplay() const    { return m_factionDisplay;   }
	MapDisplay*             getMapDisplay()	             { return m_mapDisplay;       }
	const MapDisplay*       getMapDisplay() const        { return m_mapDisplay;       }
	const Selection*        getSelection()	const        { return selection;          }
	Selection*              getSelection()               { return selection;          }
	const SelectionQuad*    getSelectionQuad() const     { return &selectionQuad;     }
	const MapObject*        getSelectedObject() const    { return selectedObject;     }

	bool isValidPosObjWorld() const         { return validPosObjWorld;          }
	bool isSelecting() const                { return selectionQuad.isEnabled(); }
	bool isSelectingPos() const             { return selectingPos;              }
	bool isSelected(const Unit *unit) const { return selection->hasUnit(unit);  }
	bool isPlacingBuilding() const;
	bool isDragging() const                 { return dragging;                  }
	bool isNeedSelectionUpdate() const      { return needSelectionUpdate;       }
	bool isTeamColourModeActive() const     { return m_teamCoulorMode;          }

	//set
	void invalidatePosObjWorld()            { validPosObjWorld= false;          }
	void setComputeSelectionFlag()          { computeSelection= true;           }

	//events
	void update();

	void tick();

	void onSelectionUpdated();

	void panelButtonPressed(int posDisplay);
	void taxButtonPressed(int posDisplay);
	void commandButtonPressed(int posDisplay);
	void unloadRequest(int carryIndex);
	void degarrisonRequest(int carryIndex);
	void factionUnloadRequest(int carryIndex);

    void formationButtonPressed(int posDisplay);
    void hierarchyButtonPressed(int posDisplay);

	void mouseDownLeft(int x, int y);
	void mouseDownRight(int x, int y);
	void mouseUpLeft(int x, int y);
	void mouseUpRight(int x, int y);
	void mouseDoubleClickLeft(int x, int y);
	void mouseMove(int x, int y);

	void groupKey(int groupIndex);
	void hotKey(UserCommand cmd);
	bool cancelPending();

	//bool mouseValid(int x, int y){
	//	return computePosDisplay(x, y) != invalidPos; //in display coords
	//}

	// slots for signals from minimap
	void onLeftClickOrder(Vec2i cellPos);
	void onRightClickOrder(Vec2i cellPos);

	// slots for resource depletion and unit death
    void onResourceDepleted(Vec2i cellPos);
	void onUnitDied(Unit *unit);

	//misc
	void onSelectionChanged() { resetState(); }
	void onSelectionStateChanged() { m_selectionDirty = true; }
	void invalidateActivePos() { activePos = invalidPos; }

	void load(const XmlNode *node);
	void save(XmlNode *node) const;

private:
	//void mouseDownLeftDisplay(int posDisplay);

	// orders
	void giveDefaultOrders(const Vec2i &targetPos, Unit *targetUnit);
	void giveOneClickOrders();
	void giveTwoClickOrders(const Vec2i &targetPos, Unit *targetUnit);

	// hotkeys
	void centerCameraOnSelection();
	void centerCameraOnLastEvent();
	void selectInterestingUnit(InterestingUnitType iut);
	void clickCommonCommand(CmdClass commandClass);

	// 'close' (hide) LuaConsole
	void onCloseLuaConsole(Widget*);

	// handle command button-click / hotkey
	void onFirstTierSelect(int posDisplay);
	// handle producible button-click / hotkey(?)
	void onSecondTierSelect(int posDisplay);

    void onFormationSelect(int posDisplay);
    void onHierarchySelect(int posDisplay);

	// misc
	void addOrdersResultToConsole(CmdClass cc, CmdResult rr);
	bool isSharedCommandClass(CmdClass commandClass);

public:
	void resetState(bool redoDisplay = true);
	void computeDisplay();
	void computePortraitInfo(int posDisplay);
	void computeTaxInfo(int posDisplay);
	void computeCommandInfo(int posDisplay);
	void computeFormationInfo(int posDisplay);
	void computeHierarchyInfo(int posDisplay);
	void updateSelection(bool doubleClick, UnitVector &units);

private:
	bool computeTarget(const Vec2i &screenPos, Vec2i &worldPos, UnitVector &units, const MapObject *&obj);
	void computeBuildPositions(const Vec2i &end);
	void computeSelectionPanel();
	void computeHousedUnitsPanel();
	void computeGarrisonedUnitsPanel();
	void computeTaxPanel();
	void computeCommandPanel();
	void computeFormationPanel();
	void computeHierarchyPanel();
	void computeCommandTip(const CommandType *ct, const ProducibleType *pt = 0);
	void computeFormationTip(FormationCommand fc);
	void computeHierarchyTip(const CommandType *hc);
	void selectAllUnitsOfType(UnitVector &out_units, const Unit *refUnit, int radius);
};

}} //end namespace

#endif

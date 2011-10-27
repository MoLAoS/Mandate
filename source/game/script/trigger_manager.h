// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2009-2010 James McCulloch
//                2009-2010 Nathan Turner
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_SCRIPT_TRIGGER_MANAGER_H_
#define _GLEST_SCRIPT_TRIGGER_MANAGER_H_

#include <string>
#include <queue>
#include <set>
#include <map>
#include <limits>

#include "lua_script.h"
#include "vec.h"
#include "timer.h"

#include "logger.h"

#include "game_constants.h"
#include "forward_decs.h"

namespace Glest {
namespace Gui {
	class LuaConsole;
}
using Gui::LuaConsole;
using namespace Shared::Lua;
using namespace Shared::Math;

using Shared::Platform::Chrono;
using std::queue;
using std::set;
using std::numeric_limits;

using Entities::Unit;
using Gui::Console;
using Gui::GameState;
using Sim::World;
	
namespace Script {

// =====================================================
//	class ScriptTimer
// =====================================================

class ScriptTimer {
private:
	string name;
	bool real;
	bool periodic;
	int64 targetTime;
	int64 interval;
	bool active;

public:
	ScriptTimer(const string &name, bool real, int interval, bool periodic)
		: name(name), real(real), periodic(periodic), interval(interval), active(true) {
			reset();
	}

	const string &getName()	const	{return name;}
	bool isPeriodic() const			{return periodic;}
	bool isAlive() const			{return active;}
	bool isReady() const;

	void kill()						{active = false;}
	void reset();
};

// =====================================================
//	class Region, and Derivitives
// =====================================================

struct Region {
	virtual bool isInside(const Vec2i &pos) const = 0;
};

struct Rect : public Region {
	int x, y, w, h; // top-left coords + width and height

	Rect() : x(0), y(0), w(0), h(0) { }
	Rect(const int v) : x(v), y(v), w(v), h(v) { }
	Rect(const Vec4i &v) : x(v.x), y(v.y), w(v.z), h(v.w) { }
	Rect(const int x, const int y, const int w, const int h) : x(x), y(y), w(w), h(h) { }

	virtual bool isInside(const Vec2i &pos) const {
		return pos.x >= x && pos.y >= y && pos.x < x + w && pos.y < y + h;
	}
};

struct Circle : public Region {
	int x, y; // centre
	float radius;

	Circle() : x(-1), y(-1), radius(numeric_limits<float>::quiet_NaN()) { }
	Circle(const Vec2i &pos, const float radius) : x(pos.x), y(pos.y), radius(radius) { }
	Circle(const int x, const int y, const float r) : x(x), y(y), radius(r) { }

	virtual bool isInside(const Vec2i &pos) const {
		return pos.dist(Vec2i(x,y)) <= radius;
	}
};

struct CompoundRegion : public Region {
	vector<Region*> regions;

	CompoundRegion() { }
	CompoundRegion(Region *ptr) { regions.push_back(ptr); }

	template<typename InIter>
	void addRegions(InIter start, InIter end) {
		copy(start,end,regions.end());
	}

	virtual bool isInside(const Vec2i &pos) const {
		for ( vector<Region*>::const_iterator it = regions.begin(); it != regions.end(); ++it ) {
			if ( (*it)->isInside(pos) ) {
				return true;
			}
		}
		return false;
	}
};

// =====================================================
// some trigger info structs...
// =====================================================

struct PosTrigger {
	Region *region;
	string evnt;
	int user_dat;
	PosTrigger() : region(NULL), evnt(""), user_dat(0) {}
};

struct Trigger {
	string evnt;
	int user_dat;
	Trigger() : evnt(""), user_dat(0) {}
};

WRAPPED_ENUM( SetTriggerRes
	, OK
 	, BAD_UNIT_ID
	, BAD_FACTION_INDEX
	, UNKNOWN_EVENT
	, UNKNOWN_REGION
	, DUPLICATE_TRIGGER
	, INVALID_THRESHOLD
)

// =====================================================
//	class TriggerManager
// =====================================================

class TriggerManager {
	typedef map<string,Region*>			Regions;
	typedef set<string>					Events;
	typedef vector<PosTrigger>			PosTriggers;
	typedef map<int,PosTriggers>		PosTriggerMap;
	typedef map<int,Trigger>			TriggerMap;

	Events  events;
	Regions regions;

	PosTriggerMap	unitPosTriggers;
	PosTriggerMap	factionPosTriggers;
	TriggerMap		attackedTriggers;
	TriggerMap		hpBelowTriggers;
	TriggerMap		hpAboveTriggers;
	TriggerMap		commandCallbacks;
	TriggerMap		deathTriggers;

public:
	TriggerManager() {}
	~TriggerManager();

	/** clean-up */
	void reset();

	/** register a rectangular region @param name unique region name @param rect the rectangle (x,y,w,h) */
	bool registerRegion(const string &name, const Rect &rect);

	/** register an event @param name unique event name */
	int  registerEvent(const string &name);

	/** get region by name */ 
	const Region* getRegion(string &name) {
		Regions::iterator it = regions.find(name);
		if ( it == regions.end() ) return NULL;
		else return it->second;
	}

public:
	// Set Triggers...

	/** set a position trigger on a unit
	  * @param unitId id of unit to put trigger on 
	  * @param region the name of a registered region
	  * @param eventName name of the event to fire
	  * @param userData optional extra data to send to the event handler
	  * @return 0 on success, -1 if unitId in not valid, -2 if event is not registered,
	  * -3 if region not registered, -4 if unit allready has a trigger for this region/event pair */
	SetTriggerRes addUnitPosTrigger(int unitId, const string &region, const string &eventName, int userData=0);

	bool removeUnitPosTriggers(int unitId);

	/** set a position trigger for all units of a faction
	  * @param ndx index of the faction to put the trigger on
	  * @param region the name of a registered region 
	  * @param eventName name of the event to fire
	  * @param userData optional extra data to send to the event handler */
	SetTriggerRes addFactionPosTrigger(int ndx, const string &region, const string &eventName, int userData);

	/** set a 'command complete trigger' on a unit
	  * @param unitId id of unit to put trigger on 
	  * @param eventName name of the event to fire when current command finishes
	  * @param userData optional extra data to send to the event handler
	  * @return 0 on success, -1 if unitId in not valid */
	SetTriggerRes addCommandCallback(int unitId, const string &eventName, int userData=0);

	/** set a 'hp below trigger' on a unit
	  * @param unitId id of unit to put trigger on 
	  * @param threshold event fires when the units hp falls to this or below
	  * @param eventName name of the event to fire
	  * @param userData optional extra data to send to the event handler
	  * @return 0 on success, -1 if unitId in not valid, -2 if units hp is currently below threshold */
	SetTriggerRes addHPBelowTrigger(int unitId, int threshold, const string &eventName, int userData=0);

	/** set a 'hp above trigger' on a unit
	  * @param unitId id of unit to put trigger on 
	  * @param threshold event fires when the units hp rises to this or above
	  * @param eventName name of the event to fire
	  * @param userData optional extra data to send to the event handler
	  * @return 0 on success, -1 if unitId in not valid, -2 if units hp is currently above threshold */
	SetTriggerRes addHPAboveTrigger(int unitId, int threshold, const string &eventName, int userData=0);

	/** set an 'attacked trigger' on a unit
	  * @param unitId id of unit to put trigger on 
	  * @param eventName name of the event to fire when the unit is attacked
	  * @param userData optional extra data to send to the event handler */
	SetTriggerRes addAttackedTrigger(int unitId, const string &eventName, int userData=0);

	/** set a 'death trigger' on a unit
	  * @param unitId id of unit to put trigger on 
	  * @param eventName name of the event to fire when current the unit dies
	  * @param userData optional extra data to send to the event handler */
	SetTriggerRes addDeathTrigger(int unitId, const string &eventName, int userData=0);

private:
	// generic trigger check & fire (for the simple trigger conditions)
	void checkTrigger(TriggerMap &triggerMap, const Unit *unit);

public:
	// Engine interface, actually all called from ScriptManager.

	/** checks position triggers, called whenever a unit is moved or created, unloaded, etc? */
	void unitMoved(const Unit *unit);

	/** check death triggers and removes any other triggers for unit */
	void unitDied(const Unit *unit);

	/** called when a unit notices a 'watched' command is no longer being executed */
	void commandCallback(const Unit *unit) {checkTrigger(commandCallbacks, unit);}

	/** called when a unit with a hp-below trigger set has its hp decreased to or below the threshold  */
	void onHPBelow(const Unit *unit) {checkTrigger(hpBelowTriggers, unit);}

	/** called when a unit with a hp-above trigger set has its hp increased to or above the threshold  */
	void onHPAbove(const Unit *unit) {checkTrigger(hpAboveTriggers, unit);}

	/** called when a unit with an attacked-trigger set takes damage from an attack */
	void onAttacked(const Unit *unit) {checkTrigger(attackedTriggers, unit);}
};

}}//end namespace

#endif

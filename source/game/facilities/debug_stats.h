// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================
#ifndef _GLEST_DEBUG_DEBUGSTATS_INCLUDED_
#define _GLEST_DEBUG_DEBUGSTATS_INCLUDED_

#include <deque>
#include "types.h"
#include "timer.h"
#include "game_constants.h"
#include "util.h"

namespace Glest { namespace Debug {

using std::stringstream;
using namespace Shared::Platform;

STRINGY_ENUM( TimerSection,

	RENDER_2D,
	RENDER_3D,
	RENDER_SURFACE,
	RENDER_WATER,
	RENDER_MODELS,
	RENDER_OBJECTS,
	RENDER_UNITS,
	RENDER_SHADOWS,
	RENDER_SELECT,

	PATHFINDER_TOTAL,
	PATHFINDER_LOWLEVEL,
	PATHFINDER_HIERARCHICAL//,
	//AI_TOTAL
)

STRINGY_ENUM( DebugSection,
	PERFORMANCE,
	RENDERER,
	CAMERA,
	GUI,
	WORLD,
	RESOURCES,
	CLUSTER_MAP,
	PARTICLE_USE
)


class DebugStats {
public:
	typedef std::deque<int64> TickRecords;

private:
	// Performance
	Chrono		m_totalTimers[TimerSection::COUNT];
	Chrono		m_currentTickTimers[TimerSection::COUNT];
	TickRecords	m_tickRecords[TimerSection::COUNT];

	bool		m_reportFlags[TimerSection::COUNT];
	bool		m_reportTotals;
	bool		m_reportLastTick;
	bool		m_reportAvgLast5;
	bool		m_reportRatio;

	string		m_sectoinNames[TimerSection::COUNT];

	int64		m_startTime;


	// Debug sections
	bool		m_debugSections[DebugSection::COUNT];

	int			m_lastRenderFps, m_lastWorldFps;

	string		m_performanceReportCache;

private:
	int64 avg(const TickRecords &records);
	void reportTotal(TimerSection section, stringstream &stream);
	void reportLast(TimerSection section, stringstream &stream);
	void reportLast5(TimerSection section, stringstream &stream);
	float getTimeRatio(TimerSection section) const;
	void doPerformanceReport();
	void reportPerformance(ostream &stream) { stream << m_performanceReportCache; }

public:
	DebugStats();

	void init();
	void enterSection(TimerSection section);
	void exitSection(TimerSection section);
	void tick(int renderFps, int worldFps);

	void report(ostream &stream);
};

extern DebugStats *g_debugStats; // hokey pokey

struct StackTimer {
	TimerSection m_section;
	StackTimer(TimerSection section) : m_section(section) {
		g_debugStats->enterSection(m_section);
	}
	~StackTimer() {
		g_debugStats->exitSection(m_section);
	}
};

#define SECTION_TIMER(section) StackTimer section##_stackTimer(TimerSection::section)

}}

#endif

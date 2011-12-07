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

#include "properties.h"

namespace Glest { namespace Debug {

using std::stringstream;
using Shared::Util::Properties;
using namespace Shared::Platform;

STRINGY_ENUM( TimerSection,
	RENDER_2D,
	RENDER_3D,
	RENDER_SWAP_BUFFERS,
	RENDER_SURFACE,
	RENDER_WATER,
	RENDER_INTERPOLATE,
	RENDER_MODELS,
	RENDER_OBJECTS,
	RENDER_UNITS,
	RENDER_SHADOWS,
	RENDER_SELECT,

	WORLD_TOTAL,

	PATHFINDER_TOTAL,
	PATHFINDER_LOWLEVEL,
	PATHFINDER_HIERARCHICAL//,
	//AI_TOTAL
)

STRINGY_ENUM( TimerReportFlag,
	LAST_SEC,
	LAST_5_SEC,
	TOTAL_TIME,
	TOTAL_RATIO
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

	//string		m_sectionNames[TimerSection::COUNT];
	bool		m_reportSections[TimerSection::COUNT];
	bool        m_reportFlags[TimerReportFlag::COUNT];

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

	void loadConfig();
	void saveConfig();
	void init();

	void enterSection(TimerSection section) {
		m_totalTimers[section].start();
		m_currentTickTimers[section].start();
	}
	void exitSection(TimerSection section) {
		m_totalTimers[section].stop();
		m_currentTickTimers[section].stop();
	}
	void tick(int renderFps, int worldFps);

	bool isEnabled(DebugSection section) const { return m_debugSections[section]; }
	bool isEnabled(TimerSection section) const { return m_reportSections[section]; }
	bool isEnabled(TimerReportFlag flag) const { return m_reportFlags[flag]; }

	void setEnabled(DebugSection section, bool enable) { m_debugSections[section] = enable; }
	void setEnabled(TimerSection section, bool enable) { m_reportSections[section] = enable; }
	void setEnabled(TimerReportFlag flag, bool enable) { m_reportFlags[flag] = enable; }

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

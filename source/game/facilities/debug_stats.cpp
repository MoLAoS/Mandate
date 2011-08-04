// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "debug_stats.h"
#include "conversion.h"
#include "renderer.h"
#include "game_camera.h"
#include "game.h"
#include "cluster_map.h"
#include "properties.h"
#include "util.h"

namespace Glest { namespace Debug {

using Graphics::Renderer;
using Gui::GameCamera;
using namespace Shared::Util;
using namespace Shared::Debug;

DebugStats *g_debugStats = 0;

string formatEnumName(const string &enumName) {
	string result;
	bool cap = true;
	foreach_const(string, it, enumName) {
		const char &c = *it;
		if (cap) {
			result.push_back(c);
			cap = false;
		} else if (c == '_') {
			result.push_back(' ');
			cap = true;
		} else if (isalpha(c) && isupper(c)) {
			result.push_back(tolower(c));
		} else {
			result.push_back(c);
		}
	}
	return result;
}

int64 DebugStats::avg(const TickRecords &records) {
	if (records.empty()) {
		return 0;
	}
	int64 sum = 0;
	foreach_const (TickRecords, it, records) {
		sum += *it;
	}
	return sum / records.size();
}

DebugStats::DebugStats() {
	loadConfig();	
	m_lastRenderFps = 0;
	m_lastWorldFps = 0;
	foreach_enum (TimerSection, s) {
		m_currentTickTimers[s] = Chrono();
		m_totalTimers[s] = Chrono();
	}
	foreach_enum (TimerSection, s) {
		int a = m_currentTickTimers[s].getMillis();
		if (a) {
			DEBUG_HOOK();
		}

		assert(m_currentTickTimers[s].getMillis() == 0);
		assert(m_totalTimers[s].getMillis() == 0);
	}
}

void DebugStats::loadConfig() {
	Properties p;
	if (fileExists("debug.ini")) {
		try {
			p.load("debug.ini");
		} catch (std::exception &e) {
		}
	}
	foreach_enum (DebugSection, ds) {
		m_debugSections[ds] = p.getBool(DebugSectionNames[ds], false);
	}
	foreach_enum (TimerSection, ts) {
		m_reportSections[ts] = p.getBool(TimerSectionNames[ts], false);
	}
	foreach_enum (TimerReportFlag, trf) {
		m_reportFlags[trf] = p.getBool(TimerReportFlagNames[trf], false);
	}
	if (!fileExists("debug.ini")) {
		p.save("debug.ini");
	}
}

void DebugStats::saveConfig() {
	Properties p;
	foreach_enum (DebugSection, ds) {
		p.setBool(DebugSectionNames[ds], m_debugSections[ds]);
	}
	foreach_enum (TimerSection, ts) {
		p.setBool(TimerSectionNames[ts], m_reportSections[ts]);
	}
	foreach_enum (TimerReportFlag, trf) {
		p.setBool(TimerReportFlagNames[trf], m_reportFlags[trf]);
	}
	p.save("debug.ini");
}

void DebugStats::init() {
	m_startTime = Chrono::getCurMillis();
}

float DebugStats::getTimeRatio(TimerSection section) const {
	float totalElapsed = float(Chrono::getCurMillis() - m_startTime);
	return m_totalTimers[section].getMillis() / totalElapsed;
}

void DebugStats::tick(int renderFps, int worldFps) {
	m_lastRenderFps = renderFps;
	m_lastWorldFps = worldFps;
	foreach_enum (TimerSection, s) {
		int64 time = m_currentTickTimers[s].getMillis();
		m_currentTickTimers[s].reset();
		m_tickRecords[s].push_back(time);
		if (m_tickRecords[s].size() > 5) {
			m_tickRecords[s].pop_front();
		}
	}

	doPerformanceReport();
}

void DebugStats::reportTotal(TimerSection section, stringstream &stream) {
	int64 time = m_totalTimers[section].getMillis();
	stream << "   " << formatEnumName(TimerSectionNames[section]) << " : " << formatTime(time) << endl;
}

void DebugStats::reportLast(TimerSection section, stringstream &stream) {
	int64 time = m_tickRecords[section].empty() ? int64(0) : m_tickRecords[section].back();
	stream << "   " << formatEnumName(TimerSectionNames[section]) << " : " << formatTime(time) << endl;
}

void DebugStats::reportLast5(TimerSection section, stringstream &stream) {
	int64 time = avg(m_tickRecords[section]);
	stream << "   " << formatEnumName(TimerSectionNames[section]) << " : " << formatTime(time) << endl;
}

void DebugStats::report(ostream &stream) {
	if (m_debugSections[DebugSection::RENDERER]) {
		Renderer &renderer = g_renderer;
		stream << "\nRender Stats:\n"
			<< "   Frames Per Sec: " << m_lastRenderFps << endl
			<< "   Triangle count: " << renderer.getTriangleCount() << endl
			<< "   Vertex count: " << renderer.getPointCount() << endl;
	}
	if (m_debugSections[DebugSection::CAMERA]) {
		const GameCamera &gameCamera = *g_gameState.getGameCamera();
		stream << "\nCamera Info:\n"
			<< "   GameCamera pos: " << gameCamera.getPos() << endl
			<< "   Camera VAng : " << gameCamera.getVAng() << endl;
	}
	if (m_debugSections[DebugSection::GUI]) {
		stream << "\nGUI stats:\n"
			<< "   Mouse Pos (screen coords): " << g_gameState.getMousePos() << endl
			<< "   Last Click Pos (cell coords): " << g_userInterface.getPosObjWorld() << endl;
	}
	if (m_debugSections[DebugSection::WORLD]) {
		stream << "\nWorld stats:\n"
			<< "   Frames per Sec: " << m_lastWorldFps << endl
			<< "   Total frame count: " << g_world.getFrameCount() << endl
			<< "   Time of day: " << g_world.getTimeFlow()->describeTime() << endl;
	}
	if (m_debugSections[DebugSection::PERFORMANCE]) {
		stream << "\nPerformance stats:\n"
			<< m_performanceReportCache;
	}
	if (m_debugSections[DebugSection::RESOURCES]) {
		const World &world = g_world;
		stream << "\nPlayer Resources:\n";
		for (int i=0; i < world.getFactionCount(); ++i) {
			stream << "   Player " << i << " res: ";
			for (int j=0; j < world.getTechTree()->getResourceTypeCount(); ++j) {
				stream << world.getFaction(i)->getResource(j)->getAmount() << " ";
			}
			stream << endl;
		}
	}
	if (m_debugSections[DebugSection::CLUSTER_MAP]) {
		stream << "ClusterMap size (Field::LAND):\n"
			<< "   Nodes = " << Search::Transition::NumTransitions(Field::LAND) << endl
			<< "   Edges = " << Search::Edge::NumEdges(Field::LAND) << endl;
	}
	if (m_debugSections[DebugSection::PARTICLE_USE]) {
		stream << "Particle usage counts:\n";
		foreach_enum (ParticleUse, use) {
			stream << "   " << ParticleUseNames[use] << " : " << ParticleSystem::getParticleUse(use) << endl;
		}
	}
}

void DebugStats::doPerformanceReport() {
	if (!m_debugSections[DebugSection::PERFORMANCE]) {
		m_performanceReportCache = "No data.\n";
		return;
	}
	stringstream stream;
	if (m_reportFlags[TimerReportFlag::TOTAL_TIME]) {
		stream << "Total time taken this game:\n";
		foreach_enum (TimerSection, s) {
			if (m_reportSections[s]) {
				reportTotal(s, stream);
			}
		}
	}
	if (m_reportFlags[TimerReportFlag::LAST_SEC]) {
		stream << "Time taken in the last second:\n";
		foreach_enum (TimerSection, s) {
			if (m_reportSections[s]) {
				reportLast(s, stream);
			}
		}
	}
	if (m_reportFlags[TimerReportFlag::LAST_5_SEC]) {
		stream << "Average time per sec in the last 5:\n";
		foreach_enum (TimerSection, s) {
			if (m_reportSections[s]) {
				reportLast5(s, stream);
			}
		}
	}
	if (m_reportFlags[TimerReportFlag::TOTAL_RATIO]) {
		stream << "Percentage of time since game start:\n";
		foreach_enum (TimerSection, s) {
			if (m_reportSections[s]) {
				stream << "   " << formatEnumName(TimerSectionNames[s]) << " : " << (getTimeRatio(s) * 100.f) << " %" << endl;
			}
		}
	}
	m_performanceReportCache = stream.str();
}

}}

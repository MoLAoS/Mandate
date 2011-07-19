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

namespace Glest { namespace Debug {

using Graphics::Renderer;
using Gui::GameCamera;
using namespace Shared::Util;

DebugStats *g_debugStats = 0;

inline string formatTime(int64 ms) {
	int sec = ms / 1000;
	if (sec) {
		int millis = ms % 1000;
		return intToStr(sec) + " s, " + intToStr(millis) + " ms.";
	}
	return intToStr(int(ms)) + " ms.";
}

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
	// sections
	m_debugSections[DebugSection::PERFORMANCE] = true;
	m_debugSections[DebugSection::RENDERER] = true;
	m_debugSections[DebugSection::CAMERA] = false;
	m_debugSections[DebugSection::GUI] = true;
	m_debugSections[DebugSection::WORLD] = true;
	m_debugSections[DebugSection::RESOURCES] = false;
	m_debugSections[DebugSection::CLUSTER_MAP] = false;
	m_debugSections[DebugSection::PARTICLE_USE] = false;

	// performance sections
	foreach_enum (TimerSection, s) {
		m_reportFlags[s] = false;
		m_sectoinNames[s] = formatEnumName(TimerSectionNames[s]);
	}
	m_reportFlags[TimerSection::RENDER_3D] = true;
	m_reportFlags[TimerSection::RENDER_SURFACE] = true;
	m_reportFlags[TimerSection::RENDER_WATER] = true;
	m_reportFlags[TimerSection::RENDER_OBJECTS] = true;
	m_reportFlags[TimerSection::RENDER_UNITS] = true;
	m_reportFlags[TimerSection::RENDER_SHADOWS] = true;

	// performance stats to show
	m_reportTotals = false;
	m_reportLastTick = false;
	m_reportAvgLast5 = true;
	m_reportRatio = false;
	
	m_lastRenderFps = 0;
	m_lastWorldFps = 0;
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
	stream << "   " << m_sectoinNames[section] << " : " << formatTime(time) << endl;
}

void DebugStats::reportLast(TimerSection section, stringstream &stream) {
	int64 time = m_tickRecords[section].empty() ? 0 : m_tickRecords[section].back();
	stream << "   " << m_sectoinNames[section] << " : " << formatTime(time) << endl;
}

void DebugStats::reportLast5(TimerSection section, stringstream &stream) {
	int64 time = avg(m_tickRecords[section]);
	stream << "   " << m_sectoinNames[section] << " : " << formatTime(time) << endl;
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
	if (m_reportTotals) {
		stream << "Total time taken this game:\n";
		foreach_enum (TimerSection, s) {
			if (m_reportFlags[s]) {
				reportTotal(s, stream);
			}
		}
	}
	if (m_reportLastTick) {
		stream << "Time taken in the last second:\n";
		foreach_enum (TimerSection, s) {
			if (m_reportFlags[s]) {
				reportLast(s, stream);
			}
		}
	}
	if (m_reportAvgLast5) {
		stream << "Average time taken (per sec) in the last 5 sec:\n";
		foreach_enum (TimerSection, s) {
			if (m_reportFlags[s]) {
				reportLast5(s, stream);
			}
		}
	}

	if (m_reportRatio) {
		stream << "Percentage of time since game start:\n";
		foreach_enum (TimerSection, s) {
			if (m_reportFlags[s]) {
				stream << "   " << m_sectoinNames[s] << " : " << (getTimeRatio(s) * 100.f) << " %" << endl;
			}
		}
	}
	m_performanceReportCache = stream.str();
}

void DebugStats::init() {
	m_startTime = Chrono::getCurMillis();
}

void DebugStats::enterSection(TimerSection section) {
	m_totalTimers[section].start();
	m_currentTickTimers[section].start();
}

void DebugStats::exitSection(TimerSection section) {
	m_totalTimers[section].stop();
	m_currentTickTimers[section].stop();
}



}}

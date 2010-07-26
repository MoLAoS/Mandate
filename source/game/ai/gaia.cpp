// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "gaia.h"
#include "sim_interface.h"
#include "unit_type.h"

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

namespace Glest { namespace Plan {
using namespace Search;

Gaia::Gaia(Faction *f) : m_faction(f), m_rand(0) {}

Gaia::~Gaia() {
}

void jumble(vector<Vec2i> &list, Random &rand) {
	for (int i=0; i < list.size(); ++i) {
		int j = rand.randRange(0, list.size());
		if (i == j) continue;
		swap(list[i], list[j]);
	}
}

const int minUpdateDelay = 800;
const int maxUpdateDelay = 2400;

void Gaia::init() {
	vector<const UnitType*> typesByField[Field::COUNT];
	for (int i=0; i < m_faction->getType()->getUnitTypeCount(); ++i) {
		Field f = m_faction->getType()->getUnitType(i)->getField();
		typesByField[f].push_back(m_faction->getType()->getUnitType(i));
	}

	foreach_enum (Field, f) {
		if (typesByField[f].empty()) {
			continue;
		}
		g_cartographer.buildGlestimalMap(f, m_spawnPoints[f]);

		size_t mapMax = g_map.getW() * g_map.getH() / 2048;
		m_maxCount[f] = std::min(mapMax, m_spawnPoints[f].size());
		cout << "\nMax Glestimals in Field::" << FieldNames[f] << " == " << m_maxCount;

		jumble(m_spawnPoints[f], m_rand);

		for (int i=0; i < m_maxCount[f]; ++i) {
			int ui = m_rand.randRange(0, typesByField[f].size());
			const UnitType *ut = typesByField[f][ui];
			Vec2i pos = m_spawnPoints[f][i];
			Unit *glestimal = g_simInterface->getUnitFactory().newInstance(pos, ut, m_faction, &g_map);

			if (g_world.placeUnit(pos, 5, glestimal)) {
				glestimal->create();
				m_updateTable[glestimal->getId()] = m_rand.randRange(minUpdateDelay, maxUpdateDelay);
			} else {
				g_simInterface->getUnitFactory().deleteUnit(glestimal);
				cout << "\nCould not place starting glestimal :(\n";
			}
		}
	}
}

IF_DEBUG_EDITION(
	void Gaia::showSpawnPoints() {
		foreach (vector<Vec2i>, it, m_spawnPoints[Field::LAND]) {
			g_debugRenderer.addCellHighlight(*it);
		}
		g_debugRenderer.regionHilights = true;
	}
)

void Gaia::update() {
	const Units &units = m_faction->getUnits();
	const int frame = g_world.getFrameCount();
	foreach_const (Units, it, units) {
		Unit *glestimal = *it;
		if (m_updateTable[glestimal->getId()] == frame) {
			m_updateTable[glestimal->getId()] = frame + m_rand.randRange(minUpdateDelay, maxUpdateDelay);
			const UnitType *ut = glestimal->getType();

			const CommandType *ct = ut->getFirstCtOfClass(CommandClass::ATTACK);
			if (!ct) {
				ct = ut->getFirstCtOfClass(CommandClass::MOVE);
			}
			if (!ct) {
				continue;
			}
			Vec2i target = glestimal->getPos() + 
				Vec2i(m_rand.randRange(-12,12), m_rand.randRange(-12,12));
			g_map.clampPos(target);
			glestimal->giveCommand(new Command(ct, CommandFlags(), target));
		}
	}
}

}}
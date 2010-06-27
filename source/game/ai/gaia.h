// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_AI_GAIA_H_
#define _GLEST_AI_GAIA_H_

#include "ai_interface.h"
#include "cartographer.h"
#include "random.h"

using Shared::Util::Random;

namespace Glest { namespace Plan {

class Gaia : public AiInterface {
private:
	Faction*		m_faction;
	vector<Vec2i>	m_spawnPoints[Field::COUNT];
	size_t			m_maxCount[Field::COUNT];
	Random			m_rand;
	map<int, int>	m_updateTable;

public:
	Gaia(Faction *glestimalFaction);
	~Gaia();

	void init();
	void update();

	IF_DEBUG_EDITION(
		void showSpawnPoints();
	)
};

}}

#endif

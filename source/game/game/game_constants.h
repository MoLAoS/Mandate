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

#ifndef _GLEST_GAME_GAMECONSTANTS_H_
#define _GLEST_GAME_GAMECONSTANTS_H_

// The 'Globals'
#define g_gameState			(*GameState::getInstance())
#define g_world				(World::getInstance())
#define g_map				(*World::getInstance().getMap())
#define g_camera			(*GameState::getInstance()->getGameCamera())
#define g_gameSettings		(GameState::getInstance()->getGameSettings())
#define g_userInterface		(*Gui::UserInterface::getCurrentGui())
#define g_console			(*GameState::getInstance()->getConsole())
#define g_config			(Config::getInstance())
#define g_routePlanner		(*World::getInstance().getRoutePlanner())
#define g_cartographer		(*World::getInstance().getCartographer())
#define g_renderer			(Renderer::getInstance())
#define g_soundRenderer		(SoundRenderer::getInstance())
#define g_logger			(Logger::getInstance())
#define g_errorLog			(Logger::getErrorLog())
#define g_lang				(Lang::getInstance())
#define g_metrics			(Metrics::getInstance())
#define g_coreData			(CoreData::getInstance())
#define g_fileFactory		(*FSFactory::getInstance())
#define g_simInterface		(Program::getInstance()->getSimulationInterface())

#if _GAE_DEBUG_EDITION_
#	define g_debugRenderer	(Glest::Debug::getDebugRenderer())
#	define IF_DEBUG_EDITION(x) x
#	define IF_NOT_DEBUG_EDITION(x)
#else
#	define IF_DEBUG_EDITION(x)
#	define IF_NOT_DEBUG_EDITION(x) x
#endif

#define WORLD_FPS (GameConstants::updateFps / GameConstants::defaultUpdateInterval)

#include "util.h"
using Shared::Util::EnumNames;

#include "simulation_enums.h"
#include "prototypes_enums.h"
#include "entities_enums.h"
#include "search_enums.h"

namespace Glest {
	namespace Net {

		STRINGY_ENUM(
			MessageType,
					NO_MSG,
					INTRO,
					PING,
					AI_SYNC,
					READY,
					LAUNCH,
					GAME_SPEED,
					COMMAND_LIST,
					TEXT,
					KEY_FRAME,
					SKILL_CYCLE_TABLE,
					SYNC_ERROR,
					QUIT,
					INVALID_MSG
		)
	}
}

namespace Glest { 

#ifndef NDEBUG
	void no_op();
#	define DEBUG_HOOK() no_op()
#else
#	define DEBUG_HOOK()
#endif


#define LOG_STUFF 1

#if defined(LOG_STUFF) && LOG_STUFF
#	define LOG(x) g_logger.add(x)
#	define STREAM_LOG(x) {stringstream ss; ss << x; g_logger.add(ss.str()); }
#	define GAME_LOG(x) STREAM_LOG( "Frame " << g_world.getFrameCount() << ": " << x )
#else
#	define LOG(x)
#	define STREAM_LOG(x)
#	define GAME_LOG(x)
#endif

// =====================================================
//	namespace GameConstants
// =====================================================

namespace GameConstants {
	/** skill speed divider @see somewhere else */
	const float speedDivider = 100.f;
	/** number of frames until a corpse is removed, (world frames) */
	const int maxDeadCount = 1000;
	/** time of selection circle effect 'flashes' (fraction of updateFps world frames) */ 
	const float highlightTime = 0.5f;
	/** the invalid unit ID */
	const int invalidId = -1;

	const int maxPlayers = 4;
	const int serverPort = 61357;

	const int defaultUpdateInterval = 6;
	const int updateFps = 240; // == GameSpeeds_lcm
	const int cameraFps = 100;
	const int networkFramePeriod = 5;
	const int networkExtraLatency = 250;

	const int cellScale = 2;
	const int mapScale = 2;
	const int clusterSize = 16;

	const int saveGameVersion = 3;
}

namespace Gui {

/** click count [could be WRAPPED_ENUM in Gui ?]
  */
REGULAR_ENUM( Clicks,
					ONE,
					TWO
			);

/** weather set
  * <ul><li><b>SUNNY</b> Sunny weather, no weather particle system.</li>
  *		<li><b>RAINY</b> Rainy weather..</li>
  *		<li><b>SNOWY</b> Snowy.</li></ul>
  */
REGULAR_ENUM( Weather,
					SUNNY,
					RAINY,
					SNOWY
			);

} // namespace Game


}//end namespace

#endif

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
#define theGame				(*GameState::getInstance())
#define theWorld			(World::getInstance())
#define theMap				(*World::getInstance().getMap())
#define theCamera			(*GameState::getInstance()->getGameCamera())
#define theGameSettings		(GameState::getInstance()->getGameSettings())
#define theGui				(*Gui::UserInterface::getCurrentGui())
#define theConsole			(*GameState::getInstance()->getConsole())
#define theConfig			(Config::getInstance())
#define theRoutePlanner		(*World::getInstance().getRoutePlanner())
#define theCartographer		(*World::getInstance().getCartographer())
#define theRenderer			(Renderer::getInstance())
#define theNetworkManager	(NetworkManager::getInstance())
#define theSoundRenderer	(SoundRenderer::getInstance())
#define theLogger			(Logger::getInstance())
#define theLang				(Lang::getInstance())
#define theFileFactory		(*FSFactory::getInstance())

#if _GAE_DEBUG_EDITION_
#	define theDebugRenderer	(Glest::Debug::getDebugRenderer())
#	define IF_DEBUG_EDITION(x) x
#	define IF_NOT_DEBUG_EDITION(x)
#	define WORLD_FPS (theConfig.getGsWorldUpdateFps())
#else
#	define IF_DEBUG_EDITION(x)
#	define IF_NOT_DEBUG_EDITION(x) x
#	define WORLD_FPS (GameConstants::updateFps)
#endif

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

#if defined(LOG_STUFF) && LOG_STUFF
#	define LOG(x) theLogger.add(x)
#	define STREAM_LOG(x) {stringstream ss; ss << x; theLogger.add(ss.str()); }
#else
#	define LOG(x)
#	define STREAM_LOG(x)
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
	IF_NOT_DEBUG_EDITION(
		const int updateFps = 40;
	)
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

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

//TODO
// Rationialise singletons, make return all references if plausible,
// if not, make them ALL return pointers
//
#define theWorld		(World::getInstance())
#define theGame			(*Game::getInstance())
#define theCamera		(*Game::getInstance()->getGameCamera())
#define theGameSettings (Game::getInstance()->getGameSettings())
#define theConsole		(*Game::getInstance()->getConsole())

namespace Glest{ namespace Game{

// =====================================================
//	class GameConstants
// =====================================================

enum ControlType{
    ctClosed,
	ctCpu,
	ctCpuUltra,
	ctNetwork,
	ctHuman,

	ctCount
};

// implemented in game.cpp
extern const char* controlTypeNames[ctCount];

class GameConstants{
public:
	static const int maxPlayers= 4;
	static const int serverPort= 61357;
//	static const int updateFps= 40;
	static const int cameraFps= 100;
	static const int networkFramePeriod= 10;
	static const int networkExtraLatency= 200;
};

}}//end namespace

#endif

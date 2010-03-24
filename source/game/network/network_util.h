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

#ifndef _GLEST_GAME_NETWORKUTIL_H_
#define _GLEST_GAME_NETWORKUTIL_H_

#include "network_manager.h"

namespace Glest{ namespace Game{

class ServerInterface;
class ClientInterface;

// need to be inline, or you get multiple defs if you use them from more than one translation unit
inline bool isLocal()							{return NetworkManager::getInstance().isLocal();}
inline bool isNetworkGame()						{return NetworkManager::getInstance().isNetworkGame();}
inline bool isNetworkServer()					{return NetworkManager::getInstance().isNetworkServer();}
inline bool isNetworkClient()					{return NetworkManager::getInstance().isNetworkClient();}
inline ServerInterface *getServerInterface()	{return NetworkManager::getInstance().getServerInterface();}
inline ClientInterface *getClientInterface()	{return NetworkManager::getInstance().getClientInterface();}

}}//end namespace

#endif
// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_EXCEPTIONS_H_
#define _GLEST_GAME_EXCEPTIONS_H_

#include <stdexcept>

using std::string;
using std::runtime_error;

namespace Glest { namespace Game {

class GameException : public runtime_error {
	string fileName;
public:
	GameException(const string &msg) : runtime_error(msg) {}
	GameException(const string &fileName, const string &msg) :
			runtime_error(msg),
			fileName(fileName) {}
	virtual ~GameException() throw() {}
};

class MapException : public GameException {
public:
	MapException(const string &fileName, const string &msg) :
			GameException(fileName, msg) {}
	virtual ~MapException() throw() {}
};

}}//end namespace

#endif

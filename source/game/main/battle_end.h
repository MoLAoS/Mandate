// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_BATTLEEND_H_
#define _GLEST_GAME_BATTLEEND_H_

#include "program.h"
#include "stats.h"

namespace Glest { namespace Main {

class GameStatsWidget : public Container {
public:
	GameStatsWidget(Container* parent, Vec2i p, Vec2i s);
	
	virtual string desc() { return string("[GameStatsWidget: ") + descPosDim() + "]"; }
};

// =====================================================
// 	class BattleEnd  
//
///	ProgramState representing the end of the game
// =====================================================

class BattleEnd: public ProgramState {
private:
	bool isQuickExit;
public:
	BattleEnd(Program &program, bool quickExit=false);
	~BattleEnd();
	virtual int getUpdateFps() const {return 10;}
	virtual void update();
	virtual void renderBg();
	virtual void renderFg();
	virtual void keyDown(const Key &key);
	virtual void mouseDownLeft(int x, int y);
};

}}//end namespace

#endif

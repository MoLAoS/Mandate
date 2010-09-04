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

#ifndef _GLEST_GAME_INTRO_H_
#define _GLEST_GAME_INTRO_H_

#include <vector>

#include "program.h"
#include "font.h"
#include "texture.h"
#include "widgets.h"

namespace Glest { namespace Main {

// =====================================================
// 	class Intro  
//
///	ProgramState representing the intro
// =====================================================

class Intro: public ProgramState {
private:
	int timer;

	Widgets::PicturePanel *logoPanel;
	Widgets::StaticText *lblAdvanced, *lblEngine, *lblVersion, *lblWebsite;

public:
	Intro(Program &program);
	virtual int getUpdateFps() const {return 60;}
	virtual void update();
	virtual void renderBg();
	virtual void renderFg();
	virtual void keyDown(const Key &key);
	virtual void mouseUpLeft(int x, int y);
};

}}//end namespace

#endif

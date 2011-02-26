// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_TEST_PANE_H_
#define _GLEST_GAME_TEST_PANE_H_

#include "program.h"

namespace Glest { namespace Main {

// =====================================================
// 	class TestPane
//
///	ProgramState to test widgets on
// =====================================================

class TestPane: public ProgramState {
public:
	TestPane(Program &program);
	~TestPane();
	virtual void update();
	virtual void renderBg();
	virtual void renderFg();
	virtual void keyDown(const Key &key);
	virtual void mouseDownLeft(int x, int y);
};

}}//end namespace

#endif

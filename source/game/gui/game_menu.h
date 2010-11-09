// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_GUI_GAME_MENU_H_
#define _GLEST_GAME_GUI_GAME_MENU_H_

#include <string>

#include "compound_widgets.h"

namespace Glest { namespace Gui {
using namespace Widgets;

class GameMenu : public Frame, public sigslot::has_slots {
public:
	typedef GameMenu* Ptr;

private:

private:
	GameMenu(Vec2i pos, Vec2i size);

	void onReturnToGame(Button*);
	void onExit(Button*);
	void onQuit(Button*);
	void onSaveGame(Button*);
	void onDebugToggle(Button*);
	void onTogglePhotoMode(Button*);

public:
	static GameMenu* showDialog(Vec2i pos, Vec2i size);

	virtual Vec2i getPrefSize() const { return Vec2i(-1); }
	virtual Vec2i getMinSize() const { return Vec2i(-1); }
	virtual string desc() { return string("[GameMenu: ") + descPosDim() + "]"; }
};

}}

#endif
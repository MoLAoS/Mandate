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

class GameMenu : public Frame {
public:
	typedef GameMenu* Ptr;

private:

private:
	GameMenu(Vec2i pos, Vec2i size);

	void onReturnToGame(Widget*);
	void onExit(Widget*);
	void onQuit(Widget*);
	void onSaveGame(Widget*);
	void onDebugToggle(Widget*);
	void onTogglePhotoMode(Widget*);

public:
	static GameMenu* showDialog(Vec2i pos, Vec2i size);

	virtual Vec2i getPrefSize() const override { return Vec2i(-1); }
	virtual Vec2i getMinSize() const override { return Vec2i(-1); }
	virtual string descType() const override { return "GameMenu"; }
};

}}

#endif
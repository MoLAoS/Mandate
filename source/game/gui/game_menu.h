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

#include "framed_widgets.h"

namespace Glest { namespace Gui {
using namespace Widgets;

class GameMenu : public Frame {
private:
	Button *m_pinWidgetsBtn;
	CellStrip *m_btnStrip;

private:
	void onReturnToGame(Widget*);
	void onOptions(Widget*);
	void onExit(Widget*);
	void onQuit(Widget*);
	void onSaveGame(Widget*);
	//void onDebugToggle(Widget*);
	void onTogglePhotoMode(Widget*);
	void onPinWidgets(Widget*);

public:
	GameMenu();
	void init();
	/*static GameMenu* showDialog(Vec2i pos, Vec2i size);*/

	virtual Vec2i getPrefSize() const override { return Vec2i(-1); }
	virtual Vec2i getMinSize() const override { return Vec2i(-1); }
	virtual string descType() const override { return "GameMenu"; }
};

}}

#endif
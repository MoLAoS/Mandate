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

#ifndef _GLEST_GAME_MAINMENU_H_
#define _GLEST_GAME_MAINMENU_H_

#include "lang.h"
#include "console.h"
#include "vec.h"
#include "world.h"
#include "program.h"
#include "components.h"
#include "menu_background.h"
#include "game_settings.h"
#include "widgets.h"

#include "sigslot.h"

namespace Glest { namespace Menu {
using namespace Main;

//misc consts
struct MapInfo {
	Vec2i size;
	int players;
	string desc;

	void load(string file);
};

class MenuState;

WRAPPED_ENUM( MenuStates,
	ROOT,
	NEW_GAME,
	JOIN_GAME,
	SCENARIO,
	LOAD_GAME,
	OPTIONS,
	ABOUT,
	GFX_INFO
);

// =====================================================
// 	class MainMenu
//
///	Main menu ProgramState
// =====================================================

class MainMenu: public ProgramState {
private:
	MenuBackground menuBackground;
	MenuState *state;

	static const string stateNames[MenuStates::COUNT];
	Camera stateCameras[MenuStates::COUNT];

	bool setCameraOnSetState;
	bool totalConversion;
	bool gaeLogoOnRootMenu;
	bool gplLogoOnRootMenu;

	int mouseX, mouseY;
	int mouse2dAnim;
	int fps, lastFps;

private:
	MainMenu(const MainMenu &);
	const MainMenu &operator =(const MainMenu &);

	void loadXml();

public:
	MainMenu(Program &program);
	~MainMenu();

	MenuBackground *getMenuBackground()	{return &menuBackground;}

	virtual void renderBg();
	virtual void renderFg();
	virtual void update();
	virtual void tick();
	virtual void init();
	virtual int getUpdateFps() const {return 40;}
	virtual void mouseMove(int x, int y, const MouseState &mouseState);
	virtual void mouseDownLeft(int x, int y);
	virtual void mouseDownRight(int x, int y);
	virtual void keyDown(const Key &key);
	virtual void keyPress(char c);

	void setState(MenuState *state);
	void setCameraTarget(MenuStates state);

	bool isTotalConversion() const	{ return totalConversion; }
	bool gaeLogoOnRoot() const		{ return gaeLogoOnRootMenu; }
	bool gplLogoOnRoot() const		{ return gplLogoOnRootMenu; }
};

// ===============================
// 	class MenuState
// ===============================

class MenuState: public sigslot::has_slots {
protected:
	Program &program;
	MainMenu *mainMenu;
	bool	m_transition;

protected:
	float	m_fade;
	bool	m_fadeIn,
			m_fadeOut;

private:
	const MenuState &operator =(const MenuState &);

public:
	MenuState(Program &program, MainMenu *mainMenu)
			: program(program), mainMenu(mainMenu)
	 		, m_fade(0.f)
			, m_fadeIn(true)
			, m_fadeOut(false)
			, m_transition(false) {
		program.setFade(m_fade);
	}

	virtual ~MenuState() {}
	virtual void mouseClick(int x, int y, MouseButton mouseButton) {}
	virtual void mouseMove(int x, int y, const MouseState &mouseState) {}
	virtual void render() {}
	virtual void update();
	virtual void keyDown(const Key &key){}
	virtual void keyPress(char c){}

	virtual MenuStates getIndex() const = 0;

	void doFadeIn() { m_fadeIn = true; m_fadeOut = false; }
	void doFadeOut() { m_fadeIn = false; m_fadeOut = true; }
};

}}//end namespace

#endif

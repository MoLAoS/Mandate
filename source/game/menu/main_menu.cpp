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

#include "pch.h"
#include "main_menu.h"

#include "renderer.h"
#include "sound.h"
#include "config.h"
#include "program.h"
#include "game_util.h"
#include "game.h"
#include "platform_util.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "faction.h"
#include "metrics.h"
#include "network_message.h"
#include "socket.h"
#include "menu_state_root.h"
#include "FSFactory.hpp"

#include "leak_dumper.h"

using namespace Shared::Sound;
using namespace Shared::Platform;
using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Xml;
using namespace Shared::PhysFS;

namespace Glest { namespace Menu {

// =====================================================
//  class MapInfo
// =====================================================

void MapInfo::load(string file) {
	struct MapFileHeader {
		int32 version;
		int32 maxPlayers;
		int32 width;
		int32 height;
		int32 altFactor;
		int32 waterLevel;
		int8 title[128];
	};

	Lang &lang = Lang::getInstance();

	try {
		FileOps *f = FSFactory::getInstance()->getFileOps();
		string path;
		if (fileExists(file + ".mgm")) {
			path = file + ".mgm";
		} else {
			path = file + ".gbm";
		}
		f->openRead(path.c_str());

		MapFileHeader header;
		f->read(&header, sizeof(MapFileHeader), 1);
		size.x = header.width;
		size.y = header.height;
		players = header.maxPlayers;
		desc = lang.get("MaxPlayers") + ": " + intToStr(players) + "\n";
		desc += lang.get("Size") + ": " + intToStr(size.x) + " x " + intToStr(size.y);
		delete f;
	} catch (exception e) {
		throw runtime_error("Error loading map file: " + file + '\n' + e.what());
	}
}

// =====================================================
//  class MainMenu
// =====================================================

const string MainMenu::stateNames[] = {
	"root",
	"new-game",
	"join-game",
	"scenario",
	"loadgame",
	"config",
	"about",
	"info",
	"test"
};

// ===================== PUBLIC ========================

MainMenu::MainMenu(Program &program)
		: ProgramState(program)
		, setCameraOnSetState(true)
		, totalConversion(false)
		, gaeLogoOnRootMenu(false) {
	loadXml();
	mouseX = 100;
	mouseY = 100;

	state = NULL;

	fps = lastFps = 0;

	setState(new MenuStateRoot(program, this));
	g_program.setTechTitle("");
}

MainMenu::~MainMenu() {
	delete state;
	g_renderer.endMenu();
	g_soundRenderer.stopAllSounds();
}

void MainMenu::init() {
	g_renderer.initMenu(this);
}

// synchronous render update
void MainMenu::renderBg() {
	++fps;
	g_renderer.clearBuffers();

	//3d
	g_renderer.reset3dMenu();
	g_renderer.clearZBuffer();
	g_renderer.loadCameraMatrix(menuBackground.getCamera());
	g_renderer.renderMenuBackground(&menuBackground);
	g_renderer.renderParticleManager(ResourceScope::MENU);
}

void MainMenu::renderFg() {
	//2d
	g_renderer.reset2d();
	state->render();
	g_renderer.swapBuffers();
}

// synchronous update
void MainMenu::update() {
	g_renderer.updateParticleManager(ResourceScope::MENU);
	mouse2dAnim = (mouse2dAnim + 1) % Renderer::maxMouse2dAnim;
	menuBackground.update();
	state->update();
}

void MainMenu::tick() {
	lastFps = fps;
	fps = 0;
	string s = "FPS: " + intToStr(lastFps);
	state->setDebugString(s);
}

void MainMenu::mouseMove(int x, int y, const MouseState &ms) {
	mouseX = x;
	mouseY = y;
	state->mouseMove(x, y, ms);
}

void MainMenu::mouseDownLeft(int x, int y) {
	state->mouseClick(x, y, MouseButton::LEFT);
}

void MainMenu::mouseDownRight(int x, int y) {
	state->mouseClick(x, y, MouseButton::RIGHT);
}

void MainMenu::keyDown(const Key &key) {
	state->keyDown(key);
}

void MainMenu::keyPress(char c) {
	state->keyPress(c);
}

void MainMenu::setState(MenuState *state) {
	delete this->state;
	this->state = state;
	GraphicComponent::resetFade();

	if (setCameraOnSetState) {
		menuBackground.setTargetCamera(&stateCameras[state->getIndex()]);
	} else {
		setCameraOnSetState = true;
	}
//	g_program.initMouse();
}

void MainMenu::setCameraTarget(MenuStates state) {
	menuBackground.setTargetCamera(&stateCameras[state]);
	setCameraOnSetState = false;
}

void MainMenu::loadXml() {
	//camera
	XmlTree xmlTree;
	xmlTree.load("data/core/menu/menu.xml");
	const XmlNode *menuNode = xmlTree.getRootNode();
	const XmlNode *cameraNode = menuNode->getChild("camera");

	foreach_enum (MenuStates, state) {
		//position
		const XmlNode *positionNode = cameraNode->getChild(stateNames[state] + "-position");
		Vec3f startPosition;
		startPosition.x = positionNode->getAttribute("x")->getFloatValue();
		startPosition.y = positionNode->getAttribute("y")->getFloatValue();
		startPosition.z = positionNode->getAttribute("z")->getFloatValue();
		stateCameras[state].setPosition(startPosition);

		//rotation
		const XmlNode *rotationNode = cameraNode->getChild(stateNames[state] + "-rotation");
		Vec3f startRotation;
		startRotation.x = rotationNode->getAttribute("x")->getFloatValue();
		startRotation.y = rotationNode->getAttribute("y")->getFloatValue();
		startRotation.z = rotationNode->getAttribute("z")->getFloatValue();
		stateCameras[state].setOrientation(Quaternion(EulerAngles(
			degToRad(startRotation.x), degToRad(startRotation.y), degToRad(startRotation.z))));
	}

	const XmlNode *logoNode = menuNode->getOptionalChild("logos");
	if (logoNode) {
		totalConversion = logoNode->getOptionalBoolValue("total-conversion", false);
		gaeLogoOnRootMenu = logoNode->getOptionalBoolValue("gae-logo", totalConversion);
		gplLogoOnRootMenu = logoNode->getOptionalBoolValue("gpl-logo", true);
	}
}

void MenuState::update() {
	if (m_fadeIn) {
		m_fade += 0.05;
		if (m_fade > 1.f) {
			m_fade = 1.f;
			m_fadeIn = false;
		}
		program.setFade(m_fade);
	} else if (m_fadeOut) {
		m_fade -= 0.05;
		if (m_fade < 0.f) {
			m_fade = 0.f;
			m_transition = true;
			m_fadeOut = false;
		}
		program.setFade(m_fade);
	}
}

// =====================================================
//  class MenuState
// =====================================================

MenuState::MenuState(Program &program, MainMenu *mainMenu)
		: program(program), mainMenu(mainMenu)
 		, m_fade(0.f)
		, m_fadeIn(true)
		, m_fadeOut(false)
		, m_transition(false) {
	program.setFade(m_fade);
	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	Vec2i pos(10, 50);
	pos.y -= int(font->getMetrics()->getHeight());
	m_debugText = new StaticText(&program, pos, Vec2i(0));
	m_debugText->setTextParams("FPS: ", Vec4f(1.f), font, false);
	if (!g_config.getMiscDebugMode()) {
		m_debugText->setVisible(false);
	}
}

void MenuState::setDebugString(const string &s) {
	m_debugText->setText(s);
	m_debugText->setSize(m_debugText->getPrefSize());
}

// =====================================================
//  class MenuStateTest
// =====================================================

CellStrip *ws;

MenuStateTest::MenuStateTest(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu) {
	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	// create
	int gap = (g_metrics.getScreenW() - 450) / 4;
	int x = gap, w = 150, y = g_config.getDisplayHeight() - 80, h = 30;
	m_returnButton = new Button(&program, Vec2i(x, y), Vec2i(w, h));
	m_returnButton->setTextParams(g_lang.get("Return"), Vec4f(1.f), font);
	m_returnButton->Clicked.connect(this, &MenuStateTest::onButtonClick);

	Anchors anchors;
	anchors.set(Edge::LEFT, 10, true);
	anchors.set(Edge::RIGHT, 10, true);
	anchors.set(Edge::TOP, 15, true);
	anchors.set(Edge::BOTTOM, 15, true);

	BorderStyle bs;
	bs.setSolid(g_widgetConfig.getColourIndex(Colour(255u, 0u, 0u, 255u)));
	bs.setSizes(1);

	Vec2i size = Vec2i(std::min(g_config.getDisplayWidth() / 3, 300),
	                   std::min(g_config.getDisplayHeight() / 2, 300));
	Vec2i pos = g_metrics.getScreenDims() / 2 - size / 2;
	ws = new CellStrip(&program, Orientation::VERTICAL, RootMenuItem::COUNT);
	ws->setBorderStyle(bs);
	ws->setPos(pos);
	ws->setSize(size);

	// some buttons
	for (int i=0; i < RootMenuItem::COUNT; ++i) {
		ws->getCell(i)->setAnchors(anchors);
		Button *btn = new Button(ws->getCell(i), Vec2i(0), Vec2i(150, 40), false);
		BorderStyle borderStyle;
		borderStyle.setSizes(3);
		int li = g_widgetConfig.getColourIndex(Colour(0xBFu, 0x5Eu, 0x5Eu, 0xAF));
		int di = g_widgetConfig.getColourIndex(Colour(0x6Fu, 0x00u, 0x00u, 0xAF));
		borderStyle.setRaise(li, di);
		btn->setBorderStyle(borderStyle);

		BackgroundStyle backStyle;
		int bi = g_widgetConfig.getColourIndex(Colour(0x9Fu, 0x00u, 0x00u, 0xAF));
		backStyle.setColour(bi);
		btn->setBackgroundStyle(backStyle);

		btn->setTextParams(g_lang.get(RootMenuItemNames[i]), Vec4f(1.f), font, true);
	}
}

void MenuStateTest::update() {
	MenuState::update();
	if (m_transition) {
		program.clear();
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
	}
}

void MenuStateTest::onButtonClick(Button* btn) {
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	soundRenderer.playFx(g_coreData.getClickSoundA());
	mainMenu->setCameraTarget(MenuStates::ROOT);
	doFadeOut();
}


}}//end namespace

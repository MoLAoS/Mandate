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
	"info"
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
	//renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);

	if (g_config.getMiscDebugMode()) {
		Font *font = g_coreData.getFTMenuFontNormal();
		string s = "FPS: " + intToStr(lastFps);
		g_renderer.renderText(s, font, Vec3f(1.f), 10, 60, false);
	}
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

}}//end namespace

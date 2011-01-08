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

#include "core_data.h"

#include "logger.h"
#include "renderer.h"
#include "graphics_interface.h"
#include "config.h"
#include "util.h"

#include "leak_dumper.h"
#include "profiler.h"

using Glest::Util::Logger;
using namespace Shared::Sound;
using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Graphics;

namespace Glest { namespace Global {

// =====================================================
// 	class CoreData
// =====================================================

// ===================== PUBLIC ========================

CoreData &CoreData::getInstance(){
	static CoreData coreData;
	return coreData;
}

CoreData::~CoreData(){
	deleteValues(waterSounds.getSounds().begin(), waterSounds.getSounds().end());
}

Texture2D* loadTexture(const string &path, bool mipmap = false) {
	Texture2D *tex = g_renderer.newTexture2D(ResourceScope::GLOBAL);
	tex->setMipmap(mipmap);
	tex->getPixmap()->load(path);
	return tex;
}

Texture2D* loadAlphaTexture(const string &path, bool mipmap = false) {
	Texture2D *tex = g_renderer.newTexture2D(ResourceScope::GLOBAL);
	tex->setMipmap(mipmap);
	tex->setFormat(Texture::fAlpha);
	tex->getPixmap()->init(1);
	tex->getPixmap()->load(path);
	return tex;
}

Font* loadFreeTypeFont(string path, int size) {
	Font *font = g_renderer.newFreeTypeFont(ResourceScope::GLOBAL);
	font->setType(path);
	font->setSize(size);
	return font;
}

bool CoreData::load() {
	g_logger.add("Core data");

	const string dir = "data/core";

	// textures
	try {
		backgroundTexture = loadTexture(dir + "/menu/textures/back.tga");
		fireTexture = loadAlphaTexture(dir + "/misc_textures/fire_particle.tga");
		snowTexture = loadAlphaTexture(dir + "/misc_textures/snow_particle.tga");
		customTexture = loadTexture(dir + "/menu/textures/custom_texture.tga");
		logoTexture = loadTexture(dir + "/menu/textures/logo.tga");
		gplTexture = loadTexture(dir + "/menu/textures/gplv3.tga");
		gaeSplashTexture = loadTexture(dir + "/menu/textures/gaesplash.tga");
		checkBoxCrossTexture = loadTexture(dir + "/menu/textures/button_small_unchecked.tga");
		checkBoxTickTexture = loadTexture(dir + "/menu/textures/button_small_checked.tga");
		vertScrollUpTexture = loadTexture(dir + "/menu/textures/button_small_up.tga");
		vertScrollDownTexture = loadTexture(dir + "/menu/textures/button_small_down.tga");
		vertScrollUpHoverTex = loadTexture(dir + "/menu/textures/button_small_up_hover.tga");
		vertScrollDownHoverTex = loadTexture(dir + "/menu/textures/button_small_down_hover.tga");
		waterSplashTexture = loadAlphaTexture(dir + "/misc_textures/water_splash.tga", true);
		buttonSmallTexture = loadTexture(dir + "/menu/textures/button_small.tga", true);
		buttonBigTexture = loadTexture(dir + "/menu/textures/button_big.tga", true);
		textEntryTexture = loadTexture(dir + "/menu/textures/textentry.tga", true);
		greenTickOverlay = loadTexture(dir + "/menu/textures/green_tick.png");
		orangeQuestionOverlay = loadTexture(dir + "/menu/textures/orange_question.png");
		redCrossOverlay = loadTexture(dir + "/menu/textures/red_cross.png");
		mouseTexture = loadTexture(dir + "/misc_textures/mouse.png");
	} catch (runtime_error &e) {
		g_errorLog.add(string("Error loading core data.\n") + e.what());
		return false;
	}

	// Display/Console font
	m_FTDisplay = loadFreeTypeFont(dir + "/menu/fonts/TinDog.ttf", computeFontSize(10));
	m_FTDisplayBig = loadFreeTypeFont(dir + "/menu/fonts/TinDog.ttf", computeFontSize(12));

	// Misc fonts
	advancedEngineFontSmall = loadFreeTypeFont(dir + "/menu/fonts/dum1.ttf", computeFontSize(24));
	advancedEngineFontBig = loadFreeTypeFont(dir + "/menu/fonts/dum1wide.ttf", computeFontSize(36));
	
	// Menu fonts...
	m_FTMenuFontNormal = loadFreeTypeFont(dir + "/menu/fonts/TinDog.ttf", computeFontSize(18));
	m_FTMenuFontSmall = loadFreeTypeFont(dir + "/menu/fonts/TinDog.ttf", computeFontSize(14));
	m_FTMenuFontBig = loadFreeTypeFont(dir + "/menu/fonts/TinDog.ttf", computeFontSize(22));
	m_FTMenuFontVeryBig = loadFreeTypeFont(dir + "/menu/fonts/TinDog.ttf", computeFontSize(26));

	//sounds
	try {
		clickSoundA.load(dir + "/menu/sound/click_a.wav");
		clickSoundB.load(dir + "/menu/sound/click_b.wav");
		clickSoundC.load(dir + "/menu/sound/click_c.wav");
		introMusic.open(dir + "/menu/music/intro_music.ogg");
		introMusic.setNext(&menuMusic);
		menuMusic.open(dir + "/menu/music/menu_music.ogg");
		menuMusic.setNext(&menuMusic);
		waterSounds.resize(6);
		for (int i = 0; i < 6; ++i) {
			waterSounds[i]= new StaticSound();
			waterSounds[i]->load(dir + "/water_sounds/water" + intToStr(i) + ".wav");
		}
	} catch (runtime_error &e) {
		g_errorLog.add(string("Error loading core data.\n") + e.what());
		return false;		
	}
	return true;
}

void CoreData::closeSounds(){
	introMusic.close();
    menuMusic.close();
}

int CoreData::computeFontSize(int size) {
	int screenH = g_config.getDisplayHeight();
	int rs= int(g_config.getRenderFontScaler() * size * float(screenH) / 768.f);
	if (rs < 12) {
		rs = 12;
	}
	return rs;
}

// ================== PRIVATE ========================

}}//end namespace

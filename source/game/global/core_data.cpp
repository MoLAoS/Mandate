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

Font* loadBitmapFont(string name, int size, int width = Font::wNormal) {
	Font *font = g_renderer.newFont(ResourceScope::GLOBAL);
	font->setType(name);
	font->setSize(size);
	font->setWidth(width);
	return font;
}

Font* loadFreeTypeFont(string path, int size) {
	Font *font = g_renderer.newFreeTypeFont(ResourceScope::GLOBAL);
	font->setType(path);
	font->setSize(size);
	return font;
}

void CoreData::load() {
	Config &config = Config::getInstance();
	Renderer &renderer = Renderer::getInstance();
	Logger::getInstance().add("Core data");

	const string dir = "data/core";

	//textures
	backgroundTexture = loadTexture(dir + "/menu/textures/back.tga");
	fireTexture = loadAlphaTexture(dir + "/misc_textures/fire_particle.tga");
	snowTexture = loadAlphaTexture(dir + "/misc_textures/snow_particle.tga");
	customTexture = loadTexture(dir + "/menu/textures/custom_texture.tga");
	logoTexture = loadTexture(dir + "/menu/textures/logo.tga");
	gplTexture = loadTexture(dir + "/menu/textures/gplv3.tga");
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

	//display font
	displayFont = loadBitmapFont(config.getRenderFontDisplay(), computeFontSize(15));
	m_FTDisplay = loadFreeTypeFont(dir + "/menu/fonts/TinDog.ttf", 10);

	//menu fonts
	menuFontSmall = loadBitmapFont(config.getRenderFontMenu(), computeFontSize(12));
	menuFontNormal = loadBitmapFont(config.getRenderFontMenu(), computeFontSize(16), Font::wBold);
	menuFontBig = loadBitmapFont(config.getRenderFontMenu(), computeFontSize(20));
	menuFontVeryBig = loadBitmapFont(config.getRenderFontMenu(), computeFontSize(25));

	//console font
	consoleFont = loadBitmapFont(Config::getInstance().getRenderFontConsole(), computeFontSize(16));

	// FreeType fonts...
	freeTypeFont = loadFreeTypeFont(dir + "/menu/fonts/dum1.ttf", computeFontSize(24));
	advancedEngineFont = loadFreeTypeFont(dir + "/menu/fonts/dum1wide.ttf", computeFontSize(36));
	freeTypeMenuFont = loadFreeTypeFont(dir + "/menu/fonts/circula-medium.otf", computeFontSize(18));
	
	m_FTMenuFontNormal = loadFreeTypeFont(dir + "/menu/fonts/circula-medium.otf", computeFontSize(18));
	m_FTMenuFontSmall = loadFreeTypeFont(dir + "/menu/fonts/circula-medium.otf", computeFontSize(14));
	m_FTMenuFontBig = loadFreeTypeFont(dir + "/menu/fonts/circula-medium.otf", computeFontSize(22));
	m_FTMenuFontVeryBig = loadFreeTypeFont(dir + "/menu/fonts/circula-medium.otf", computeFontSize(26));

//	freeTypeMenuFont = loadFreeTypeFont(dir + "/menu/fonts/zekton_free.ttf", 14);

	//sounds
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
}

void CoreData::closeSounds(){
	introMusic.close();
    menuMusic.close();
}

int CoreData::computeFontSize(int size){
	int screenH = Config::getInstance().getDisplayHeight();
	int rs= int(size * float(screenH) / 768.f);
	if (rs < 12) {
		rs = 12;
	}
	return rs;
}

// ================== PRIVATE ========================

}}//end namespace

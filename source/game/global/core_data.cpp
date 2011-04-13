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

bool CoreData::load() {
	g_logger.logProgramEvent("Loading core data");

	const string dir = "data/core";

	// textures
	g_logger.logProgramEvent("\tTextures");
	try {
		backgroundTexture = loadTexture(dir + "/menu/textures/back.tga");
		fireTexture = loadAlphaTexture(dir + "/misc_textures/fire_particle.tga");
		snowTexture = loadAlphaTexture(dir + "/misc_textures/snow_particle.tga");
		logoTexture = loadTexture(dir + "/menu/textures/logo.tga");
		gplTexture = loadTexture(dir + "/menu/textures/gplv3.tga");
		gaeSplashTexture = loadTexture(dir + "/menu/textures/gaesplash.tga");
		waterSplashTexture = loadAlphaTexture(dir + "/misc_textures/water_splash.tga", true);
	} catch (runtime_error &e) {
		g_logger.logError(string("Error loading core data.\n") + e.what());
		return false;
	}

	// sounds
	g_logger.logProgramEvent("\tSounds");
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
		g_logger.logError(string("Error loading core data.\n") + e.what());
		return false;		
	}
	g_logger.logProgramEvent("\tCore data successfully loaded.");
	return true;
}

void CoreData::closeSounds(){
	introMusic.close();
    menuMusic.close();
}

}} // end namespace

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

#include "config.h"
#include "util.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class Config
// =====================================================

Config::Config(const char* fileName) :
		Properties(),
		fileName(fileName) {
	load(fileName);

	aiLog = getInt("AiLog");
	aiRedir = getBool("AiRedir");
	checkGlCaps = getBool("CheckGlCaps");
	colorBits = getInt("ColorBits");
	consoleMaxLines = getInt("ConsoleMaxLines");
	consoleTimeout = getInt("ConsoleTimeout");
	dayTime = getFloat("DayTime");
	debugMode = getBool("DebugMode");
	depthBits = getInt("DepthBits");
	enableCommandMinimap = getBool("EnableCommandMinimap");
	factoryGraphics = getString("FactoryGraphics");
	factorySound = getString("FactorySound");
	fastestSpeed = getFloat("FastestSpeed", 1.0f, 10.0f);
	filter = getString("Filter");
	filterMaxAnisotropy = getInt("FilterMaxAnisotropy");
	firstTime = getBool("FirstTime");
	focusArrows = getBool("FocusArrows");
	fogOfWar = getBool("FogOfWar");
	fogOfWarSmoothing = getBool("FogOfWarSmoothing");
	fogOfWarSmoothingFrameSkip = getInt("FogOfWarSmoothingFrameSkip");
	fontConsole = getString("FontConsole");
	fontDisplay = getString("FontDisplay");
	fontMenu = getString("FontMenu");
	lang = getString("Lang");
	maxCameraDistance = getFloat("MaxCameraDistance", 0.f, 1024.f);
	maxFPS = getInt("MaxFPS");
	maxLights = getInt("MaxLights", 0, 8);
	maxRenderDistance = getFloat("MaxRenderDistance", 20.f, 1024.f);
	minCameraDistance = getFloat("MinCameraDistance", 0.f, 64.f);
	networkConsistencyChecks = getBool("NetworkConsistencyChecks");
//	platform = getString("Platform");
	photoMode = getBool("PhotoMode");
	randStartLocs = getBool("RandStartLocs");
	refreshFrequency = getInt("RefreshFrequency");
	screenHeight = getInt("ScreenHeight");
	screenWidth = getInt("ScreenWidth");
	scrollSpeed = getFloat("ScrollSpeed");
	serverIp = getString("ServerIp");
//	serverPort = getInt("ServerPort");
	shadowAlpha = getFloat("ShadowAlpha");
	shadowFrameSkip = getInt("ShadowFrameSkip");
	shadowTextureSize = getInt("ShadowTextureSize");
	shadows = getString("Shadows");
	slowestSpeed = getFloat("SlowestSpeed", 0.01f, 1.0f);
	soundStaticBuffers = getInt("SoundStaticBuffers");
	soundStreamingBuffers = getInt("SoundStreamingBuffers");
	soundVolumeAmbient = getInt("SoundVolumeAmbient");
	soundVolumeFx = getInt("SoundVolumeFx");
	soundVolumeMusic = getInt("SoundVolumeMusic");
	stencilBits = getInt("StencilBits");
	textures3D = getBool("Textures3D");
	windowed = getBool("Windowed");
}

void Config::save(const char *path) {
	setInt("AiLog", aiLog);
	setBool("AiRedir", aiRedir);
	setBool("CheckGlCaps", checkGlCaps);
	setInt("ColorBits", colorBits);
	setInt("ConsoleMaxLines", consoleMaxLines);
	setInt("ConsoleTimeout", consoleTimeout);
	setFloat("DayTime", dayTime);
	setBool("DebugMode", debugMode);
	setInt("DepthBits", depthBits);
	setBool("EnableCommandMinimap", enableCommandMinimap);
	setString("FactoryGraphics", factoryGraphics);
	setString("FactorySound", factorySound);
	setFloat("FastestSpeed", fastestSpeed);
	setString("Filter", filter);
	setInt("FilterMaxAnisotropy", filterMaxAnisotropy);
	setBool("FirstTime", firstTime);
	setBool("FocusArrows", focusArrows);
	setBool("FogOfWar", fogOfWar);
	setBool("FogOfWarSmoothing", fogOfWarSmoothing);
	setInt("FogOfWarSmoothingFrameSkip", fogOfWarSmoothingFrameSkip);
	setString("FontConsole", fontConsole);
	setString("FontDisplay", fontDisplay);
	setString("FontMenu", fontMenu);
	setString("Lang", lang);
	setFloat("MaxCameraDistance", maxCameraDistance);
	setInt("MaxFPS", maxFPS);
	setInt("MaxLights", maxLights);
	setFloat("MaxRenderDistance", maxRenderDistance);
	setFloat("MinCameraDistance", minCameraDistance);
	setBool("NetworkConsistencyChecks", networkConsistencyChecks);
//	setString("Platform", platform);
	setBool("PhotoMode", photoMode);
	setBool("RandStartLocs", randStartLocs);
	setInt("RefreshFrequency", refreshFrequency);
	setInt("ScreenHeight", screenHeight);
	setInt("ScreenWidth", screenWidth);
	setFloat("ScrollSpeed", scrollSpeed);
	setString("ServerIp", serverIp);
//	setInt("ServerPort", serverPort);
	setFloat("ShadowAlpha", shadowAlpha);
	setInt("ShadowFrameSkip", shadowFrameSkip);
	setInt("ShadowTextureSize", shadowTextureSize);
	setString("Shadows", shadows);
	setFloat("SlowestSpeed", slowestSpeed);
	setInt("SoundStaticBuffers", soundStaticBuffers);
	setInt("SoundStreamingBuffers", soundStreamingBuffers);
	setInt("SoundVolumeAmbient", soundVolumeAmbient);
	setInt("SoundVolumeFx", soundVolumeFx);
	setInt("SoundVolumeMusic", soundVolumeMusic);
	setInt("StencilBits", stencilBits);
	setBool("Textures3D", textures3D);
	setBool("Windowed", windowed);

	Properties::save(path);
}

}}// end namespace

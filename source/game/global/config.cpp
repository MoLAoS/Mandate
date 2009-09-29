// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//				  2008-2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

// This file is auto-generated from config.cpp.template using ../config.db and the script
// ../mkconfig.sh.  To modify actual config settings, edit config.db and re-run ../mkconfig.sh.

#include "pch.h"
#include "config.h"
#include "util.h"

#include "leak_dumper.h"


namespace Glest { namespace Game {

// =====================================================
// 	class Config
// =====================================================

Config::Config(const char* fileName) : fileName(fileName) {
	Properties *p = new Properties();
	if(Shared::Util::fileExists(fileName)) {
		p->load(fileName, true);
	}

	cameraFov = p->getFloat("CameraFov", 45.f, 0.f, 360.f);
	cameraInvertXAxis = p->getBool("CameraInvertXAxis", true);
	cameraInvertYAxis = p->getBool("CameraInvertYAxis", true);
	cameraMaxDistance = p->getFloat("CameraMaxDistance", 35.f, 20.f, 1024.f);
	cameraMaxYaw = p->getFloat("CameraMaxYaw", 77.50f, 0.f, 360.f);
	cameraMinDistance = p->getFloat("CameraMinDistance", 8.f, 0.f, 20.f);
	cameraMinYaw = p->getFloat("CameraMinYaw", 20.f, 0.f, 360.f);
	displayHeight = p->getInt("DisplayHeight", 768);
	displayRefreshFrequency = p->getInt("DisplayRefreshFrequency", 75);
	displayWidth = p->getInt("DisplayWidth", 1024);
	displayWindowed = p->getBool("DisplayWindowed", true);
	gsAutoRepairEnabled = p->getBool("GsAutoRepairEnabled", true);
	gsAutoReturnEnabled = p->getBool("GsAutoReturnEnabled", false);
	gsDayTime = p->getFloat("GsDayTime", 1000.f);
	gsFogOfWarEnabled = p->getBool("GsFogOfWarEnabled", true);
	gsSpeedFastest = p->getFloat("GsSpeedFastest", 2.f, 1.0f, 10.0f);
	gsSpeedSlowest = p->getFloat("GsSpeedSlowest", 0.5f, 0.01f, 1.0f);
	gsWorldUpdateFps = p->getInt("GsWorldUpdateFps", 40);
	miscAiLog = p->getInt("MiscAiLog", 0);
	miscAiRedir = p->getBool("MiscAiRedir", false);
	miscCatchExceptions = p->getBool("MiscCatchExceptions", true);
	miscDebugKeys = p->getBool("MiscDebugKeys", false);
	miscDebugMode = p->getBool("MiscDebugMode", false);
	miscDebugTextureMode = p->getInt("MiscDebugTextureMode", 0);
	miscDebugTextures = p->getBool("MiscDebugTextures", false);
	miscFirstTime = p->getBool("MiscFirstTime", true);
	netChangeSpeedAllowed = p->getBool("NetChangeSpeedAllowed", false);
	netConsistencyChecks = p->getBool("NetConsistencyChecks", false);
	netFps = p->getInt("NetFps", 4);
	netMinFullUpdateInterval = p->getInt("NetMinFullUpdateInterval", 60000, 250, 1000000000);
	netPauseAllowed = p->getBool("NetPauseAllowed", false);
	netPlayerName = p->getString("NetPlayerName", "Player");
	netServerIp = p->getString("NetServerIp", "192.168.1.1");
	netServerPort = p->getInt("NetServerPort", 12345, 0, 65535);
	pathFinderMaxNodes = p->getInt("PathFinderMaxNodes", 1024);
	pathFinderUseAStar = p->getBool("PathFinderUseAStar", false);
	renderCheckGlCaps = p->getBool("RenderCheckGlCaps", true);
	renderColorBits = p->getInt("RenderColorBits", 32);
	renderDepthBits = p->getInt("RenderDepthBits", isWindows()?32:16);
	renderDistanceMax = p->getFloat("RenderDistanceMax", 64.f, 1.f, 65536.f);
	renderDistanceMin = p->getFloat("RenderDistanceMin", 1.f, 0.0f, 65536.f);
	renderFilter = p->getString("RenderFilter", "Bilinear");
	renderFilterMaxAnisotropy = p->getInt("RenderFilterMaxAnisotropy", 1);
	renderFogOfWarSmoothing = p->getBool("RenderFogOfWarSmoothing", 1);
	renderFogOfWarSmoothingFrameSkip = p->getInt("RenderFogOfWarSmoothingFrameSkip", 3);
	renderFontConsole = p->getString("RenderFontConsole", getDefaultFontStr());
	renderFontDisplay = p->getString("RenderFontDisplay", getDefaultFontStr());
	renderFontMenu = p->getString("RenderFontMenu", getDefaultFontStr());
	renderFov = p->getFloat("RenderFov", 60.f, 0.01f, 360.f);
	renderFpsMax = p->getInt("RenderFpsMax", 60, 0, 1000000);
	renderGraphicsFactory = p->getString("RenderGraphicsFactory", "OpenGL");
	renderLightsMax = p->getInt("RenderLightsMax", 1, 0, 8);
	renderShadowAlpha = p->getFloat("RenderShadowAlpha", 0.2f, 0.f, 1.f);
	renderShadowFrameSkip = p->getInt("RenderShadowFrameSkip", 2);
	renderShadowTextureSize = p->getInt("RenderShadowTextureSize", 512);
	renderShadows = p->getString("RenderShadows", "Projected");
	renderStencilBits = p->getInt("RenderStencilBits", 0);
	renderTextures3D = p->getBool("RenderTextures3D", 1);
	soundFactory = p->getString("SoundFactory", isWindows()?"DirectSound8":"OpenAL");
	soundStaticBuffers = p->getInt("SoundStaticBuffers", 16);
	soundStreamingBuffers = p->getInt("SoundStreamingBuffers", 5);
	soundVolumeAmbient = p->getInt("SoundVolumeAmbient", 80);
	soundVolumeFx = p->getInt("SoundVolumeFx", 80);
	soundVolumeMusic = p->getInt("SoundVolumeMusic", 90);
	uiConsoleMaxLines = p->getInt("UiConsoleMaxLines", 10);
	uiConsoleTimeout = p->getInt("UiConsoleTimeout", 20);
	uiEnableCommandMinimap = p->getBool("UiEnableCommandMinimap", true);
	uiFocusArrows = p->getBool("UiFocusArrows", true);
	uiLastMap = p->getString("UiLastMap", "four_rivers");
	uiLastRandStartLocs = p->getBool("UiLastRandStartLocs", false);
	uiLastScenario = p->getString("UiLastScenario", "glest_classic/anarchy");
	uiLastScenarioCatagory = p->getString("UiLastScenarioCatagory", "glest_classic");
	uiLastTechTree = p->getString("UiLastTechTree", "magitech");
	uiLastTileset = p->getString("UiLastTileset", "forest");
	uiLocale = p->getString("UiLocale", "en");
	uiPhotoMode = p->getBool("UiPhotoMode", false);
	uiRandomStartLocations = p->getBool("UiRandomStartLocations", false);
	uiScrollSpeed = p->getFloat("UiScrollSpeed", 1.5f);

	delete p;
}

void Config::save(const char *path) {
	Properties *p = new Properties();

	p->setFloat("CameraFov", cameraFov);
	p->setBool("CameraInvertXAxis", cameraInvertXAxis);
	p->setBool("CameraInvertYAxis", cameraInvertYAxis);
	p->setFloat("CameraMaxDistance", cameraMaxDistance);
	p->setFloat("CameraMaxYaw", cameraMaxYaw);
	p->setFloat("CameraMinDistance", cameraMinDistance);
	p->setFloat("CameraMinYaw", cameraMinYaw);
	p->setInt("DisplayHeight", displayHeight);
	p->setInt("DisplayRefreshFrequency", displayRefreshFrequency);
	p->setInt("DisplayWidth", displayWidth);
	p->setBool("DisplayWindowed", displayWindowed);
	p->setBool("GsAutoRepairEnabled", gsAutoRepairEnabled);
	p->setBool("GsAutoReturnEnabled", gsAutoReturnEnabled);
	p->setFloat("GsDayTime", gsDayTime);
	p->setBool("GsFogOfWarEnabled", gsFogOfWarEnabled);
	p->setFloat("GsSpeedFastest", gsSpeedFastest);
	p->setFloat("GsSpeedSlowest", gsSpeedSlowest);
	p->setInt("GsWorldUpdateFps", gsWorldUpdateFps);
	p->setInt("MiscAiLog", miscAiLog);
	p->setBool("MiscAiRedir", miscAiRedir);
	p->setBool("MiscCatchExceptions", miscCatchExceptions);
	p->setBool("MiscDebugKeys", miscDebugKeys);
	p->setBool("MiscDebugMode", miscDebugMode);
	p->setInt("MiscDebugTextureMode", miscDebugTextureMode);
	p->setBool("MiscDebugTextures", miscDebugTextures);
	p->setBool("MiscFirstTime", miscFirstTime);
	p->setBool("NetChangeSpeedAllowed", netChangeSpeedAllowed);
	p->setBool("NetConsistencyChecks", netConsistencyChecks);
	p->setInt("NetFps", netFps);
	p->setInt("NetMinFullUpdateInterval", netMinFullUpdateInterval);
	p->setBool("NetPauseAllowed", netPauseAllowed);
	p->setString("NetPlayerName", netPlayerName);
	p->setString("NetServerIp", netServerIp);
	p->setInt("NetServerPort", netServerPort);
	p->setInt("PathFinderMaxNodes", pathFinderMaxNodes);
	p->setBool("PathFinderUseAStar", pathFinderUseAStar);
	p->setBool("RenderCheckGlCaps", renderCheckGlCaps);
	p->setInt("RenderColorBits", renderColorBits);
	p->setInt("RenderDepthBits", renderDepthBits);
	p->setFloat("RenderDistanceMax", renderDistanceMax);
	p->setFloat("RenderDistanceMin", renderDistanceMin);
	p->setString("RenderFilter", renderFilter);
	p->setInt("RenderFilterMaxAnisotropy", renderFilterMaxAnisotropy);
	p->setBool("RenderFogOfWarSmoothing", renderFogOfWarSmoothing);
	p->setInt("RenderFogOfWarSmoothingFrameSkip", renderFogOfWarSmoothingFrameSkip);
	p->setString("RenderFontConsole", renderFontConsole);
	p->setString("RenderFontDisplay", renderFontDisplay);
	p->setString("RenderFontMenu", renderFontMenu);
	p->setFloat("RenderFov", renderFov);
	p->setInt("RenderFpsMax", renderFpsMax);
	p->setString("RenderGraphicsFactory", renderGraphicsFactory);
	p->setInt("RenderLightsMax", renderLightsMax);
	p->setFloat("RenderShadowAlpha", renderShadowAlpha);
	p->setInt("RenderShadowFrameSkip", renderShadowFrameSkip);
	p->setInt("RenderShadowTextureSize", renderShadowTextureSize);
	p->setString("RenderShadows", renderShadows);
	p->setInt("RenderStencilBits", renderStencilBits);
	p->setBool("RenderTextures3D", renderTextures3D);
	p->setString("SoundFactory", soundFactory);
	p->setInt("SoundStaticBuffers", soundStaticBuffers);
	p->setInt("SoundStreamingBuffers", soundStreamingBuffers);
	p->setInt("SoundVolumeAmbient", soundVolumeAmbient);
	p->setInt("SoundVolumeFx", soundVolumeFx);
	p->setInt("SoundVolumeMusic", soundVolumeMusic);
	p->setInt("UiConsoleMaxLines", uiConsoleMaxLines);
	p->setInt("UiConsoleTimeout", uiConsoleTimeout);
	p->setBool("UiEnableCommandMinimap", uiEnableCommandMinimap);
	p->setBool("UiFocusArrows", uiFocusArrows);
	p->setString("UiLastMap", uiLastMap);
	p->setBool("UiLastRandStartLocs", uiLastRandStartLocs);
	p->setString("UiLastScenario", uiLastScenario);
	p->setString("UiLastScenarioCatagory", uiLastScenarioCatagory);
	p->setString("UiLastTechTree", uiLastTechTree);
	p->setString("UiLastTileset", uiLastTileset);
	p->setString("UiLocale", uiLocale);
	p->setBool("UiPhotoMode", uiPhotoMode);
	p->setBool("UiRandomStartLocations", uiRandomStartLocations);
	p->setFloat("UiScrollSpeed", uiScrollSpeed);

	p->save(path);
	delete p;
}

}}// end namespace

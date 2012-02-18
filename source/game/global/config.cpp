// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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
#include "platform_util.h"
#include "leak_dumper.h"

namespace Glest { namespace Global {

using Shared::Util::fileExists;

// =====================================================
// 	class Config
// =====================================================

Config::Config(const char* fileName) {
	Properties *p = new Properties();
	if (fileExists(fileName)) {
		p->load(fileName, true);
	}

	int width=800, height=600;
	getScreenMode(width, height);

	aiLogLevel = p->getInt("AiLogLevel", 1, 1, 3);
	aiLoggingEnabled = p->getBool("AiLoggingEnabled", true);
	cameraInvertXAxis = p->getBool("CameraInvertXAxis", true);
	cameraInvertYAxis = p->getBool("CameraInvertYAxis", true);
	cameraMaxDistance = p->getFloat("CameraMaxDistance", 64.f, 32.f, 2048.f);
	cameraMaxYaw = p->getFloat("CameraMaxYaw", 77.50f, 0.f, 360.f);
	cameraMinDistance = p->getFloat("CameraMinDistance", 8.f, 0.f, 20.f);
	cameraMinYaw = p->getFloat("CameraMinYaw", 20.f, 0.f, 360.f);
	displayHeight = p->getInt("DisplayHeight", height);
	displayRefreshFrequency = p->getInt("DisplayRefreshFrequency", 60);
	displayWidth = p->getInt("DisplayWidth", width);
	displayWindowPosX = p->getInt("DisplayWindowPosX", -1);
	displayWindowPosY = p->getInt("DisplayWindowPosY", -1);
	displayWindowed = p->getBool("DisplayWindowed", false);
	gsAutoRepairEnabled = p->getBool("GsAutoRepairEnabled", true);
	gsAutoReturnEnabled = p->getBool("GsAutoReturnEnabled", false);
	gsDayTime = p->getFloat("GsDayTime", 1000.f);
	gsWorldUpdateFps = p->getInt("GsWorldUpdateFps", 40);
	miscCatchExceptions = p->getBool("MiscCatchExceptions", true);
	miscDebugKeys = p->getBool("MiscDebugKeys", false);
	miscDebugMode = p->getBool("MiscDebugMode", false);
	miscFirstTime = p->getBool("MiscFirstTime", true);
	netAnnouceOnLAN = p->getBool("NetAnnouceOnLAN", true);
	netAnnouncePort = p->getInt("NetAnnouncePort", 4950, 1024, 65535);
	netConsistencyChecks = p->getBool("NetConsistencyChecks", false);
	netPlayerName = p->getString("NetPlayerName", "Player");
	netServerIp = p->getString("NetServerIp", "192.168.1.1");
	netServerPort = p->getInt("NetServerPort", 61357, 1024, 65535);
	renderCheckGlCaps = p->getBool("RenderCheckGlCaps", true);
	renderColorBits = p->getInt("RenderColorBits", 32);
	renderCompressTextures = p->getBool("RenderCompressTextures", true);
	renderDepthBits = p->getInt("RenderDepthBits", isWindows()?32:16);
	renderDistanceMax = p->getFloat("RenderDistanceMax", 64.f, 1.f, 65536.f);
	renderDistanceMin = p->getFloat("RenderDistanceMin", 1.f, 0.0f, 65536.f);
	renderEnableBumpMapping = p->getBool("RenderEnableBumpMapping", true);
	renderEnableSpecMapping = p->getBool("RenderEnableSpecMapping", true);
	renderFilter = p->getString("RenderFilter", "Bilinear");
	renderFilterMaxAnisotropy = p->getInt("RenderFilterMaxAnisotropy", 1);
	renderFogOfWarSmoothing = p->getBool("RenderFogOfWarSmoothing", true);
	renderFogOfWarSmoothingFrameSkip = p->getInt("RenderFogOfWarSmoothingFrameSkip", 3);
	renderFontScaler = p->getFloat("RenderFontScaler", 1.f);
	renderFov = p->getFloat("RenderFov", 60.f, 0.01f, 360.f);
	renderFpsMax = p->getInt("RenderFpsMax", 60, 0, 1000000);
	renderGraphicsFactory = p->getString("RenderGraphicsFactory", "OpenGL");
	renderInterpolationMethod = p->getString("RenderInterpolationMethod", "SIMD");
	renderLightsMax = p->getInt("RenderLightsMax", 1, 0, 8);
	renderModelTestShaders = p->getString("RenderModelTestShaders", "basic,bump_map");
	renderMouseCursorType = p->getString("RenderMouseCursorType", "ImageSetMouseCursor");
	renderShadowAlpha = p->getFloat("RenderShadowAlpha", 0.2f, 0.f, 1.f);
	renderShadowFrameSkip = p->getInt("RenderShadowFrameSkip", 2);
	renderShadowTextureSize = p->getInt("RenderShadowTextureSize", 512);
	renderShadows = p->getString("RenderShadows", "Projected");
	renderTerrainRenderer = p->getInt("RenderTerrainRenderer", 2, 1, 2);
	renderTestingShaders = p->getBool("RenderTestingShaders", false);
	renderTextures3D = p->getBool("RenderTextures3D", true);
	renderUseShaders = p->getBool("RenderUseShaders", true);
	renderUseVBOs = p->getBool("RenderUseVBOs", false);
	soundFactory = p->getString("SoundFactory", isWindows()?"DirectSound8":"OpenAL");
	soundStaticBuffers = p->getInt("SoundStaticBuffers", 16);
	soundStreamingBuffers = p->getInt("SoundStreamingBuffers", 5);
	soundVolumeAmbient = p->getInt("SoundVolumeAmbient", 80);
	soundVolumeFx = p->getInt("SoundVolumeFx", 80);
	soundVolumeMusic = p->getInt("SoundVolumeMusic", 90);
	uiConsoleMaxLines = p->getInt("UiConsoleMaxLines", 10);
	uiConsoleTimeout = p->getInt("UiConsoleTimeout", 20);
	uiFocusArrows = p->getBool("UiFocusArrows", true);
	uiLastDisplayPosX = p->getInt("UiLastDisplayPosX", -1);
	uiLastDisplayPosY = p->getInt("UiLastDisplayPosY", -1);
	uiLastDisplaySize = p->getInt("UiLastDisplaySize", 1, 1, 3);
	uiLastMinimapPosX = p->getInt("UiLastMinimapPosX", -1);
	uiLastMinimapPosY = p->getInt("UiLastMinimapPosY", -1);
	uiLastMinimapSize = p->getInt("UiLastMinimapSize", 2, 1, 3);
	uiLastOptionsPage = p->getInt("UiLastOptionsPage", 0, 0, 5);
	uiLastResourceBarPosX = p->getInt("UiLastResourceBarPosX", -1);
	uiLastResourceBarPosY = p->getInt("UiLastResourceBarPosY", -1);
	uiLastResourceBarSize = p->getInt("UiLastResourceBarSize", 1, 1, 3);
	uiLastScenario = p->getString("UiLastScenario", "glest_classic/anarchy");
	uiLastScenarioCatagory = p->getString("UiLastScenarioCatagory", "glest_classic");
	uiLocale = p->getString("UiLocale", "en");
	uiMoveCameraAtScreenEdge = p->getBool("UiMoveCameraAtScreenEdge", true);
	uiPhotoMode = p->getBool("UiPhotoMode", false);
	uiPinWidgets = p->getBool("UiPinWidgets", false);
	uiResourceNames = p->getBool("UiResourceNames", true);
	uiScrollSpeed = p->getFloat("UiScrollSpeed", 1.5f);
	uiTeamColourMode = p->getInt("UiTeamColourMode", 1, 1, 3);

	delete p;

	if (!fileExists(fileName)) {
		save();
	}
}

void Config::save(const char *path) {
	Properties *p = new Properties();

	p->setInt("AiLogLevel", aiLogLevel);
	p->setBool("AiLoggingEnabled", aiLoggingEnabled);
	p->setBool("CameraInvertXAxis", cameraInvertXAxis);
	p->setBool("CameraInvertYAxis", cameraInvertYAxis);
	p->setFloat("CameraMaxDistance", cameraMaxDistance);
	p->setFloat("CameraMaxYaw", cameraMaxYaw);
	p->setFloat("CameraMinDistance", cameraMinDistance);
	p->setFloat("CameraMinYaw", cameraMinYaw);
	p->setInt("DisplayHeight", displayHeight);
	p->setInt("DisplayRefreshFrequency", displayRefreshFrequency);
	p->setInt("DisplayWidth", displayWidth);
	p->setInt("DisplayWindowPosX", displayWindowPosX);
	p->setInt("DisplayWindowPosY", displayWindowPosY);
	p->setBool("DisplayWindowed", displayWindowed);
	p->setBool("GsAutoRepairEnabled", gsAutoRepairEnabled);
	p->setBool("GsAutoReturnEnabled", gsAutoReturnEnabled);
	p->setFloat("GsDayTime", gsDayTime);
	p->setInt("GsWorldUpdateFps", gsWorldUpdateFps);
	p->setBool("MiscCatchExceptions", miscCatchExceptions);
	p->setBool("MiscDebugKeys", miscDebugKeys);
	p->setBool("MiscDebugMode", miscDebugMode);
	p->setBool("MiscFirstTime", miscFirstTime);
	p->setBool("NetAnnouceOnLAN", netAnnouceOnLAN);
	p->setInt("NetAnnouncePort", netAnnouncePort);
	p->setBool("NetConsistencyChecks", netConsistencyChecks);
	p->setString("NetPlayerName", netPlayerName);
	p->setString("NetServerIp", netServerIp);
	p->setInt("NetServerPort", netServerPort);
	p->setBool("RenderCheckGlCaps", renderCheckGlCaps);
	p->setInt("RenderColorBits", renderColorBits);
	p->setBool("RenderCompressTextures", renderCompressTextures);
	p->setInt("RenderDepthBits", renderDepthBits);
	p->setFloat("RenderDistanceMax", renderDistanceMax);
	p->setFloat("RenderDistanceMin", renderDistanceMin);
	p->setBool("RenderEnableBumpMapping", renderEnableBumpMapping);
	p->setBool("RenderEnableSpecMapping", renderEnableSpecMapping);
	p->setString("RenderFilter", renderFilter);
	p->setInt("RenderFilterMaxAnisotropy", renderFilterMaxAnisotropy);
	p->setBool("RenderFogOfWarSmoothing", renderFogOfWarSmoothing);
	p->setInt("RenderFogOfWarSmoothingFrameSkip", renderFogOfWarSmoothingFrameSkip);
	p->setFloat("RenderFontScaler", renderFontScaler);
	p->setFloat("RenderFov", renderFov);
	p->setInt("RenderFpsMax", renderFpsMax);
	p->setString("RenderGraphicsFactory", renderGraphicsFactory);
	p->setString("RenderInterpolationMethod", renderInterpolationMethod);
	p->setInt("RenderLightsMax", renderLightsMax);
	p->setString("RenderModelTestShaders", renderModelTestShaders);
	p->setString("RenderMouseCursorType", renderMouseCursorType);
	p->setFloat("RenderShadowAlpha", renderShadowAlpha);
	p->setInt("RenderShadowFrameSkip", renderShadowFrameSkip);
	p->setInt("RenderShadowTextureSize", renderShadowTextureSize);
	p->setString("RenderShadows", renderShadows);
	p->setInt("RenderTerrainRenderer", renderTerrainRenderer);
	p->setBool("RenderTestingShaders", renderTestingShaders);
	p->setBool("RenderTextures3D", renderTextures3D);
	p->setBool("RenderUseShaders", renderUseShaders);
	p->setBool("RenderUseVBOs", renderUseVBOs);
	p->setString("SoundFactory", soundFactory);
	p->setInt("SoundStaticBuffers", soundStaticBuffers);
	p->setInt("SoundStreamingBuffers", soundStreamingBuffers);
	p->setInt("SoundVolumeAmbient", soundVolumeAmbient);
	p->setInt("SoundVolumeFx", soundVolumeFx);
	p->setInt("SoundVolumeMusic", soundVolumeMusic);
	p->setInt("UiConsoleMaxLines", uiConsoleMaxLines);
	p->setInt("UiConsoleTimeout", uiConsoleTimeout);
	p->setBool("UiFocusArrows", uiFocusArrows);
	p->setInt("UiLastDisplayPosX", uiLastDisplayPosX);
	p->setInt("UiLastDisplayPosY", uiLastDisplayPosY);
	p->setInt("UiLastDisplaySize", uiLastDisplaySize);
	p->setInt("UiLastMinimapPosX", uiLastMinimapPosX);
	p->setInt("UiLastMinimapPosY", uiLastMinimapPosY);
	p->setInt("UiLastMinimapSize", uiLastMinimapSize);
	p->setInt("UiLastOptionsPage", uiLastOptionsPage);
	p->setInt("UiLastResourceBarPosX", uiLastResourceBarPosX);
	p->setInt("UiLastResourceBarPosY", uiLastResourceBarPosY);
	p->setInt("UiLastResourceBarSize", uiLastResourceBarSize);
	p->setString("UiLastScenario", uiLastScenario);
	p->setString("UiLastScenarioCatagory", uiLastScenarioCatagory);
	p->setString("UiLocale", uiLocale);
	p->setBool("UiMoveCameraAtScreenEdge", uiMoveCameraAtScreenEdge);
	p->setBool("UiPhotoMode", uiPhotoMode);
	p->setBool("UiPinWidgets", uiPinWidgets);
	p->setBool("UiResourceNames", uiResourceNames);
	p->setFloat("UiScrollSpeed", uiScrollSpeed);
	p->setInt("UiTeamColourMode", uiTeamColourMode);

	p->save(path);
	delete p;
}

}} // end namespace


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

#ifndef WIN32
#include "platform_util.h"
#endif

#include "leak_dumper.h"

namespace Glest { namespace Global {

// =====================================================
// 	class Config
// =====================================================

Config::Config(const char* fileName) {
	Properties *p = new Properties();
	if(Shared::Util::fileExists(fileName)) {
		p->load(fileName, true);
	}

	int width=1024, height=768;
#ifndef WIN32
	getScreenMode(width, height);
#endif

	cameraInvertXAxis = p->getBool("CameraInvertXAxis", true);
	cameraInvertYAxis = p->getBool("CameraInvertYAxis", true);
	cameraMaxDistance = p->getFloat("CameraMaxDistance", 64.f, 32.f, 2048.f);
	cameraMaxYaw = p->getFloat("CameraMaxYaw", 77.50f, 0.f, 360.f);
	cameraMinDistance = p->getFloat("CameraMinDistance", 8.f, 0.f, 20.f);
	cameraMinYaw = p->getFloat("CameraMinYaw", 20.f, 0.f, 360.f);
	displayHeight = p->getInt("DisplayHeight", height);
	displayRefreshFrequency = p->getInt("DisplayRefreshFrequency", 60);
	displayWidth = p->getInt("DisplayWidth", width);
	displayWindowed = p->getBool("DisplayWindowed", false);
	gsAutoRepairEnabled = p->getBool("GsAutoRepairEnabled", true);
	gsAutoReturnEnabled = p->getBool("GsAutoReturnEnabled", false);
	gsDayTime = p->getFloat("GsDayTime", 1000.f);
	gsWorldUpdateFps = p->getInt("GsWorldUpdateFps", 40);
	miscCatchExceptions = p->getBool("MiscCatchExceptions", true);
	miscDebugKeys = p->getBool("MiscDebugKeys", false);
	miscDebugMode = p->getBool("MiscDebugMode", false);
	miscFirstTime = p->getBool("MiscFirstTime", true);
	netConsistencyChecks = p->getBool("NetConsistencyChecks", false);
	netPlayerName = p->getString("NetPlayerName", "Player");
	netServerIp = p->getString("NetServerIp", "192.168.1.1");
	netServerPort = p->getInt("NetServerPort", 12345, 1024, 65535);
	renderCheckGlCaps = p->getBool("RenderCheckGlCaps", true);
	renderColorBits = p->getInt("RenderColorBits", 32);
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
	renderModelShader = p->getString("RenderModelShader", "basic");
	renderModelTestShaders = p->getString("RenderModelTestShaders", "basic,bump_map");
	renderShadowAlpha = p->getFloat("RenderShadowAlpha", 0.2f, 0.f, 1.f);
	renderShadowFrameSkip = p->getInt("RenderShadowFrameSkip", 2);
	renderShadowTextureSize = p->getInt("RenderShadowTextureSize", 512);
	renderShadows = p->getString("RenderShadows", "Projected");
	renderTestingShaders = p->getBool("RenderTestingShaders", false);
	renderTextures3D = p->getBool("RenderTextures3D", true);
	renderUseShaders = p->getBool("RenderUseShaders", true);
	soundFactory = p->getString("SoundFactory", isWindows()?"DirectSound8":"OpenAL");
	soundStaticBuffers = p->getInt("SoundStaticBuffers", 16);
	soundStreamingBuffers = p->getInt("SoundStreamingBuffers", 5);
	soundVolumeAmbient = p->getInt("SoundVolumeAmbient", 80);
	soundVolumeFx = p->getInt("SoundVolumeFx", 80);
	soundVolumeMusic = p->getInt("SoundVolumeMusic", 90);
	uiConsoleMaxLines = p->getInt("UiConsoleMaxLines", 10);
	uiConsoleTimeout = p->getInt("UiConsoleTimeout", 20);
	uiFocusArrows = p->getBool("UiFocusArrows", true);
	uiLastScenario = p->getString("UiLastScenario", "glest_classic/anarchy");
	uiLastScenarioCatagory = p->getString("UiLastScenarioCatagory", "glest_classic");
	uiLocale = p->getString("UiLocale", "en");
	uiPhotoMode = p->getBool("UiPhotoMode", false);
	uiScrollSpeed = p->getFloat("UiScrollSpeed", 1.5f);

	delete p;

	if (!Shared::Util::fileExists(fileName)) {
		save();
	}
}

void Config::save(const char *path) {
	Properties *p = new Properties();

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
	p->setInt("GsWorldUpdateFps", gsWorldUpdateFps);
	p->setBool("MiscCatchExceptions", miscCatchExceptions);
	p->setBool("MiscDebugKeys", miscDebugKeys);
	p->setBool("MiscDebugMode", miscDebugMode);
	p->setBool("MiscFirstTime", miscFirstTime);
	p->setBool("NetConsistencyChecks", netConsistencyChecks);
	p->setString("NetPlayerName", netPlayerName);
	p->setString("NetServerIp", netServerIp);
	p->setInt("NetServerPort", netServerPort);
	p->setBool("RenderCheckGlCaps", renderCheckGlCaps);
	p->setInt("RenderColorBits", renderColorBits);
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
	p->setString("RenderModelShader", renderModelShader);
	p->setString("RenderModelTestShaders", renderModelTestShaders);
	p->setFloat("RenderShadowAlpha", renderShadowAlpha);
	p->setInt("RenderShadowFrameSkip", renderShadowFrameSkip);
	p->setInt("RenderShadowTextureSize", renderShadowTextureSize);
	p->setString("RenderShadows", renderShadows);
	p->setBool("RenderTestingShaders", renderTestingShaders);
	p->setBool("RenderTextures3D", renderTextures3D);
	p->setBool("RenderUseShaders", renderUseShaders);
	p->setString("SoundFactory", soundFactory);
	p->setInt("SoundStaticBuffers", soundStaticBuffers);
	p->setInt("SoundStreamingBuffers", soundStreamingBuffers);
	p->setInt("SoundVolumeAmbient", soundVolumeAmbient);
	p->setInt("SoundVolumeFx", soundVolumeFx);
	p->setInt("SoundVolumeMusic", soundVolumeMusic);
	p->setInt("UiConsoleMaxLines", uiConsoleMaxLines);
	p->setInt("UiConsoleTimeout", uiConsoleTimeout);
	p->setBool("UiFocusArrows", uiFocusArrows);
	p->setString("UiLastScenario", uiLastScenario);
	p->setString("UiLastScenarioCatagory", uiLastScenarioCatagory);
	p->setString("UiLocale", uiLocale);
	p->setBool("UiPhotoMode", uiPhotoMode);
	p->setFloat("UiScrollSpeed", uiScrollSpeed);

	p->save(path);
	delete p;
}

}} // end namespace


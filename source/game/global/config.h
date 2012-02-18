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

// This file is auto-generated from config.h.template using ../config.db and the script
// ../mkconfig.sh.  To modify actual config settings, edit config.db and re-run ../mkconfig.sh.

#ifndef _GLEST_GAME_CONFIG_H_
#define _GLEST_GAME_CONFIG_H_

#include "properties.h"

namespace Glest { namespace Global {

using Shared::Util::Properties;

// =====================================================
// 	class Config
//
//	Game configuration
// =====================================================

class Config {
private:

	int aiLogLevel;
	bool aiLoggingEnabled;
	bool cameraInvertXAxis;
	bool cameraInvertYAxis;
	float cameraMaxDistance;
	float cameraMaxYaw;
	float cameraMinDistance;
	float cameraMinYaw;
	int displayHeight;
	int displayRefreshFrequency;
	int displayWidth;
	int displayWindowPosX;
	int displayWindowPosY;
	bool displayWindowed;
	bool gsAutoRepairEnabled;
	bool gsAutoReturnEnabled;
	float gsDayTime;
	int gsWorldUpdateFps;
	bool miscCatchExceptions;
	bool miscDebugKeys;
	bool miscDebugMode;
	bool miscFirstTime;
	bool netAnnouceOnLAN;
	int netAnnouncePort;
	bool netConsistencyChecks;
	string netPlayerName;
	string netServerIp;
	int netServerPort;
	bool renderCheckGlCaps;
	int renderColorBits;
	bool renderCompressTextures;
	int renderDepthBits;
	float renderDistanceMax;
	float renderDistanceMin;
	bool renderEnableBumpMapping;
	bool renderEnableSpecMapping;
	string renderFilter;
	int renderFilterMaxAnisotropy;
	bool renderFogOfWarSmoothing;
	int renderFogOfWarSmoothingFrameSkip;
	float renderFontScaler;
	float renderFov;
	int renderFpsMax;
	string renderGraphicsFactory;
	string renderInterpolationMethod;
	int renderLightsMax;
	string renderModelTestShaders;
	string renderMouseCursorType;
	float renderShadowAlpha;
	int renderShadowFrameSkip;
	int renderShadowTextureSize;
	string renderShadows;
	int renderTerrainRenderer;
	bool renderTestingShaders;
	bool renderTextures3D;
	bool renderUseShaders;
	bool renderUseVBOs;
	string soundFactory;
	int soundStaticBuffers;
	int soundStreamingBuffers;
	int soundVolumeAmbient;
	int soundVolumeFx;
	int soundVolumeMusic;
	int uiConsoleMaxLines;
	int uiConsoleTimeout;
	bool uiFocusArrows;
	int uiLastDisplayPosX;
	int uiLastDisplayPosY;
	int uiLastDisplaySize;
	int uiLastMinimapPosX;
	int uiLastMinimapPosY;
	int uiLastMinimapSize;
	int uiLastOptionsPage;
	int uiLastResourceBarPosX;
	int uiLastResourceBarPosY;
	int uiLastResourceBarSize;
	string uiLastScenario;
	string uiLastScenarioCatagory;
	string uiLocale;
	bool uiMoveCameraAtScreenEdge;
	bool uiPhotoMode;
	bool uiPinWidgets;
	bool uiResourceNames;
	float uiScrollSpeed;
	int uiTeamColourMode;

	Config(const char* fileName);
	
	static bool isWindows() {
#		ifdef WIN32
			return true;
#		else
			return false;
#		endif
	}

public:
	static Config &getInstance() {
		static Config singleton("glestadv.ini");
		return singleton;
	}

	void save(const char *path = "glestadv.ini");

	int getAiLogLevel() const					{return aiLogLevel;}
	bool getAiLoggingEnabled() const			{return aiLoggingEnabled;}
	bool getCameraInvertXAxis() const			{return cameraInvertXAxis;}
	bool getCameraInvertYAxis() const			{return cameraInvertYAxis;}
	float getCameraMaxDistance() const			{return cameraMaxDistance;}
	float getCameraMaxYaw() const				{return cameraMaxYaw;}
	float getCameraMinDistance() const			{return cameraMinDistance;}
	float getCameraMinYaw() const				{return cameraMinYaw;}
	int getDisplayHeight() const				{return displayHeight;}
	int getDisplayRefreshFrequency() const		{return displayRefreshFrequency;}
	int getDisplayWidth() const					{return displayWidth;}
	int getDisplayWindowPosX() const			{return displayWindowPosX;}
	int getDisplayWindowPosY() const			{return displayWindowPosY;}
	bool getDisplayWindowed() const				{return displayWindowed;}
	bool getGsAutoRepairEnabled() const			{return gsAutoRepairEnabled;}
	bool getGsAutoReturnEnabled() const			{return gsAutoReturnEnabled;}
	float getGsDayTime() const					{return gsDayTime;}
	int getGsWorldUpdateFps() const				{return gsWorldUpdateFps;}
	bool getMiscCatchExceptions() const			{return miscCatchExceptions;}
	bool getMiscDebugKeys() const				{return miscDebugKeys;}
	bool getMiscDebugMode() const				{return miscDebugMode;}
	bool getMiscFirstTime() const				{return miscFirstTime;}
	bool getNetAnnouceOnLAN() const				{return netAnnouceOnLAN;}
	int getNetAnnouncePort() const				{return netAnnouncePort;}
	bool getNetConsistencyChecks() const		{return netConsistencyChecks;}
	string getNetPlayerName() const				{return netPlayerName;}
	string getNetServerIp() const				{return netServerIp;}
	int getNetServerPort() const				{return netServerPort;}
	bool getRenderCheckGlCaps() const			{return renderCheckGlCaps;}
	int getRenderColorBits() const				{return renderColorBits;}
	bool getRenderCompressTextures() const		{return renderCompressTextures;}
	int getRenderDepthBits() const				{return renderDepthBits;}
	float getRenderDistanceMax() const			{return renderDistanceMax;}
	float getRenderDistanceMin() const			{return renderDistanceMin;}
	bool getRenderEnableBumpMapping() const		{return renderEnableBumpMapping;}
	bool getRenderEnableSpecMapping() const		{return renderEnableSpecMapping;}
	string getRenderFilter() const				{return renderFilter;}
	int getRenderFilterMaxAnisotropy() const	{return renderFilterMaxAnisotropy;}
	bool getRenderFogOfWarSmoothing() const		{return renderFogOfWarSmoothing;}
	int getRenderFogOfWarSmoothingFrameSkip() const{return renderFogOfWarSmoothingFrameSkip;}
	float getRenderFontScaler() const			{return renderFontScaler;}
	float getRenderFov() const					{return renderFov;}
	int getRenderFpsMax() const					{return renderFpsMax;}
	string getRenderGraphicsFactory() const		{return renderGraphicsFactory;}
	string getRenderInterpolationMethod() const	{return renderInterpolationMethod;}
	int getRenderLightsMax() const				{return renderLightsMax;}
	string getRenderModelTestShaders() const	{return renderModelTestShaders;}
	string getRenderMouseCursorType() const		{return renderMouseCursorType;}
	float getRenderShadowAlpha() const			{return renderShadowAlpha;}
	int getRenderShadowFrameSkip() const		{return renderShadowFrameSkip;}
	int getRenderShadowTextureSize() const		{return renderShadowTextureSize;}
	string getRenderShadows() const				{return renderShadows;}
	int getRenderTerrainRenderer() const		{return renderTerrainRenderer;}
	bool getRenderTestingShaders() const		{return renderTestingShaders;}
	bool getRenderTextures3D() const			{return renderTextures3D;}
	bool getRenderUseShaders() const			{return renderUseShaders;}
	bool getRenderUseVBOs() const				{return renderUseVBOs;}
	string getSoundFactory() const				{return soundFactory;}
	int getSoundStaticBuffers() const			{return soundStaticBuffers;}
	int getSoundStreamingBuffers() const		{return soundStreamingBuffers;}
	int getSoundVolumeAmbient() const			{return soundVolumeAmbient;}
	int getSoundVolumeFx() const				{return soundVolumeFx;}
	int getSoundVolumeMusic() const				{return soundVolumeMusic;}
	int getUiConsoleMaxLines() const			{return uiConsoleMaxLines;}
	int getUiConsoleTimeout() const				{return uiConsoleTimeout;}
	bool getUiFocusArrows() const				{return uiFocusArrows;}
	int getUiLastDisplayPosX() const			{return uiLastDisplayPosX;}
	int getUiLastDisplayPosY() const			{return uiLastDisplayPosY;}
	int getUiLastDisplaySize() const			{return uiLastDisplaySize;}
	int getUiLastMinimapPosX() const			{return uiLastMinimapPosX;}
	int getUiLastMinimapPosY() const			{return uiLastMinimapPosY;}
	int getUiLastMinimapSize() const			{return uiLastMinimapSize;}
	int getUiLastOptionsPage() const			{return uiLastOptionsPage;}
	int getUiLastResourceBarPosX() const		{return uiLastResourceBarPosX;}
	int getUiLastResourceBarPosY() const		{return uiLastResourceBarPosY;}
	int getUiLastResourceBarSize() const		{return uiLastResourceBarSize;}
	string getUiLastScenario() const			{return uiLastScenario;}
	string getUiLastScenarioCatagory() const	{return uiLastScenarioCatagory;}
	string getUiLocale() const					{return uiLocale;}
	bool getUiMoveCameraAtScreenEdge() const	{return uiMoveCameraAtScreenEdge;}
	bool getUiPhotoMode() const					{return uiPhotoMode;}
	bool getUiPinWidgets() const				{return uiPinWidgets;}
	bool getUiResourceNames() const				{return uiResourceNames;}
	float getUiScrollSpeed() const				{return uiScrollSpeed;}
	int getUiTeamColourMode() const				{return uiTeamColourMode;}

	void setAiLogLevel(int val)					{aiLogLevel = val;}
	void setAiLoggingEnabled(bool val)			{aiLoggingEnabled = val;}
	void setCameraInvertXAxis(bool val)			{cameraInvertXAxis = val;}
	void setCameraInvertYAxis(bool val)			{cameraInvertYAxis = val;}
	void setCameraMaxDistance(float val)		{cameraMaxDistance = val;}
	void setCameraMaxYaw(float val)				{cameraMaxYaw = val;}
	void setCameraMinDistance(float val)		{cameraMinDistance = val;}
	void setCameraMinYaw(float val)				{cameraMinYaw = val;}
	void setDisplayHeight(int val)				{displayHeight = val;}
	void setDisplayRefreshFrequency(int val)	{displayRefreshFrequency = val;}
	void setDisplayWidth(int val)				{displayWidth = val;}
	void setDisplayWindowPosX(int val)			{displayWindowPosX = val;}
	void setDisplayWindowPosY(int val)			{displayWindowPosY = val;}
	void setDisplayWindowed(bool val)			{displayWindowed = val;}
	void setGsAutoRepairEnabled(bool val)		{gsAutoRepairEnabled = val;}
	void setGsAutoReturnEnabled(bool val)		{gsAutoReturnEnabled = val;}
	void setGsDayTime(float val)				{gsDayTime = val;}
	void setGsWorldUpdateFps(int val)			{gsWorldUpdateFps = val;}
	void setMiscCatchExceptions(bool val)		{miscCatchExceptions = val;}
	void setMiscDebugKeys(bool val)				{miscDebugKeys = val;}
	void setMiscDebugMode(bool val)				{miscDebugMode = val;}
	void setMiscFirstTime(bool val)				{miscFirstTime = val;}
	void setNetAnnouceOnLAN(bool val)			{netAnnouceOnLAN = val;}
	void setNetAnnouncePort(int val)			{netAnnouncePort = val;}
	void setNetConsistencyChecks(bool val)		{netConsistencyChecks = val;}
	void setNetPlayerName(string val)			{netPlayerName = val;}
	void setNetServerIp(string val)				{netServerIp = val;}
	void setNetServerPort(int val)				{netServerPort = val;}
	void setRenderCheckGlCaps(bool val)			{renderCheckGlCaps = val;}
	void setRenderColorBits(int val)			{renderColorBits = val;}
	void setRenderCompressTextures(bool val)	{renderCompressTextures = val;}
	void setRenderDepthBits(int val)			{renderDepthBits = val;}
	void setRenderDistanceMax(float val)		{renderDistanceMax = val;}
	void setRenderDistanceMin(float val)		{renderDistanceMin = val;}
	void setRenderEnableBumpMapping(bool val)	{renderEnableBumpMapping = val;}
	void setRenderEnableSpecMapping(bool val)	{renderEnableSpecMapping = val;}
	void setRenderFilter(string val)			{renderFilter = val;}
	void setRenderFilterMaxAnisotropy(int val)	{renderFilterMaxAnisotropy = val;}
	void setRenderFogOfWarSmoothing(bool val)	{renderFogOfWarSmoothing = val;}
	void setRenderFogOfWarSmoothingFrameSkip(int val){renderFogOfWarSmoothingFrameSkip = val;}
	void setRenderFontScaler(float val)			{renderFontScaler = val;}
	void setRenderFov(float val)				{renderFov = val;}
	void setRenderFpsMax(int val)				{renderFpsMax = val;}
	void setRenderGraphicsFactory(string val)	{renderGraphicsFactory = val;}
	void setRenderInterpolationMethod(string val){renderInterpolationMethod = val;}
	void setRenderLightsMax(int val)			{renderLightsMax = val;}
	void setRenderModelTestShaders(string val)	{renderModelTestShaders = val;}
	void setRenderMouseCursorType(string val)	{renderMouseCursorType = val;}
	void setRenderShadowAlpha(float val)		{renderShadowAlpha = val;}
	void setRenderShadowFrameSkip(int val)		{renderShadowFrameSkip = val;}
	void setRenderShadowTextureSize(int val)	{renderShadowTextureSize = val;}
	void setRenderShadows(string val)			{renderShadows = val;}
	void setRenderTerrainRenderer(int val)		{renderTerrainRenderer = val;}
	void setRenderTestingShaders(bool val)		{renderTestingShaders = val;}
	void setRenderTextures3D(bool val)			{renderTextures3D = val;}
	void setRenderUseShaders(bool val)			{renderUseShaders = val;}
	void setRenderUseVBOs(bool val)				{renderUseVBOs = val;}
	void setSoundFactory(string val)			{soundFactory = val;}
	void setSoundStaticBuffers(int val)			{soundStaticBuffers = val;}
	void setSoundStreamingBuffers(int val)		{soundStreamingBuffers = val;}
	void setSoundVolumeAmbient(int val)			{soundVolumeAmbient = val;}
	void setSoundVolumeFx(int val)				{soundVolumeFx = val;}
	void setSoundVolumeMusic(int val)			{soundVolumeMusic = val;}
	void setUiConsoleMaxLines(int val)			{uiConsoleMaxLines = val;}
	void setUiConsoleTimeout(int val)			{uiConsoleTimeout = val;}
	void setUiFocusArrows(bool val)				{uiFocusArrows = val;}
	void setUiLastDisplayPosX(int val)			{uiLastDisplayPosX = val;}
	void setUiLastDisplayPosY(int val)			{uiLastDisplayPosY = val;}
	void setUiLastDisplaySize(int val)			{uiLastDisplaySize = val;}
	void setUiLastMinimapPosX(int val)			{uiLastMinimapPosX = val;}
	void setUiLastMinimapPosY(int val)			{uiLastMinimapPosY = val;}
	void setUiLastMinimapSize(int val)			{uiLastMinimapSize = val;}
	void setUiLastOptionsPage(int val)			{uiLastOptionsPage = val;}
	void setUiLastResourceBarPosX(int val)		{uiLastResourceBarPosX = val;}
	void setUiLastResourceBarPosY(int val)		{uiLastResourceBarPosY = val;}
	void setUiLastResourceBarSize(int val)		{uiLastResourceBarSize = val;}
	void setUiLastScenario(string val)			{uiLastScenario = val;}
	void setUiLastScenarioCatagory(string val)	{uiLastScenarioCatagory = val;}
	void setUiLocale(string val)				{uiLocale = val;}
	void setUiMoveCameraAtScreenEdge(bool val)	{uiMoveCameraAtScreenEdge = val;}
	void setUiPhotoMode(bool val)				{uiPhotoMode = val;}
	void setUiPinWidgets(bool val)				{uiPinWidgets = val;}
	void setUiResourceNames(bool val)			{uiResourceNames = val;}
	void setUiScrollSpeed(float val)			{uiScrollSpeed = val;}
	void setUiTeamColourMode(int val)			{uiTeamColourMode = val;}
};

}}//end namespace

#endif



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

namespace Glest { namespace Game {

using Shared::Util::Properties;

// =====================================================
// 	class Config
//
//	Game configuration
// =====================================================

class Config {
private:
	Properties *properties;
	string fileName;

	float cameraFov;
	bool cameraInvertXAxis;
	bool cameraInvertYAxis;
	float cameraMaxDistance;
	float cameraMaxYaw;
	float cameraMinDistance;
	float cameraMinYaw;
	int displayHeight;
	int displayRefreshFrequency;
	int displayWidth;
	bool displayWindowed;
	bool gsAutoRepairEnabled;
	bool gsAutoReturnEnabled;
	float gsDayTime;
	//bool gsFogOfWarEnabled;
	//bool gsShroudOfDarknessEnabled;
	float gsSpeedFastest;
	float gsSpeedSlowest;
	int gsWorldUpdateFps;
	int miscAiLog;
	bool miscAiRedir;
	bool miscAutoTest;
	bool miscCatchExceptions;
	bool miscDebugKeys;
	bool miscDebugMode;
	bool miscFirstTime;
	//bool netChangeSpeedAllowed;
	bool netConsistencyChecks;
	//int netFps;
	//int netMinFullUpdateInterval;
	//bool netPauseAllowed;
	string netPlayerName;
	string netServerIp;
	int netServerPort;
	bool renderCheckGlCaps;
	int renderColorBits;
	int renderDepthBits;
	float renderDistanceMax;
	float renderDistanceMin;
	int renderFilterMaxAnisotropy;
	string renderFilter;
	bool renderFogOfWarSmoothing;
	int renderFogOfWarSmoothingFrameSkip;
	string renderFontConsole;
	string renderFontDisplay;
	string renderFontMenu;
	float renderFov;
	int renderFpsMax;
	string renderGraphicsFactory;
	int renderLightsMax;
	float renderShadowAlpha;
	int renderShadowFrameSkip;
	string renderShadows;
	int renderShadowTextureSize;
	int renderStencilBits;
	bool renderTextures3D;
	string soundFactory;
	int soundStaticBuffers;
	int soundStreamingBuffers;
	int soundVolumeAmbient;
	int soundVolumeFx;
	int soundVolumeMusic;
	int uiConsoleMaxLines;
	int uiConsoleTimeout;
	bool uiEnableCommandMinimap;
	bool uiFocusArrows;
	string uiLastMap;
	bool uiLastRandStartLocs;
	string uiLastScenarioCatagory;
	string uiLastScenario;
	string uiLastTechTree;
	string uiLastTileset;
	string uiLocale;
	bool uiPhotoMode;
	bool uiRandomStartLocations;
	float uiScrollSpeed;

	Config(const char* fileName);

	static bool isWindows() {
#if defined(WIN32) || defined(WIN64)
		return true;
#else
		return false;
#endif
	}

	const char *getDefaultFontStr() const {
		if(isWindows()) {
			return "Verdana";
		} else {
			return "-*-*-*-*-*-12-*-*-*-*-*-*-*";
		}
	}

public:
	static Config &getInstance() {
		static Config singleton("glestadv.ini");
		return singleton;
	}

	void save(const char *path = "glestadv.ini");

	float getCameraFov() const						{return cameraFov;}
	bool getCameraInvertXAxis() const				{return cameraInvertXAxis;}
	bool getCameraInvertYAxis() const				{return cameraInvertYAxis;}
	float getCameraMaxDistance() const				{return cameraMaxDistance;}
	float getCameraMaxYaw() const					{return cameraMaxYaw;}
	float getCameraMinDistance() const				{return cameraMinDistance;}
	float getCameraMinYaw() const					{return cameraMinYaw;}
	int getDisplayHeight() const					{return displayHeight;}
	int getDisplayRefreshFrequency() const			{return displayRefreshFrequency;}
	int getDisplayWidth() const						{return displayWidth;}
	bool getDisplayWindowed() const					{return displayWindowed;}
	bool getGsAutoRepairEnabled() const				{return gsAutoRepairEnabled;}
	bool getGsAutoReturnEnabled() const				{return gsAutoReturnEnabled;}
	float getGsDayTime() const						{return gsDayTime;}
	//bool getGsFogOfWarEnabled() const				{return gsFogOfWarEnabled;}
	//bool getGsShroudOfDarknessEnabled() const		{return gsShroudOfDarknessEnabled;}
	float getGsSpeedFastest() const					{return gsSpeedFastest;}
	float getGsSpeedSlowest() const					{return gsSpeedSlowest;}
	int getGsWorldUpdateFps() const					{return gsWorldUpdateFps;}
	int getMiscAiLog() const						{return miscAiLog;}
	bool getMiscAiRedir() const						{return miscAiRedir;}
	bool getMiscAutoTest() const					{return miscAutoTest;}
	bool getMiscCatchExceptions() const				{return miscCatchExceptions;}
	bool getMiscDebugKeys() const					{return miscDebugKeys;}
	bool getMiscDebugMode() const					{return miscDebugMode;}
	bool getMiscFirstTime() const					{return miscFirstTime;}
//	bool getNetChangeSpeedAllowed() const			{return netChangeSpeedAllowed;}
	bool getNetConsistencyChecks() const			{return netConsistencyChecks;}
//	int getNetFps() const							{return netFps;}
//	int getNetMinFullUpdateInterval() const			{return netMinFullUpdateInterval;}
//	bool getNetPauseAllowed() const					{return netPauseAllowed;}
	const string &getNetPlayerName() const			{return netPlayerName;}
	const string &getNetServerIp() const			{return netServerIp;}
	int getNetServerPort() const					{return netServerPort;}
	bool getRenderCheckGlCaps() const				{return renderCheckGlCaps;}
	int getRenderColorBits() const					{return renderColorBits;}
	int getRenderDepthBits() const					{return renderDepthBits;}
	float getRenderDistanceMax() const				{return renderDistanceMax;}
	float getRenderDistanceMin() const				{return renderDistanceMin;}
	int getRenderFilterMaxAnisotropy() const		{return renderFilterMaxAnisotropy;}
	const string &getRenderFilter() const			{return renderFilter;}
	bool getRenderFogOfWarSmoothing() const			{return renderFogOfWarSmoothing;}
	int getRenderFogOfWarSmoothingFrameSkip() const	{return renderFogOfWarSmoothingFrameSkip;}
	const string &getRenderFontConsole() const		{return renderFontConsole;}
	const string &getRenderFontDisplay() const		{return renderFontDisplay;}
	const string &getRenderFontMenu() const			{return renderFontMenu;}
	float getRenderFov() const						{return renderFov;}
	int getRenderFpsMax() const						{return renderFpsMax;}
	const string &getRenderGraphicsFactory() const	{return renderGraphicsFactory;}
	int getRenderLightsMax() const					{return renderLightsMax;}
	float getRenderShadowAlpha() const				{return renderShadowAlpha;}
	int getRenderShadowFrameSkip() const			{return renderShadowFrameSkip;}
	const string &getRenderShadows() const			{return renderShadows;}
	int getRenderShadowTextureSize() const			{return renderShadowTextureSize;}
	int getRenderStencilBits() const				{return renderStencilBits;}
	bool getRenderTextures3D() const				{return renderTextures3D;}
	const string &getSoundFactory() const			{return soundFactory;}
	int getSoundStaticBuffers() const				{return soundStaticBuffers;}
	int getSoundStreamingBuffers() const			{return soundStreamingBuffers;}
	int getSoundVolumeAmbient() const				{return soundVolumeAmbient;}
	int getSoundVolumeFx() const					{return soundVolumeFx;}
	int getSoundVolumeMusic() const					{return soundVolumeMusic;}
	int getUiConsoleMaxLines() const				{return uiConsoleMaxLines;}
	int getUiConsoleTimeout() const					{return uiConsoleTimeout;}
	bool getUiEnableCommandMinimap() const			{return uiEnableCommandMinimap;}
	bool getUiFocusArrows() const					{return uiFocusArrows;}
	const string &getUiLastMap() const				{return uiLastMap;}
	bool getUiLastRandStartLocs() const				{return uiLastRandStartLocs;}
	const string &getUiLastScenarioCatagory() const	{return uiLastScenarioCatagory;}
	const string &getUiLastScenario() const			{return uiLastScenario;}
	const string &getUiLastTechTree() const			{return uiLastTechTree;}
	const string &getUiLastTileset() const			{return uiLastTileset;}
	const string &getUiLocale() const				{return uiLocale;}
	bool getUiPhotoMode() const						{return uiPhotoMode;}
	bool getUiRandomStartLocations() const			{return uiRandomStartLocations;}
	float getUiScrollSpeed() const					{return uiScrollSpeed;}

	void setCameraFov(float v)						{cameraFov = v;}
	void setCameraInvertXAxis(bool v)				{cameraInvertXAxis = v;}
	void setCameraInvertYAxis(bool v)				{cameraInvertYAxis = v;}
	void setCameraMaxDistance(float v)				{cameraMaxDistance = v;}
	void setCameraMaxYaw(float v)					{cameraMaxYaw = v;}
	void setCameraMinDistance(float v)				{cameraMinDistance = v;}
	void setCameraMinYaw(float v)					{cameraMinYaw = v;}
	void setDisplayHeight(int v)					{displayHeight = v;}
	void setDisplayRefreshFrequency(int v)			{displayRefreshFrequency = v;}
	void setDisplayWidth(int v)						{displayWidth = v;}
	void setDisplayWindowed(bool v)					{displayWindowed = v;}
	void setGsAutoRepairEnabled(bool v)				{gsAutoRepairEnabled = v;}
	void setGsAutoReturnEnabled(bool v)				{gsAutoReturnEnabled = v;}
	void setGsDayTime(float v)						{gsDayTime = v;}
	//void setGsFogOfWarEnabled(bool v)				{gsFogOfWarEnabled = v;}
	//void setGsShroudOfDarknessEnabled(bool v)		{gsShroudOfDarknessEnabled = v;}
	void setGsSpeedFastest(float v)					{gsSpeedFastest = v;}
	void setGsSpeedSlowest(float v)					{gsSpeedSlowest = v;}
	void setGsWorldUpdateFps(int v)					{gsWorldUpdateFps = v;}
	void setMiscAiLog(int v)						{miscAiLog = v;}
	void setMiscAiRedir(bool v)						{miscAiRedir = v;}
	void setMiscAutoTest(bool v)					{miscAutoTest = v;}
	void setMiscCatchExceptions(bool v)				{miscCatchExceptions = v;}
	void setMiscDebugKeys(bool v)					{miscDebugKeys = v;}
	void setMiscDebugMode(bool v)					{miscDebugMode = v;}
	void setMiscFirstTime(bool v)					{miscFirstTime = v;}
//	void setNetChangeSpeedAllowed(bool v)			{netChangeSpeedAllowed = v;}
	void setNetConsistencyChecks(bool v)			{netConsistencyChecks = v;}
//	void setNetFps(int v)							{netFps = v;}
//	void setNetMinFullUpdateInterval(int v)			{netMinFullUpdateInterval = v;}
//	void setNetPauseAllowed(bool v)					{netPauseAllowed = v;}
	void setNetPlayerName(const string &v)			{netPlayerName = v;}
	void setNetServerIp(const string &v)			{netServerIp = v;}
	void setNetServerPort(int v)					{netServerPort = v;}
	void setRenderCheckGlCaps(bool v)				{renderCheckGlCaps = v;}
	void setRenderColorBits(int v)					{renderColorBits = v;}
	void setRenderDepthBits(int v)					{renderDepthBits = v;}
	void setRenderDistanceMax(float v)				{renderDistanceMax = v;}
	void setRenderDistanceMin(float v)				{renderDistanceMin = v;}
	void setRenderFilterMaxAnisotropy(int v)		{renderFilterMaxAnisotropy = v;}
	void setRenderFilter(const string &v)			{renderFilter = v;}
	void setRenderFogOfWarSmoothing(bool v)			{renderFogOfWarSmoothing = v;}
	void setRenderFogOfWarSmoothingFrameSkip(int v)	{renderFogOfWarSmoothingFrameSkip = v;}
	void setRenderFontConsole(const string &v)		{renderFontConsole = v;}
	void setRenderFontDisplay(const string &v)		{renderFontDisplay = v;}
	void setRenderFontMenu(const string &v)			{renderFontMenu = v;}
	void setRenderFov(float v)						{renderFov = v;}
	void setRenderFpsMax(int v)						{renderFpsMax = v;}
	void setRenderGraphicsFactory(const string &v)	{renderGraphicsFactory = v;}
	void setRenderLightsMax(int v)					{renderLightsMax = v;}
	void setRenderShadowAlpha(float v)				{renderShadowAlpha = v;}
	void setRenderShadowFrameSkip(int v)			{renderShadowFrameSkip = v;}
	void setRenderShadows(const string &v)			{renderShadows = v;}
	void setRenderShadowTextureSize(int v)			{renderShadowTextureSize = v;}
	void setRenderStencilBits(int v)				{renderStencilBits = v;}
	void setRenderTextures3D(bool v)				{renderTextures3D = v;}
	void setSoundFactory(const string &v)			{soundFactory = v;}
	void setSoundStaticBuffers(int v)				{soundStaticBuffers = v;}
	void setSoundStreamingBuffers(int v)			{soundStreamingBuffers = v;}
	void setSoundVolumeAmbient(int v)				{soundVolumeAmbient = v;}
	void setSoundVolumeFx(int v)					{soundVolumeFx = v;}
	void setSoundVolumeMusic(int v)					{soundVolumeMusic = v;}
	void setUiConsoleMaxLines(int v)				{uiConsoleMaxLines = v;}
	void setUiConsoleTimeout(int v)					{uiConsoleTimeout = v;}
	void setUiEnableCommandMinimap(bool v)			{uiEnableCommandMinimap = v;}
	void setUiFocusArrows(bool v)					{uiFocusArrows = v;}
	void setUiLastMap(const string &v)				{uiLastMap = v;}
	void setUiLastRandStartLocs(bool v)				{uiLastRandStartLocs = v;}
	void setUiLastScenarioCatagory(const string &v)	{uiLastScenarioCatagory = v;}
	void setUiLastScenario(const string &v)			{uiLastScenario = v;}
	void setUiLastTechTree(const string &v)			{uiLastTechTree = v;}
	void setUiLastTileset(const string &v)			{uiLastTileset = v;}
	void setUiLocale(const string &v)				{uiLocale = v;}
	void setUiPhotoMode(bool v)						{uiPhotoMode = v;}
	void setUiRandomStartLocations(bool v)			{uiRandomStartLocations = v;}
	void setUiScrollSpeed(float v)					{uiScrollSpeed = v;}
};

}}//end namespace

#endif



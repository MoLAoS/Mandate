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
// 	Properties *properties;
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
	int gsWorldUpdateFps;
	bool miscCatchExceptions;
	bool miscDebugKeys;
	bool miscDebugMode;
	bool miscFirstTime;
	bool netConsistencyChecks;
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
	float renderFontScaler;
	bool renderFogOfWarSmoothing;
	int renderFogOfWarSmoothingFrameSkip;
	float renderFov;
	int renderFpsMax;
	string renderGraphicsFactory;
	bool renderInterpolateWithSIMD;
	int renderLightsMax;
	float renderShadowAlpha;
	int renderShadowFrameSkip;
	string renderShadows;
	int renderShadowTextureSize;
	int renderStencilBits;
	bool renderTextures3D;
	bool renderUseShaders;
	bool renderUseVBOs;
	string renderUnitShaders;
	string soundFactory;
	int soundStaticBuffers;
	int soundStreamingBuffers;
	int soundVolumeAmbient;
	int soundVolumeFx;
	int soundVolumeMusic;
	int uiConsoleMaxLines;
	int uiConsoleTimeout;
	bool uiFocusArrows;
	string uiLastScenarioCatagory;
	string uiLastScenario;
	string uiLocale;
	bool uiPhotoMode;
	float uiScrollSpeed;

	Config(const char* fileName);

	static bool isWindows() {
#if defined(WIN32) || defined(WIN64)
		return true;
#else
		return false;
#endif
	}

public:
	static Config &getInstance() {
		static Config singleton("glestadv.ini");
		return singleton;
	}

	void save(const char *path = "glestadv.ini");

	float getCameraFov() const					{return cameraFov;}
	bool getCameraInvertXAxis() const			{return cameraInvertXAxis;}
	bool getCameraInvertYAxis() const			{return cameraInvertYAxis;}
	float getCameraMaxDistance() const			{return cameraMaxDistance;}
	float getCameraMaxYaw() const				{return cameraMaxYaw;}
	float getCameraMinDistance() const			{return cameraMinDistance;}
	float getCameraMinYaw() const				{return cameraMinYaw;}
	int getDisplayHeight() const				{return displayHeight;}
	int getDisplayRefreshFrequency() const		{return displayRefreshFrequency;}
	int getDisplayWidth() const					{return displayWidth;}
	bool getDisplayWindowed() const				{return displayWindowed;}
	bool getGsAutoRepairEnabled() const			{return gsAutoRepairEnabled;}
	bool getGsAutoReturnEnabled() const			{return gsAutoReturnEnabled;}
	float getGsDayTime() const					{return gsDayTime;}
	int getGsWorldUpdateFps() const				{return gsWorldUpdateFps;}
	bool getMiscCatchExceptions() const			{return miscCatchExceptions;}
	bool getMiscDebugKeys() const				{return miscDebugKeys;}
	bool getMiscDebugMode() const				{return miscDebugMode;}
	bool getMiscFirstTime() const				{return miscFirstTime;}
	bool getNetConsistencyChecks() const		{return netConsistencyChecks;}
	string getNetPlayerName() const				{return netPlayerName;}
	string getNetServerIp() const				{return netServerIp;}
	int getNetServerPort() const				{return netServerPort;}
	bool getRenderCheckGlCaps() const			{return renderCheckGlCaps;}
	int getRenderColorBits() const				{return renderColorBits;}
	int getRenderDepthBits() const				{return renderDepthBits;}
	float getRenderDistanceMax() const			{return renderDistanceMax;}
	float getRenderDistanceMin() const			{return renderDistanceMin;}
	int getRenderFilterMaxAnisotropy() const	{return renderFilterMaxAnisotropy;}
	string getRenderFilter() const				{return renderFilter;}
	float getRenderFontScaler() const			{return renderFontScaler;}
	bool getRenderFogOfWarSmoothing() const		{return renderFogOfWarSmoothing;}
	int getRenderFogOfWarSmoothingFrameSkip() const{return renderFogOfWarSmoothingFrameSkip;}
	float getRenderFov() const					{return renderFov;}
	int getRenderFpsMax() const					{return renderFpsMax;}
	string getRenderGraphicsFactory() const		{return renderGraphicsFactory;}
	bool getRenderInterpolateWithSIMD() const	{return renderInterpolateWithSIMD;}
	int getRenderLightsMax() const				{return renderLightsMax;}
	float getRenderShadowAlpha() const			{return renderShadowAlpha;}
	int getRenderShadowFrameSkip() const		{return renderShadowFrameSkip;}
	string getRenderShadows() const				{return renderShadows;}
	int getRenderShadowTextureSize() const		{return renderShadowTextureSize;}
	int getRenderStencilBits() const			{return renderStencilBits;}
	bool getRenderTextures3D() const			{return renderTextures3D;}
	bool getRenderUseShaders() const			{return renderUseShaders;}
	bool getRenderUseVBOs() const               {return renderUseVBOs;}
	string getRenderUnitShaders() const			{return renderUnitShaders;}
	string getSoundFactory() const				{return soundFactory;}
	int getSoundStaticBuffers() const			{return soundStaticBuffers;}
	int getSoundStreamingBuffers() const		{return soundStreamingBuffers;}
	int getSoundVolumeAmbient() const			{return soundVolumeAmbient;}
	int getSoundVolumeFx() const				{return soundVolumeFx;}
	int getSoundVolumeMusic() const				{return soundVolumeMusic;}
	int getUiConsoleMaxLines() const			{return uiConsoleMaxLines;}
	int getUiConsoleTimeout() const				{return uiConsoleTimeout;}
	bool getUiFocusArrows() const				{return uiFocusArrows;}
	string getUiLastScenarioCatagory() const	{return uiLastScenarioCatagory;}
	string getUiLastScenario() const			{return uiLastScenario;}
	string getUiLocale() const					{return uiLocale;}
	bool getUiPhotoMode() const					{return uiPhotoMode;}
	float getUiScrollSpeed() const				{return uiScrollSpeed;}

	void setCameraFov(float val)				{cameraFov = val;}
	void setCameraInvertXAxis(bool val)			{cameraInvertXAxis = val;}
	void setCameraInvertYAxis(bool val)			{cameraInvertYAxis = val;}
	void setCameraMaxDistance(float val)		{cameraMaxDistance = val;}
	void setCameraMaxYaw(float val)				{cameraMaxYaw = val;}
	void setCameraMinDistance(float val)		{cameraMinDistance = val;}
	void setCameraMinYaw(float val)				{cameraMinYaw = val;}
	void setDisplayHeight(int val)				{displayHeight = val;}
	void setDisplayRefreshFrequency(int val)	{displayRefreshFrequency = val;}
	void setDisplayWidth(int val)				{displayWidth = val;}
	void setDisplayWindowed(bool val)			{displayWindowed = val;}
	void setGsAutoRepairEnabled(bool val)		{gsAutoRepairEnabled = val;}
	void setGsAutoReturnEnabled(bool val)		{gsAutoReturnEnabled = val;}
	void setGsDayTime(float val)				{gsDayTime = val;}
	void setGsWorldUpdateFps(int val)			{gsWorldUpdateFps = val;}
	void setMiscCatchExceptions(bool val)		{miscCatchExceptions = val;}
	void setMiscDebugKeys(bool val)				{miscDebugKeys = val;}
	void setMiscDebugMode(bool val)				{miscDebugMode = val;}
	void setMiscFirstTime(bool val)				{miscFirstTime = val;}
	void setNetConsistencyChecks(bool val)		{netConsistencyChecks = val;}
	void setNetPlayerName(string val)			{netPlayerName = val;}
	void setNetServerIp(string val)				{netServerIp = val;}
	void setNetServerPort(int val)				{netServerPort = val;}
	void setRenderCheckGlCaps(bool val)			{renderCheckGlCaps = val;}
	void setRenderColorBits(int val)			{renderColorBits = val;}
	void setRenderDepthBits(int val)			{renderDepthBits = val;}
	void setRenderDistanceMax(float val)		{renderDistanceMax = val;}
	void setRenderDistanceMin(float val)		{renderDistanceMin = val;}
	void setRenderFilterMaxAnisotropy(int val)	{renderFilterMaxAnisotropy = val;}
	void setRenderFilter(string val)			{renderFilter = val;}
	void setRenderFontScaler(float val)			{renderFontScaler = val;}
	void setRenderFogOfWarSmoothing(bool val)	{renderFogOfWarSmoothing = val;}
	void setRenderFogOfWarSmoothingFrameSkip(int val){renderFogOfWarSmoothingFrameSkip = val;}
	void setRenderFov(float val)				{renderFov = val;}
	void setRenderFpsMax(int val)				{renderFpsMax = val;}
	void setRenderGraphicsFactory(string val)	{renderGraphicsFactory = val;}
	void setRenderInterpolateWithSIMD(bool val)	{renderInterpolateWithSIMD = val;}
	void setRenderLightsMax(int val)			{renderLightsMax = val;}
	void setRenderShadowAlpha(float val)		{renderShadowAlpha = val;}
	void setRenderShadowFrameSkip(int val)		{renderShadowFrameSkip = val;}
	void setRenderShadows(string val)			{renderShadows = val;}
	void setRenderShadowTextureSize(int val)	{renderShadowTextureSize = val;}
	void setRenderStencilBits(int val)			{renderStencilBits = val;}
	void setRenderTextures3D(bool val)			{renderTextures3D = val;}
	void setRenderUseShaders(bool val)			{renderUseShaders = val;}
	void setRenderUseVBOs(bool val)             {renderUseVBOs = val;}
	void setRenderUnitShaders(string val)		{renderUnitShaders = val;}
	void setSoundFactory(string val)			{soundFactory = val;}
	void setSoundStaticBuffers(int val)			{soundStaticBuffers = val;}
	void setSoundStreamingBuffers(int val)		{soundStreamingBuffers = val;}
	void setSoundVolumeAmbient(int val)			{soundVolumeAmbient = val;}
	void setSoundVolumeFx(int val)				{soundVolumeFx = val;}
	void setSoundVolumeMusic(int val)			{soundVolumeMusic = val;}
	void setUiConsoleMaxLines(int val)			{uiConsoleMaxLines = val;}
	void setUiConsoleTimeout(int val)			{uiConsoleTimeout = val;}
	void setUiFocusArrows(bool val)				{uiFocusArrows = val;}
	void setUiLastScenarioCatagory(string val)	{uiLastScenarioCatagory = val;}
	void setUiLastScenario(string val)			{uiLastScenario = val;}
	void setUiLocale(string val)				{uiLocale = val;}
	void setUiPhotoMode(bool val)				{uiPhotoMode = val;}
	void setUiScrollSpeed(float val)			{uiScrollSpeed = val;}
};

}}//end namespace

#endif



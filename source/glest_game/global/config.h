// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_CONFIG_H_
#define _GLEST_GAME_CONFIG_H_

#include "properties.h"

namespace Glest{ namespace Game{

using Shared::Util::Properties;

// =====================================================
// 	class Config
//
//	Game configuration
// =====================================================

class Config : protected Properties {
private:
	string fileName;

	int aiLog;
	bool aiRedir;
	float cameraFOV;
	bool cameraInvertXAxis;
	bool cameraInvertYAxis;
	float cameraMaxDistance;
	float cameraMaxYaw;
	float cameraMinDistance;
	float cameraMinYaw;
	bool catchExceptions;
	bool checkGlCaps;
	int colorBits;
	int consoleMaxLines;
	int consoleTimeout;
	float dayTime;
	bool debugMode;
	int depthBits;
	bool enableCommandMinimap;
	string factoryGraphics;
	string factorySound;
	float fastestSpeed;
	string filter;
	int filterMaxAnisotropy;
	bool firstTime;
	bool focusArrows;
	bool fogOfWar;
	bool fogOfWarSmoothing;
	int fogOfWarSmoothingFrameSkip;
	string fontConsole;
	string fontDisplay;
	string fontMenu;
	string lang;
	int maxFPS;
	int maxLights;
	float maxRenderDistance;
	bool multiplayerPauseAllowed;
	bool networkConsistencyChecks;
	int networkFPS;
	bool photoMode;
	bool randStartLocs;
	int refreshFrequency;
	int screenHeight;
	int screenWidth;
	float scrollSpeed;
	string serverIp;
	float shadowAlpha;
	int shadowFrameSkip;
	int shadowTextureSize;
	string shadows;
	float slowestSpeed;
	int soundStaticBuffers;
	int soundStreamingBuffers;
	int soundVolumeAmbient;
	int soundVolumeFx;
	int soundVolumeMusic;
	int stencilBits;
	bool textures3D;
	bool windowed;
	int worldUpdateFPS;

	Config(const char* fileName);

public:
	static Config &getInstance() {
		static Config singleton("glestadv.ini");
		return singleton;
	}

	void save(const char *path = "glestadv.ini");
	string toString()							{return Properties::toString();}

	int getAiLog() const						{return aiLog;}
	bool getAiRedir() const						{return aiRedir;}
	float getCameraFOV() const					{return cameraFOV;}
	bool getCameraInvertXAxis() const			{return cameraInvertXAxis;}
	bool getCameraInvertYAxis() const			{return cameraInvertYAxis;}
	float getCameraMaxDistance() const			{return cameraMaxDistance;}
	float getCameraMaxYaw() const				{return cameraMaxYaw;}
	float getCameraMinDistance() const			{return cameraMinDistance;}
	float getCameraMinYaw() const				{return cameraMinYaw;}
	bool getCatchExceptions() const				{return catchExceptions;}
	bool getCheckGlCaps() const					{return checkGlCaps;}
	int getColorBits() const					{return colorBits;}
	int getConsoleMaxLines() const				{return consoleMaxLines;}
	int getConsoleTimeout() const				{return consoleTimeout;}
	float getDayTime() const					{return dayTime;}
	bool getDebugMode() const					{return debugMode;}
	int getDepthBits() const					{return depthBits;}
	bool getEnableCommandMinimap() const		{return enableCommandMinimap;}
	string getFactoryGraphics() const			{return factoryGraphics;}
	string getFactorySound() const				{return factorySound;}
	float getFastestSpeed() const				{return fastestSpeed;}
	string getFilter() const					{return filter;}
	int getFilterMaxAnisotropy() const			{return filterMaxAnisotropy;}
	bool getFirstTime() const					{return firstTime;}
	bool getFocusArrows() const					{return focusArrows;}
	bool getFogOfWar() const					{return fogOfWar;}
	bool getFogOfWarSmoothing() const			{return fogOfWarSmoothing;}
	int getFogOfWarSmoothingFrameSkip() const	{return fogOfWarSmoothingFrameSkip;}
	string getFontConsole() const				{return fontConsole;}
	string getFontDisplay() const				{return fontDisplay;}
	string getFontMenu() const					{return fontMenu;}
	string getLang() const						{return lang;}
	int getMaxFPS() const						{return maxFPS;}
	int getMaxLights() const					{return maxLights;}
	float getMaxRenderDistance() const			{return maxRenderDistance;}
	bool getMultiplayerPauseAllowed() const		{return multiplayerPauseAllowed;}
	bool getNetworkConsistencyChecks() const	{return networkConsistencyChecks;}
	int getNetworkFPS() const					{return networkFPS;}
	bool getPhotoMode() const					{return photoMode;}
	bool getRandStartLocs() const				{return randStartLocs;}
	int getRefreshFrequency() const				{return refreshFrequency;}
	int getScreenHeight() const					{return screenHeight;}
	int getScreenWidth() const					{return screenWidth;}
	float getScrollSpeed() const				{return scrollSpeed;}
	string getServerIp() const					{return serverIp;}
	float getShadowAlpha() const				{return shadowAlpha;}
	int getShadowFrameSkip() const				{return shadowFrameSkip;}
	int getShadowTextureSize() const			{return shadowTextureSize;}
	string getShadows() const					{return shadows;}
	float getSlowestSpeed() const				{return slowestSpeed;}
	int getSoundStaticBuffers() const			{return soundStaticBuffers;}
	int getSoundStreamingBuffers() const		{return soundStreamingBuffers;}
	int getSoundVolumeAmbient() const			{return soundVolumeAmbient;}
	int getSoundVolumeFx() const				{return soundVolumeFx;}
	int getSoundVolumeMusic() const				{return soundVolumeMusic;}
	int getStencilBits() const					{return stencilBits;}
	bool getTextures3D() const					{return textures3D;}
	bool getWindowed() const					{return windowed;}
	int getWorldUpdateFPS() const				{return worldUpdateFPS;}

	void setAiLog(int val)						{aiLog = val;}
	void setAiRedir(bool val)					{aiRedir = val;}
	void setCameraFOV(float val)				{cameraFOV = val;}
	void setCameraInvertXAxis(bool val)			{cameraInvertXAxis = val;}
	void setCameraInvertYAxis(bool val)			{cameraInvertYAxis = val;}
	void setCameraMaxDistance(float val)		{cameraMaxDistance = val;}
	void setCameraMaxYaw(float val)				{cameraMaxYaw = val;}
	void setCameraMinDistance(float val)		{cameraMinDistance = val;}
	void setCameraMinYaw(float val)				{cameraMinYaw = val;}
	void setCatchExceptions(bool val)			{catchExceptions = val;}
	void setCheckGlCaps(bool val)				{checkGlCaps = val;}
	void setColorBits(int val)					{colorBits = val;}
	void setConsoleMaxLines(int val)			{consoleMaxLines = val;}
	void setConsoleTimeout(int val)				{consoleTimeout = val;}
	void setDayTime(float val)					{dayTime = val;}
	void setDebugMode(bool val)					{debugMode = val;}
	void setDepthBits(int val)					{depthBits = val;}
	void setEnableCommandMinimap(bool val)		{enableCommandMinimap = val;}
	void setFactoryGraphics(string val)			{factoryGraphics = val;}
	void setFactorySound(string val)			{factorySound = val;}
	void setFastestSpeed(float val)				{fastestSpeed = val;}
	void setFilter(string val)					{filter = val;}
	void setFilterMaxAnisotropy(int val)		{filterMaxAnisotropy = val;}
	void setFirstTime(bool val)					{firstTime = val;}
	void setFocusArrows(bool val)				{focusArrows = val;}
	void setFogOfWar(bool val)					{fogOfWar = val;}
	void setFogOfWarSmoothing(bool val)			{fogOfWarSmoothing = val;}
	void setFogOfWarSmoothingFrameSkip(int val)	{fogOfWarSmoothingFrameSkip = val;}
	void setFontConsole(string val)				{fontConsole = val;}
	void setFontDisplay(string val)				{fontDisplay = val;}
	void setFontMenu(string val)				{fontMenu = val;}
	void setLang(string val)					{lang = val;}
	void setMaxFPS(int val)						{maxFPS = val;}
	void setMaxLights(int val)					{maxLights = val;}
	void setMaxRenderDistance(float val)		{maxRenderDistance = val;}
	void setMultiplayerPauseAllowed(bool val)	{multiplayerPauseAllowed = val;}
	void setNetworkConsistencyChecks(bool val)	{networkConsistencyChecks = val;}
	void setNetworkFPS(int val)					{networkFPS = val;}
	void setPhotoMode(bool val)					{photoMode = val;}
	void setRandStartLocs(bool val)				{randStartLocs = val;}
	void setRefreshFrequency(int val)			{refreshFrequency = val;}
	void setScreenHeight(int val)				{screenHeight = val;}
	void setScreenWidth(int val)				{screenWidth = val;}
	void setScrollSpeed(float val)				{scrollSpeed = val;}
	void setServerIp(string val)				{serverIp = val;}
	void setShadowAlpha(float val)				{shadowAlpha = val;}
	void setShadowFrameSkip(int val)			{shadowFrameSkip = val;}
	void setShadowTextureSize(int val)			{shadowTextureSize = val;}
	void setShadows(string val)					{shadows = val;}
	void setSlowestSpeed(float val)				{slowestSpeed = val;}
	void setSoundStaticBuffers(int val)			{soundStaticBuffers = val;}
	void setSoundStreamingBuffers(int val)		{soundStreamingBuffers = val;}
	void setSoundVolumeAmbient(int val)			{soundVolumeAmbient = val;}
	void setSoundVolumeFx(int val)				{soundVolumeFx = val;}
	void setSoundVolumeMusic(int val)			{soundVolumeMusic = val;}
	void setStencilBits(int val)				{stencilBits = val;}
	void setTextures3D(bool val)				{textures3D = val;}
	void setWindowed(bool val)					{windowed = val;}
	void setWorldUpdateFPS(int val)				{worldUpdateFPS = val;}

};

}}//end namespace

#endif



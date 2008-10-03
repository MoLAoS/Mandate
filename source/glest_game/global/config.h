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
	int fastSpeedLoops;
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
	float maxCameraDistance;
	int maxFPS;
	int maxLights;
	float maxRenderDistance;
	float minCameraDistance;
	bool networkConsistencyChecks;
	bool photoMode;
//	string platform;
	bool randStartLocs;
	int refreshFrequency;
	int screenHeight;
	int screenWidth;
	float scrollSpeed;
	string serverIp;
//	int serverPort;
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


private:
	Config(const char* fileName);

public:
	static Config &getInstance(){
		static Config singleton("glestadv.ini");
		return singleton;
	}

	void save(const char *path = "glestadv.ini");
	string toString()							{return Properties::toString();}

	int getAiLog() const						{return aiLog;}
	bool getAiRedir() const						{return aiRedir;}
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
	int getFastSpeedLoops() const				{return fastSpeedLoops;}
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
	float getMaxCameraDistance() const			{return maxCameraDistance;}
	int getMaxFPS() const						{return maxFPS;}
	int getMaxLights() const					{return maxLights;}
	float getMaxRenderDistance() const			{return maxRenderDistance;}
	float getMinCameraDistance() const			{return minCameraDistance;}
	bool getNetworkConsistencyChecks() const	{return networkConsistencyChecks;}
//	string getPlatform() const					{return platform;}
	bool getPhotoMode() const					{return photoMode;}
	bool getRandStartLocs() const				{return randStartLocs;}
	int getRefreshFrequency() const				{return refreshFrequency;}
	int getScreenHeight() const					{return screenHeight;}
	int getScreenWidth() const					{return screenWidth;}
	float getScrollSpeed() const				{return scrollSpeed;}
	string getServerIp() const					{return serverIp;}
//	int getServerPort() const					{return serverPort;}
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

	void setAiLog(int val)						{aiLog = val;}
	void setAiRedir(bool val)					{aiRedir = val;}
	void setCheckGlCaps(bool val)				{checkGlCaps = val;}
	void setColorBits(int val)					{colorBits = val;}
	void setConsoleMaxLines(int val)			{consoleMaxLines = val;}
	void setConsoleTimeout(int val)				{consoleTimeout = val;}
	void setDayTime(float val)					{dayTime = val;}
	void setDebugMode(bool val)					{debugMode = val;}
	void setDepthBits(int val)					{depthBits = val;}
	void setFactoryGraphics(string val)			{factoryGraphics = val;}
	void setFactorySound(string val)			{factorySound = val;}
	void setFastestSpeed(float val)				{fastestSpeed = val;}
	void setFastSpeedLoops(int val)				{fastSpeedLoops = val;}
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
	void setMaxCameraDistance(float val)		{maxCameraDistance = val;}
	void setMaxLights(int val)					{maxLights = val;}
	void setMaxRenderDistance(float val)		{maxRenderDistance = val;}
	void setMinCameraDistance(float val)		{maxCameraDistance = val;}
	void setNetworkConsistencyChecks(bool val)	{networkConsistencyChecks = val;}
//	void setPlatform(string val)				{platform = val;}
	void setPhotoMode(bool val)					{photoMode = val;}
	void setRandStartLocs(bool val)				{randStartLocs = val;}
	void setRefreshFrequency(int val)			{refreshFrequency = val;}
	void setScreenHeight(int val)				{screenHeight = val;}
	void setScreenWidth(int val)				{screenWidth = val;}
	void setScrollSpeed(float val)				{scrollSpeed = val;}
	void setServerIp(string val)				{serverIp = val;}
//	void setServerPort(int val)					{serverPort = val;}
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
};

}}//end namespace

#endif



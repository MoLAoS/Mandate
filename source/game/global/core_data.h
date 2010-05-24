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

#ifndef _GLEST_GAME_COREDATA_H_
#define _GLEST_GAME_COREDATA_H_

#include <string>

#include "sound.h"
#include "font.h"
#include "texture.h"
#include "sound_container.h"

using namespace Glest::Sound;

namespace Glest { namespace Global {

using Shared::Graphics::Texture2D;
using Shared::Graphics::Texture3D;
using Shared::Graphics::Font;
using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;

// =====================================================
// 	class CoreData  
//
/// Data shared ammont all the ProgramStates
// =====================================================

class CoreData{
private:
    StrSound introMusic;
    StrSound menuMusic;
	StaticSound clickSoundA;
    StaticSound clickSoundB;
    StaticSound clickSoundC;
	SoundContainer waterSounds;
	
	Texture2D *logoTexture;
	Texture2D *gplTexture;
    Texture2D *backgroundTexture;
    Texture2D *fireTexture;
    Texture2D *snowTexture;
	Texture2D *waterSplashTexture;
    Texture2D *customTexture;
	Texture2D *buttonSmallTexture;
	Texture2D *buttonBigTexture;
	Texture2D *textEntryTexture;
	Texture2D *checkBoxTickTexture;
	Texture2D *checkBoxCrossTexture;

    Font *displayFont;
	Font *menuFontNormal;
	Font *menuFontSmall;
	Font *menuFontBig;
	Font *menuFontVeryBig;
	Font *consoleFont;

	Font *freeTypeFont;
	Font *advancedEngineFont;
	Font *freeTypeMenuFont;
	
public:
	static CoreData &getInstance();
	~CoreData();

    void load();
	void closeSounds();

	Texture2D *getBackgroundTexture() const		{return backgroundTexture;}
	Texture2D *getFireTexture() const			{return fireTexture;}
	Texture2D *getSnowTexture() const			{return snowTexture;}
	Texture2D *getLogoTexture() const			{return logoTexture;}
	Texture2D *getGplTexture() const			{return gplTexture;}
	Texture2D *getWaterSplashTexture() const	{return waterSplashTexture;}
	Texture2D *getCustomTexture() const			{return customTexture;}
	Texture2D *getButtonSmallTexture() const	{return buttonSmallTexture;}
	Texture2D *getButtonBigTexture() const		{return buttonBigTexture;}
	Texture2D *getTextEntryTexture() const		{return textEntryTexture;}
	
	Texture2D *getCheckBoxTickTexture() const	{return checkBoxTickTexture;}
	Texture2D *getCheckBoxCrossTexture() const	{return checkBoxCrossTexture;}

	StrSound *getIntroMusic() 				{return &introMusic;}
	StrSound *getMenuMusic() 				{return &menuMusic;}
    StaticSound *getClickSoundA()			{return &clickSoundA;}
    StaticSound *getClickSoundB()			{return &clickSoundB;}
    StaticSound *getClickSoundC()			{return &clickSoundC;}
	StaticSound *getWaterSound()			{return waterSounds.getRandSound();}

	Font *getDisplayFont() const			{return displayFont;}
    Font *getMenuFontSmall() const		{return menuFontSmall;}
    Font *getMenuFontNormal() const		{return menuFontNormal;}
    Font *getMenuFontBig() const			{return menuFontBig;}
	Font *getMenuFontVeryBig() const		{return menuFontVeryBig;}
    Font *getConsoleFont() const			{return consoleFont;}
	Font *getFreeTypeFont() const			{return freeTypeFont;}
	Font *getAdvancedEngineFont() const	{return advancedEngineFont;}
	Font *getfreeTypeMenuFont() const		{return freeTypeMenuFont;}

private:
	CoreData(){};

	int computeFontSize(int size);
};

}} //end namespace

#endif

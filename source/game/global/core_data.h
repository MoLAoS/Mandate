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
	
	typedef Texture2D* TexPtr;

	TexPtr	logoTexture,
			gplTexture,
			backgroundTexture,
			fireTexture,
			snowTexture,
			waterSplashTexture,
			customTexture,
			buttonSmallTexture,
			buttonBigTexture,
			textEntryTexture,
			checkBoxTickTexture,
			checkBoxCrossTexture,
			vertScrollUpTexture,
			vertScrollDownTexture,
			vertScrollUpHoverTex,
			vertScrollDownHoverTex;
	
/*
    Font *displayFont;
	Font *menuFontNormal;
	Font *menuFontSmall;
	Font *menuFontBig;
	Font *menuFontVeryBig;
	Font *consoleFont;
*/
	Font *m_FTMenuFontNormal;
	Font *m_FTMenuFontSmall;
	Font *m_FTMenuFontBig;
	Font *m_FTMenuFontVeryBig;

	Font *advancedEngineFontSmall;
	Font *advancedEngineFontBig;
	//Font *freeTypeMenuFont;

	Font *m_FTDisplay;
	
public:
	static CoreData &getInstance();
	~CoreData();

	bool load();
	void closeSounds();

	TexPtr getBackgroundTexture() const		{return backgroundTexture;}
	TexPtr getFireTexture() const			{return fireTexture;}
	TexPtr getSnowTexture() const			{return snowTexture;}
	TexPtr getLogoTexture() const			{return logoTexture;}
	TexPtr getGplTexture() const			{return gplTexture;}
	TexPtr getWaterSplashTexture() const	{return waterSplashTexture;}
	TexPtr getCustomTexture() const			{return customTexture;}
	TexPtr getButtonSmallTexture() const	{return buttonSmallTexture;}
	TexPtr getButtonBigTexture() const		{return buttonBigTexture;}
	TexPtr getTextEntryTexture() const		{return textEntryTexture;}
	TexPtr getVertScrollUpTexture() const	{return vertScrollUpTexture;}
	TexPtr getVertScrollDownTexture() const {return vertScrollDownTexture;}
	TexPtr getVertScrollUpHoverTex() const	{return vertScrollUpHoverTex;}
	TexPtr getVertScrollDownHoverTex() const{return vertScrollDownHoverTex;}
	TexPtr getCheckBoxTickTexture() const	{return checkBoxTickTexture;}
	TexPtr getCheckBoxCrossTexture() const	{return checkBoxCrossTexture;}

	StrSound *getIntroMusic() 				{return &introMusic;}
	StrSound *getMenuMusic() 				{return &menuMusic;}
    StaticSound *getClickSoundA()			{return &clickSoundA;}
    StaticSound *getClickSoundB()			{return &clickSoundB;}
    StaticSound *getClickSoundC()			{return &clickSoundC;}
	StaticSound *getWaterSound()			{return waterSounds.getRandSound();}

	Font *getGAEFontBig() const			{ return advancedEngineFontBig;	 }
	Font *getGAEFontSmall() const		{ return advancedEngineFontSmall;}

	Font* getFTMenuFontNormal() const	{ return m_FTMenuFontNormal;	}
	Font* getFTMenuFontSmall() const	{ return m_FTMenuFontSmall;		}
	Font* getFTMenuFontBig() const		{ return m_FTMenuFontBig;		}
	Font* getFTMenuFontVeryBig() const	{ return m_FTMenuFontVeryBig;	}

	Font* getFTDisplayFont() const		{ return m_FTDisplay;	}

private:
	CoreData(){};

	int computeFontSize(int size);
};

}} //end namespace

#endif

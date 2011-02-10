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

WRAPPED_ENUM( FontSize, SMALL, NORMAL, BIG, HUGE );

struct FontSet {
	Font *m_fonts[FontSize::COUNT];

	FontSet() {memset(this, 0, sizeof(*this));}
	Font* operator[](FontSize size) const {return m_fonts[size];}
	void load(const string &path, int size);
};

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
			gaeSplashTexture,
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
			vertScrollDownHoverTex,
			greenTickOverlay,
			orangeQuestionOverlay,
			redCrossOverlay,
			mouseTexture;
	
	FontSet m_menuFont;
	FontSet m_gameFont;
	FontSet m_fancyFont;
	
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
	TexPtr getGaeSplashTexture() const		{return gaeSplashTexture;}
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
	TexPtr getGreenTickOverlay() const		{return greenTickOverlay;}
	TexPtr getQuestionOverlay() const		{return orangeQuestionOverlay;}
	TexPtr getRedCrossOverlay() const		{return redCrossOverlay;}
	TexPtr getMouseTexture() const			{return mouseTexture;}

	StrSound *getIntroMusic() 				{return &introMusic;}
	StrSound *getMenuMusic() 				{return &menuMusic;}
    StaticSound *getClickSoundA()			{return &clickSoundA;}
    StaticSound *getClickSoundB()			{return &clickSoundB;}
    StaticSound *getClickSoundC()			{return &clickSoundC;}
	StaticSound *getWaterSound()			{return waterSounds.getRandSound();}

	Font *getGAEFontBig() const	        { return m_fancyFont[FontSize::BIG];    }
	Font *getGAEFontSmall() const       { return m_fancyFont[FontSize::NORMAL]; }

	Font* getFTMenuFontNormal() const   { return m_menuFont[FontSize::NORMAL];  }
	Font* getFTMenuFontSmall() const    { return m_menuFont[FontSize::SMALL];   }
	Font* getFTMenuFontBig() const      { return m_menuFont[FontSize::BIG];     }
	Font* getFTMenuFontVeryBig() const  { return m_menuFont[FontSize::HUGE];    }

	Font* getFTDisplayFont() const      { return m_gameFont[FontSize::NORMAL];  }
	Font* getFTDisplayFontBig() const   { return m_gameFont[FontSize::BIG];     }

private:
	CoreData(){};

	int computeFontSize(int size);
};

}} //end namespace

#endif

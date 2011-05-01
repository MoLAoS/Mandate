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
/// Data shared amongst all the ProgramStates
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
			mouseTexture;
		
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
	TexPtr getMouseTexture() const			{return mouseTexture;}

	StrSound *getIntroMusic() 				{return &introMusic;}
	StrSound *getMenuMusic() 				{return &menuMusic;}
    StaticSound *getClickSoundA()			{return &clickSoundA;}
    StaticSound *getClickSoundB()			{return &clickSoundB;}
    StaticSound *getClickSoundC()			{return &clickSoundC;}
	StaticSound *getWaterSound()			{return waterSounds.getRandSound();}

private:
	CoreData(){};
};

}} //end namespace

#endif

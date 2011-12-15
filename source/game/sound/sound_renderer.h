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

#ifndef _GLEST_GAME_SOUNDRENDERER_H_
#define _GLEST_GAME_SOUNDRENDERER_H_

#include "sound.h"
#include "sound_player.h"
#include "window.h"
#include "vec.h"
#include "music_playlist_type.h"


#include <set>

namespace Glest { namespace Sound {


using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;
using Shared::Sound::SoundPlayer;
using Shared::Sound::RequestNextStream;
using Shared::Math::Vec3f;
using Glest::ProtoTypes::MusicPlaylistType;


// =====================================================
// 	class SoundRenderer
//
///	Wrapper to acces the shared library sound engine
// =====================================================

class SoundRenderer{
public:
	static const int ambientFade;
	static const float audibleDist;
private:
	SoundPlayer *soundPlayer;

	//volume
	float fxVolume;
	float musicVolume;
	float ambientVolume;


	StrSound *musicStream;
	std::set<StrSound*> ambientStreams;
    typedef std::vector<const MusicPlaylistType*> vectorMusicPlaylist;
    struct stMusic {
        vectorMusicPlaylist musicList;
        MusicPlaylistType *curPlaylist;

        stMusic() {
            curPlaylist=0;
        }
    };
    typedef std::vector<stMusic> MusicPlaylistQueue;
    MusicPlaylistQueue musicPlaylistQueue;

private:
	SoundRenderer();

public:
	//misc
	~SoundRenderer();
	static SoundRenderer &getInstance();
	void init(Window *window);
	void update();
	SoundPlayer *getSoundPlayer() const	{return soundPlayer;}

    // streams
	void playStream(StrSound *strSound, bool loop= true, int64 fadeOn=0, RequestNextStream cbFunc=0);
	void stopStream(StrSound *strSound, int64 fadeOff=0);

	//music
	void playMusic(StrSound *strSound, bool loop= true, RequestNextStream cbFunc=0);
	void stopMusic(StrSound *strSound);
    void addPlaylist(const MusicPlaylistType *playlist);
    void startMusicPlaylist();
    StrSound *getNextMusicTrack();
    void removePlaylist(const MusicPlaylistType *playlist);

	//fx
	void playFx(StaticSound *staticSound, Vec3f soundPos, Vec3f camPos);
	void playFx(StaticSound *staticSound);

	//ambient
	//void playAmbient(StaticSound *staticSound);
	void playAmbient(StrSound *strSound, bool loop = true);
	void stopAmbient(StrSound *strSound);
	
	//volume
	float getFxVolume() const { return fxVolume; }
	float getMusicVolume() const { return musicVolume; }
	float getAmbientVolume() const { return ambientVolume; }

	void setFxVolume(float v);
	void setMusicVolume(float v);
	void setAmbientVolume(float v);

	//misc
	void stopAllSounds();
	void loadConfig();
};

}}//end namespace

#endif

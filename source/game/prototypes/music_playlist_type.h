// ==============================================================
//	This file is part of Glest Advanced Engine
//
//	GPL V2. see source/liscence.txt
// ==============================================================

#ifndef _GLEST_GAME_MUSICPLAYLIST_H_
#define _GLEST_GAME_MUSICPLAYLIST_H_


namespace Shared { 
    namespace Xml {
        class XmlNode;
    }

    namespace Sound {
        class StrSound; 
    }
}

using namespace Shared::Xml;
using Shared::Sound::StrSound;


namespace Glest { namespace ProtoTypes {

// ===============================
// 	class MusicPlaylistType
// ===============================

/** A list of music files to play when in a game. Music files can be played in sequence or randomly. */
class MusicPlaylistType {
public:

    enum eActivationType {
        eActivationType_AddToCurrent= 0,
        eActivationType_ReplaceCurrent
    };

    enum eActivationMode {
        eActivationMode_Wait= 0,
        eActivationMode_Interrupt,
    };

	typedef vector<StrSound*> MusicTracks;

private:
	MusicTracks         m_tracks;
    bool                m_bRandom;
    bool                m_bLooping;
    eActivationType     m_activationType;
    eActivationMode     m_activation;

    int                 m_curTrackIndex;

public:
	MusicPlaylistType();
	~MusicPlaylistType();

    virtual void preload( const XmlNode *pXMLNode, const string &dir );

    void setRandom(bool random) { m_bRandom= random; }
    bool isRandom() const { return m_bRandom; }

    void setLoop(bool loop) { m_bLooping= loop; }
    bool isLooping() const { return m_bLooping; }

    void setActivationType(eActivationType actType) { m_activationType= actType; }
    eActivationType getActivationType() const { return m_activationType; }

    void setActivationMode(eActivationMode actMode) { m_activation= actMode; }
    eActivationMode getActivationMode() const { return m_activation; }

    MusicTracks& getTrackList() { return m_tracks; }
	void addTrack(const std::string &path);
    bool empty() const { return m_tracks.empty(); }
    StrSound* getNextTrack();
};

// ===============================
// 	class UnitMusicPlaylistType
// ===============================

/** A list of music files to play related to a unit. Music files can be played in sequence or randomly. 
    I still strongly believe this shouldn't be done in the engine but handled through the scripting
    solution (lua or other, when available) **/
class UnitMusicPlaylistType: public MusicPlaylistType {
public:

    enum eActivationTime {
        eActivationTime_UnitBuildStarted= 0,
        eActivationTime_UnitCreated
    };

private:
    eActivationTime     m_activationTime;

public:
	UnitMusicPlaylistType();

    virtual void preload( const XmlNode *pXMLNode, const string &dir );

    void setActivationTime(eActivationTime actTime) { m_activationTime= actTime; }
    eActivationTime getActivationTime() const { return m_activationTime; }
};

}} // namespace Glest::ProtoTypes

#endif

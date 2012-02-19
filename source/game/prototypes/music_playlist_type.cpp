// ==============================================================
//	This file is part of Glest Advanced Engine
//
//	GPL V2. see source/liscence.txt
// ==============================================================

#include "pch.h"
#include "music_playlist_type.h"

#include "xml_parser.h"
#include "sound.h"
#include "leak_dumper.h"
#include "logger.h"

using namespace Shared::Xml;
using namespace Shared::Sound;


namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class UpgradeType
// =====================================================

// ==================== misc ====================

MusicPlaylistType::MusicPlaylistType() 
    : m_bRandom(false)
    , m_bLooping(false)
    , m_activationType(eActivationType_AddToCurrent)
    , m_activation(eActivationMode_Wait)
    , m_curTrackIndex(-1) {
}

MusicPlaylistType::~MusicPlaylistType() {
	for (int i = 0; i < m_tracks.size(); ++i) {
        delete m_tracks[i];
	}
}

void MusicPlaylistType::preload( const XmlNode *pXMLNode, const string &dir ) {
    const XmlAttribute *randomAttrib = pXMLNode->getAttribute("randomize", false);
    m_bRandom = (randomAttrib && randomAttrib->getBoolValue() ? true : false);

    const XmlAttribute *loopingAttrib = pXMLNode->getAttribute("loop", false);
    m_bLooping = (loopingAttrib && loopingAttrib->getBoolValue() ? true : false);

    const XmlAttribute *actTypeAttrib = pXMLNode->getAttribute("activation-type", false);
    if(actTypeAttrib) {
        if(strcmp(actTypeAttrib->getRestrictedValue().c_str(), "add")== 0) {
            m_activationType= eActivationType_AddToCurrent;
        }
        else if(strcmp(actTypeAttrib->getRestrictedValue().c_str(), "replace")== 0) {
            m_activationType= eActivationType_ReplaceCurrent;
        }
        else {
		    throw runtime_error("Unsupported value specified for activation-type!");
        }
    }

    const XmlAttribute *actModeAttrib = pXMLNode->getAttribute("activation-mode", false);
    if(actModeAttrib) {
        if(strcmp(actModeAttrib->getRestrictedValue().c_str(), "wait")== 0) {
            m_activation= eActivationMode_Wait;
        }
        else if(strcmp(actModeAttrib->getRestrictedValue().c_str(), "interrupt")== 0) {
            m_activation= eActivationMode_Interrupt;
        }
        else {
		    throw runtime_error("Unsupported value specified for activation-mode!");
        }
    }

    for (int i=0; i < pXMLNode->getChildCount(); ++i) {
	    addTrack(dir + "/" + pXMLNode->getChild("music-file", i)->getAttribute("path")->getRestrictedValue());
    }
	if (m_tracks.empty()) {
		throw runtime_error("No tracks in play-list!");
	}
}

void MusicPlaylistType::addTrack(const std::string &path) {
	StrSound *sound = new StrSound();
	try	{
		sound->open(path);
		m_tracks.push_back(sound);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
		delete sound;
	}
}

StrSound* MusicPlaylistType::getNextTrack() {
	if (empty()) {
		m_curTrackIndex = -1;
	} else if (m_curTrackIndex == -1) {
        // playlist is starting up
        m_curTrackIndex = 0;
        if (m_bRandom) {
		    int seed = int(Chrono::getCurTicks());
		    Random random(seed);
		    m_curTrackIndex = random.randRange(0, m_tracks.size() - 1);
        }
    } else if (!m_bRandom) {
        // playlist in sequential order
        m_curTrackIndex++;
        if (m_curTrackIndex >= m_tracks.size()) {
			m_curTrackIndex = m_bLooping ? 0 : -1;
        }
    } else {
		// random playlist
        if (!m_bLooping) {
            m_curTrackIndex = -1;
        } else if (m_tracks.size() == 2) {
            m_curTrackIndex = !m_curTrackIndex;
        } else {
            int seed = int(Chrono::getCurTicks());
	        Random random(seed);

            int rndIndex = m_curTrackIndex;
            while (rndIndex == m_curTrackIndex) {
		        rndIndex = random.randRange(0, m_tracks.size() - 1);
            }
            m_curTrackIndex = rndIndex;
        }
    }

    return (m_curTrackIndex >= 0) ? m_tracks[m_curTrackIndex] : 0; 
}


UnitMusicPlaylistType::UnitMusicPlaylistType() 
    : MusicPlaylistType()
    , m_activationTime(eActivationTime_UnitCreated) {
}

void UnitMusicPlaylistType::preload( const XmlNode *pXMLNode, const string &dir )
{
    MusicPlaylistType::preload(pXMLNode, dir);

    const XmlAttribute *actTimeAttrib = pXMLNode->getAttribute("activation-time", false);
    if(actTimeAttrib) {
        if(strcmp(actTimeAttrib->getRestrictedValue().c_str(), "build")== 0) {
            m_activationTime= eActivationTime_UnitBuildStarted;
        }
        else if(strcmp(actTimeAttrib->getRestrictedValue().c_str(), "created")== 0) {
            m_activationTime= eActivationTime_UnitCreated;
        }
    }
}

}}//end namespace

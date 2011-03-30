// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _SHARED_SOUND_SOUNDFACTORYXA2_H_
#define _SHARED_SOUND_SOUNDFACTORYXA2_H_

#include "sound_factory.h"
#include "sound_player_xa2.h"

namespace Shared { namespace Sound { namespace Xa2 {

// =====================================================
//	class SoundFactoryXa2
// =====================================================

class SoundFactoryXa2 : public SoundFactory {
public:
	virtual SoundPlayer *newSoundPlayer()   { return new SoundPlayerXa2(); }
};

}}}//end namespace

#endif

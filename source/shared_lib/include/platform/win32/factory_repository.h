// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_FACTORYREPOSITORY_H_
#define _SHARED_PLATFORM_FACTORYREPOSITORY_H_

#include <string>

#include "graphics_factory.h"
#include "sound_factory.h"

#include "graphics_factory_gl.h"
#include "sound_factory_ds8.h"

#if _GAE_USE_XAUDIO2_
#	include "sound_factory_xa2.h"
	using Shared::Sound::Xa2::SoundFactoryXa2;
#endif

using std::string;

using Shared::Graphics::GraphicsFactory;
using Shared::Sound::SoundFactory;
using Shared::Graphics::Gl::GraphicsFactoryGl;
using Shared::Sound::Ds8::SoundFactoryDs8;

namespace Shared { namespace Platform {

// =====================================================
//	class FactoryRepository
// =====================================================

class FactoryRepository{
private:
	FactoryRepository(){};
	FactoryRepository(FactoryRepository &);
	void operator=(FactoryRepository &);

private:
	GraphicsFactoryGl graphicsFactoryGl;
	SoundFactoryDs8 soundFactoryDs8;
#	if _GAE_USE_XAUDIO2_
		SoundFactoryXa2 soundFactoryXa2;
#	endif

public:
	static FactoryRepository &getInstance();

	GraphicsFactory *getGraphicsFactory(const string &name);
	SoundFactory *getSoundFactory(const string &name);
};

}}//end namespace

#endif

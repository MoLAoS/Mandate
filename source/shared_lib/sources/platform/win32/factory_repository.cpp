// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "factory_repository.h"

#include "leak_dumper.h"

namespace Shared{ namespace Platform{

// =====================================================
//	class FactoryRepository
// =====================================================

FactoryRepository &FactoryRepository::getInstance(){
	static FactoryRepository factoryRepository;
	return factoryRepository;
}

GraphicsFactory *FactoryRepository::getGraphicsFactory(const string &name){
	if(name == "OpenGL"){
		return &graphicsFactoryGl;
	}

	throw runtime_error("Unknown graphics factory: " + name);
}

SoundFactory *FactoryRepository::getSoundFactory(const string &name){
	if (name == "DirectSound8") {
		return &soundFactoryDs8;

#	if _GAE_USE_XAUDIO2_

	} else if (name == "XAudio2") {
		return &soundFactoryXa2;

#	endif

	}
	throw runtime_error("Unknown sound factory: " + name);
}

}}//end namespace

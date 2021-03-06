// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SOUNDCONTAINER_H_
#define _GLEST_GAME_SOUNDCONTAINER_H_

#include <vector>

#include "sound.h"
#include "random.h"

using std::vector;
using Shared::Util::Random;
using Shared::Sound::StaticSound;

namespace Glest { namespace Sound {

// =====================================================
// 	class SoundContainer
//
/// Holds a list of sounds that are usually played at random
// =====================================================
typedef vector<StaticSound*> Sounds;

class SoundContainer{
private:
	Sounds sounds;
	mutable Random random;
	mutable int lastSound;

public:
	SoundContainer();

	void resize(int size)			{sounds.resize(size);}
	void add(StaticSound *sound)		{ sounds.push_back(sound); }
	StaticSound *&operator[](int i)	{return sounds[i];}

	const Sounds &getSounds() const	{return sounds;}
	StaticSound *getRandSound() const;
};

}}//end namespace

#endif

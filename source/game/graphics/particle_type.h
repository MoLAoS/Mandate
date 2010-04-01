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

#ifndef _GLEST_GAME_PARTICLETYPE_H_
#define _GLEST_GAME_PARTICLETYPE_H_

#include <string>

#include "particle.h"
#include "factory.h"
#include "texture.h"
#include "vec.h"
#include "xml_parser.h"

#include "game_particle.h"

using std::string;

namespace Glest{ namespace Game{

using Shared::Graphics::ParticleSystemBase;
using Shared::Graphics::ParticleSystem;
//using Shared::Graphics::AttackParticleSystem;
//using Shared::Graphics::ProjectileParticleSystem;
//using Shared::Graphics::SplashParticleSystem;
using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using Shared::Math::Vec4f;
using Shared::Graphics::Model;
using Shared::Util::MultiFactory;
using Shared::Xml::XmlNode;

// ===========================================================
//	class ParticleSystemType
//
///	A type of particle system
// ===========================================================

class ParticleSystemType : public ParticleSystemBase {
public:
	ParticleSystemType();
	void load(const XmlNode *particleSystemNode, const string &dir);
	virtual ParticleSystem *create() = 0;

//protected:
//	void setValues(AttackParticleSystem *ats);
};

// ===========================================================
//	class ParticleSystemTypeProjectile
// ===========================================================

class ParticleSystemTypeProjectile: public ParticleSystemType {
public:
	enum ProjectileStart {
		psSelf,
		psTarget,
		psSky
	};

private:
	string trajectory;
	float trajectorySpeed;
	float trajectoryScale;
	float trajectoryFrequency;
	ProjectileStart start;
	bool tracking;

public:
	void load(const string &dir, const string &path);
	virtual ParticleSystem *create();
	ProjectileParticleSystem *createProjectileParticleSystem() {return (ProjectileParticleSystem*)create();}

	ProjectileStart getStart() const	{return start;}
	bool isTracking() const				{return tracking;}
};

// ===========================================================
//	class ParticleSystemTypeSplash
// ===========================================================

class ParticleSystemTypeSplash: public ParticleSystemType {
private:
	int emissionRateFade;
	float verticalSpreadA;
	float verticalSpreadB;
	float horizontalSpreadA;
	float horizontalSpreadB;

public:
	void load(const string &dir, const string &path);
	virtual ParticleSystem *create();
	SplashParticleSystem *createSplashParticleSystem() {return (SplashParticleSystem*)create();}
};

class ParticleSystemTypeCompound: public ParticleSystemTypeSplash {
public:
	void load(const string &dir, const string &path);
};


}}//end namespace

#endif

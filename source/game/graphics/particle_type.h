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

#ifndef _GLEST_GAME_PARTICLETYPE_H_
#define _GLEST_GAME_PARTICLETYPE_H_

#include <string>

#include "game_particle.h"

using std::string;
using Shared::Util::Random;
using Shared::Xml::XmlNode;
using Shared::Graphics::ParticleSystemBase;
using Shared::Graphics::ParticleSystem;

using Glest::Entities::Projectile;
using Glest::Entities::Splash;

namespace Glest { namespace ProtoTypes {

// ===========================================================
//	class ParticleSystemType
//
///	A type of particle system
// ===========================================================
/// base class for particle system proto-types
class ParticleSystemType : public ParticleSystemBase {
public:
	ParticleSystemType();
	void load(const XmlNode *particleSystemNode, const string &dir);
	virtual ParticleSystem *create() = 0;

//protected:
//	void setValues(AttackParticleSystem *ats);
};

// ===========================================================
//	class ProjectileType
// ===========================================================

class ProjectileType: public ParticleSystemType {
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
	Projectile *createProjectileParticleSystem() {return (Projectile*)create();}

	ProjectileStart getStart() const	{return start;}
	bool isTracking() const				{return tracking;}
};

// ===========================================================
//	class SplashType
// ===========================================================

class SplashType: public ParticleSystemType {
private:
	int emissionRateFade;
	float verticalSpreadA;
	float verticalSpreadB;
	float horizontalSpreadA;
	float horizontalSpreadB;

public:
	void load(const string &dir, const string &path);
	virtual ParticleSystem *create();
	Splash *createSplashParticleSystem() {return (Splash*)create();}
};

class ParticleSystemTypeCompound: public SplashType {
public:
	void load(const string &dir, const string &path);
};


}}//end namespace

#endif

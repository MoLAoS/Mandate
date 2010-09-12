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

#include "game_particle.h"

using std::string;
using Shared::Util::Random;
using Shared::Xml::XmlNode;
using Shared::Graphics::ParticleSystemBase;
using Shared::Graphics::ParticleSystem;

using Glest::Entities::UnitParticleSystem;
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
private:
	TrajectoryType trajectory;
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

// ===========================================================
//	class UnitParticleSystemType 
//
///	A particle system type that is attached to units.
// ===========================================================

class UnitParticleSystemType : public ParticleSystemType {
protected:
	//string type;
	Vec3f direction;
	bool relative;
	bool relativeDirection; // ?
	bool fixed;

	bool teamColorEnergy;
	bool teamColorNoEnergy;

	int maxParticles;

public:
	UnitParticleSystemType();
	void load(const XmlNode *particleSystemNode, const string &dir);
	void load(const string &dir, const string &path);

	Vec3f getDirection() const { return direction; }
	bool isRelative() const { return relative; }
	bool isRelativeDirection() const { return relativeDirection; }
	bool isFixed() const { return fixed; }

	bool hasTeamColorEnergy() const { return teamColorEnergy; }
	bool hasTeamColorNoEnergy() const { return teamColorNoEnergy; }

	UnitParticleSystem* createUnitParticleSystem() const {
		return new UnitParticleSystem(*this, maxParticles);
	}
	virtual ParticleSystem* create() { return createUnitParticleSystem(); }
};
typedef vector<UnitParticleSystemType*> UnitParticleSystemTypes;


}}//end namespace

#endif

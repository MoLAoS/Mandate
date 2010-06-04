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
#ifndef _GAME_PARTICLE_INCLUDED_
#define _GAME_PARTICLE_INCLUDED_

#include "particle.h"

using Shared::Util::Random;
using namespace Shared::Graphics;

namespace Glest { 

namespace Sim { class ParticleDamager; }
namespace ProtoTypes { class UnitParticleSystemType; }
using ProtoTypes::UnitParticleSystemType;

namespace Entities {
class Unit;
class Splash;
// ===========================================================================
//  AttackParticleSystem
//
/// Base class for Projectiles and Splashes
// ===========================================================================

class AttackParticleSystem : public ParticleSystem {
protected:
	Vec3f direction;

public:
	AttackParticleSystem(int particleCount);
	AttackParticleSystem(const ParticleSystemBase &model, int particleCount);
	virtual ~AttackParticleSystem() {}

	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);
	virtual Vec3f getDirection() const {return direction;}
};

// =====================================================
//	class Projectile
// =====================================================

class Projectile : public AttackParticleSystem {
	friend class Splash;
private:
	Splash *nextParticleSystem;

	Vec3f lastPos;
	Vec3f startPos;
	Vec3f endPos;
	Vec3f flatPos;

	int startFrame;
	int endFrame;

	const Unit *target;

	Vec3f xVector;
	Vec3f yVector;
	Vec3f zVector;

	TrajectoryType trajectory;
	float trajectorySpeed;

	//parabolic
	float trajectoryScale;
	float trajectoryFrequency;

	Random random;
	int id;

	Sim::ParticleDamager *damager;

public:
	Projectile(int particleCount= 1000);
	Projectile(const ParticleSystemBase &model, int particleCount= 1000);
	virtual ~Projectile();

	void link(Splash *particleSystem);
	void setDamager(Sim::ParticleDamager *damager);

	virtual void update();
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);

	void setTrajectory(TrajectoryType trajectory)			{this->trajectory= trajectory;}
	void setTrajectorySpeed(float trajectorySpeed)			{this->trajectorySpeed= trajectorySpeed;}
	void setTrajectoryScale(float trajectoryScale)			{this->trajectoryScale= trajectoryScale;}
	void setTrajectoryFrequency(float trajectoryFrequency)	{this->trajectoryFrequency= trajectoryFrequency;}
	
	void setPath(Vec3f startPos, Vec3f endPos, int endFrameOffset = -1);

	void setTarget(const Unit *target)						{this->target = target;}
	const Unit* getTarget() const							{return this->target; }

	virtual bool isProjectile() const						{ return true; }

	int getEndFrame() const { return endFrame;	}
};

// =====================================================
//	class Splash
// =====================================================

class Splash : public AttackParticleSystem {
public:
	friend class Projectile;

private:
	Projectile *prevParticleSystem;

	int emissionRateFade;
	float verticalSpreadA;
	float verticalSpreadB;
	float horizontalSpreadA;
	float horizontalSpreadB;

public:
	Splash(int particleCount = 1000);
	Splash(const ParticleSystemBase &model, int particleCount = 1000);
	virtual ~Splash();

	virtual void update();
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);

	void setEmissionRateFade(int emissionRateFade)		{this->emissionRateFade = emissionRateFade;}
	void setVerticalSpreadA(float verticalSpreadA)		{this->verticalSpreadA = verticalSpreadA;}
	void setVerticalSpreadB(float verticalSpreadB)		{this->verticalSpreadB = verticalSpreadB;}
	void setHorizontalSpreadA(float horizontalSpreadA)	{this->horizontalSpreadA = horizontalSpreadA;}
	void setHorizontalSpreadB(float horizontalSpreadB)	{this->horizontalSpreadB = horizontalSpreadB;}

};

// =====================================================
//	class UnitParticleSystem
// =====================================================

class UnitParticleSystem : public ParticleSystem {
private:
	Vec3f cRotation;
	Vec3f fixedAddition; // ??
	Vec3f oldPos;

	Vec3f direction;
	Vec3f teamColour;

	//REFACTOR: see note in definition of setTeamColour()
	bool teamColorEnergy;
	bool teamColorNoEnergy;

	// belong in proto-type ??
	int maxParticleEnergy;
	int varParticleEnergy;

	bool relative;
	bool relativeDirection;
	bool fixed;
	float rotation;

public:
	UnitParticleSystem(int particleCount = 2000);
	UnitParticleSystem(const UnitParticleSystemType &model, int particleCount = 2000);

	//virtual
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);
	virtual void update();
	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);

	//set params
	//void setWind(float windAngle, float windSpeed); // ??

	void setDirection(Vec3f direction)					{this->direction = direction;}
	
	void setRotation(float rotation)					{this->rotation = rotation;}
	void setRelative(bool relative)						{this->relative = relative;}
	void setRelativeDirection(bool relativeDirection)	{this->relativeDirection = relativeDirection;}
	void setFixed(bool fixed)							{this->fixed = fixed;}
	void setTeamColour(Vec3f teamColour);
	
};
typedef vector<UnitParticleSystem*> UnitParticleSystems;


}}

#endif
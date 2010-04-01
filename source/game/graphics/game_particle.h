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


using namespace Shared::Graphics;

namespace Glest { namespace Game {

class Unit;
class ParticleDamager;
class SplashParticleSystem;

// ===========================================================================
//  AttackParticleSystem
//
/// Base class for Projectiles and Splashes
// ===========================================================================

class AttackParticleSystem: public ParticleSystem {
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
//	class ProjectileParticleSystem
// =====================================================

class ProjectileParticleSystem: public AttackParticleSystem {
public:
	friend class SplashParticleSystem;

	enum Trajectory {
		tLinear,
		tParabolic,
		tSpiral,
		tRandom
	};
	static Trajectory strToTrajectory(const string &str);

private:
	SplashParticleSystem *nextParticleSystem;

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

	Trajectory trajectory;
	float trajectorySpeed;

	//parabolic
	float trajectoryScale;
	float trajectoryFrequency;

	Random random;
	int id;

	ParticleDamager *damager;

public:
	ProjectileParticleSystem(int particleCount= 1000);
	ProjectileParticleSystem(const ParticleSystemBase &model, int particleCount= 1000);
	virtual ~ProjectileParticleSystem();

	void link(SplashParticleSystem *particleSystem);
	void setDamager(ParticleDamager *damager);

	virtual void update();
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);

	void setTrajectory(Trajectory trajectory)				{this->trajectory= trajectory;}
	void setTrajectorySpeed(float trajectorySpeed)			{this->trajectorySpeed= trajectorySpeed;}
	void setTrajectoryScale(float trajectoryScale)			{this->trajectoryScale= trajectoryScale;}
	void setTrajectoryFrequency(float trajectoryFrequency)	{this->trajectoryFrequency= trajectoryFrequency;}
	
	void setPath(Vec3f startPos, Vec3f endPos, int endFrameOffset = -1);

	void setTarget(const Unit *target)					{this->target = target;}
	const Unit* getTarget() const							{return this->target; }

	virtual bool isProjectile() const						{ return true; }

	int getEndFrame() const { return endFrame;	}
};

// =====================================================
//	class SplashParticleSystem
// =====================================================

class SplashParticleSystem: public AttackParticleSystem {
public:
	friend class ProjectileParticleSystem;

private:
	ProjectileParticleSystem *prevParticleSystem;

	int emissionRateFade;
	float verticalSpreadA;
	float verticalSpreadB;
	float horizontalSpreadA;
	float horizontalSpreadB;

public:
	SplashParticleSystem(int particleCount = 1000);
	SplashParticleSystem(const ParticleSystemBase &model, int particleCount = 1000);
	virtual ~SplashParticleSystem();

	virtual void update();
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);

	void setEmissionRateFade(int emissionRateFade)		{this->emissionRateFade = emissionRateFade;}
	void setVerticalSpreadA(float verticalSpreadA)		{this->verticalSpreadA = verticalSpreadA;}
	void setVerticalSpreadB(float verticalSpreadB)		{this->verticalSpreadB = verticalSpreadB;}
	void setHorizontalSpreadA(float horizontalSpreadA)	{this->horizontalSpreadA = horizontalSpreadA;}
	void setHorizontalSpreadB(float horizontalSpreadB)	{this->horizontalSpreadB = horizontalSpreadB;}

};

}}

#endif
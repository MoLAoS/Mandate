// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//                2008-2009 Daniel Santos
//                2009-2010 Titus Tscharntke
//                2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _GAME_PARTICLE_INCLUDED_
#define _GAME_PARTICLE_INCLUDED_

#include "particle.h"
#include "timer.h"
#include "forward_decs.h"

using Shared::Util::Random;
using Shared::Platform::Chrono;
using namespace Shared::Graphics;

namespace Glest { 

//namespace Sim { class ParticleDamager; }
namespace ProtoTypes { class UnitParticleSystemType; }
using ProtoTypes::UnitParticleSystemType;

namespace Entities {
class Unit;
class Splash;

// =====================================================
//	class GameParticleSystem
// =====================================================

class GameParticleSystem : public ParticleSystem {
protected:
	int64 lastVisCheck; // time last visibility check was made

	static const int64 test_interval = 200;

	void doVisibiltyChecks(ParticleUse use);

public:
	GameParticleSystem(ParticleUse use, bool visible, int particleCount)
			: ParticleSystem(particleCount) {
		this->visible = visible;
		if (visible) {
			initArray(use);
		}
		lastVisCheck = 0;
	}

	GameParticleSystem(ParticleUse use, bool visible, const ParticleSystemBase &model, int particleCount)
			: ParticleSystem(model, particleCount) {
		this->visible = visible;
		if (visible) {
			initArray(use);
		}
		lastVisCheck = 0;
	}

	void checkVisibilty(ParticleUse use, bool log = false);

	virtual bool isFinished() const override {
		if (state == sFade) {
			return !particles || !aliveParticleCount;
		}
		return false;
	}

	MEMORY_CHECK_DECLARATIONS(GameParticleSystem)
};

// =====================================================
//	class FireParticleSystem
// =====================================================

class FireParticleSystem : public GameParticleSystem {
public:
	FireParticleSystem(bool visible, int particleCount = 2000);

	//virtual
	virtual void update() override;
	virtual void initParticle(Particle *p, int particleIndex) override;
	virtual void updateParticle(Particle *p) override;
};

// ===========================================================================
//  DirectedParticleSystem
//
/// Base class for Projectiles and Splashes
// ===========================================================================

class DirectedParticleSystem : public GameParticleSystem {
protected:
	Vec3f direction;
	bool fog;

public:
	DirectedParticleSystem(ParticleUse use, bool visible, const ParticleSystemBase &model, int particleCount);
	virtual ~DirectedParticleSystem() {}

	virtual void render(ParticleRenderer *pr, ModelRenderer *mr) override;
	Vec3f getDirection() const {return direction;}
	void setDirection(const Vec3f &dir) { direction = dir; }
};

// =====================================================
//	interface ProjectileCallback
// =====================================================

class ProjectileCallback {
public:
	virtual void projectileArrived(ParticleSystem *system) = 0;
};

// =====================================================
//	class Projectile
// =====================================================

class Projectile : public DirectedParticleSystem {
	friend class Sim::EntityFactory<Projectile>;
	friend class Splash;

private:
	int m_id;                   // persist
	Splash *nextParticleSystem; // no-persist

	Vec3f lastPos;   // persist (for convenience, could recalc...)
	Vec3f startPos;  // persist
	Vec3f endPos;    // persist
	Vec3f flatPos;   // persist (for convenience, could recalc...)

	int startFrame;  // persist
	int endFrame;    // persist

	const Unit *target; // swizzle

	Vec3f xVector; // no-persist
	Vec3f yVector; // no-persist
	Vec3f zVector; // no-persist

	TrajectoryType trajectory; // no-persist (get from type)
	float trajectorySpeed;     // no-persist (calculate from ???)

	// parabolic
	float trajectoryScale;     // no-persist
	float trajectoryFrequency; // no-persist

	Random random;             // no-persist

	ProjectileCallback  *callback;
	//Sim::ParticleDamager *damager; // swizzle

public:
	struct CreateParams {
		bool visible;
		const ParticleSystemBase &model;
		int particleCount;

		CreateParams(bool visible, const ParticleSystemBase &model, int particleCount)
			: visible(visible), model(model), particleCount(particleCount) {}
	};

private:
	Projectile(CreateParams params);
	virtual ~Projectile();

	void setId(int i) { m_id = i; }

public:
	void link(Splash *particleSystem);
	void setCallback(ProjectileCallback *cb);

	int getId() const { return m_id; }

	virtual void update() override;
	virtual void initParticle(Particle *p, int particleIndex) override;
	virtual void updateParticle(Particle *p) override;

	void setTrajectory(TrajectoryType trajectory)			{this->trajectory= trajectory;}
	void setTrajectorySpeed(float trajectorySpeed)			{this->trajectorySpeed= trajectorySpeed;}
	void setTrajectoryScale(float trajectoryScale)			{this->trajectoryScale= trajectoryScale;}
	void setTrajectoryFrequency(float trajectoryFrequency)	{this->trajectoryFrequency= trajectoryFrequency;}
	
	void setPath(Vec3f startPos, Vec3f endPos, int endFrameOffset = -1);

	void setTarget(const Unit *target)						{this->target = target;}
	const Unit* getTarget() const							{return this->target; }

	virtual bool isProjectile() const override				{ return true; }

	int getEndFrame() const { return endFrame;	}
};

// =====================================================
//	class Splash
// =====================================================

class Splash : public DirectedParticleSystem {
public:
	friend class Projectile;

private:
	Projectile *prevParticleSystem;

	float emissionRateFade;
	float verticalSpreadA;
	float verticalSpreadB;
	float horizontalSpreadA;
	float horizontalSpreadB;

public:
	Splash(bool visible, const ParticleSystemBase &model, int particleCount = 1000);
	virtual ~Splash();

	virtual void update() override;
	virtual void initParticle(Particle *p, int particleIndex) override;
	virtual void updateParticle(Particle *p) override;

	void setEmissionRateFade(float emissionRateFade)	{this->emissionRateFade = emissionRateFade;}
	void setVerticalSpreadA(float verticalSpreadA)		{this->verticalSpreadA = verticalSpreadA;}
	void setVerticalSpreadB(float verticalSpreadB)		{this->verticalSpreadB = verticalSpreadB;}
	void setHorizontalSpreadA(float horizontalSpreadA)	{this->horizontalSpreadA = horizontalSpreadA;}
	void setHorizontalSpreadB(float horizontalSpreadB)	{this->horizontalSpreadB = horizontalSpreadB;}
};

// =====================================================
//	class UnitParticleSystem
// =====================================================

class UnitParticleSystem : public GameParticleSystem {
private:
	Vec3f cRotation;
	Vec3f fixedAddition; // ??
	Vec3f oldPos;

	Vec3f direction;

	// belong in proto-type ??
	int maxParticleEnergy;
	int varParticleEnergy;

	bool relative;
	bool relativeDirection;
	bool fixed;
	float rotation;

	const UnitParticleSystemType *type;

public:
	UnitParticleSystem(bool visible, const UnitParticleSystemType &model, int particleCount = 2000);
	~UnitParticleSystem();

	//virtual
	virtual void initParticle(Particle *p, int particleIndex) override;
	virtual void updateParticle(Particle *p) override;
	virtual void update() override;
	virtual void render(ParticleRenderer *pr, ModelRenderer *mr) override;

	//set params
	//void setWind(float windAngle, float windSpeed); // ??

	void setDirection(Vec3f direction)					{this->direction = direction;}
	
	void setRotation(float rotation)					{this->rotation = rotation;}
	void setRelative(bool relative)						{this->relative = relative;}
	void setRelativeDirection(bool relativeDirection)	{this->relativeDirection = relativeDirection;}
	void setFixed(bool fixed)							{this->fixed = fixed;}
	
	const UnitParticleSystemType* getType() const {return type;}
};
typedef list<UnitParticleSystem*> UnitParticleSystems;


}}

#endif
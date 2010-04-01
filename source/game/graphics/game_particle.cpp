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

#include "pch.h"

#include "game_particle.h"
#include "particle_renderer.h"
#include "logger.h"
#include "world.h"
#include "unit_updater.h"

#define PARTICLE_LOG(x) {}//{ theLogger.add(intToStr(theWorld.getFrameCount()) + " :: " + x); }

namespace Glest { namespace Game {

// ===========================================================================
//  AttackParticleSystem
// ===========================================================================

AttackParticleSystem::AttackParticleSystem(int particleCount)
		: ParticleSystem(particleCount)
		, direction(1.0f, 0.0f, 0.0f) {
}

AttackParticleSystem::AttackParticleSystem(const ParticleSystemBase &type, int particleCount)
		: ParticleSystem(type, particleCount) 
		, direction(1.0f, 0.0f, 0.0f) {	
}

void AttackParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr) {
	if (active) {
		if (model) {
			pr->renderSingleModel(this, mr);
		}
		switch (primitiveType) {
		case Particle::ptQuad:
			pr->renderSystem(this);
			break;
		case Particle::ptLine:
			pr->renderSystemLine(this);
			break;
		default:
			assert(false);
		}
	}
}

// ===========================================================================
//  ProjectileParticleSystem
// ===========================================================================

ProjectileParticleSystem::ProjectileParticleSystem(int particleCount)
		: AttackParticleSystem(particleCount)
		, trajectory(tLinear)
		, trajectorySpeed(1.f)
		, trajectoryScale(1.f)
		, trajectoryFrequency(1.f)
		, nextParticleSystem(0)
		, target(0)
		, damager(0) {
	setEmissionRate(20);
	setColor(Vec4f(1.0f, 0.3f, 0.0f, 0.5f));
	setEnergy(100);
	setEnergyVar(50);
	setSize(0.4f);
	setSpeed(0.14f);
}


ProjectileParticleSystem::ProjectileParticleSystem(const ParticleSystemBase &model, int particleCount)
		: AttackParticleSystem(model, particleCount)
		, trajectory(tLinear)
		, trajectorySpeed(1.f)
		, trajectoryScale(1.f)
		, trajectoryFrequency(1.f)
		, nextParticleSystem(0)
		, target(0)
		, damager(0) {
}

ProjectileParticleSystem::~ProjectileParticleSystem() {
	if (nextParticleSystem != NULL) {
		nextParticleSystem->prevParticleSystem = NULL;
	}
	delete damager;
}

void ProjectileParticleSystem::setDamager(ParticleDamager *dmgr) {
	assert(!damager);
	damager = dmgr;
}


void ProjectileParticleSystem::link(SplashParticleSystem *particleSystem) {
	nextParticleSystem = particleSystem;
	nextParticleSystem->setState(sPause);
	nextParticleSystem->prevParticleSystem = this;
}

void ProjectileParticleSystem::update() {
	if (state == sPlay) {
		if (target) {
			endPos = target->getCurrVector();
		}
		lastPos = pos;

		assert(startFrame >= 0 && endFrame >= 0 && startFrame < endFrame);

		float t = clamp(
			(float(theWorld.getFrameCount()) - float(startFrame))
			/ (float(endFrame) - float(startFrame)), 0.f, 1.f);

		//Vec3f flatVector;
/*
		if (trajectory == tRandom) {
			Vec3f currentTargetVector = endPos - pos;
			currentTargetVector.normalize();

			float varRotation = random.randRange(0.f, twopi);
			float varAngle = random.randRange(-pi, pi) * trajectoryScale;
			float varPitch = cosf(varRotation) * varAngle;
			float varYaw = sinf(varRotation) * varAngle;

			float d = Vec2f(currentTargetVector.z, currentTargetVector.x).length();
			float yaw = atan2f(currentTargetVector.x, currentTargetVector.z) + varYaw;
			float pitch = asinf(currentTargetVector.y) + varPitch;
			float pc = cosf(pitch);

			Vec3f newVector(pc * sinf(yaw), sinf(pitch), pc * cosf(yaw));

			float lengthVariance = 1.f;//random.randRange(0.125f, 1.f);
			//currentEmissionRate = (int)roundf(emissionRate * lengthVariance);
			flatVector = newVector * (trajectorySpeed * lengthVariance);
		} else {*/
			//flatVector = zVector * trajectorySpeed;
		//}

		PARTICLE_LOG( "updating particle now @" + Vec3fToStr(flatPos) )

		flatPos = startPos + (endPos - startPos) * t;
		Vec3f targetVector = endPos - startPos;
		//Vec3f currentVector = flatPos - startPos;

		// ratio
		//float t = clamp(currentVector.length() / targetVector.length(), 0.0f, 1.0f);

		// trajectory
		switch (trajectory) {
			case tLinear:
				pos = flatPos;
				break;

			case tParabolic: {
					float scaledT = 2.0f * (t - 0.5f);
					float paraboleY = (1.0f - scaledT * scaledT) * trajectoryScale;

					pos = flatPos;
					pos.y += paraboleY;
				}
				break;

			case tSpiral:
				pos = flatPos;
				pos += xVector * cosf(t * trajectoryFrequency * targetVector.length()) * trajectoryScale;
				pos += yVector * sinf(t * trajectoryFrequency * targetVector.length()) * trajectoryScale;
				break;

			case tRandom:
				if (flatPos.dist(endPos) < 0.5f) {
					pos = flatPos;
				} else {
					pos = flatPos;
					//pos += xVector * cos(t*trajectoryFrequency*targetVector.length())*trajectoryScale;
					//pos += yVector * sin(t*trajectoryFrequency*targetVector.length())*trajectoryScale;
				}
				break;
			default:
				throw runtime_error("Unknown projectile trajectory: " + intToStr(trajectory));
		}
	}

	direction = pos - lastPos;
	direction.normalize();

	if (theWorld.getFrameCount() == endFrame) {
		state = sFade;
		model = NULL;

		assert(damager);
		damager->execute(this);

		if (nextParticleSystem) {
			nextParticleSystem->setState(sPlay);
			nextParticleSystem->setPos(endPos);
		}
	}
	ParticleSystem::update();
}

void ProjectileParticleSystem::initParticle(Particle *p, int particleIndex) {
	ParticleSystem::initParticle(p, particleIndex);

	float t = static_cast<float>(particleIndex) / emissionRate;

	p->pos = pos + (lastPos - pos) * t;
	p->lastPos = lastPos;
	p->speed = Vec3f(random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f)) * speed;
	p->accel = Vec3f(0.0f, -gravity, 0.0f);

	updateParticle(p);
}

inline void ProjectileParticleSystem::updateParticle(Particle *p) {
	float energyRatio = clamp(static_cast<float>(p->energy) / energy, 0.f, 1.f);

	p->lastPos += p->speed;
	p->pos += p->speed;
	p->speed += p->accel;
	p->color = color * energyRatio + colorNoEnergy * (1.0f - energyRatio);
	p->color2 = color2 * energyRatio + color2NoEnergy * (1.0f - energyRatio);
	p->size = size * energyRatio + sizeNoEnergy * (1.0f - energyRatio);
	p->energy--;
}

void ProjectileParticleSystem::setPath(Vec3f startPos, Vec3f endPos, int frames) {
	//compute axis
	zVector = endPos - startPos;
	zVector.normalize();
	yVector = Vec3f(0.0f, 1.0f, 0.0f);
	xVector = zVector.cross(yVector);

	//apply offset
	startPos += xVector * offset.x;
	startPos += yVector * offset.y;
	startPos += zVector * offset.z;

	pos = startPos;
	lastPos = startPos;
	flatPos = startPos;

	//recompute axis
	zVector = endPos - startPos;
	zVector.normalize();
	yVector = Vec3f(0.0f, 1.0f, 0.0f);
	xVector = zVector.cross(yVector);

	// set members
	this->startPos = startPos;
	this->endPos = endPos;

	// calculate arrival (unless being told by server)
	if (frames  == -1) {
		Vec3f flatVector = zVector * trajectorySpeed;
		Vec3f traj = endPos - startPos;
		assert(traj.length() > 0.5f);
		frames = int((traj.length() - 0.5f) / flatVector.length());
		// just in case
		if (frames < 1) frames = 1;
	}

	startFrame = theWorld.getFrameCount();
	endFrame = startFrame + frames;
	// preProcess updates, when will it arrive ??
	PARTICLE_LOG( "Creating projectile @ " + Vec3fToStr(startPos) + " going to " + Vec3fToStr(endPos) )
	PARTICLE_LOG( "Projectile should arrive at " + intToStr(endFrame) )
}

ProjectileParticleSystem::Trajectory ProjectileParticleSystem::strToTrajectory(const string &str) {
	if(str == "linear") {
		return tLinear;
	} else if(str == "parabolic") {
		return tParabolic;
	} else if(str == "spiral") {
		return tSpiral;
	} else if(str == "random") {
		return tRandom;
	} else {
		throw runtime_error("Unknown particle system trajectory: " + str);
	}
}

// ===========================================================================
//  SplashParticleSystem
// ===========================================================================

SplashParticleSystem::SplashParticleSystem(const ParticleSystemBase &model,  int particleCount)
		: AttackParticleSystem(model, particleCount)
		, prevParticleSystem(0)
		, emissionRateFade(1)
		, verticalSpreadA(1.f)
		, verticalSpreadB(0.f)
		, horizontalSpreadA(1.f)
		, horizontalSpreadB(0.f) {
}

SplashParticleSystem::SplashParticleSystem(int particleCount)
		: AttackParticleSystem(particleCount) 
		, prevParticleSystem(0)
		, emissionRateFade(1)
		, verticalSpreadA(1.f)
		, verticalSpreadB(0.f)
		, horizontalSpreadA(1.f)
		, horizontalSpreadB(0.f) {
	setColor(Vec4f(1.0f, 0.3f, 0.0f, 0.8f));
	setEnergy(100);
	setEnergyVar(50);
	setSize(1.0f);
	setSpeed(0.003f);
}

SplashParticleSystem::~SplashParticleSystem() {
	if (prevParticleSystem != NULL) {
		prevParticleSystem->nextParticleSystem = NULL;
	}
}

inline void SplashParticleSystem::update() {
	ParticleSystem::update();
	if (state != sPause) {
		emissionRate -= emissionRateFade;
	}
}

void SplashParticleSystem::initParticle(Particle *p, int particleIndex){
	p->pos = pos;
	p->lastPos = p->pos;
	p->energy = energy;
	p->size = size;
	p->color = color;
	p->speed = Vec3f(
			horizontalSpreadA * random.randRange(-1.0f, 1.0f) + horizontalSpreadB,
			verticalSpreadA * random.randRange(-1.0f, 1.0f) + verticalSpreadB,
			horizontalSpreadA * random.randRange(-1.0f, 1.0f) + horizontalSpreadB);
	p->speed.normalize();
	p->speed = p->speed * speed;
	p->accel = Vec3f(0.0f, -gravity, 0.0f);
}

inline void SplashParticleSystem::updateParticle(Particle *p) {
	float energyRatio = clamp(static_cast<float>(p->energy) / energy, 0.f, 1.f);

	p->lastPos = p->pos;
	p->pos = p->pos + p->speed;
	p->speed = p->speed + p->accel;
	p->energy--;
	p->color = color * energyRatio + colorNoEnergy * (1.0f - energyRatio);
	p->size = size * energyRatio + sizeNoEnergy * (1.0f - energyRatio);
}


}}
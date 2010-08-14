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

#define PARTICLE_LOG(x) {}//{ g_logger.add(intToStr(g_world.getFrameCount()) + " :: " + x); }

namespace Glest { namespace Entities {

// ===========================================================================
//  AttackParticleSystem
// ===========================================================================

AttackParticleSystem::AttackParticleSystem(int particleCount)
		: ParticleSystem(particleCount)
		, direction(1.0f, 0.0f, 0.0f) {
}

AttackParticleSystem::AttackParticleSystem(const ParticleSystemBase &protoType, int particleCount)
		: ParticleSystem(protoType, particleCount) 
		, direction(1.0f, 0.0f, 0.0f) {	
}

void AttackParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr) {
	if (active) {
		if (model) {
			pr->renderSingleModel(this, mr);
		}
		switch (primitiveType) {
		case PrimitiveType::QUAD:
			pr->renderSystem(this);
			break;
		case PrimitiveType::LINE:
			pr->renderSystemLine(this);
			break;
		default:
			assert(false);
		}
	}
}

// ===========================================================================
//  Projectile
// ===========================================================================

Projectile::Projectile(int particleCount)
		: AttackParticleSystem(particleCount)
		, nextParticleSystem(0)
		, target(0)
		, trajectory(TrajectoryType::LINEAR)
		, trajectorySpeed(1.f)
		, trajectoryScale(1.f)
		, trajectoryFrequency(1.f)
		, damager(0) {
	setEmissionRate(20);
	setColor(Vec4f(1.0f, 0.3f, 0.0f, 0.5f));
	setEnergy(100);
	setEnergyVar(50);
	setSize(0.4f);
	setSpeed(0.14f);
}


Projectile::Projectile(const ParticleSystemBase &protoType, int particleCount)
		: AttackParticleSystem(protoType, particleCount)
		, nextParticleSystem(0)
		, target(0)
		, trajectory(TrajectoryType::LINEAR)
		, trajectorySpeed(1.f)
		, trajectoryScale(1.f)
		, trajectoryFrequency(1.f)
		, damager(0) {
}

Projectile::~Projectile() {
	if (nextParticleSystem != NULL) {
		nextParticleSystem->prevParticleSystem = NULL;
	}
	delete damager;
}

void Projectile::setDamager(ParticleDamager *dmgr) {
	assert(!damager);
	damager = dmgr;
}


void Projectile::link(Splash *particleSystem) {
	nextParticleSystem = particleSystem;
	nextParticleSystem->setState(sPause);
	nextParticleSystem->prevParticleSystem = this;
}

void Projectile::update() {
	if (state == sPlay) {
		if (target) {
			endPos = target->getCurrVector();
		}
		lastPos = pos;

		assert(startFrame >= 0 && endFrame >= 0 && startFrame < endFrame);

		float t = clamp((g_world.getFrameCount() - startFrame) / float(endFrame - startFrame), 0.f, 1.f);

		//Vec3f flatVector;
/*
		if (trajectory == TrajectoryType::RANDOM) {
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
			case TrajectoryType::LINEAR:
				pos = flatPos;
				break;

			case TrajectoryType::PARABOLIC: {
					float scaledT = 2.0f * (t - 0.5f);
					float paraboleY = (1.0f - scaledT * scaledT) * trajectoryScale;

					pos = flatPos;
					pos.y += paraboleY;
				}
				break;

			case TrajectoryType::SPIRAL:
				pos = flatPos;
				pos += xVector * cosf(t * trajectoryFrequency * targetVector.length()) * trajectoryScale;
				pos += yVector * sinf(t * trajectoryFrequency * targetVector.length()) * trajectoryScale;
				break;

			case TrajectoryType::RANDOM:
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

	if (g_world.getFrameCount() == endFrame) {
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

void Projectile::initParticle(Particle *p, int particleIndex) {
	ParticleSystem::initParticle(p, particleIndex);

	float t = float(particleIndex) / emissionRate;

	p->pos = pos + (lastPos - pos) * t;
	p->lastPos = lastPos;
	p->speed = Vec3f(random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f)) * speed;
	p->accel = Vec3f(0.0f, -gravity, 0.0f);

	updateParticle(p);
}

inline void Projectile::updateParticle(Particle *p) {
	float energyRatio = clamp(float(p->energy) / energy, 0.f, 1.f);

	p->lastPos += p->speed;
	p->pos += p->speed;
	p->speed += p->accel;
	p->color = color * energyRatio + colorNoEnergy * (1.0f - energyRatio);
	p->color2 = color2 * energyRatio + color2NoEnergy * (1.0f - energyRatio);
	p->size = size * energyRatio + sizeNoEnergy * (1.0f - energyRatio);
	p->energy--;
}

void Projectile::setPath(Vec3f startPos, Vec3f endPos, int frames) {
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

	startFrame = g_world.getFrameCount();
	endFrame = startFrame + frames;
	// preProcess updates, when will it arrive ??
	PARTICLE_LOG( "Creating projectile @ " + Vec3fToStr(startPos) + " going to " + Vec3fToStr(endPos) )
	PARTICLE_LOG( "Projectile should arrive at " + intToStr(endFrame) )
}

// ===========================================================================
//  Splash
// ===========================================================================

Splash::Splash(const ParticleSystemBase &model,  int particleCount)
		: AttackParticleSystem(model, particleCount)
		, prevParticleSystem(0)
		, emissionRateFade(1)
		, verticalSpreadA(1.f)
		, verticalSpreadB(0.f)
		, horizontalSpreadA(1.f)
		, horizontalSpreadB(0.f) {
}

Splash::Splash(int particleCount)
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

Splash::~Splash() {
	if (prevParticleSystem != NULL) {
		prevParticleSystem->nextParticleSystem = NULL;
	}
}

inline void Splash::update() {
	ParticleSystem::update();
	if (state != sPause) {
		emissionRate -= emissionRateFade;
	}
}

void Splash::initParticle(Particle *p, int particleIndex){
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

inline void Splash::updateParticle(Particle *p) {
	float energyRatio = clamp(static_cast<float>(p->energy) / energy, 0.f, 1.f);

	p->lastPos = p->pos;
	p->pos = p->pos + p->speed;
	p->speed = p->speed + p->accel;
	p->energy--;
	p->color = color * energyRatio + colorNoEnergy * (1.0f - energyRatio);
	p->size = size * energyRatio + sizeNoEnergy * (1.0f - energyRatio);
}


// ===========================================================================
//  UnitParticleSystem
// ===========================================================================

/*
UnitParticleSystem::UnitParticleSystem(int particleCount): ParticleSystem(particleCount){
	radius = 0.5f;
	speed = 0.01f;
//	windSpeed = Vec3f(0.f);

	size = 0.6f;
	colorNoEnergy = Vec4f(1.f, 0.5f, 0.f, 1.f);

	primitiveType = PrimitiveType::QUAD;
	offset = Vec3f(0.0f);
	direction = Vec3f(0.0f, 1.0f, 0.0f);
	gravity = 0.0f;

	fixed = false;
	rotation = 0.0f;
	relativeDirection=true;

	cRotation = Vec3f(1.f, 1.f, 1.f);
	fixedAddition = Vec3f(0.f, 0.f, 0.f);

}
*/
UnitParticleSystem::UnitParticleSystem(const UnitParticleSystemType &protoType, int particleCount)
		: ParticleSystem(protoType, particleCount){
	// are these not set in prototype?
	type = &protoType;
	radius = 0.5f; // not set in prototype?
	
	rotation = 0.0f;
	cRotation = Vec3f(1.f, 1.f, 1.f);
	fixedAddition = Vec3f(0.f, 0.f, 0.f);

	direction = protoType.getDirection();
	relative = protoType.isRelative();
	relativeDirection = protoType.isRelativeDirection();
	fixed = protoType.isFixed();
	teamColorEnergy = protoType.hasTeamColorEnergy();
	teamColorNoEnergy = protoType.hasTeamColorNoEnergy();

	varParticleEnergy = protoType.getEnergyVar();
	maxParticleEnergy = protoType.getEnergy();
}


void UnitParticleSystem::render(ParticleRenderer *pr,ModelRenderer *mr){
	switch (primitiveType) {
		case PrimitiveType::QUAD:
			pr->renderSystem(this);
			break;
		case PrimitiveType::LINE:
			pr->renderSystemLine(this);
			break;
		default:
			assert(false);
	}
}
 
void UnitParticleSystem::setTeamColour(Vec3f teamColour) {
	this->teamColour = teamColour;
	Vec3f tmpCol;

	//REFACTOR: teamColorEnergy & teamColorNoEnergy are in the proto-type
	// why are they also here in the instance ???
	if (teamColorEnergy) {
		color = Vec4f(teamColour.x, teamColour.y, teamColour.z, color.w);
	}
	if (teamColorNoEnergy) {
		colorNoEnergy = Vec4f(teamColour.x, teamColour.y, teamColour.z, colorNoEnergy.w);
	}
}

void UnitParticleSystem::initParticle(Particle *p, int particleIndex) {
	ParticleSystem::initParticle(p, particleIndex);

	float ang = random.randRange(-2.f * pi, 2.f * pi);
	float mod = fabsf(random.randRange(-radius, radius));

	float x = sinf(ang) * mod;
	float y = cosf(ang) * mod;

	float radRatio = sqrtf(sqrtf(mod / radius));

	p->color = color;
	p->energy = int(maxParticleEnergy * radRatio) + random.randRange(-varParticleEnergy, varParticleEnergy);

	p->lastPos = pos;
	oldPos = pos;
	p->size = size;

	p->speed = Vec3f(
				direction.x + direction.x * random.randRange(-0.5f, 0.5f),
				direction.y + direction.y * random.randRange(-0.5f, 0.5f),
				direction.z + direction.z * random.randRange(-0.5f, 0.5f)
	);
	p->speed = p->speed * speed;
	p->accel = Vec3f(0.0f, -gravity, 0.0f);

	if (!relative) {
		p->pos = Vec3f(
			pos.x + x + offset.x, 
			pos.y + random.randRange(-radius/2, radius/2) + offset.y, 
			pos.z + y + offset.z
		);
	} else { // rotate it according to rotation
		float rad = degToRad(rotation);
		p->pos = Vec3f(
				pos.x + x + offset.z * sinf(rad) + offset.x * cosf(rad),
				pos.y + random.randRange(-radius / 2, radius / 2) + offset.y, 
				pos.z + y + (offset.z * cosf(rad) - offset.x * sinf(rad))
		); 
		if (relativeDirection) {
			p->speed = Vec3f(
				p->speed.z * sinf(rad) + p->speed.x * cosf(rad), 
				p->speed.y, 
				(p->speed.z * cosf(rad) - p->speed.x * sinf(rad))
			);
		}
	}
}

void UnitParticleSystem::update() {
	if (fixed) {
		fixedAddition = Vec3f(pos.x - oldPos.x, pos.y - oldPos.y, pos.z - oldPos.z);
		oldPos = pos;
	}
	ParticleSystem::update();
}

void UnitParticleSystem::updateParticle(Particle *p) {
	float energyRatio = clamp(float(p->energy) / maxParticleEnergy, 0.f, 1.f);

	p->lastPos += p->speed;
	p->pos += p->speed;
	if (fixed) {
		p->lastPos += fixedAddition;
		p->pos += fixedAddition;
	}
	p->speed += p->accel;
	p->color = color * energyRatio + colorNoEnergy * (1.f - energyRatio);
	p->size = size * energyRatio + sizeNoEnergy * (1.f - energyRatio);
	--p->energy;
}

// ================= SET PARAMS ====================

/*void UnitParticleSystem::setWind(float windAngle, float windSpeed) {
	this->windSpeed.x = sinf(degToRad(windAngle)) * windSpeed;
	this->windSpeed.y = 0.f;
	this->windSpeed.z = cosf(degToRad(windAngle)) * windSpeed;
}*/

}}
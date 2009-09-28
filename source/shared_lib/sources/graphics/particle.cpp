// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "particle.h"

#include <cassert>
#include <algorithm>

#include "util.h"
#include "particle_renderer.h"
#include "math_util.h"

#include "leak_dumper.h"


using namespace Shared::Util;

namespace Shared{ namespace Graphics{


// =====================================================
//	class ParticleSystemBase
// =====================================================

// =====================================================
//	class ParticleSystemBase
// =====================================================

ParticleSystemBase::ParticleSystemBase() :
		random((intptr_t)this & 0xffffffff),
		blendMode(Particle::bmOne),
		primitiveType(Particle::ptQuad),
		texture(NULL),
		model(NULL),
		offset(0.f),
		color(1.f),
		color2(1.f),
		colorNoEnergy(0.f),
		color2NoEnergy(0.f),
		size(1.f),
		sizeNoEnergy(1.f),
		speed(1.f),
		gravity(0.f),
		mass(0.f),
		density(0.f),
		emissionRate(15),
		energy(250),
		energyVar(50),
		radius(0.f),
		drawCount(1) {
}

ParticleSystemBase::ParticleSystemBase(const ParticleSystemBase &model) :
		random((intptr_t)this & 0xffffffff),
		blendMode(model.blendMode),
		primitiveType(model.primitiveType),
		texture(model.texture),
		model(model.model),
		offset(model.offset),
		color(model.color),
		color2(model.color2),
		colorNoEnergy(model.colorNoEnergy),
		color2NoEnergy(model.color2NoEnergy),
		size(model.size),
		sizeNoEnergy(model.sizeNoEnergy),
		speed(model.speed),
		gravity(model.gravity),
		mass(model.mass),
		density(model.density),
		emissionRate(model.emissionRate),
		energy(model.energy),
		energyVar(model.energyVar),
		radius(model.radius),
		drawCount(model.drawCount) {
}

// =====================================================
//	class ParticleSystem
// =====================================================

ParticleSystem::ParticleSystem(int particleCount) :
		ParticleSystemBase(),
		particleCount(particleCount),
		particles(new Particle[particleCount]),
		state(sPlay),
		active(true),
		visible(true),
		aliveParticleCount(0),
		pos(0.f),
		windSpeed(0.f),
		particleObserver(NULL) {
}

ParticleSystem::ParticleSystem(const ParticleSystemBase &model, int particleCount) :
		ParticleSystemBase(model),
		particleCount(particleCount),
		particles(new Particle[particleCount]),
		state(sPlay),
		active(true),
		visible(true),
		aliveParticleCount(0),
		pos(0.f),
		windSpeed(0.f),
		particleObserver(NULL) {
}

ParticleSystem::~ParticleSystem() {
	delete [] particles;
}

// =============== VIRTUAL ======================

//updates all living particles and creates new ones
void ParticleSystem::update() {

	if (state != sPause) {
		for (int i = 0; i < aliveParticleCount; ++i) {
			updateParticle(&particles[i]);
			if (deathTest(&particles[i])) {
				//kill the particle
				killParticle(&particles[i]);

				//mantain alive particles at front of the array
				if (aliveParticleCount > 0) {
					particles[i] = particles[aliveParticleCount];
				}
			}
		}

		if (state != sFade) {
			for (int i = 0; i < emissionRate; ++i) {
				Particle *p = createParticle();
				initParticle(p, i);
			}
		}

		if (state == sPlayLast) {
			state = sFade;
		}
	}
}

inline void ParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr) {
	if (active) {
		pr->renderSystem(this);
	}
}


// =============== MISC =========================

void ParticleSystem::fade() {
	assert(state == sPlay);
	state = sFade;

	if (particleObserver) {
		particleObserver->update(this);
	}
}

// =============== PROTECTED =========================

// if there is one dead particle it returns it else, return the particle with
// less energy
Particle *ParticleSystem::createParticle() {

	//if any dead particles
	if (aliveParticleCount < particleCount) {
		++aliveParticleCount;
		return &particles[aliveParticleCount-1];
	}

	//if not
	int minEnergy = particles[0].energy;
	int minEnergyParticle = 0;
	for (int i = 0; i < particleCount; ++i) {
		if (particles[i].energy < minEnergy) {
			minEnergy = particles[i].energy;
			minEnergyParticle = i;
		}
	}

	return &particles[minEnergyParticle];
}

inline void ParticleSystem::initParticle(Particle *p, int particleIndex) {
	p->pos = pos;
	p->lastPos = pos;
	p->speed = Vec3f(0.0f);
	p->accel = Vec3f(0.0f);
	p->color = getColor();
	p->color2 = getColor2();
	p->size = getSize();
	p->energy = getRandEnergy();
//	p->color = Vec4f(1.0f, 1.0f, 1.0f, 1.0);
}

inline void ParticleSystem::updateParticle(Particle *p) {
	p->lastPos = p->pos;
	p->pos = p->pos + p->speed;
	p->speed = p->speed + p->accel;
	p->energy--;
}

// ===========================================================================
//  FireParticleSystem
// ===========================================================================

FireParticleSystem::FireParticleSystem(int particleCount): ParticleSystem(particleCount) {
	setRadius(0.5f);
	setSpeed(0.01f);
	setSize(0.6f);
	setColorNoEnergy(Vec4f(1.0f, 0.5f, 0.0f, 1.0f));
}

void FireParticleSystem::initParticle(Particle *p, int particleIndex) {
// ParticleSystem::initParticle(p, particleIndex);

	float ang = random.randRange(-twopi, twopi);
	float mod = fabsf(random.randRange(-radius, radius));

	float x = sinf(ang) * mod;
	float y = cosf(ang) * mod;

	float radRatio = sqrtf(sqrtf(mod / radius));
	Vec4f halfColorNoEnergy = colorNoEnergy * 0.5f;

	p->color = halfColorNoEnergy + halfColorNoEnergy * radRatio;
	p->energy = static_cast<int>(energy * radRatio) + random.randRange(-energyVar, energyVar);
	float halfRadius = radius * 0.5f;
	p->pos = Vec3f(pos.x + x, pos.y + random.randRange(-halfRadius, halfRadius), pos.z + y);
	p->lastPos = pos;
	p->size = size;
	p->speed = Vec3f(speed * random.randRange(-0.125f, 0.125f),
			speed * random.randRange(0.5f, 1.5f),
			speed * random.randRange(-0.125f, 0.125f));
}

inline void FireParticleSystem::updateParticle(Particle *p) {
	p->lastPos = p->pos;
	p->pos = p->pos+p->speed;
	p->energy--;

	if(p->color.x > 0.0f)
		p->color.x *= 0.98f;
	if(p->color.y > 0.0f)
		p->color.y *= 0.98f;
	if(p->color.w > 0.0f)
		p->color.w *= 0.98f;

	p->speed.x *= 1.001f;
}


// ================= SET PARAMS ====================


// ===========================================================================
//  RainParticleSystem
// ===========================================================================


RainParticleSystem::RainParticleSystem(int particleCount):ParticleSystem(particleCount) {
	setRadius(20.f);
	setEmissionRate(25);
	setSize(3.0f);
	setColor(Vec4f(0.5f, 0.5f, 0.5f, 0.3f));
	setColor2(Vec4f(0.5f, 0.5f, 0.5f, 0.0f));
	setSpeed(0.2f);
}

void RainParticleSystem::initParticle(Particle *p, int particleIndex){
	ParticleSystem::initParticle(p, particleIndex);

	float x = random.randRange(-radius, radius);
	float y = random.randRange(-radius, radius);

	p->color = color;
	p->color2 = color2;
	p->energy = 10000;
	p->pos = Vec3f(pos.x + x, pos.y, pos.z + y);
	p->lastPos = p->pos;
	p->speed = Vec3f(random.randRange(-speed / 10, speed / 10), -speed,
			random.randRange(-speed / 10, speed / 10)) + windSpeed;
}

inline void RainParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr) {
	pr->renderSystemLine(this);
}

inline bool RainParticleSystem::deathTest(Particle *p) {
	return p->pos.y < 0;
}

// ===========================================================================
//  SnowParticleSystem
// ===========================================================================

SnowParticleSystem::SnowParticleSystem(int particleCount) : ParticleSystem(particleCount) {
	setRadius(30.0f);
	setEmissionRate(2);
	setSize(0.2f);
	setColor(Vec4f(0.8f, 0.8f, 0.8f, 0.8f));
	setSpeed(0.025f);
}

void SnowParticleSystem::initParticle(Particle *p, int particleIndex) {
	ParticleSystem::initParticle(p, particleIndex);

	float x = random.randRange(-radius, radius);
	float y = random.randRange(-radius, radius);

	p->color = color;
	p->energy = 10000;
	p->pos = Vec3f(pos.x + x, pos.y, pos.z + y);
	p->speed = Vec3f(0.0f, -speed, 0.0f) + windSpeed;
	p->speed.x += random.randRange(-0.005f, 0.005f);
	p->speed.y += random.randRange(-0.005f, 0.005f);
}

inline bool SnowParticleSystem::deathTest(Particle *p) {
	return p->pos.y < 0;
}

// ===========================================================================
//  AttackParticleSystem
// ===========================================================================

AttackParticleSystem::AttackParticleSystem(int particleCount): ParticleSystem(particleCount) {
	primitiveType = Particle::ptQuad;
	offset = Vec3f(0.0f);
	gravity = 0.0f;
	direction = Vec3f(1.0f, 0.0f, 0.0f);
}

AttackParticleSystem::AttackParticleSystem(const ParticleSystemBase &type, int particleCount) :
		ParticleSystem(type, particleCount) {
	direction = Vec3f(1.0f, 0.0f, 0.0f);
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

ProjectileParticleSystem::ProjectileParticleSystem(int particleCount) :
		AttackParticleSystem(particleCount) {
	setEmissionRate(20);
	setColor(Vec4f(1.0f, 0.3f, 0.0f, 0.5f));
	setEnergy(100);
	setEnergyVar(50);
	setSize(0.4f);
	setSpeed(0.14f);

	trajectory = tLinear;
	trajectorySpeed = 1.0f;
	trajectoryScale = 1.0f;
	trajectoryFrequency = 1.0f;

	nextParticleSystem = NULL;
	target = NULL;
}


ProjectileParticleSystem::ProjectileParticleSystem(const ParticleSystemBase &model, int particleCount) :
		AttackParticleSystem(model, particleCount) {
	trajectory = tLinear;
	trajectorySpeed = 1.0f;
	trajectoryScale = 1.0f;
	trajectoryFrequency = 1.0f;

	nextParticleSystem = NULL;
	target = NULL;
}

ProjectileParticleSystem::~ProjectileParticleSystem() {
	if (nextParticleSystem != NULL) {
		nextParticleSystem->prevParticleSystem = NULL;
	}
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

		Vec3f targetVector = endPos - startPos;
		Vec3f currentVector = flatPos - startPos;
		Vec3f flatVector;

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
		} else {
			flatVector = zVector * trajectorySpeed;
		}

		flatPos += flatVector;
		if (endPos.dist(flatPos) <= flatVector.length() || endPos.dist(pos) > endPos.dist(lastPos)) {
			pos = endPos;
			state = sPlayLast;
			model = NULL;
		} else {

			// ratio
			float t = clamp(currentVector.length() / targetVector.length(), 0.0f, 1.0f);

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
				assert(false);
			}
		}

		direction = pos - lastPos;
		direction.normalize();

		//arrive destination
		if (state == sPlayLast) {
			if (particleObserver) {
				particleObserver->update(this);
			}

			if (nextParticleSystem) {
				nextParticleSystem->setState(sPlay);
				nextParticleSystem->setPos(endPos);
			}
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

void ProjectileParticleSystem::setPath(Vec3f startPos, Vec3f endPos) {

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
		: AttackParticleSystem(model, particleCount) {

	prevParticleSystem = NULL;

	emissionRateFade = 1;
	verticalSpreadA = 1.0f;
	verticalSpreadB = 0.0f;
	horizontalSpreadA = 1.0f;
	horizontalSpreadB = 0.0f;
}

SplashParticleSystem::SplashParticleSystem(int particleCount)
		: AttackParticleSystem(particleCount) {
	setColor(Vec4f(1.0f, 0.3f, 0.0f, 0.8f));
	setEnergy(100);
	setEnergyVar(50);
	setSize(1.0f);
	setSpeed(0.003f);

	prevParticleSystem = NULL;

	emissionRateFade = 1;
	verticalSpreadA = 1.0f;
	verticalSpreadB = 0.0f;
	horizontalSpreadA = 1.0f;
	horizontalSpreadB = 0.0f;
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

// ===========================================================================
//  ParticleManager
// ===========================================================================

ParticleManager::~ParticleManager(){
	end();
}

void ParticleManager::render(ParticleRenderer *pr, ModelRenderer *mr) const{
	list<ParticleSystem*>::const_iterator it;

	for (it=particleSystems.begin(); it!=particleSystems.end(); it++){
		if((*it)->getVisible()){
			(*it)->render(pr, mr);
		}
	}
}

void ParticleManager::update(){
	list<ParticleSystem*>::iterator it;

	for (it = particleSystems.begin(); it != particleSystems.end(); it++) {
		(*it)->update();
		if ((*it)->isEmpty()) {
			delete *it;
			*it = NULL;
		}
	}

	particleSystems.remove(NULL);
}

void ParticleManager::manage(ParticleSystem *ps) {
	particleSystems.push_back(ps);
}


}}//end namespace

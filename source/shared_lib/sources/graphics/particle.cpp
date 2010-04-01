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
#include <sstream>
#include <exception>

#include "util.h"
#include "particle_renderer.h"
#include "math_util.h"
#include "lang_features.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Shared{ namespace Graphics{

const char* Particle::blendFactorNames[BLEND_FUNC_COUNT] = {
	"zero",
	"one",
	"src_color",
	"one_minus_src_color",
	"dst_color",
	"one_minus_dst_color",
	"src_alpha",
	"one_minus_src_alpha",
	"dst_alpha",
	"one_minus_dst_alpha",
	"constant_color",
	"one_minus_constant_color",
	"constant_alpha",
	"one_minus_constant_alpha",
	"src_alpha_saturate"
};

const char* Particle::blendEquationNames[BLEND_EQUATION_COUNT] = {
	"func_add",
	"func_subtract",
	"func_reverse_subtract",
	"min",
	"max"
};

static __cold __noreturn void puke(const char*options[], size_t optionCount,
		const string &badValue, const string &typeDesc) {
	stringstream str;
	str << "\"" << badValue << "\" is not a valid " << typeDesc << ".  Valid values are: ";
	for(size_t i = 0; i < optionCount; ++i) {
		str << (i ? ", " : "") << options[i];
	}
	str << ".";
	throw range_error(str.str());
}

Particle::BlendFactor Particle::getBlendFactor(const string &s) {
	for(size_t i = 0; i < BLEND_FUNC_COUNT; ++i) {
		if(s == blendFactorNames[i]) {
			return static_cast<Particle::BlendFactor>(i);
		}
	}
	puke(blendFactorNames, BLEND_FUNC_COUNT, s, "blend function factor");
}

Particle::BlendEquation Particle::getBlendEquation(const string &s) {
	for(size_t i = 0; i < BLEND_EQUATION_COUNT; ++i) {
		if(s == blendEquationNames[i]) {
			return static_cast<Particle::BlendEquation>(i);
		}
	}
	puke(blendEquationNames, BLEND_EQUATION_COUNT, s, "blend equation mode");
}

// =====================================================
//	class ParticleSystemBase
// =====================================================

// =====================================================
//	class ParticleSystemBase
// =====================================================

ParticleSystemBase::ParticleSystemBase() :
		random(0),
		srcBlendFactor(Particle::BLEND_FUNC_SRC_ALPHA),
		destBlendFactor(Particle::BLEND_FUNC_ONE),
		blendEquationMode(Particle::BLEND_EQUATION_FUNC_ADD),
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
		random(0),
		srcBlendFactor(model.srcBlendFactor),
		destBlendFactor(model.destBlendFactor),
		blendEquationMode(model.blendEquationMode),
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
		windSpeed(0.f) {
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
		windSpeed(0.f) {
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
//  ParticleManager
// ===========================================================================

ParticleManager::~ParticleManager(){
	end();
}

void ParticleManager::render(ParticleRenderer *pr, ModelRenderer *mr) const{
	list<ParticleSystem*>::const_iterator it;

	for (it=particleSystems.begin(); it!=particleSystems.end(); ++it){
		if((*it)->getVisible()){
			(*it)->render(pr, mr);
		}
	}
}

void ParticleManager::update(){
	list<ParticleSystem*>::iterator it;

	for (it = particleSystems.begin(); it != particleSystems.end(); ++it) {
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

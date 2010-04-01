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

#ifndef _SHARED_GRAPHICS_PARTICLE_H_
#define _SHARED_GRAPHICS_PARTICLE_H_

#include <list>
#include <cassert>

#include "vec.h"
#include "pixmap.h"
#include "texture_manager.h"
#include "random.h"
#include "entity.h"
#include "xml_parser.h"
#include "math_util.h"

using std::list;
using Shared::Util::Random;
using Shared::Xml::XmlNode;

namespace Shared{ namespace Graphics{

class ParticleSystem;
class FireParticleSystem;
class RainParticleSystem;
class SnowParticleSystem;
class ProjectileParticleSystem;
class SplashParticleSystem;
class ParticleRenderer;
class ModelRenderer;
class Model;

// =====================================================
//	class WindSystem
// =====================================================
/*
class WindSystem {
public:
	virtual Vec3f getWind() = 0;
};*/

// =====================================================
//	class Particle
// =====================================================


class Particle {
public:
	enum BlendFactor {
		BLEND_FUNC_ZERO,
		BLEND_FUNC_ONE,
		BLEND_FUNC_SRC_COLOR,
		BLEND_FUNC_ONE_MINUS_SRC_COLOR,
		BLEND_FUNC_DST_COLOR,
		BLEND_FUNC_ONE_MINUS_DST_COLOR,
		BLEND_FUNC_SRC_ALPHA,
		BLEND_FUNC_ONE_MINUS_SRC_ALPHA,
		BLEND_FUNC_DST_ALPHA,
		BLEND_FUNC_ONE_MINUS_DST_ALPHA,
		BLEND_FUNC_CONSTANT_COLOR,
		BLEND_FUNC_ONE_MINUS_CONSTANT_COLOR,
		BLEND_FUNC_CONSTANT_ALPHA,
		BLEND_FUNC_ONE_MINUS_CONSTANT_ALPHA,
		BLEND_FUNC_SRC_ALPHA_SATURATE,

		BLEND_FUNC_COUNT
	};

	static const char* blendFactorNames[BLEND_FUNC_COUNT];

	enum BlendEquation {
		BLEND_EQUATION_FUNC_ADD,
		BLEND_EQUATION_FUNC_SUBTRACT,
		BLEND_EQUATION_FUNC_REVERSE_SUBTRACT,
		BLEND_EQUATION_MIN,
		BLEND_EQUATION_MAX,

		BLEND_EQUATION_COUNT
	};

	static const char* blendEquationNames[BLEND_EQUATION_COUNT];

	enum PrimitiveType {
		ptQuad,
		ptLine
	};

	enum ProjectileStart {
		psSelf,
		psTarget,
		psSky
	};

public:
	//attributes
	Vec3f pos;
	Vec3f lastPos;
	Vec3f speed;
	Vec3f accel;
	Vec4f color;
	Vec4f color2;
	float size;
	int energy;

public:
	//void init(const ParticleSystem &ps, const Vec3f &pos, const Vec3f &speed, const Vec3f &accel);
	//void init2(const ParticleSystem &ps, const Vec3f &pos, const Vec3f &speed, const Vec3f &accel);

	static BlendFactor getBlendFactor(const string &);
	static BlendEquation getBlendEquation(const string &);

	//get
	Vec3f getPos() const		{return pos;}
	Vec3f getLastPos() const	{return lastPos;}
	Vec3f getSpeed() const		{return speed;}
	Vec3f getAccel() const		{return accel;}
	Vec4f getColor() const		{return color;}
	Vec4f getColor2() const		{return color2;}
	float getSize() const		{return size;}
	int getEnergy()	const		{return energy;}

};

// =====================================================
//	class ParticleSystemType
// =====================================================

/** Defines the parameters for a particle system. */
class ParticleSystemBase {
protected:
	Random random;

	Particle::BlendFactor srcBlendFactor;
	Particle::BlendFactor destBlendFactor;
	Particle::BlendEquation blendEquationMode;
	Particle::PrimitiveType primitiveType;
	Texture *texture;
	Model *model;
	Vec3f offset;
	Vec4f color;
	Vec4f color2;
	Vec4f colorNoEnergy;
	Vec4f color2NoEnergy;
	float size;
	float sizeNoEnergy;
	float speed;
	float gravity;
	float mass;
	float density;
	int emissionRate;
	int energy;
	int energyVar;
	float radius;
	int drawCount;

//	static ParticleSystemType *fireParticleSystemType;
//	static ParticleSystemType *rainParticleSystemType;
//	static ParticleSystemType *snowParticleSystemType;

public:
	//constructor and destructor
	ParticleSystemBase();
	ParticleSystemBase(const ParticleSystemBase &model);
	virtual ~ParticleSystemBase(){}

	//get
	Particle::BlendFactor getSrcBlendFactor() const		{return srcBlendFactor;}
	Particle::BlendFactor getDestBlendFactor() const	{return destBlendFactor;}
	Particle::BlendEquation getBlendEquationMode() const{return blendEquationMode;}
	Particle::PrimitiveType getPrimitiveType() const	{return primitiveType;}
	Texture *getTexture() const							{return texture;}
	Model *getModel() const								{return model;}
	const Vec3f &getOffset() const						{return offset;}
	const Vec4f &getColor() const						{return color;}
	const Vec4f &getColor2() const						{return color2;}
	const Vec4f &getColorNoEnergy() const				{return colorNoEnergy;}
	const Vec4f &getColor2NoEnergy() const				{return color2NoEnergy;}
	float getSize() const 								{return size;}
	float getSizeNoEnergy() const						{return sizeNoEnergy;}
	float getSpeed() const								{return speed;}
	float getGravity() const							{return gravity;}
	float getMass() const								{return mass;}
	float getDensity() const							{return density;}
	int getEmissionRate() const							{return emissionRate;}
	int getEnergy() const								{return energy;}
	int getEnergyVar() const							{return energyVar;}
	float getRadius() const								{return radius;}
	int getDrawCount() const							{return drawCount;}

	//set
	void setSrcBlendFactor(Particle::BlendFactor v)		{srcBlendFactor = v;}
	void setDestBlendFactor(Particle::BlendFactor v)	{destBlendFactor = v;}
	void setBlendEquationMode(Particle::BlendEquation v){blendEquationMode = v;}
	void setPrimitiveType(Particle::PrimitiveType v)	{primitiveType = v;}
	void setTexture(Texture *v)							{texture = v;}
	void setModel(Model *v)								{model = v;}
	void setOffset(const Vec3f &v)						{offset = v;}
	void setColor(const Vec4f &v)						{color = v;}
	void setColor2(const Vec4f &v)						{color2 = v;}
	void setColorNoEnergy(const Vec4f &v)				{colorNoEnergy = v;}
	void setColor2NoEnergy(const Vec4f &v)				{color2NoEnergy = v;}
	void setSize(float v)								{size = v;}
	void setSizeNoEnergy(float v)						{sizeNoEnergy = v;}
	void setSpeed(float v)								{speed = v;}
	void setGravity(float v)							{gravity = v;}
	void setMass(float v)								{mass = v;}
	void setEmissionRate(int v)							{emissionRate = v;}
	void setEnergy(int v)								{energy = v;}
	void setEnergyVar(int v)							{energyVar = v;}
	void setRadius(float v)								{radius = v;}
	void setDrawCount(int v)							{drawCount = v;}

//	void load(const XmlNode *particleSystemNode, const string &dir);
//	virtual ParticleSystem *create() = 0;
	int getRandEnergy() 				{return energy + random.randRange(-energyVar, energyVar);}
	void fudgeGravity()					{gravity = mass * density;}
	void fudgeMassDensity()				{mass = 1.f; density = gravity;}

	virtual bool isProjectile() const	{ return false; }
};

// =====================================================
//	class ParticleSystem
// =====================================================

class ParticleSystem : public ParticleSystemBase {
protected:
	enum State {
		sPause,		// No updates
		sPlay,
		sFade		// No new particles
	};

protected:
	int particleCount;
	Particle *particles;
	State state;
	bool active;
	bool visible;
	int aliveParticleCount;
	Vec3f pos;
	Vec3f windSpeed;

public:
	//constructor and destructor
	ParticleSystem(int particleCount = 1000);
	ParticleSystem(const ParticleSystemBase &type, int particleCount = 1000);
	virtual ~ParticleSystem();

	//public
	virtual void update();
	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);

	//get
	State getState() const						{return state;}
	Vec3f getPos() const						{return pos;}
	Particle *getParticle(int i)				{return &particles[i];}
	const Particle *getParticle(int i) const	{return &particles[i];}
	int getAliveParticleCount() const			{return aliveParticleCount;}
	bool getActive() const						{return active;}
	bool getVisible() const						{return visible;}
	const Vec3f &getWindSpeed() const			{return windSpeed;}

	virtual Vec3f getDirection() const { return Vec3f(0.f); }

	//set
	void setState(State state)							{this->state = state;}
	void setPos(Vec3f pos)								{this->pos = pos;}
	void setActive(bool active)							{this->active = active;}
	void setVisible(bool visible)						{this->visible = visible;}
	void setWindSpeed(const Vec3f &windSpeed)			{this->windSpeed = windSpeed;}
	void setWindSpeed2(float hAngle, float hSpeed, float vSpeed = 0.f) {
		this->windSpeed.x = sinf(degToRad(hAngle)) * hSpeed;
		this->windSpeed.y = vSpeed;
		this->windSpeed.z = cosf(degToRad(hAngle)) * hSpeed;
	}

	//misc
	void fade();
	int isEmpty() const {
		assert(aliveParticleCount >= 0);
		return !aliveParticleCount && state != sPause;
	}

protected:
	//protected
	Particle *createParticle();
	void killParticle(Particle *p)				{aliveParticleCount--;}

	//virtual protected
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);
	virtual bool deathTest(Particle *p)			{return p->energy <= 0;}
};

// =====================================================
//	class FireParticleSystem
// =====================================================

class FireParticleSystem: public ParticleSystem {
public:
	FireParticleSystem(int particleCount = 2000);
	FireParticleSystem(const ParticleSystemBase &type, int particleCount = 2000);

	//virtual
	virtual void initParticle(Particle *p, int particleIndex);
	virtual void updateParticle(Particle *p);
};

// =====================================================
//	class RainParticleSystem
// =====================================================

class RainParticleSystem: public ParticleSystem {
public:
	RainParticleSystem(int particleCount = 4000);
	RainParticleSystem(const ParticleSystemBase &model, int particleCount = 4000);

	//virtual
	virtual void render(ParticleRenderer *pr, ModelRenderer *mr);
	virtual void initParticle(Particle *p, int particleIndex);
	virtual bool deathTest(Particle *p);
};

// =====================================================
//	class SnowParticleSystem
// =====================================================

class SnowParticleSystem: public ParticleSystem {
public:
	SnowParticleSystem(int particleCount = 4000);
	SnowParticleSystem(const ParticleSystemBase &type, int particleCount = 4000);

	//virtual
	virtual void initParticle(Particle *p, int particleIndex);
	virtual bool deathTest(Particle *p);
};

// =====================================================
//	class ParticleManager
// =====================================================

class ParticleManager {
protected:
	list<ParticleSystem*> particleSystems;

public:
	virtual ~ParticleManager();
	virtual void update();
	void render(ParticleRenderer *pr, ModelRenderer *mr) const;
	void manage(ParticleSystem *ps);
	void end() {
		while(!particleSystems.empty()) {
			delete particleSystems.front();
			particleSystems.pop_front();
		}
	}
	const list<ParticleSystem*>& getList() const { return particleSystems; }
};

}}//end namespace

#endif

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
#include "math_util.h"
#include "xml_parser.h"
#include "util.h"

using std::list;

namespace Shared{ 

using Xml::XmlNode;
using Util::Random;
	
namespace Graphics{

class ParticleSystem;
class FireParticleSystem;
class RainParticleSystem;
class SnowParticleSystem;
class ParticleRenderer;
class ModelRenderer;
class Model;

#define PARTICLE_LOG(x) /*{cout << x << endl;}*/

STRINGY_ENUM( BlendFactor,
	ZERO,
	ONE,
	SRC_COLOR,
	ONE_MINUS_SRC_COLOR,
	DST_COLOR,
	ONE_MINUS_DST_COLOR,
	SRC_ALPHA,
	ONE_MINUS_SRC_ALPHA,
	DST_ALPHA,
	ONE_MINUS_DST_ALPHA,
	CONSTANT_COLOR,
	ONE_MINUS_CONSTANT_COLOR,
	CONSTANT_ALPHA,
	ONE_MINUS_CONSTANT_ALPHA,
	SRC_ALPHA_SATURATE
);

STRINGY_ENUM( BlendMode,
	FUNC_ADD,
	FUNC_SUBTRACT,
	FUNC_REVERSE_SUBTRACT,
	MIN,
	MAX
);

STRINGY_ENUM( PrimitiveType,
	QUAD,
	LINE
);

STRINGY_ENUM( TrajectoryType,
	LINEAR,
	PARABOLIC,
	SPIRAL,
	RANDOM
);

STRINGY_ENUM( ProjectileStart,
	SELF,
	TARGET,
	SKY
);

STRINGY_ENUM( ParticleUse,
	WEATHER,
	FIRE,
	PROJECTILE,
	SPLASH,
	UNIT
);

// =====================================================
//	class Particle
// =====================================================

class Particle {
public:
	//attributes
	Vec3f pos;		// 12 bytes
	Vec3f lastPos;	// 12 bytes
	Vec3f speed;	// 12 bytes
	Vec3f accel;	// 12 bytes
	Vec4f color;	// 16 bytes // belongs in (or already is in) owner system's proto-type ?
	Vec4f color2;	// 16 bytes // belongs in (or already is in) owner system's proto-type ?
	float size;		// 4 bytes
	int energy;		// 4 bytes
					// 88 bytes

public:
	//get
	Vec3f getPos() const		{return pos;}
	Vec3f getLastPos() const	{return lastPos;}
	Vec3f getSpeed() const		{return speed;}
	Vec3f getAccel() const		{return accel;}
	Vec4f getColor() const		{return color;}
	Vec4f getColor2() const		{return color2;}
	float getSize() const		{return size;}
	int getEnergy()	const		{return energy;}

	MEMORY_CHECK_DECLARATIONS(Particle);
};

// =====================================================
//	class ParticleSystemType
// =====================================================
/// Particle systems base class, for instance-types and proto-types (hmmm...)
/** Defines the parameters for a particle system. */
class ParticleSystemBase {
protected:
	Random random;

	BlendFactor		srcBlendFactor;		// belongs only in proto-type
	BlendFactor		destBlendFactor;	// belongs only in proto-type
	BlendMode		blendEquationMode;	// belongs only in proto-type
	PrimitiveType	primitiveType;		// belongs only in proto-type

	Vec3f offset;
	Vec4f color;
	Vec4f color2;
	Vec4f colorNoEnergy;
	Vec4f color2NoEnergy;

	bool teamColorEnergy;
	bool teamColorNoEnergy;

	float size;
	float sizeNoEnergy;
	float speed;
	float gravity;
	
	float emissionRate;
	float emissionRateRemainder;
	int energy;
	int energyVar;
	
	float radius;
	int drawCount;

	Texture *texture;	// belongs only in proto-type
	Model *model;		// belongs only in proto-type

public:
	//constructor and destructor
	ParticleSystemBase();
	ParticleSystemBase(const ParticleSystemBase &model);
	virtual ~ParticleSystemBase(){}

	//get
	BlendFactor getSrcBlendFactor() const				{return srcBlendFactor;}
	BlendFactor getDestBlendFactor() const				{return destBlendFactor;}
	BlendMode getBlendEquationMode() const				{return blendEquationMode;}
	PrimitiveType getPrimitiveType() const				{return primitiveType;}
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
	float getEmissionRate() const						{return emissionRate;}
	int getEnergy() const								{return energy;}
	int getEnergyVar() const							{return energyVar;}
	float getRadius() const								{return radius;}
	int getDrawCount() const							{return drawCount;}

	//set
	void setSrcBlendFactor(BlendFactor v)				{srcBlendFactor = v;}
	void setDestBlendFactor(BlendFactor v)				{destBlendFactor = v;}
	void setBlendEquationMode(BlendMode v)				{blendEquationMode = v;}
	void setPrimitiveType(PrimitiveType v)				{primitiveType = v;}
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
	void setEmissionRate(float v)						{emissionRate = v;}
	void setEnergy(int v)								{energy = v;}
	void setEnergyVar(int v)							{energyVar = v;}
	void setRadius(float v)								{radius = v;}
	void setDrawCount(int v)							{drawCount = v;}

	int getRandEnergy() 				{return energy + random.randRange(-energyVar, energyVar);}

	virtual bool isProjectile() const	{ return false; }
};

// =====================================================
//	class ParticleSystem
// =====================================================
/// base class for particle system instances
class ParticleSystem : public ParticleSystemBase {
protected:
	enum State {
		sPause,		// No updates
		sPlay,
		sFade		// No new particles
	};

private:
	//static int idCounter;
	static int particleCounts[ParticleUse::COUNT];

	static void addParticleUse(ParticleUse use, int n);
	static void remParticleUse(ParticleUse use, int n);

public:
	static int getParticleUse(ParticleUse use) {
		assert(use >= 0 && use < ParticleUse::COUNT);
		return particleCounts[use];
	}

protected:
	//int id;
	ParticleUse use;
	int particleCount;
	Particle *particles;
	State state;
	bool active;
	bool visible;
	int aliveParticleCount;
	Vec3f pos;
	Vec3f windSpeed;
	float emissionRateRemainder;


public:
	//constructor and destructor
	ParticleSystem(int particleCount = 1000);
	ParticleSystem(const ParticleSystemBase &protoType, int particleCount = 1000);
	virtual ~ParticleSystem();

	void initArray(ParticleUse use);
	void freeArray();

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
	void setTeamColour(Vec3f teamColour);

	//misc
	void fade();

	bool anyParticle() const { return aliveParticleCount; }

	virtual bool isFinished() const { // to be overridden by GameParticleSystems
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
public:
	typedef list<ParticleSystem*> ParticleSystemList;

protected:
	ParticleSystemList particleSystems;

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
	// used to remove dead targets from target tracking projectiles, to be replaced by target's 
	// Unit::Died being connected to tracking proj's Projectile::onTargetDied().
	///@todo remove me, I'm dangerous.
	const ParticleSystemList& getList() const { return particleSystems; }
};

}}//end namespace

#endif

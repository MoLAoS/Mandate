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

#ifndef _GLEST_GAME_GAMECAMERA_H_
#define _GLEST_GAME_GAMECAMERA_H_

#include "vec.h"
#include "math_util.h"
#include "config.h"

#include <limits>

namespace Shared { namespace Xml {
	class XmlNode;
}}

using namespace Shared::Math;

using Shared::Xml::XmlNode;
using std::numeric_limits;

#define FLOATINFINITY numeric_limits<float>::infinity()

namespace Glest { namespace Gui {
using Global::Config;

// =====================================================
// 	class GameCamera
//
/// A basic camera that holds information about the game view
// =====================================================

class GameCamera{
public:
	static const float startingVAng;
	static const float startingHAng;
	static float vTransitionMult;
	static float hTransitionMult;
	static const float defaultHeight;
	static const float centerOffsetZ;
	static float moveScale;

public:
	WRAPPED_ENUM( State, GAME, SCENARIO );

private:
	Vec3f pos;
	Vec3f destPos;

	Vec2f ang;
	Vec2f destAng;

	float rotate;

	Vec3f moveMouse;
	Vec3f moveKey;

	State state;

	int limitX;
	int limitY;

	//config
	float speed;
	bool clampBounds;
	float maxRenderDistance;
	float maxHeight;
	float minHeight;
	float maxCameraDist;
	float minCameraDist;
	float minVAng;
	float maxVAng;
	float fov;

	//Scenario camera vectors
	// DO NOT set these directly from LUA, calculate them based on move time.
	Vec3f linearVelocity;
	Vec2f angularVelocity;
	// frame delay before moving for different effects.
	int linearDelay;
	int angularDelay;
	// keep track of how many frames the camera has been moving for.
	int currLinearFrame, totalLinearFrames;
	int currAngularFrame, totalAngularFrames;

public:
	GameCamera();

	void init(int limitX, int limitY);

	bool isMoving() {return moveMouse != Vec3f(0.f) || moveKey != Vec3f(0.f);}

	//get
	float getHAng() const		{return ang.y;};
	float getVAng() const		{return ang.x;}
	State getState() const		{return state;}
	const Vec3f &getPos() const	{return pos;}

	//set
	void setRotate(float rotate){this->rotate= rotate;}
	void setPos(Vec2f pos);
	void setAngles(float hAng, float vAng);
	void setDest(const Vec2i &pos, int height = -1, float hAngle = FLOATINFINITY, float vAngle = FLOATINFINITY);

	void setCameraMotion(const Vec2i &posit, const Vec2i &angle,
			int linearFrameCount = 40, int angularFrameCount = 40,
			int linearFrameDelay = 0, int angularFrameDelay = 0);

	void setMoveX(float f, bool mouse){
		if(mouse){
			this->moveMouse.x = f;
		}else{
			this->moveKey.x = f;
		}
	}
	void setMoveY(float f)		{this->moveMouse.y= f;}
	void setMoveZ(float f, bool mouse){
		if(mouse){
			this->moveMouse.z = f;
		}else{
			this->moveKey.z = f;
		}
	}

	void stop() {
		destPos = pos;
		destAng = ang;
	}

	//other
	/** Update the camera @return true if the camera moved or rotated this update */
	bool update();
	void reset(bool angle = true, bool height = true);
	void resetScenario();

	void centerXZ(float x, float z);
	void rotateHV(float h, float v);
	void transitionXYZ(float x, float y, float z);
	void transitionVH(float v, float h);
	void zoom(float dist);
	void moveForwardH(float dist, float response);	// response: 1.0 for immediate, 0 for full inertia
	void moveSideH(float dist, float response);

	void load(const XmlNode *node);
	void save(XmlNode *node) const;

private:
	void clampPosXYZ(float x1, float x2, float y1, float y2, float z1, float z2);
	void clampPosXZ(float x1, float x2, float z1, float z2);
	void clampAng();
	void moveUp(float dist);
};

}} //end namespace

#endif

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
#include "game_camera.h"

#include <cstdlib>

#include "config.h"
#include "game_constants.h"
#include "xml_parser.h"
#include "metrics.h"

#include "leak_dumper.h"

using std::numeric_limits;
using namespace Shared::Math;
using Shared::Xml::XmlNode;

namespace Glest { namespace Gui {
using Global::Metrics;

// =====================================================
// 	class GameCamera
// =====================================================

// ================== PUBLIC =====================

const float GameCamera::startingVAng= -60.f;
const float GameCamera::startingHAng= 0.f;
float GameCamera::vTransitionMult= 8.f;
float GameCamera::hTransitionMult= 8.f;
const float GameCamera::defaultHeight= 20.f;
const float GameCamera::centerOffsetZ= 8.0f;
float GameCamera::moveScale = 32.f;

// ================= Constructor =================

GameCamera::GameCamera()
		: pos(0.f, defaultHeight, 0.f)
		, destPos(0.f, defaultHeight, 0.f)
		, destAng(startingVAng, startingHAng) {
	state = State::GAME;

	// config
	speed = 15.f / GameConstants::cameraFps;
	clampBounds = !g_config.getUiPhotoMode();

	ang = Vec2f(startingVAng, startingHAng);

	rotate = 0;

	moveMouse = Vec3f(0.f);
	moveKey = Vec3f(0.f);

	maxRenderDistance = g_config.getRenderDistanceMax();
	maxHeight = g_config.getCameraMaxDistance();
	minHeight = g_config.getCameraMinDistance();
	maxCameraDist = g_config.getCameraMaxDistance();
	minCameraDist = g_config.getCameraMinDistance();

	minVAng = -g_config.getCameraMaxYaw();
	//maxVAng = -config.getCameraMinYaw(); // not compatible with SceneCuller, is set below.

	fov = g_config.getRenderFov();

	float vFov = fov / g_metrics.getAspectRatio();

	maxVAng = -(vFov / 2);
}

void GameCamera::init(int limitX, int limitY){
	this->limitX= limitX;
	this->limitY= limitY;
}

// ==================== Misc =====================

void GameCamera::setPos(Vec2f pos){
	this->pos= Vec3f(pos.x, this->pos.y, pos.y);
	clampPosXZ(0.0f, (float)limitX, 0.0f, (float)limitY);
	destPos.x = pos.x;
	destPos.z = pos.y;
}

void GameCamera::setAngles(float h, float v) {
	destAng.x = ang.x = v;
	destAng.y = ang.y = h;
	clampAng();
}

void GameCamera::setDest(const Vec2i &pos, int height, float hAngle, float vAngle) {
	destPos.x = float(pos.x);
	destPos.z = float(pos.y);
	if (height != -1) {
		destPos.y = clamp(float(height), minHeight, maxHeight);
	}
	if (vAngle != FLOATINFINITY) {
		destAng.x = vAngle;
	}
	if (hAngle != FLOATINFINITY) {
		destAng.y = hAngle;
	}
	clampAng();
}

void GameCamera::setCameraMotion(const Vec2i &posit, const Vec2i &angle,
		int linearFrameCount, int angularFrameCount,
		int linearFrameDelay, int angularFrameDelay) {
	resetScenario();
	destPos.x = float(posit.x);
	destPos.y = pos.y;
	destPos.z = float(posit.y);
	// To be consistent with setDest(..) swap x and y in the rotation.
	destAng.x = float(angle.y);
	
	float y = float(angle.x);
	while (y >= 360.f) y -= 360.f;
	while (y < 0.f) y += 360.f;
	destAng.y = y;

	totalLinearFrames = linearFrameCount;
	totalAngularFrames = angularFrameCount;
	linearDelay = linearFrameDelay;
	angularDelay = angularFrameDelay;
	linearVelocity = Vec3f(destPos - pos) / float(linearFrameCount);
	angularVelocity.x = float(destAng.x - ang.x) / angularFrameCount;
	y -= ang.y;
	if (y < 0.f) {
		y += 360.f;
	}
	if (y > 180.f) {
		y = -(360.f - y);
	}
	angularVelocity.y = y / angularFrameCount;
}

bool GameCamera::update() {
	Vec3f prevPos = pos;
	Vec2f prevAng = ang;

	Vec3f move = moveMouse + moveKey;

	// move XZ
	if (move.z) {
		moveForwardH(speed * move.z, 0.9f);
	}
	if (move.x) {
		moveSideH(speed * move.x, 0.9f);
	}

	if (state == State::SCENARIO) { // scenario doing something tricky,
		if (currLinearFrame < totalLinearFrames) {
			if (linearDelay == 0) {
				pos += linearVelocity;
				currLinearFrame++;
			} else {
				linearDelay--;
			}
		}
		if (currAngularFrame < totalAngularFrames) {
			if (angularDelay == 0) {
				ang.x += angularVelocity.y;
				ang.y += angularVelocity.x;
				currAngularFrame++;
			} else {
				angularDelay--;
			}
		}
		if (currAngularFrame >= totalAngularFrames && currLinearFrame >= totalLinearFrames) {
			state = State::GAME;
		}
	} else { // else game state
		if (fabs(rotate) == 1.f) {
			rotateHV(speed*5*rotate, 0);
		}
		if (move.y > 0.f) {
			moveUp(speed * move.y);
			if (clampBounds && pos.y < maxHeight) {
				rotateHV(0.f, -speed * 1.7f * move.y);
			}
		}
		if (move.y < 0.f) {
			moveUp(speed * move.y);
			if (clampBounds && pos.y > minHeight) {
				rotateHV(0.f, -speed * 1.7f * move.y);
			}
		}
		if (abs(destAng.x - ang.x) > 0.01f) {
			ang.x += (destAng.x - ang.x) / hTransitionMult;
		}
		if (abs(destAng.y - ang.y) > 0.01f) {
			if (abs(destAng.y - ang.y) > 180) {
				if (destAng.y > ang.y) {
					ang.y += (destAng.y - ang.y - 360) / vTransitionMult;
				} else {
					ang.y += (destAng.y - ang.y + 360) / vTransitionMult;
				}
			} else {
				ang.y += (destAng.y - ang.y) / vTransitionMult;
			}
		}
		if (abs(destPos.x - pos.x) > 0.01f) {
			pos.x += (destPos.x - pos.x) / moveScale;
		}
		if (abs(destPos.y - pos.y) > 0.01f) {
			pos.y += (destPos.y - pos.y) / moveScale;
		}
		if (abs(destPos.z - pos.z) > 0.01f) {
			pos.z += (destPos.z - pos.z) / moveScale;
		}
	}
	clampAng();

	if (clampBounds) {
		clampPosXYZ(0.0f, (float)limitX, minHeight, maxHeight, 0.0f, (float)limitY);
	}
	return (pos != prevPos || ang != prevAng);
}

void GameCamera::resetScenario() {
	linearVelocity = Vec3f(0.f);
	angularVelocity = Vec2f(0.f);
	linearDelay = 0;
	angularDelay = 0;
	currAngularFrame = 0;
	currLinearFrame = 0;
	totalAngularFrames = 0;
	totalLinearFrames = 0;
	state = State::SCENARIO;
}

void GameCamera::reset(bool angle, bool height) {
	if (angle) {
		destAng.x = startingVAng;
		destAng.y = startingHAng;
	}
	if (height) {
		destPos.y = defaultHeight;
	}
}

void GameCamera::centerXZ(float x, float z){
	destPos.x = pos.x= x;
	destPos.z = pos.z= z+centerOffsetZ;
}

void GameCamera::transitionXYZ(float x, float y, float z) {
	destPos.x += x;
	destPos.y += y;
	destPos.z += z;
	clampPosXYZ(0.0f, (float)limitX, minHeight, maxHeight, 0.0f, (float)limitY);
}

void GameCamera::transitionVH(float v, float h) {
	destAng.x += v;
	destPos.y -= v * destPos.y / 100.f;
	destAng.y += h;
	clampAng();
}

void GameCamera::zoom(float dist) {
	float flatDist = dist * cosf(degToRad(ang.x));
	Vec3f offset(flatDist * sinf(degToRad(ang.y)), dist * sinf(degToRad(ang.x)), flatDist * -cosf(degToRad(ang.y)));
	float mult = 1.f;
	if (destPos.y + offset.y < minHeight) {
		mult = abs((destPos.y - minHeight) / offset.y);
	} else if (destPos.y + offset.y > maxHeight) {
		mult = abs((maxHeight - destPos.y) / offset.y);
	}
	destPos += offset * mult;
}

void GameCamera::load(const XmlNode *node) {
	destPos = node->getChildVec3fValue("pos");
	destAng = node->getChildVec2fValue("angle");
}

void GameCamera::save(XmlNode *node) const {
	node->addChild("pos", pos);
	node->addChild("angle", ang);
}

// ==================== PRIVATE ====================

void GameCamera::clampPosXZ(float x1, float x2, float z1, float z2){
	if(pos.x < x1)		pos.x = x1;
	if(destPos.x < x1)	destPos.x = x1;
	if(pos.z < z1)		pos.z = z1;
	if(destPos.z < z1)	destPos.z = z1;
	if(pos.x > x2)		pos.x = x2;
	if(destPos.x > x2)	destPos.x = x2;
	if(pos.z > z2)		pos.z = z2;
	if(destPos.z > z2)	destPos.z = z2;
}

void GameCamera::clampPosXYZ(float x1, float x2, float y1, float y2, float z1, float z2){
	if(pos.x < x1)		pos.x = x1;
	if(destPos.x < x1)	destPos.x = x1;
	if(pos.y < y1)		pos.y = y1;
	if(destPos.y < y1)	destPos.y = y1;
	if(pos.z < z1)		pos.z = z1;
	if(destPos.z < z1)	destPos.z = z1;
	if(pos.x > x2)		pos.x = x2;
	if(destPos.x > x2)	destPos.x = x2;
	if(pos.y > y2)		pos.y = y2;
	if(destPos.y > y2)	destPos.y = y2;
	if(pos.z > z2)		pos.z = z2;
	if(destPos.z > z2)	destPos.z = z2;
}

void GameCamera::rotateHV(float h, float v){
	destAng.x = ang.x += v;
	destAng.y = ang.y += h;
	clampAng();
}

void GameCamera::clampAng() {
	if(ang.x > maxVAng)		ang.x = maxVAng;
	if(destAng.x > maxVAng)	destAng.x = maxVAng;
	if(ang.x < minVAng)		ang.x = minVAng;
	if(destAng.x < minVAng)	destAng.x = minVAng;
	if(ang.y > 360.f)		ang.y -= 360.f;
	if(destAng.y > 360.f)	destAng.y -= 360.f;
	if(ang.y < 0.f)			ang.y += 360.f;
	if(destAng.y < 0.f)		destAng.y = 360.f;
}

//move camera forwad but never change heightFactor
void GameCamera::moveForwardH(float d, float response) {
	Vec3f offset(sinf(degToRad(ang.y)) * d, 0.f, -cosf(degToRad(ang.y)) * d);
	destPos += offset;
	pos.x += offset.x * response;
	pos.z += offset.z * response;
}

//move camera to a side but never change heightFactor
void GameCamera::moveSideH(float d, float response){
	Vec3f offset(sinf(degToRad(ang.y+90)) * d, 0.f, -cosf(degToRad(ang.y+90)) * d);
	destPos += offset;
	pos.x += (destPos.x - pos.x) * response;
	pos.z += (destPos.z - pos.z) * response;
}

void GameCamera::moveUp(float d){
//	pos.y+= d;
	destPos.y += d;
}

}}//end namespace

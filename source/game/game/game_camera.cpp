// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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

namespace Glest { namespace Game {

// =====================================================
// 	class GameCamera
// =====================================================

// ================== PUBLIC =====================

const float GameCamera::startingVAng= -60.f;
const float GameCamera::startingHAng= 0.f;
const float GameCamera::vTransitionMult= 0.125f;
const float GameCamera::hTransitionMult= 0.125f;
const float GameCamera::defaultHeight= 20.f;
const float GameCamera::centerOffsetZ= 8.0f;

// ================= Constructor =================

GameCamera::GameCamera() : pos(0.f, defaultHeight, 0.f),
		destPos(0.f, defaultHeight, 0.f), destAng(startingVAng, startingHAng) {
	Config &config = Config::getInstance();
    state= sGame;

	//config
	speed= 15.f / GameConstants::cameraFps;
	clampBounds= !Config::getInstance().getUiPhotoMode();

	vAng= startingVAng;
    hAng= startingHAng;

    rotate=0;

	moveMouse= Vec3f(0.f);
	moveKey= Vec3f(0.f);

	maxRenderDistance = config.getRenderDistanceMax();
	maxHeight = config.getCameraMaxDistance();
	minHeight = config.getCameraMinDistance();
	maxCameraDist = config.getCameraMaxDistance();
	minCameraDist = config.getCameraMinDistance();
	
	minVAng = -config.getCameraMaxYaw();
	//maxVAng = -config.getCameraMinYaw();

	fov = config.getRenderFov();

	float vFov = fov / Metrics::getInstance().getAspectRatio();
	
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
	destAng.x = vAng = v;
	destAng.y = hAng = h;
	clampAng();
}

void GameCamera::setDest(const Vec2i &pos, int height, float hAngle, float vAngle) {
	destPos.x = float(pos.x);
	destPos.z = float(pos.y);
	if (height != -1) {
		destPos.y = clamp(float(height), minHeight, maxHeight);
	}
	if (vAngle != INFINITY) {
		destAng.x = vAngle;
	}
	if (hAngle != INFINITY) {
		destAng.y = hAngle;
	}
	clampAng();
}

void GameCamera::update() {
	Vec3f move = moveMouse + moveKey;

	//move XZ
	if(move.z){
        moveForwardH(speed * move.z, 0.9f);
	}
	if(move.x){
        moveSideH(speed * move.x, 0.9f);
	}

	//free state
	if(state==sFree){
		if(fabs(rotate) == 1.f){
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
	}

	//game state
	if(abs(destAng.x - vAng) > 0.01f) {
		vAng+= (destAng.x - vAng) * hTransitionMult;
	}
	if(abs(destAng.y - hAng) > 0.01f) {
		if(abs(destAng.y - hAng) > 180) {
			if(destAng.y > hAng) {
				hAng+= (destAng.y - hAng - 360) * vTransitionMult;
			} else {
				hAng+= (destAng.y - hAng + 360) * vTransitionMult;
			}
		} else {
			hAng+= (destAng.y - hAng) * vTransitionMult;
		}
	}
	const float move_scale = 32.f;
	if(abs(destPos.x - pos.x) > 0.01f) {
		pos.x += (destPos.x - pos.x) / move_scale;
	}
	if(abs(destPos.y - pos.y) > 0.01f) {
		pos.y += (destPos.y - pos.y) / move_scale;
	}
	if(abs(destPos.z - pos.z) > 0.01f) {
		pos.z += (destPos.z - pos.z) / move_scale;
	}

	clampAng();

	if(clampBounds){
		clampPosXYZ(0.0f, (float)limitX, minHeight, maxHeight, 0.0f, (float)limitY);
	}
}

void GameCamera::switchState(){
	if(state==sGame){
		state= sFree;
	}
	else{
		state= sGame;
		destAng.x = startingVAng;
		destAng.y = startingHAng;
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
	float flatDist = dist * cosf(degToRad(vAng));
	Vec3f offset(flatDist * sinf(degToRad(hAng)), dist * sinf(degToRad(vAng)), flatDist * -cosf(degToRad(hAng)));
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
	node->addChild("angle", Vec2f(vAng, hAng));
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
	destAng.x = vAng += v;
	destAng.y = hAng += h;
	clampAng();
}

void GameCamera::clampAng() {
	if(vAng > maxVAng)		vAng = maxVAng;
	if(destAng.x > maxVAng)	destAng.x = maxVAng;
	if(vAng < minVAng)		vAng = minVAng;
	if(destAng.x < minVAng)	destAng.x = minVAng;
	if(hAng > 360.f)		hAng -= 360.f;
	if(destAng.y > 360.f)	destAng.y -= 360.f;
	if(hAng < 0.f)			hAng += 360.f;
	if(destAng.y < 0.f)		destAng.y = 360.f;
}

//move camera forwad but never change heightFactor
void GameCamera::moveForwardH(float d, float response) {
	Vec3f offset(sinf(degToRad(hAng)) * d, 0.f, -cosf(degToRad(hAng)) * d);
	destPos += offset;
	pos.x += offset.x * response;
	pos.z += offset.z * response;
}

//move camera to a side but never change heightFactor
void GameCamera::moveSideH(float d, float response){
	Vec3f offset(sinf(degToRad(hAng+90)) * d, 0.f, -cosf(degToRad(hAng+90)) * d);
	destPos += offset;
	pos.x += (destPos.x - pos.x) * response;
	pos.z += (destPos.z - pos.z) * response;
}

void GameCamera::moveUp(float d){
//	pos.y+= d;
	destPos.y += d;
}

}}//end namespace

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
#include "menu_background.h"

#include <ctime>

#include "renderer.h"
#include "core_data.h"
#include "config.h"
#include "xml_parser.h"
#include "util.h"
#include "game_constants.h"
#include "logger.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Glest::Graphics;

namespace Glest { namespace Menu {
using Util::Logger;

// =====================================================
// 	class MenuBackground
// =====================================================

MenuBackground::MenuBackground(){

	Renderer &renderer= Renderer::getInstance();

	//load data

	XmlTree xmlTree;
	xmlTree.load("data/core/menu/menu.xml");
	const XmlNode *menuNode= xmlTree.getRootNode();

	//water
	const XmlNode *waterNode= menuNode->getChild("water");
	water= waterNode->getAttribute("value")->getBoolValue();
	if(water){
		waterHeight= waterNode->getAttribute("height")->getFloatValue();

		//water texture
		waterTexture= renderer.newTexture2D(ResourceScope::MENU);
		waterTexture->getPixmap()->init(4);
		waterTexture->getPixmap()->load("data/core/menu/textures/water.tga");
	}

	//fog
	const XmlNode *fogNode= menuNode->getChild("fog");
	fog= fogNode->getAttribute("value")->getBoolValue();
	if(fog){
		fogDensity= fogNode->getAttribute("density")->getFloatValue();
	}

	// rain
	rain = menuNode->getChild("rain")->getAttribute("value")->getBoolValue();
	if(rain){
		RainParticleSystem *rps= new RainParticleSystem();
		rps->setSpeed(12.f / WORLD_FPS);
		rps->setEmissionRate(25.f);
		rps->setWindSpeed2(-90.f, 4.f / WORLD_FPS);
		rps->setPos(Vec3f(0.f, 25.f, 0.f));
		rps->setColor(Vec4f(1.f, 1.f, 1.f, 0.2f));
		rps->setRadius(30.f);
		renderer.manageParticleSystem(rps, ResourceScope::MENU);

		for(int i=0; i<raindropCount; ++i){
			raindropStates[i]= random.randRange(0.f, 1.f);
			raindropPos[i]= computeRaindropPos();
		}
	} else if (menuNode->getOptionalBoolValue("snow")) {
		SnowParticleSystem *sps = new SnowParticleSystem(1200);
		sps->setSpeed(1.5f / WORLD_FPS);
		sps->setEmissionRate(2.f);
		sps->setWindSpeed2(-90.f, 0.5f / WORLD_FPS);
		sps->setPos(Vec3f(0.f, 25.f, 0.f));
		sps->setRadius(30.f);
		sps->setTexture(g_coreData.getSnowTexture());
		renderer.manageParticleSystem(sps, ResourceScope::MENU);
	}

	//camera
	const XmlNode *cameraNode= menuNode->getChild("camera");

	//position
	const XmlNode *positionNode= cameraNode->getChild("start-position");
	Vec3f startPosition;
    startPosition.x= positionNode->getAttribute("x")->getFloatValue();
	startPosition.y= positionNode->getAttribute("y")->getFloatValue();
	startPosition.z= positionNode->getAttribute("z")->getFloatValue();
	camera.setPosition(startPosition);

	//rotation
	const XmlNode *rotationNode= cameraNode->getChild("start-rotation");
	Vec3f startRotation;
    startRotation.x= rotationNode->getAttribute("x")->getFloatValue();
	startRotation.y= rotationNode->getAttribute("y")->getFloatValue();
	startRotation.z= rotationNode->getAttribute("z")->getFloatValue();
	camera.setOrientation(Quaternion(EulerAngles(
		degToRad(startRotation.x),
		degToRad(startRotation.y),
		degToRad(startRotation.z))));

	// load main model
	mainModel= renderer.newModel(ResourceScope::MENU);
	mainModel->load("data/core/menu/main_model/menu_main.g3d", 2, 2);

	// models
	for(int i=0; i<5; ++i){
		characterModels[i]= renderer.newModel(ResourceScope::MENU);
		try {
			characterModels[i]->load("data/core/menu/about_models/character"+intToStr(i)+".g3d", 2, 2);
		} catch (runtime_error &e) {
			g_logger.logError(e.what());
		}
	}

	// about position
	positionNode= cameraNode->getChild("about-position");
	aboutPosition.x= positionNode->getAttribute("x")->getFloatValue();
	aboutPosition.y= positionNode->getAttribute("y")->getFloatValue();
	aboutPosition.z= positionNode->getAttribute("z")->getFloatValue();
	rotationNode= cameraNode->getChild("about-rotation");

	targetCamera = 0;
	t = 0.f;
	fade = 0.f;
	anim = 0.f;
}

void MenuBackground::setTargetCamera(const Camera *targetCamera){
	this->targetCamera= targetCamera;
	this->lastCamera= camera;
	t= 0.f;
}

void MenuBackground::update(){
	// rain drops
	for (int i=0; i < raindropCount; ++i) {
		raindropStates[i] += 1.f / WORLD_FPS;
		if (raindropStates[i] >= 1.f) {
			raindropStates[i] = 0.f;
			raindropPos[i] = computeRaindropPos();
		}
	}

	// camera transition?
	if (targetCamera != 0) {
		t += ((0.1f + (1.f - t) / 10.f) / 20.f) * (40.f / WORLD_FPS);

		//interpolate position
		camera.setPosition(lastCamera.getPosition().lerp(t, targetCamera->getPosition()));

		//interpolate orientation
		Quaternion q= lastCamera.getOrientation().lerp(t, targetCamera->getOrientation());
		camera.setOrientation(q);

		if (t >= 1.f) {
			targetCamera = 0;
			t = 0.f;
		}
	}

	// fade-in
	if (fade < 1.f) {
		fade += 0.6f / WORLD_FPS;
		if (fade > 1.f) {
			fade = 1.f;
		}
	}

	// animation
	anim += (0.6f / WORLD_FPS) / 5 + random.randRange(0.f, (0.6f / WORLD_FPS) / 5.f);
	if (anim > 1.f) {
		anim = 0.f;
	}
}

Vec2f MenuBackground::computeRaindropPos(){
	float f= static_cast<float>(meshSize);
	return Vec2f(random.randRange(-f, f), random.randRange(-f, f));
}

}}//end namespace


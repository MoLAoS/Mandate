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
#include "particle_type.h"

#include "util.h"
#include "core_data.h"
#include "xml_parser.h"
#include "renderer.h"
#include "config.h"
#include "game_constants.h"

#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class ParticleSystemType
// =====================================================

ParticleSystemType::ParticleSystemType() {
}

void ParticleSystemType::load(const XmlNode *particleSystemNode, const string &dir) {

	Renderer &renderer = Renderer::getInstance();

	const XmlNode *blendFuncNode = particleSystemNode->getChild("blend-func", 0, false);
	if(blendFuncNode) {
		srcBlendFactor = Particle::getBlendFactor(blendFuncNode->getRestrictedAttribute("src"));
		destBlendFactor = Particle::getBlendFactor(blendFuncNode->getRestrictedAttribute("dest"));
	}

	const XmlNode *blendEquationNode = particleSystemNode->getChild("blend-equation", 0, false);
	if(blendEquationNode) {
		blendEquationMode = Particle::getBlendEquation(blendEquationNode->getRestrictedAttribute("mode"));
	}

	//texture
	const XmlNode *textureNode = particleSystemNode->getChild("texture");
    //texture enabled
	if (textureNode->getAttribute("value")->getBoolValue()) {
		Texture2D *texture = renderer.newTexture2D(rsGame);
		if (textureNode->getAttribute("luminance")->getBoolValue()) {
			texture->setFormat(Texture::fAlpha);
			texture->getPixmap()->init(1);
		} else {
			texture->getPixmap()->init(4);
		}
		texture->load(dir + "/" + textureNode->getAttribute("path")->getRestrictedValue());
		this->texture = texture;
	} else {
		texture = NULL;
	}

	//model
	const XmlNode *modelNode = particleSystemNode->getChild("model");
    //model enabled
	if (modelNode->getAttribute("value")->getBoolValue()) {
		string path = modelNode->getAttribute("path")->getRestrictedValue();
		model = renderer.newModel(rsGame);
		model->load(dir + "/" + path);
	} else {
		model = NULL;
	}

	//primitive type
	string primativeTypeStr = particleSystemNode->getChildRestrictedValue("primitive");
	if(primativeTypeStr == "quad") {
		primitiveType = Particle::ptQuad;
	} else 	if(primativeTypeStr == "line") {
		primitiveType = Particle::ptLine;
	} else {
		throw runtime_error("Invalid primitive type: " + primativeTypeStr);
	}

	//offset
	offset = particleSystemNode->getChildVec3fValue("offset");

	//color
	color = particleSystemNode->getChildColor4Value("color");

	//color2
	const XmlNode *color2Node = particleSystemNode->getChild("color2", 0, false);
	if(color2Node) {
		color2 = color2Node->getColor4Value();
	} else {
		color2 = color;
	}

	//color no energy
	colorNoEnergy = particleSystemNode->getChildColor4Value("color-no-energy");

	//color2 no energy
	const XmlNode *color2NoEnergyNode = particleSystemNode->getChild("color2-no-energy", 0, false);
	if(color2NoEnergyNode) {
		color2NoEnergy = color2NoEnergyNode->getColor4Value();
	} else {
		color2NoEnergy = colorNoEnergy;
	}

	//size
	size = particleSystemNode->getChildFloatValue("size");

	//sizeNoEnergy
	sizeNoEnergy = particleSystemNode->getChildFloatValue("size-no-energy");

	//speed
	speed = particleSystemNode->getChildFloatValue("speed") / Config::getInstance().getGsWorldUpdateFps();

	//gravity
	gravity= particleSystemNode->getChildFloatValue("gravity") / Config::getInstance().getGsWorldUpdateFps();

	//emission rate
	emissionRate = particleSystemNode->getChildIntValue("emission-rate");

	//energy
	energy = particleSystemNode->getChildIntValue("energy-max");

	//energy
	energyVar = particleSystemNode->getChildIntValue("energy-var");

	//speed
	energyVar= particleSystemNode->getChildIntValue("energy-var");

	//draw count
	drawCount = particleSystemNode->getOptionalIntValue("draw-count", 1);

	//inner size
	//innerSize = particleSystemNode->getChildFloatValue("inner-size");

}

/*
void ParticleSystemType::setValues(AttackParticleSystem *ats){
	ats->setTexture(texture);
	ats->setPrimitive(AttackParticleSystem::strToPrimitive(primitive));
	ats->setOffset(offset);
	ats->setColor(color);
	ats->setColorNoEnergy(colorNoEnergy);
	ats->setSpeed(speed);
	ats->setGravity(gravity);
	ats->setParticleSize(size);
	ats->setSizeNoEnergy(sizeNoEnergy);
	ats->setEmissionRate(emissionRate);
	ats->setMaxParticleEnergy(energyMax);
	ats->setVarParticleEnergy(energyVar);
	ats->setModel(model);
}*/

// ===========================================================
//	class ParticleSystemTypeProjectile
// ===========================================================

void ParticleSystemTypeProjectile::load(const string &dir, const string &path){

	try {
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *particleSystemNode = xmlTree.getRootNode();

		ParticleSystemType::load(particleSystemNode, dir);

		//trajectory values
		const XmlNode *tajectoryNode = particleSystemNode->getChild("trajectory");
		trajectory = tajectoryNode->getAttribute("type")->getRestrictedValue();

		//trajectory speed
		trajectorySpeed = tajectoryNode->getChildFloatValue("speed") / Config::getInstance().getGsWorldUpdateFps();

		if(trajectory == "parabolic" || trajectory == "spiral" || trajectory == "random") {
			//trajectory scale
			trajectoryScale = tajectoryNode->getChildFloatValue("scale");
		} else {
			trajectoryScale = 1.0f;
		}

		if(trajectory == "spiral") {
			//trajectory frequency
			trajectoryFrequency = tajectoryNode->getChildFloatValue("frequency");
		} else {
			trajectoryFrequency = 1.0f;
		}

		// projectile start
		const XmlNode *startNode = tajectoryNode->getChild("start", 0, false);
		if(startNode) {
			string name = startNode->getStringAttribute("value");

			if(name == "self") {
				start = psSelf;
			} else if(name == "target") {
				start = psTarget;
			} else if(name == "sky") {
				start = psSky;
			}
		} else {
			start = psSelf;
		}

		const XmlNode *trackingNode = tajectoryNode->getChild("tracking", 0, false);
		tracking = trackingNode && trackingNode->getBoolAttribute("value");

	} catch(const exception &e) {
		throw runtime_error("Error loading ParticleSystem: " + path + "\n" + e.what());
	}
}

ParticleSystem *ParticleSystemTypeProjectile::create() {
	ProjectileParticleSystem *ps = new ProjectileParticleSystem(*this);

	ps->setTrajectory(ProjectileParticleSystem::strToTrajectory(trajectory));
	ps->setTrajectorySpeed(trajectorySpeed);
	ps->setTrajectoryScale(trajectoryScale);
	ps->setTrajectoryFrequency(trajectoryFrequency);
	
	return ps;
}

// ===========================================================
//	class ParticleSystemTypeSplash
// ===========================================================

void ParticleSystemTypeSplash::load(const string &dir, const string &path){

	try{
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *particleSystemNode= xmlTree.getRootNode();

		ParticleSystemType::load(particleSystemNode, dir);

		//emission rate fade
		const XmlNode *emissionRateFadeNode= particleSystemNode->getChild("emission-rate-fade");
		emissionRateFade= emissionRateFadeNode->getAttribute("value")->getIntValue();

		//spread values
		const XmlNode *verticalSpreadNode= particleSystemNode->getChild("vertical-spread");
		verticalSpreadA= verticalSpreadNode->getAttribute("a")->getFloatValue(0.0f, 1.0f);
		verticalSpreadB= verticalSpreadNode->getAttribute("b")->getFloatValue(-1.0f, 1.0f);

		const XmlNode *horizontalSpreadNode= particleSystemNode->getChild("horizontal-spread");
		horizontalSpreadA= horizontalSpreadNode->getAttribute("a")->getFloatValue(0.0f, 1.0f);
		horizontalSpreadB= horizontalSpreadNode->getAttribute("b")->getFloatValue(-1.0f, 1.0f);
	}
	catch(const exception &e){
		throw runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

ParticleSystem *ParticleSystemTypeSplash::create(){
	SplashParticleSystem *ps =  new SplashParticleSystem(*this);

	ps->setEmissionRateFade(emissionRateFade);
	ps->setVerticalSpreadA(verticalSpreadA);
	ps->setVerticalSpreadB(verticalSpreadB);
	ps->setHorizontalSpreadA(horizontalSpreadA);
	ps->setHorizontalSpreadB(horizontalSpreadB);

	return ps;
}

}}//end mamespace

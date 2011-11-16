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
#include "logger.h"
#include "world.h"
#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes {
using Util::Logger;

// =====================================================
// 	class ParticleSystemType
// =====================================================

ParticleSystemType::ParticleSystemType() {
}

void ParticleSystemType::load(const XmlNode *particleSystemNode, const string &dir) {
	Renderer &renderer = Renderer::getInstance();

	// Blend functions
	const XmlNode *blendFuncNode = particleSystemNode->getChild("blend-func", 0, false);
	if(blendFuncNode) {
		string s = blendFuncNode->getRestrictedAttribute("src");
		srcBlendFactor = BlendFactorNames.match(s.c_str());
		if (srcBlendFactor == BlendFactor::INVALID) {
			throw runtime_error("'" + s + "' is not a valid Blend Funtion");
		}
		s = blendFuncNode->getRestrictedAttribute("dest");
		destBlendFactor = BlendFactorNames.match(s.c_str());
		if (destBlendFactor == BlendFactor::INVALID) {
			throw runtime_error("'" + s + "' is not a valid Blend Funtion");
		}
	}
	// Blend mode
	const XmlNode *blendEquationNode = particleSystemNode->getChild("blend-equation", 0, false);
	if(blendEquationNode) {
		string s = blendEquationNode->getRestrictedAttribute("mode");
		blendEquationMode = BlendModeNames.match(s.c_str());
		if (blendEquationMode == BlendMode::INVALID) {
			throw runtime_error("'" + s + "' is not a valid Blend Mode");
		}
	}

	// texture
	const XmlNode *textureNode = particleSystemNode->getChild("texture");
	if (textureNode->getAttribute("value")->getBoolValue()) {
		Texture2D *texture = renderer.newTexture2D(ResourceScope::GAME);
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

	// model
	const XmlNode *modelNode = particleSystemNode->getOptionalChild("model");
    if (modelNode && modelNode->getAttribute("value")->getBoolValue()) {
		string path = modelNode->getAttribute("path")->getRestrictedValue();
		model = renderer.newModel(ResourceScope::GAME);
		try {
			model->load(dir + "/" + path, 1, 1);
		} catch (runtime_error &e) {
			g_logger.logError(e.what());
		}
	} else {
		model = NULL;
	}

	// primitive type
	string pts = particleSystemNode->getChildRestrictedValue("primitive");
	primitiveType = PrimitiveTypeNames.match(pts.c_str());
	if (primitiveType == PrimitiveType::INVALID) {
		throw runtime_error("'" + pts + "' is not a valid Primtive Type");
	}

	// offset
	offset = particleSystemNode->getChildVec3fValue("offset");

	// color
	color = particleSystemNode->getChildColor4Value("color");

	// color2
	const XmlNode *color2Node = particleSystemNode->getChild("color2", 0, false);
	if(color2Node) {
		color2 = color2Node->getColor4Value();
	} else {
		color2 = color;
	}

	// color no energy
	colorNoEnergy = particleSystemNode->getChildColor4Value("color-no-energy");

	// color2 no energy
	const XmlNode *color2NoEnergyNode = particleSystemNode->getChild("color2-no-energy", 0, false);
	if(color2NoEnergyNode) {
		color2NoEnergy = color2NoEnergyNode->getColor4Value();
	} else {
		color2NoEnergy = colorNoEnergy;
	}

	teamColorEnergy = particleSystemNode->getOptionalBoolValue("teamcolorEnergy");
	teamColorNoEnergy  = particleSystemNode->getOptionalBoolValue("teamcolorNoEnergy");

	// size
	size = particleSystemNode->getChildFloatValue("size");

	// sizeNoEnergy
	sizeNoEnergy = particleSystemNode->getChildFloatValue("size-no-energy");

	// speed
	speed = particleSystemNode->getChildFloatValue("speed") / WORLD_FPS;

	// gravity
	gravity= particleSystemNode->getChildFloatValue("gravity") / WORLD_FPS;

	// emission rate
	emissionRate = particleSystemNode->getChildFloatValue("emission-rate");

	// energy
	energy = particleSystemNode->getChildIntValue("energy-max");

	// energy
	energyVar = particleSystemNode->getChildIntValue("energy-var");

	// draw count
	drawCount = particleSystemNode->getOptionalIntValue("draw-count", 1);

	//inner size
	//innerSize = particleSystemNode->getChildFloatValue("inner-size");

}

// ===========================================================
//	class ProjectileType
// ===========================================================

void ProjectileType::load(const string &dir, const string &path){

	try {
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *particleSystemNode = xmlTree.getRootNode();

		ParticleSystemType::load(particleSystemNode, dir);

		//trajectory values
		const XmlNode *trajectoryNode = particleSystemNode->getChild("trajectory");
		string ts = trajectoryNode->getAttribute("type")->getRestrictedValue();

		trajectory = TrajectoryTypeNames.match(ts.c_str());
		if (trajectory == TrajectoryType::INVALID) {
			throw runtime_error("'" + ts + "' is not a valid Trajectory Type");
		}

		//trajectory speed
		trajectorySpeed = trajectoryNode->getChildFloatValue("speed") / float(WORLD_FPS);

		if (trajectory == TrajectoryType::PARABOLIC || trajectory == TrajectoryType::SPIRAL
		|| trajectory == TrajectoryType::RANDOM) {
			//trajectory scale
			trajectoryScale = trajectoryNode->getChildFloatValue("scale");
		} else {
			trajectoryScale = 1.0f;
		}

		if (trajectory == TrajectoryType::SPIRAL) {
			//trajectory frequency
			trajectoryFrequency = trajectoryNode->getChildFloatValue("frequency");
		} else {
			trajectoryFrequency = 1.0f;
		}

		// projectile start
		const XmlNode *startNode = trajectoryNode->getChild("start", 0, false);
		if(startNode) {
			string s = startNode->getStringAttribute("value");
			start = ProjectileStartNames.match(s.c_str());
			if (start == ProjectileStart::INVALID) {
				throw runtime_error("'" + s + "' is not a valid Project Start Value");
			}
		} else {
			start = ProjectileStart::SELF;
		}

		const XmlNode *trackingNode = trajectoryNode->getChild("tracking", 0, false);
		tracking = trackingNode && trackingNode->getBoolAttribute("value");

	} catch (const std::exception &e) {
		throw runtime_error("Error loading ParticleSystem: " + path + "\n" + e.what());
	}
}

ParticleSystem *ProjectileType::create(bool vis) {
	Projectile *ps = g_world.newProjectile(vis, *this);

	ps->setTrajectory(trajectory);
	ps->setTrajectorySpeed(trajectorySpeed);
	ps->setTrajectoryScale(trajectoryScale);
	ps->setTrajectoryFrequency(trajectoryFrequency);

	return ps;
}

// ===========================================================
//	class SplashType
// ===========================================================

void SplashType::load(const string &dir, const string &path) {
	try {
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *particleSystemNode= xmlTree.getRootNode();

		ParticleSystemType::load(particleSystemNode, dir);

		//emission rate fade
		emissionRateFade = particleSystemNode->getChildFloatValue("emission-rate-fade");

		//spread values
		const XmlNode *verticalSpreadNode= particleSystemNode->getChild("vertical-spread");
		verticalSpreadA= verticalSpreadNode->getAttribute("a")->getFloatValue(0.0f, 1.0f);
		verticalSpreadB= verticalSpreadNode->getAttribute("b")->getFloatValue(-1.0f, 1.0f);

		const XmlNode *horizontalSpreadNode= particleSystemNode->getChild("horizontal-spread");
		horizontalSpreadA= horizontalSpreadNode->getAttribute("a")->getFloatValue(0.0f, 1.0f);
		horizontalSpreadB= horizontalSpreadNode->getAttribute("b")->getFloatValue(-1.0f, 1.0f);

	} catch(const std::exception &e) {
		throw runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

ParticleSystem *SplashType::create(bool vis) {
	Splash *ps = new Splash(vis, *this);

	ps->setEmissionRateFade(emissionRateFade);
	ps->setVerticalSpreadA(verticalSpreadA);
	ps->setVerticalSpreadB(verticalSpreadB);
	ps->setHorizontalSpreadA(horizontalSpreadA);
	ps->setHorizontalSpreadB(horizontalSpreadB);

	return ps;
}

// =====================================================
// 	class UnitParticleSystemType
// =====================================================

UnitParticleSystemType::UnitParticleSystemType() {
}

void UnitParticleSystemType::load(const XmlNode *particleSystemNode, const string &path) {
	try {
		ParticleSystemType::load(particleSystemNode, path);

		direction = particleSystemNode->getChildVec3fValue("direction");
		
		relative = particleSystemNode->getChildBoolValue("relative");
		relativeDirection = particleSystemNode->getOptionalBoolValue("relativeDirection");
		fixed = particleSystemNode->getChildBoolValue("fixed");
		teamColorNoEnergy = particleSystemNode->getOptionalBoolValue("teamcolorNoEnergy", false);
		teamColorEnergy = particleSystemNode->getOptionalBoolValue("teamcolorEnergy", false);
		maxParticles = particleSystemNode->getOptionalIntValue("max-particles", 200);
		if (maxParticles > 200) {
			maxParticles = 200;
		}
		radius = particleSystemNode->getOptionalFloatValue("radius", 0.5f);

		string mode = particleSystemNode->getOptionalRestrictedValue("mode", "normal");
		if (mode == "black") {
			setDestBlendFactor(BlendFactor::ONE_MINUS_SRC_ALPHA);
		}
	} catch (const std::exception &e) {
		throw runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

void UnitParticleSystemType::load(const string &dir, const string &path) {
	try {
		XmlTree xmlTree;
		xmlTree.load(path);
		load(xmlTree.getRootNode(), dir);
	} catch (const std::exception &e) {
		throw runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

}}//end mamespace

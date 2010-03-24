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
#include "time_flow.h"

#include "sound_renderer.h"
#include "config.h"
#include "game_constants.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class TimeFlow
// =====================================================

const float TimeFlow::dusk= 18.f;
const float TimeFlow::dawn= 6.f;

void TimeFlow::init(Tileset *tileset){
	firstTime= true;
	this->tileset= tileset;
	time= dawn+1.5f;
	lastTime= time;
	Config &config= Config::getInstance();
	timeInc= 24.f*(1.f/config.getGsDayTime())/Config::getInstance().getGsWorldUpdateFps();
}

void TimeFlow::update(){

	//update time
	time+= isDay()? timeInc: timeInc*2;
	if(time>24.f){
		time-= 24.f;
	}

	//sounds
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	AmbientSounds *ambientSounds= tileset->getAmbientSounds();

	//day
	if(lastTime<dawn && time>=dawn){
		soundRenderer.stopAmbient(ambientSounds->getNight());
	}

	if((lastTime<dawn && time>=dawn) || firstTime){

		//day sound
		if(ambientSounds->isEnabledDayStart() && !firstTime){
			soundRenderer.playFx(ambientSounds->getDayStart());
		}
		if(ambientSounds->isEnabledDay()){
			if(ambientSounds->getAlwaysPlayDay() || tileset->getWeather()==Weather::SUNNY){
				soundRenderer.playAmbient(ambientSounds->getDay());
			}
		}
		firstTime= false;
	}

	//night
	if(lastTime<dusk && time>=dusk){
		soundRenderer.stopAmbient(ambientSounds->getDay());
	}

	if(lastTime<dusk && time>=dusk){
		//night
		if(ambientSounds->isEnabledNightStart()){
			soundRenderer.playFx(ambientSounds->getNightStart());
		}
		if(ambientSounds->isEnabledNight()){
			if(ambientSounds->getAlwaysPlayNight() || tileset->getWeather()==Weather::SUNNY){
				soundRenderer.playAmbient(ambientSounds->getNight());
			}
		}
	}
	lastTime= time;
}

void TimeFlow::save(XmlNode *node) const {
	node->addChild("firstTime", firstTime);
	node->addChild("time", time);
	node->addChild("lastTime", lastTime);
	node->addChild("timeInc", timeInc);
}

void TimeFlow::load(const XmlNode *node) {
	firstTime = node->getChildBoolValue("firstTime");
	time = node->getChildFloatValue("time");
	lastTime = node->getChildFloatValue("lastTime");
	timeInc = node->getChildFloatValue("timeInc");
}


bool TimeFlow::isAproxTime(float time){
	return (this->time>=time) && (this->time<time+timeInc);
}

}}//end namespace

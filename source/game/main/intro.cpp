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
#include "intro.h"

#include "main_menu.h"
#include "util.h"
#include "game_util.h"
#include "config.h"
#include "program.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "metrics.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest { namespace Main {

// =====================================================
// 	class Intro
// =====================================================

Intro::Intro(Program &program) : ProgramState(program) {
	CoreData &coreData = CoreData::getInstance();
	Lang &lang = Lang::getInstance();
	const Metrics &metrics = Metrics::getInstance();
	timer=0;

	Vec2i screenSize = metrics.getScreenDims();
	Vec2i logoSize(512, 256);
	Vec2i pos(screenSize.x / 2 - logoSize.x / 2, screenSize.y / 2 - logoSize.y / 2);
	
	logoPanel = new Widgets::PicturePanel(&program, pos, logoSize);
	logoPanel->setPaddingParams(0,0);
//	logoPanel->setBorderSize(0);
	logoPanel->setImage(coreData.getLogoTexture());
	logoPanel->setAutoLayout(false);

	Font *font = coreData.getGAEFontBig();
	lblAdvanced = new Widgets::StaticText(logoPanel);
	lblAdvanced->setTextParams(lang.get("Advanced"), Vec4f(1.f), font);
	Vec2i sz = lblAdvanced->getTextDimensions() + Vec2i(10, 5);
	lblAdvanced->setPos(Vec2i(255 - sz.x, 60));
	lblAdvanced->setSize(sz);
	lblAdvanced->centreText();

	lblEngine = new Widgets::StaticText(logoPanel);
	lblEngine->setTextParams(lang.get("Engine"), Vec4f(1.f), font);
	lblEngine->setPos(Vec2i(285, 60));
	lblEngine->setSize(lblEngine->getTextDimensions() + Vec2i(10,5));
	lblEngine->centreText();

	// Version label
	font = coreData.getGAEFontSmall();
	pos = Vec2i(285 + lblEngine->getSize().x, 62);
	lblVersion = new Widgets::StaticText(logoPanel);
	lblVersion->setTextParams(gaeVersionString, Vec4f(1.f), font);
	lblVersion->setPos(pos);
	lblVersion->setSize(lblVersion->getTextDimensions() + Vec2i(10,5));
	lblVersion->centreText();

	lblWebsite = 0;

	// make everything invisible
	program.setFade(0.f);

	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.playMusic(CoreData::getInstance().getIntroMusic());
}

void Intro::update(){
	timer++;
	if (timer <= 300) {
		float fade = timer / 300.f;
		logoPanel->setFade(fade);
	} else if (timer <= 600) {
		// fade == 1.f, do nothing
	} else if (timer <= 800) {
		float fade = 1.f - (timer - 600) / 200.f;
		logoPanel->setFade(fade);

	} else if (timer == 801) {
		CoreData &coreData= CoreData::getInstance();
		const Metrics &metrics= Metrics::getInstance();
		program.clear();
		lblWebsite = new Widgets::StaticText(&program);
//		lblWebsite->setBorderParams(Widgets::BorderStyle::SOLID, 2, Vec3f(1.f), 0.5f);
		lblWebsite->setTextParams("www.glest.org", Vec4f(1.f), coreData.getGAEFontSmall());
		lblWebsite->setSize(lblWebsite->getTextDimensions() + Vec2i(10, 5));
		lblWebsite->setPos(metrics.getScreenDims() / 2 - lblWebsite->getSize() / 2);
		lblWebsite->centreText();
		lblWebsite->setFade(0.f);
	} else if (timer <= 1101) {
		float fade = (timer - 801) / 300.f;
		lblWebsite->setFade(fade);
	} else if (timer <= 1401) {
	} else if (timer <= 1601) {
		float fade = 1.f - (timer - 1401) / 200.f;
		lblWebsite->setFade(fade);
	} else {
		program.clear();
		program.setState(new MainMenu(program));
	}
}

void Intro::renderBg(){
	Renderer &renderer= Renderer::getInstance();
	renderer.reset2d();
	renderer.clearBuffers();
}

void Intro::renderFg() {
	Renderer &renderer= Renderer::getInstance();
	renderer.swapBuffers();
}

void Intro::keyDown(const Key &key){
	if(!key.isModifier()) {
		mouseUpLeft(0, 0);
	}
}

void Intro::mouseUpLeft(int x, int y){
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.stopMusic(CoreData::getInstance().getIntroMusic());
	soundRenderer.playMusic(CoreData::getInstance().getMenuMusic());
	program.clear();
	program.setState(new MainMenu(program));
}

}}//end namespace

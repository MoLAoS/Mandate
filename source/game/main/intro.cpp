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

Intro::Intro(Program &program)
		: ProgramState(program)
		, isTotalConversion(false)
		, gaeSplashOnRootMenu(false) {
	loadXml();
	CoreData &coreData = CoreData::getInstance();
	Lang &lang = Lang::getInstance();
	const Metrics &metrics = Metrics::getInstance();
	timer=0;

	Vec2i screenSize = metrics.getScreenDims();
	Vec2i logoSize(512, 256);
	Vec2i pos;
	
	if (!isTotalConversion) {
		pos = Vec2i(screenSize.x / 2 - logoSize.x / 2, screenSize.y / 2 - logoSize.y / 2);
	} else {
		pos = Vec2i(screenSize.x / 2 - logoSize.x / 2, screenSize.y / 3 * 2 - logoSize.y / 2);
	}	
	logoPanel = new Widgets::PicturePanel(&program, pos, logoSize);
	logoPanel->setImage(coreData.getLogoTexture());

	if (!isTotalConversion) {
		const Font *fontPtr = g_widgetConfig.getTitleFont();

		lblAdvanced = new Widgets::StaticText(logoPanel);
		lblAdvanced->textStyle().m_fontIndex = g_widgetConfig.getTitleFontNdx();
		lblAdvanced->setText(lang.get("AdvEng1"));
		Vec2i sz = lblAdvanced->getTextDimensions() + Vec2i(10, 5);
		lblAdvanced->setPos(Vec2i(255 - sz.w, 256 - 60 - sz.h));
		lblAdvanced->setSize(sz);
		lblAdvanced->alignText();

		lblEngine = new Widgets::StaticText(logoPanel);
		lblEngine->textStyle().m_fontIndex = g_widgetConfig.getTitleFontNdx();
		lblEngine->setText(lang.get("AdvEng2"));
		lblEngine->setPos(Vec2i(285, 256 - 60 - sz.h));
		lblEngine->setSize(lblEngine->getTextDimensions() + Vec2i(10,5));
		lblEngine->alignText();

		// Version label
		pos = Vec2i(285 + lblEngine->getSize().x, 256 - 60 - sz.h - 2);
		lblVersion = new Widgets::StaticText(logoPanel);
		lblVersion->textStyle().m_fontIndex = g_widgetConfig.getTitleFontNdx();
		lblVersion->setText(gaeVersionString);
		lblVersion->setPos(pos);
		lblVersion->setSize(lblVersion->getTextDimensions() + Vec2i(10,5));
		lblVersion->alignText();
	} else {
		lblEngine = new Widgets::StaticText(&program);
		Vec2i pos, size;
		lblEngine->setText("Glest Advanced Engine " + gaeVersionString);
		size = lblEngine->getTextDimensions() + Vec2i(5,5);
		pos = Vec2i(g_metrics.getScreenW() - size.w - 15, 10 + size.h);
		lblEngine->setPos(pos);
		lblEngine->setSize(size);

		size = Vec2i(256, 128);
		pos = Vec2i(screenSize.w / 2 - size.w / 2, screenSize.h / 3 - size.h / 2);
		splashImage = new Widgets::StaticImage(&program, pos, size);
		splashImage->setImage(coreData.getGaeSplashTexture());
	}

	lblWebsite = 0;

	// make everything invisible
	program.setFade(0.f);

	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.playMusic(CoreData::getInstance().getIntroMusic());
}

void Intro::update(){
	const int fadeTime = 250;
	timer++;
	if (timer <= fadeTime) {
		float fade = timer / float(fadeTime);
		program.setFade(fade);
	} else if (timer <= fadeTime * 2) {
		// fade == 1.f, do nothing
	} else if (timer <= fadeTime * 3) {
		float fade = 1.f - (timer - fadeTime * 2) / float(fadeTime);
		program.setFade(fade);
	} else if (timer == fadeTime * 3 + 1) {
		CoreData &coreData= CoreData::getInstance();
		const Metrics &metrics= Metrics::getInstance();
		program.clear();
		lblWebsite = new Widgets::StaticText(&program);
		int font = g_widgetConfig.getDefaultFontIndex(FontUsage::TITLE);
		int white = g_widgetConfig.getColourIndex(Colour(255u));
		lblWebsite->setText("www.glest.org");
		lblWebsite->setSize(lblWebsite->getTextDimensions() + Vec2i(10, 5));
		lblWebsite->setPos(metrics.getScreenDims() / 2 - lblWebsite->getSize() / 2);
		lblWebsite->alignText();
		program.setFade(0.f);
	} else if (timer <= fadeTime * 4) {
		float fade = (timer - fadeTime * 3) / float(fadeTime);
		program.setFade(fade);
	} else if (timer <= fadeTime * 5) {
	} else if (timer <= fadeTime * 6) {
		float fade = 1.f - (timer - fadeTime * 5) / float(fadeTime);
		program.setFade(fade);
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

void Intro::loadXml() {
	//camera
	XmlTree xmlTree;
	xmlTree.load("data/core/menu/menu.xml");
	const XmlNode *menuNode = xmlTree.getRootNode();

	const XmlNode *logoNode = menuNode->getOptionalChild("logos");
	if (logoNode) {
		isTotalConversion = logoNode->getOptionalBoolValue("total-conversion", false);
		gaeSplashOnRootMenu = logoNode->getOptionalBoolValue("gae-logo", isTotalConversion);
	}
}

}}//end namespace

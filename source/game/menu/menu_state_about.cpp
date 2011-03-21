// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "menu_state_about.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "menu_state_options.h"
#include "game_util.h"
#include "ticker_tape.h"

#include "leak_dumper.h"

namespace Glest { namespace Menu {
using namespace Util;

// =====================================================
// 	class GlestInfoWidget
// =====================================================

class GlestInfoWidget : public Widget, public TextWidget {
private:
	vector<Vec2f> m_startPositions;
	vector<Vec2f> m_endPositions;
	int           m_counter;

	static const int transitionTime = 180; // 1.5 seconds

public:
	GlestInfoWidget(Container *parent);

	void start();

	virtual void render() override;
	virtual void update() override;
	virtual string descType() const override { return "GlestInfoWidget"; }
};

GlestInfoWidget::GlestInfoWidget(Container *parent)
		: Widget(parent), TextWidget(this), m_counter(0) {
	setWidgetStyle(WidgetType::INFO_WIDGET);
	setTextFont(m_textStyle.m_fontIndex);
	for (int i=0; i < getAboutStringCount(); ++i) {
		addText(getAboutString(i));
	}
}

void GlestInfoWidget::start() {
	assert(getSize() != Vec2i(0) && "widget must be sized first.");
	const FontMetrics *fm = getFont(m_textStyle.m_fontIndex, FontSize::NORMAL)->getMetrics();
	const int &n = numSnippets();
	const int half_n = n / 2;
	const float left_x = (getWidth() - fm->getHeight()) / 2.f;  // x-coord to right justify text to
	const float right_x = (getWidth() + fm->getHeight()) / 2.f; // x-coord to left justify text to
	const float y_offset = (getHeight() - half_n * fm->getHeight()) / 2.f;
	const float left_y = y_offset;
	const float right_y = y_offset + (n % 2 == 1 ? fm->getHeight() / 2.f : 0.f);

	int max_len = -1;
	vector<Vec2f> dims;
	dims.resize(n);
	m_endPositions.resize(n);
	// pass 1: determine longest string length, set end positions
	for (int i=0; i < n; ++i) {
		// get string dimensions and store for later
		dims[i] = fm->getTextDiminsions(getText(i));
		if (dims[i].w > max_len) { // longest yet?
			max_len = int(dims[i].w);
		}
		Vec2f pos;
		if (i <= n / 2) { // left-text
			pos = Vec2f(left_x - dims[i].w, left_y + i * fm->getHeight());
		} else { // right-text
			int j = i - (n / 2 + 1);
			pos = Vec2f(right_x, right_y + j * fm->getHeight());
		}
		m_endPositions[i] = pos;
	}
	// pass 2: determine start positions
	m_startPositions.resize(n);
	for (int i=0; i < n; ++i) {
		Vec2f pos = m_endPositions[i];
		if (i <= n / 2) {
			pos.x -= max_len / 2;
		} else {
			pos.x += max_len / 2;
		}
		m_startPositions[i] = pos;
		setTextPos(Vec2i(pos), i);
		setTextColour(Vec4f(1.f, 1.f, 1.f, 0.f), i);
	}
	m_rootWindow->registerUpdate(this);
}

void GlestInfoWidget::update() {
	++m_counter;
	if (m_counter <= transitionTime) {
		float t = float(m_counter) / float(transitionTime);
		Vec4f colour(1.f, 1.f, 1.f, t);
		for (int i=0; i < numSnippets(); ++i) {
			setTextPos(Vec2i(m_startPositions[i].lerp(t, m_endPositions[i])), i);
			setTextColour(colour, i);
		}
	}
}

void GlestInfoWidget::render() {
	Widget::render();
	for (int i=0; i < numSnippets(); ++i) {
		renderText(i);
	}
}

// =====================================================
// 	class TeamInfoWidget
// =====================================================

typedef string (*TeamMemberFunc)(int i, TeamMemberField field);

class TeamInfoWidget : public CellStrip {
private:
	StaticText *m_teamLabel;
	TickerTape *m_nameTicker;
	TickerTape *m_roleTicker;

public:
	TeamInfoWidget(Container *parent);

	void setTeam(const string &teamName) {
		m_teamLabel->setText(teamName);
	}

	void setMembers(int count, TeamMemberFunc func) {
		vector<string> names, roles;
		for (int i=0; i < count; ++i) {
			names.push_back(func(i, TeamMemberField::NAME));
			roles.push_back(func(i, TeamMemberField::ROLE));
		}
		m_nameTicker->addItems(names);
		m_roleTicker->addItems(roles);
	}
};

TeamInfoWidget::TeamInfoWidget(Container *parent)
		: CellStrip(parent, Orientation::VERTICAL, 3) {
	setWidgetStyle(WidgetType::INFO_WIDGET);

	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	m_teamLabel = new StaticText(this);
	m_teamLabel->setCell(0);
	m_teamLabel->setAnchors(anchors);
	m_teamLabel->setTextParams("", m_textStyle.m_colourIndex, m_textStyle.m_fontIndex);

	m_nameTicker = new TickerTape(this, Origin::FROM_RIGHT, Alignment::FLUSH_LEFT);
	m_nameTicker->setCell(1);
	m_nameTicker->setAnchors(anchors);
	m_nameTicker->setOverlapTransitions(true);

	m_roleTicker = new TickerTape(this, Origin::FROM_LEFT, Alignment::FLUSH_RIGHT);
	m_roleTicker->setCell(2);
	m_roleTicker->setAnchors(anchors);
	m_roleTicker->setOverlapTransitions(true);
}

// =====================================================
// 	class MenuStateAbout
// =====================================================

MenuStateAbout::MenuStateAbout(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu) {
	Lang &lang = g_lang;
	Font *font = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	Font *bigFont = g_widgetConfig.getMenuFont()[FontSize::BIG];

	int itemHeight = g_widgetConfig.getDefaultItemHeight();
	// top level strip
	CellStrip *rootStrip = 
		new CellStrip(static_cast<Container*>(&program), Orientation::VERTICAL, Origin::FROM_TOP, 3);
	Vec2i pad(15, 45);
	rootStrip->setPos(pad);
	rootStrip->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));

	rootStrip->setSizeHint(0, SizeHint(-1, 5 * itemHeight));
	rootStrip->setSizeHint(1, SizeHint(-1, 3 * itemHeight));
	rootStrip->setSizeHint(2, SizeHint());

	GlestInfoWidget *infoWidget = new GlestInfoWidget(rootStrip);
	infoWidget->setCell(0);
	infoWidget->setAnchors(Anchors(Anchor(AnchorType::RIGID, 0)));

	CellStrip *strip = new CellStrip(rootStrip, Orientation::HORIZONTAL, 3);
	strip->setCell(1);
	strip->setAnchors(Anchors(Anchor(AnchorType::RIGID, 0)));
	Anchors sidePad(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::RIGID, 0));

	TeamInfoWidget *teamWidget = new TeamInfoWidget(strip);
	teamWidget->setCell(0);
	teamWidget->setAnchors(sidePad);
	teamWidget->setTeam(g_lang.get("GlestTeam"));
	teamWidget->setMembers(getGlestTeamMemberCount(), &getGlestTeamMemberField);

	teamWidget = new TeamInfoWidget(strip);
	teamWidget->setCell(1);
	teamWidget->setAnchors(sidePad);
	teamWidget->setTeam("Mod Team (place-holder)");
	//teamWidget->setMembers(glestTeamCount, glestTeamNames, glestTeamRoles);

	teamWidget = new TeamInfoWidget(strip);
	teamWidget->setCell(2);
	teamWidget->setAnchors(sidePad);
	teamWidget->setTeam(g_lang.get("GaeTeam"));
	teamWidget->setMembers(getGAETeamMemberCount(), &getGAETeamMemberField);

	rootStrip->layoutCells();
	infoWidget->start();

	//int centreX = g_metrics.getScreenW() / 2;
	//Font *font = g_widgetConfig.getMenuFont()[FontSize::SMALL];
	//Font *fontBig = g_widgetConfig.getMenuFont()[FontSize::NORMAL];
	//const FontMetrics *fm = font->getMetrics();
	//const FontMetrics *fmBig = fontBig->getMetrics();
	//Vec2i btnSize(150, 30);
	//Vec2i btnPos(centreX - btnSize.x / 2, 50);
	//m_returnButton = new Button(&program, btnPos, btnSize);
	//m_returnButton->setTextParams(g_lang.get("Return"), Vec4f(1.f), font, true);
	//m_returnButton->Clicked.connect(this, &MenuStateAbout::onReturn);

	//int topY = g_metrics.getScreenH();
	//int centreY = topY / 2;
	//int y = topY - 50;
	//int x;
	//int fh = int(fm->getHeight() + 1.f);
	//int fhBig = int(fmBig->getHeight() + 1.f);

	//Vec2i dims;
	//StaticText* label;
	//for (int i=0; i < 4; ++i) {
	//	y -= fh;
	//	label = new StaticText(&program);
	//	label->setTextParams(getAboutString1(i), Vec4f(1.f), font, false);
	//	dims = label->getTextDimensions();
	//	x = centreX - 10 - dims.x;
	//	label->setPos(x, y);

	//	if (i < 3) {
	//		label = new StaticText(&program);
	//		label->setTextParams(getAboutString2(i), Vec4f(1.f), font, false);
	//		x = centreX + 10;
	//		label->setPos(x, y - fh / 2);
	//	}
	//}

	//y -= fh;
	//int sy = y;
	//int thirdX = g_metrics.getScreenW() / 3;//centreX / 2;

	//y -= fhBig;
	//label = new StaticText(&program);
	//label->setTextParams(lang.get("GlestTeam") + ":", Vec4f(1.f), fontBig, false);
	//dims = label->getTextDimensions();
	//x = thirdX - dims.x / 2;
	//label->setPos(x, y);

	//for (int i=0; i < getGlestTeamMemberCount(); ++i) {
	//	y -= fh;
	//	label = new StaticText(&program);
	//	label->setTextParams(getGlestTeamMemberName(i), Vec4f(1.f), font, false);
	//	dims = Vec2i(fm->getTextDiminsions(getGlestTeamMemberNameNoDiacritics(i)) + Vec2f(1.f));
	//	x = thirdX - 10 - dims.x;
	//	label->setPos(x, y);

	//	label = new StaticText(&program);
	//	label->setTextParams(getGlestTeamMemberRole(i), Vec4f(1.f), font, false);
	//	x = thirdX + 10;
	//	label->setPos(x, y);		
	//}

	//y = sy;
	//thirdX *= 2;//3;

	//y -= fhBig;
	//label = new StaticText(&program);
	//label->setTextParams(lang.get("GaeTeam") + ":", Vec4f(1.f), fontBig, false);
	//dims = label->getTextDimensions();
	//x = thirdX - dims.x / 2;
	//label->setPos(x, y);

	//for (int i=0; i < getGAETeamMemberCount(); ++i) {
	//	y -= fh;
	//	label = new StaticText(&program);
	//	label->setTextParams(getGAETeamMemberName(i), Vec4f(1.f), font, false);
	//	dims = label->getTextDimensions();
	//	x = thirdX - 10 - dims.x;
	//	label->setPos(x, y);

	//	label = new StaticText(&program);
	//	label->setTextParams(getGAETeamMemberRole(i), Vec4f(1.f), font, false);
	//	x = thirdX + 10;
	//	label->setPos(x, y);		
	//}

	//y -= fh;
	//y -= fhBig;
	//label = new StaticText(&program);
	//label->setTextParams("GAE Contributors:", Vec4f(1.f), fontBig, false);
	//dims = label->getTextDimensions();
	//x = thirdX - dims.x / 2;
	//label->setPos(x, y);

	//for (int i=0; i < getGAEContributorCount(); ++i) {
	//	y -= fh;
	//	label = new StaticText(&program);
	//	label->setTextParams(getGAEContributorName(i), Vec4f(1.f), font, false);
	//	dims = label->getTextDimensions();
	//	x = thirdX - 10 - dims.x;
	//	label->setPos(x, y);

	//	label = new StaticText(&program);
	//	label->setTextParams(getGAETeamMemberRole(0), Vec4f(1.f), font, false);
	//	x = thirdX + 10;
	//	label->setPos(x, y);
	//}
}

void MenuStateAbout::onReturn(Button*) {
	mainMenu->setCameraTarget(MenuStates::ROOT);
	g_soundRenderer.playFx(g_coreData.getClickSoundB());
	doFadeOut();
}

void MenuStateAbout::update() {
	MenuState::update();

	if (m_transition) {
		program.clear();
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
	}
}

}}//end namespace

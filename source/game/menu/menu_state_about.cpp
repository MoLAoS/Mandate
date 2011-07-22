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

class GlestInfoWidget : /*public TickerTape*/ public Widget, public TextWidget {
private:
	typedef vector<TextAction> Actions;
	//vector<Vec2f> m_startPositions;
	//vector<Vec2f> m_endPositions;
	//int           m_counter;
	Actions       m_actions;

	static const int transitionTime = 120 * 3;//180; // 1.5 seconds

public:
	GlestInfoWidget(Container *parent);
	~GlestInfoWidget() {
		m_rootWindow->unregisterUpdate(this);
	}

	void start();

	virtual void render() override;
	virtual void update() override;
	virtual string descType() const override { return "GlestInfoWidget"; }
};

GlestInfoWidget::GlestInfoWidget(Container *parent)
		: Widget(parent), TextWidget(this) {
	setWidgetStyle(WidgetType::INFO_WIDGET);
	for (int i=0; i < getAboutStringCount(); ++i) {
		addText(getAboutString(i));
	}
}

inline int startDelay(int i, const int &n) {
	const int half_n = n / 2;
	if (i <= half_n) {
		i *= 2;
	} else {
		if (n % 2 == 1) {
			i = (i - half_n) * 2 - 1;
		} else {
			i = ((i + 1) - half_n) * 2 - 1;
		}
	}
	return (-1 - i) * 80;
}

void GlestInfoWidget::start() {
	assert(getSize() != Vec2i(0) && "widget must be sized first.");
	const FontMetrics *fm = getFont(m_textStyle.m_fontIndex)->getMetrics();
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
	vector<Vec2f> endPositions(n);
	// pass 1: determine longest string length, set end positions
	for (int i=0; i < n; ++i) {
		// get string dimensions and store for later
		dims[i] = fm->getTextDiminsions(getText(i));
		if (dims[i].w > max_len) { // longest yet?
			max_len = int(dims[i].w);
		}
		Vec2f pos;
		if (i <= half_n) { // left-text
			pos = Vec2f(left_x - dims[i].w, left_y + i * fm->getHeight());
		} else { // right-text
			int j = i - (half_n + 1);
			pos = Vec2f(right_x, right_y + j * fm->getHeight());
		}
		endPositions[i] = pos;
	}
	// pass 2: determine start positions, create Actions
	vector<Vec2f> startPositions(n);
	for (int i=0; i < n; ++i) {
		Vec2f pos = endPositions[i];
		if (i <= half_n) {
			pos.x -= max_len / 2;
		} else {
			pos.x += max_len / 2;
		}
		startPositions[i] = pos;
		setTextPos(Vec2i(pos), i);
		setTextColour(Vec4f(1.f), i);
		setTextFade(0.f, i);
		m_actions.push_back(TextAction(transitionTime, 0, this, i));
		int delay = startDelay(i, n);
		m_actions.back().setPosTransition(startPositions[i], endPositions[i], TransitionFunc::LOGARITHMIC);
		m_actions.back().setAlphaTransition(0.f, 1.f, TransitionFunc::LINEAR);
		m_actions.back().m_counter = delay;
	}
	m_rootWindow->registerUpdate(this);
}

void GlestInfoWidget::update() {
	Actions::iterator it = m_actions.begin();
	while (it != m_actions.end()) {
		if (it->update()) {
			it = m_actions.erase(it);
		} else {
			++it;
		}
	}
	if (m_actions.empty()) {
		m_rootWindow->unregisterUpdate(this);
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
	TickerTape *m_teamLabel;
	TickerTape *m_nameTicker;
	TickerTape *m_roleTicker;
	int         m_counter;

public:
	TeamInfoWidget(Container *parent);
	~TeamInfoWidget() {
		m_rootWindow->unregisterUpdate(this);
	}

	void setTeam(const string &teamName) {
		m_teamLabel->addItems(&teamName, 1);
		m_teamLabel->setTextFade(0.f);
		m_rootWindow->registerUpdate(this);
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

	virtual void update() override {
		++m_counter;
		if (m_counter == 120 * 5) { // @ 5 sec.
			m_teamLabel->startTicker();
		} else if (m_counter == 120 * 8) { // @ 8 sec.
			m_nameTicker->startTicker();
			m_roleTicker->startTicker();
		}
	}

	virtual void render() override {
		CellStrip::render(false);
	}
};

TeamInfoWidget::TeamInfoWidget(Container *parent)
		: CellStrip(parent, Orientation::VERTICAL, 3), m_counter(0) {
	setWidgetStyle(WidgetType::INFO_WIDGET);

	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	m_teamLabel = new TickerTape(this, SizeHint(50), Alignment::CENTERED);
	m_teamLabel->setCell(0);
	m_teamLabel->setAnchors(anchors);
	m_teamLabel->setTransitionInterval(120 * 5);
	m_teamLabel->setTransitionFunc(TransitionFunc::LINEAR);
	m_teamLabel->setDisplayInterval(-1);

	m_nameTicker = new TickerTape(this, SizeHint(25), Alignment::FLUSH_LEFT);
	m_nameTicker->setCell(1);
	m_nameTicker->setAnchors(anchors);
//	m_nameTicker->setOverlapTransitions(true);
	int offset = g_metrics.getScreenW() / 4;
	m_nameTicker->setOffsets(Vec2i(offset, 0), Vec2i(-offset, 0));
	m_nameTicker->setTransitionFunc(TransitionFunc::LOGARITHMIC);
	m_nameTicker->setTransitionInterval(120 * 2);
	m_nameTicker->setDisplayInterval(120 * 2);

	m_roleTicker = new TickerTape(this, SizeHint(75), Alignment::FLUSH_RIGHT);
	m_roleTicker->setCell(2);
	m_roleTicker->setAnchors(anchors);
//	m_roleTicker->setOverlapTransitions(true);
	m_roleTicker->setOffsets(Vec2i(-offset, 0), Vec2i(offset, 0));
	m_roleTicker->setTransitionFunc(TransitionFunc::LOGARITHMIC);
	m_roleTicker->setTransitionInterval(120 * 2);
	m_roleTicker->setDisplayInterval(120 * 2);
}

// =====================================================
// 	class MenuStateAbout
// =====================================================

MenuStateAbout::MenuStateAbout(Program &program, MainMenu *mainMenu)
		: MenuState(program, mainMenu) {
	Lang &lang = g_lang;
	const Font *font = g_widgetConfig.getMenuFont();
	const Font *bigFont = g_widgetConfig.getMenuFont();

	int itemHeight = g_widgetConfig.getDefaultItemHeight();
	// top level strip
	CellStrip *rootStrip = 
		new CellStrip(static_cast<Container*>(&program), Orientation::VERTICAL, Origin::FROM_TOP, 3);
	Vec2i pad(15, 25);
	rootStrip->setPos(pad);
	rootStrip->setSize(Vec2i(g_config.getDisplayWidth() - pad.w * 2, g_config.getDisplayHeight() - pad.h * 2));

	rootStrip->setSizeHint(0, SizeHint(-1, 7 * itemHeight));
	rootStrip->setSizeHint(1, SizeHint(-1, 3 * itemHeight));
	rootStrip->setSizeHint(2, SizeHint());

	GlestInfoWidget *infoWidget = new GlestInfoWidget(rootStrip);
	infoWidget->setCell(0);
	infoWidget->setAnchors(Anchors(Anchor(AnchorType::RIGID, 0)));

	CellStrip *strip = new CellStrip(rootStrip, Orientation::HORIZONTAL, 3);
	strip->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0));
	strip->setAnchors(a);

	TeamInfoWidget *teamWidget = new TeamInfoWidget(strip);
	teamWidget->setCell(0);
	teamWidget->setAnchors(a);
	teamWidget->setTeam(g_lang.get("GlestTeam"));
	teamWidget->setMembers(getGlestTeamMemberCount(), &getGlestTeamMemberField);

	teamWidget = new TeamInfoWidget(strip);
	teamWidget->setCell(1);
	teamWidget->setAnchors(a);
	teamWidget->setTeam(g_lang.get("GaeTeam"));
	teamWidget->setMembers(getGAETeamMemberCount(), &getGAETeamMemberField);

	teamWidget = new TeamInfoWidget(strip);
	teamWidget->setCell(2);
	teamWidget->setAnchors(a);
	teamWidget->setTeam(g_lang.get("Contributors"));
	teamWidget->setMembers(getContributorCount(), &getContributorField);

	rootStrip->layoutCells();
	infoWidget->start();

	int s = g_widgetConfig.getDefaultItemHeight();
	Vec2i btnSize(s * 7, s);
	Vec2i btnPos((g_metrics.getScreenW() - btnSize.w) / 2, g_metrics.getScreenH() - 50);
	m_returnButton = new Button(&program, btnPos, btnSize);
	m_returnButton->setText(g_lang.get("Return"));
	m_returnButton->Clicked.connect(this, &MenuStateAbout::onReturn);
	program.setFade(0.f);
}

void MenuStateAbout::onReturn(Widget*) {
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

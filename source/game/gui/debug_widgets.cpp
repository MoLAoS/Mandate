// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "debug_widgets.h"
#include "misc_widgets.h"
#include "lang.h"
#include "config.h"
#include "game.h"

namespace Glest { namespace Gui {

using namespace Widgets;
using namespace Global;
using namespace Gui;

DebugOptions::DebugOptions(Container *parent, bool menu)
		: CellStrip(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2) {
	if (menu) {
		m_stats = new DebugStats();
	} else {
		m_stats = g_gameState.getDebugStats();
	}
	WidgetConfig &cfg = g_widgetConfig;
	Anchors fillAnchors = Anchors::getFillAnchors();
	OptionPanel *leftPnl = new OptionPanel(this, 0);
	/*OptionPanel */rightPnl = new OptionPanel(this, 1);
	setSizeHint(0, SizeHint(35, -1));
	setSizeHint(1, SizeHint(65, -1));

	///todo fix hacky indentation - hailstone 11Sep2011
	ListBoxItem *link = leftPnl->addLabel(" " + g_lang.get("General"));
	link->Clicked.connect(this, &DebugOptions::onHeadingClicked);
	link = leftPnl->addLabel(" " + g_lang.get("DebugSections"));
	link->Clicked.connect(this,&DebugOptions::onHeadingClicked);
	link = leftPnl->addLabel(" " + g_lang.get("PerformanceReport"));
	link->Clicked.connect(this,&DebugOptions::onHeadingClicked);
	link = leftPnl->addLabel(" " + g_lang.get("PerformanceSections"));
	link->Clicked.connect(this,&DebugOptions::onHeadingClicked);

	ListBoxItem *heading = rightPnl->addLabel(" " + g_lang.get("General"));
	m_headings.insert(std::pair<string, ListBoxItem*>(" " + g_lang.get("General"), heading));

	m_debugMode = rightPnl->addCheckBox(g_lang.get("DebugMode"), g_config.getMiscDebugMode());
	m_debugMode->Clicked.connect(this, &DebugOptions::onCheckChanged);
	m_debugKeys = rightPnl->addCheckBox(g_lang.get("DebugKeys"), g_config.getMiscDebugKeys());
	m_debugKeys->Clicked.connect(this, &DebugOptions::onCheckChanged);

	heading = rightPnl->addLabel(" " + g_lang.get("DebugSections"));
	m_headings.insert(std::pair<string, ListBoxItem*>(" " + g_lang.get("DebugSections"), heading));

	foreach_enum (DebugSection, ds) {
		m_debugSections[ds] = rightPnl->addCheckBox(g_lang.get(DebugSectionNames[ds]), m_stats->isEnabled(ds));
		m_debugSections[ds]->Clicked.connect(this, &DebugOptions::onCheckChanged);
	}

	heading = rightPnl->addLabel(" " + g_lang.get("PerformanceReport"));
	m_headings.insert(std::pair<string, ListBoxItem*>(" " + g_lang.get("PerformanceReport"), heading));
	foreach_enum(TimerReportFlag, trf) {
		m_timerReports[trf] = rightPnl->addCheckBox(g_lang.get(TimerReportFlagNames[trf]), m_stats->isEnabled(trf));
		m_timerReports[trf]->Clicked.connect(this, &DebugOptions::onCheckChanged);
	}

	heading = rightPnl->addLabel(" " + g_lang.get("PerformanceSections"));
	m_headings.insert(std::pair<string, ListBoxItem*>(" " + g_lang.get("PerformanceSections"), heading));
	foreach_enum (TimerSection, ts) {
		m_timerSections[ts] = rightPnl->addCheckBox(g_lang.get(TimerSectionNames[ts]), m_stats->isEnabled(ts));
		m_timerSections[ts]->Clicked.connect(this, &DebugOptions::onCheckChanged);
	}
}

void DebugOptions::onHeadingClicked(Widget *widget) {
	ListBoxItem *heading = static_cast<ListBoxItem*>(widget);
	// need to set to start so the heading position is accurate
	rightPnl->setScrollPosition(0);
	rightPnl->setScrollPosition(m_headings[heading->getText()]->getPos().y);
}

void DebugOptions::onCheckChanged(Widget *widget) {
	CheckBox *cb = static_cast<CheckBox*>(widget);
	if (cb == m_debugMode) {
		g_config.setMiscDebugMode(!g_config.getMiscDebugMode());
		g_program.setFpsCounterVisible(g_config.getMiscDebugMode());
		return;
	}
	if (cb == m_debugKeys) {
		g_config.setMiscDebugKeys(!g_config.getMiscDebugKeys());
		return;
	}
	foreach_enum (DebugSection, ds) {
		if (m_debugSections[ds] == cb) {
			m_stats->setEnabled(ds, cb->isChecked());
			m_stats->saveConfig();
			return;
		}
	}
	foreach_enum (TimerSection, ts) {
		if (m_timerSections[ts] == cb) {
			m_stats->setEnabled(ts, cb->isChecked());
			m_stats->saveConfig();
			return;
		}
	}
	foreach_enum (TimerReportFlag, tf) {
		if (m_timerReports[tf] == cb) {
			m_stats->setEnabled(tf, cb->isChecked());
			m_stats->saveConfig();
			return;
		}
	}
	assert(false);
}

}}
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

class BoolOptionPanel : public CellStrip, public MouseWidget, public sigslot::has_slots {
private:
	CellStrip *m_list;
	ScrollBar *m_scrollBar;

	vector<Vec2i> m_origPositions;
	int           m_scrollOffset;

	SizeHint m_scrollSizeHint;
	SizeHint m_noScrollSizeHint;

public:
	BoolOptionPanel(CellStrip *parent, int cell);

	StaticText* addLabel(const string &txt);
	CheckBox* addCheckBox(const string &txt, bool checked);

	virtual void setSize(const Vec2i &sz) override;

	virtual bool mouseWheel(Vec2i pos, int z) override { m_scrollBar->scrollLine(z > 0); return true; }

	void onScroll(ScrollBar *sb);
};

BoolOptionPanel::BoolOptionPanel(CellStrip *parent, int cell)
		: CellStrip(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2), m_scrollOffset(0)
		, MouseWidget(this) {
	setCell(cell);
	setWidgetStyle(WidgetType::LIST_BOX);
	Anchors anchors(Anchor(AnchorType::RIGID, g_config.getDisplayWidth() / 50), Anchor(AnchorType::RIGID, g_config.getDisplayWidth() / 200));
	setAnchors(anchors);

	int sbw = g_widgetConfig.getDefaultItemHeight();

	m_noScrollSizeHint = SizeHint(0);
	m_scrollSizeHint = SizeHint(-1, sbw);

	m_list = new CellStrip(this, Orientation::VERTICAL, Origin::FROM_TOP, 0);
	m_list->setCell(0);
	m_list->setAnchors(Anchors::getFillAnchors());
	setSizeHint(0, SizeHint());
	
	m_scrollBar = new ScrollBar(this, true, int(g_widgetConfig.getDefaultItemHeight() * 1.5f));
	m_scrollBar->setCell(1);
	m_scrollBar->setAnchors(Anchors::getFillAnchors());
	m_scrollBar->ThumbMoved.connect(this, &BoolOptionPanel::onScroll);
	setSizeHint(1, m_noScrollSizeHint);
}

void BoolOptionPanel::setSize(const Vec2i &sz) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int req_h = h * m_list->getCellCount();
	if (req_h > sz.h - getBordersVert()) {
		setSizeHint(1, m_scrollSizeHint);
		m_scrollBar->setVisible(true);
		m_scrollBar->setRanges(req_h, sz.h - getBordersVert());
	} else {
		setSizeHint(1, m_noScrollSizeHint);
		m_scrollBar->setVisible(false);
	}
	CellStrip::setSize(sz);
	CellStrip::layoutCells();
	m_list->layoutCells();
	m_origPositions.resize(m_list->getChildCount());
	for (int i=0; i < m_list->getChildCount(); ++i) {
		m_origPositions[i] = m_list->getChild(i)->getPos();
		m_list->getChild(i)->setPos(Vec2i(m_origPositions[i].x, m_origPositions[i].y - m_scrollOffset));
	}
}

void BoolOptionPanel::onScroll(ScrollBar *sb) {
	m_scrollOffset = int(sb->getThumbPos());
	for (int i=0; i < m_list->getChildCount(); ++i) {
		m_list->getChild(i)->setPos(Vec2i(m_origPositions[i].x, m_origPositions[i].y - m_scrollOffset));
	}
}

StaticText* BoolOptionPanel::addLabel(const string &txt) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));

	StaticText *label = new StaticText(m_list);
	label->setAlignment(Alignment::FLUSH_LEFT);
	label->setAnchors(fillAnchors);
	label->setCell(m_list->getCellCount());
	label->setText(txt);
	m_list->addCells(1);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h));
	return label;
}

CheckBox* BoolOptionPanel::addCheckBox(const string &txt, bool checked) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int squashAmount = (h - g_widgetConfig.getDefaultItemHeight()) / 2;
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, squashAmount));

	OptionWidget *ow = new OptionWidget(m_list, txt);
	m_list->addCells(1);
	ow->setAnchors(fillAnchors);
	ow->setCell(m_list->getCellCount() - 1);
	ow->setPercentSplit(80);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h));

	CheckBoxHolder *cbh = new CheckBoxHolder(ow);
	cbh->setCell(1);
	cbh->setAnchors(fillAnchors);

	CheckBox *checkBox  = new CheckBox(cbh);
	checkBox->setCell(1);
	checkBox->setAnchors(squashAnchors);
	checkBox->setChecked(checked);
	
	return checkBox;
}

DebugOptions::DebugOptions(Container *parent, bool menu)
		: CellStrip(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2) {
	if (menu) {
		m_stats = new DebugStats();
	} else {
		m_stats = g_gameState.getDebugStats();
	}
	WidgetConfig &cfg = g_widgetConfig;
	Anchors fillAnchors = Anchors::getFillAnchors();
	BoolOptionPanel *leftPnl = new BoolOptionPanel(this, 0);
	BoolOptionPanel *rightPnl = new BoolOptionPanel(this, 1);

	leftPnl->addLabel(g_lang.get("DebugSections"));

	foreach_enum (DebugSection, ds) {
		m_debugSections[ds] = leftPnl->addCheckBox(g_lang.get(DebugSectionNames[ds]), m_stats->isEnabled(ds));
		m_debugSections[ds]->Clicked.connect(this, &DebugOptions::onCheckChanged);
	}

	leftPnl->addLabel(g_lang.get("PerformanceReport"));
	foreach_enum(TimerReportFlag, trf) {
		m_timerReports[trf] = leftPnl->addCheckBox(g_lang.get(TimerReportFlagNames[trf]), m_stats->isEnabled(trf));
		m_timerReports[trf]->Clicked.connect(this, &DebugOptions::onCheckChanged);
	}

	rightPnl->addLabel(g_lang.get("PerformanceSections"));
	foreach_enum (TimerSection, ts) {
		m_timerSections[ts] = rightPnl->addCheckBox(g_lang.get(TimerSectionNames[ts]), m_stats->isEnabled(ts));
		m_timerSections[ts]->Clicked.connect(this, &DebugOptions::onCheckChanged);
	}
}

void DebugOptions::onCheckChanged(Widget *widget) {
	CheckBox *cb = static_cast<CheckBox*>(widget);
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
// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GUI__DEBUG_WIDGETS_H_
#define _GLEST_GUI__DEBUG_WIDGETS_H_

#include <string>
#include "framed_widgets.h"
#include "debug_stats.h"

namespace Glest { namespace Gui {

using namespace Widgets;
using namespace Debug;

class DebugPanel : public MessageDialog {
public:
	DebugPanel(Container *parent) : MessageDialog(parent) { }

	void setDebugText(const string &text) {
		MessageDialog::setMessageText(text, ScrollAction::MAINTAIN);
	}

	virtual string descType() const override { return "DebugPanel"; }
};

class DebugOptions : public CellStrip, public sigslot::has_slots {
private:
	CheckBox *m_debugMode;
	CheckBox *m_debugKeys;
	CheckBox *m_debugSections[DebugSection::COUNT];
	CheckBox *m_timerSections[TimerSection::COUNT];
	CheckBox *m_timerReports[TimerReportFlag::COUNT];

	DebugStats *m_stats;
	bool m_isInGame;

public:
	DebugOptions(Container *parent, bool menu);

	void onCheckChanged(Widget *cb);
};

}}

#endif
// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_GUI_DEBUG_WIDGETS_H_
#define _GLEST_GAME_GUI_DEBUG_WIDGETS_H_

#include <string>
#include "compound_widgets.h"

namespace Glest { namespace Gui {

using namespace Widgets;

class DebugPanel : public MessageDialog {
public:
	DebugPanel(Container *parent) : MessageDialog(parent) { }

	virtual void setMessageText(const string &text) override {
		MessageDialog::setMessageText(text);
	}

	virtual string descType() const override { return "DebugPanel"; }
};

}}

#endif
// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "chat_dialog.h"

#include "window.h"
#include "console.h"
#include "network_interface.h"
#include "lang.h"
#include "keymap.h"
#include "script_manager.h"
#include "config.h"
#include "core_data.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Glest::Net;

namespace Glest { namespace Gui {

// =====================================================
//  class ChatDialog
// =====================================================

ChatDialog::ChatDialog(Container* parent)
		: BasicDialog(parent), m_teamChat(false) {
	Anchors anchors(Anchor(AnchorType::RIGID, 10));
	// cell 1
	CellStrip *content = new CellStrip(this, Orientation::VERTICAL, 2);
	content->setCell(1);
	content->setAnchors(anchors);

	anchors = Anchors(Anchor(AnchorType::RIGID, 0));

	OptionWidget *ow = new OptionWidget(content, g_lang.get("TeamOnly"));
	ow->setCell(0);
	ow->setAnchors(anchors);

	m_teamCheckBox = new CheckBox(ow);
	m_teamCheckBox->setCell(1);
	m_teamCheckBox->setAnchors(anchors);
	m_teamCheckBox->setChecked(m_teamChat);
	m_teamCheckBox->Clicked.connect(this, &ChatDialog::onCheckChanged);

	m_inputBox = new InputBox(content);
	m_inputBox->setCell(1);
	m_inputBox->setAnchors(anchors);
	m_inputBox->setText("");
	m_inputBox->InputEntered.connect(this, &ChatDialog::onInputEntered);
	m_inputBox->Escaped.connect(this, &ChatDialog::onEscaped);

	setTitleText(g_lang.get("ChatMsg"));
	setButtonText(g_lang.get("Send"), g_lang.get("Cancel"));
}

bool ChatDialog::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		BasicDialog::mouseDown(btn, pos);
		m_inputBox->gainFocus();
		return true;
	}
	return false;
}

void ChatDialog::setVisible(bool vis) {
	if (isVisible() != vis) {
		if (vis) {
			m_inputBox->gainFocus();
		} else {
			g_widgetWindow.releaseKeyboardFocus(m_inputBox);
		}
		Widget::setVisible(vis);
	}

}

void ChatDialog::onInputEntered(TextBox*) {
	if (!m_inputBox->getText().empty()) {
		Button1Clicked(this);
	}
}

}}//end namespace

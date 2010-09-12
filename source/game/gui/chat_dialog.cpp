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

ChatDialog::ChatDialog(Container::Ptr parent, Vec2i pos, Vec2i size)
		: BasicDialog(parent, pos, size), m_teamChat(false) {
	m_panel = new Panel(this);
	m_panel->setLayoutParams(true, Panel::LayoutDirection::VERTICAL);
	m_panel->setPaddingParams(10, 10);

	m_subPanel = new Panel(m_panel);
	m_subPanel->setLayoutParams(true, Panel::LayoutDirection::HORIZONTAL);
	m_subPanel->setPaddingParams(0, 10);
	m_label = new StaticText(m_subPanel);
	m_label->setTextParams(g_lang.get("TeamOnly") + ": ", Vec4f(1.f), g_coreData.getFTMenuFontNormal());
	m_label->setSize(m_label->getPrefSize());
	m_teamCheckBox = new CheckBox(m_subPanel);
	m_teamCheckBox->setSize(m_teamCheckBox->getPrefSize());
	m_teamCheckBox->setChecked(m_teamChat);
	m_teamCheckBox->Clicked.connect(this, &ChatDialog::onCheckChanged);

	m_subPanel->setSize(m_label->getWidth() + m_teamCheckBox->getWidth() + 15, 
		std::max(m_label->getHeight(), m_teamCheckBox->getHeight())); 
	m_subPanel->layoutChildren();

	m_inputBox = new InputBox(m_panel);
	m_inputBox->setTextParams("", Vec4f(1.f), g_coreData.getFTMenuFontNormal());
	m_inputBox->InputEntered.connect(this, &ChatDialog::onInputEntered);
	m_inputBox->Escaped.connect(this, &ChatDialog::onEscaped);

	init(pos, size, g_lang.get("ChatMsg"), g_lang.get("Send"), g_lang.get("Cancel"));
	setContent(m_panel);

	Vec2i sz = m_label->getPrefSize();
	sz.x = m_panel->getSize().x - 20;
	m_inputBox->setSize(sz);
	m_subPanel->layoutChildren();
	m_panel->layoutChildren();
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

void ChatDialog::onInputEntered(TextBox::Ptr) {
	if (!m_inputBox->getText().empty()) {
		Button1Clicked(this);
	}
}

}}//end namespace

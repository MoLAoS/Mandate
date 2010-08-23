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

ChatDialog::ChatDialog(WidgetWindow::Ptr window, bool teamOnly)
		: BasicDialog(window) {
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
	m_teamCheckBox->setChecked(teamOnly);
	m_teamCheckBox->Clicked.connect(this, &ChatDialog::onCheckChanged);

	m_subPanel->setSize(m_label->getWidth() + m_teamCheckBox->getWidth() + 15, 
		std::max(m_label->getHeight(), m_teamCheckBox->getHeight())); 
	m_subPanel->layoutChildren();

	m_textBox = new TextBox(m_panel);
	m_textBox->setTextParams("", Vec4f(1.f), g_coreData.getFTMenuFontNormal());
	m_textBox->InputEntered.connect(this, &ChatDialog::onInputEntered);
}

ChatDialog::Ptr ChatDialog::showDialog(Vec2i pos, Vec2i size, bool teamOnly) {
	ChatDialog::Ptr dlg = new ChatDialog(&g_widgetWindow, teamOnly);
	g_widgetWindow.setFloatingWidget(dlg, true);
	dlg->init(pos, size, g_lang.get("ChatMsg"), g_lang.get("Send"), g_lang.get("Cancel"));
	dlg->setContent(dlg->m_panel);

	Vec2i sz = dlg->m_label->getPrefSize();
	sz.x = dlg->m_panel->getSize().x - 20;
	dlg->m_textBox->setSize(sz);
	dlg->m_subPanel->layoutChildren();
	dlg->m_panel->layoutChildren();
	dlg->m_textBox->gainFocus();
	return dlg;
}

void ChatDialog::onInputEntered(TextBox::Ptr) {
	if (!m_textBox->getText().empty()) {
		Button1Clicked(this);
	}
}

}}//end namespace

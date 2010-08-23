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

#ifndef _GLEST_GAME_CHATMANAGER_H_
#define _GLEST_GAME_CHATMANAGER_H_

#include <string>

#include "keymap.h"
#include "compound_widgets.h"
#include "network_message.h"

namespace Glest { namespace Sim {
	class SimulationInterface;
}}
using Glest::Sim::SimulationInterface;
using std::string;
using Shared::Platform::Key;

namespace Glest { namespace Gui {
using namespace Widgets;
class Console;

// =====================================================
//  class ChatDialog
// =====================================================

class ChatDialog : public BasicDialog {
public:
	typedef ChatDialog* Ptr;
	static const int maxTextLength = Net::TextMessage::maxStringSize; ///@todo implement input restriction...

private:
	StaticText::Ptr	m_label;
	CheckBox::Ptr	m_teamCheckBox;
	TextBox::Ptr	m_textBox;
	Panel::Ptr		m_panel;
	Panel::Ptr		m_subPanel;

private:
	ChatDialog(WidgetWindow*, bool teamOnly);

	void onCheckChanged(Button::Ptr) { TeamChatChanged(this); }
	void onInputEntered(TextBox::Ptr);

public:
	static ChatDialog::Ptr showDialog(Vec2i pos, Vec2i size, bool teamOnly);

	string getInput() const		{ return m_textBox->getText(); }
	bool getTeamChecked() const	{ return m_teamCheckBox->isChecked(); }
	void setTeamChat(bool v)	{ m_teamCheckBox->setChecked(v); }

	sigslot::signal<Ptr>	TeamChatChanged;

	virtual string desc() { return string("[ChatDialog: ") + descPosDim() + "]"; }
};

}}//end namespace

#endif

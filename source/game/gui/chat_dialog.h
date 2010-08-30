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
	InputBox::Ptr	m_inputBox;
	Panel::Ptr		m_panel;
	Panel::Ptr		m_subPanel;

	bool m_teamChat;

private:
	void onCheckChanged(Button::Ptr) { m_teamChat = m_teamCheckBox->isChecked(); }
	void onInputEntered(TextBox::Ptr);
	void onEscaped(InputBox *) { Escaped(this); }

public:
	ChatDialog(Container::Ptr parent, Vec2i pos, Vec2i size);
//	static ChatDialog::Ptr showDialog(Vec2i pos, Vec2i size, bool teamOnly);

	string getInput() const		{ return m_inputBox->getText(); }
	bool isTeamOnly() const		{ return m_teamCheckBox->isChecked(); }

	void setTeamOnly(bool v)	{ m_teamCheckBox->setChecked(v); }
	void toggleTeamOnly()		{ m_teamCheckBox->setChecked(!m_teamCheckBox->isChecked()); }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual void setVisible(bool vis);
	virtual string desc() { return string("[ChatDialog: ") + descPosDim() + "]"; }
};

}}//end namespace

#endif

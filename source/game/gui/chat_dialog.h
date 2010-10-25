// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_GUI_CHAT_DIALOG_H_
#define _GLEST_GAME_GUI_CHAT_DIALOG_H_

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
	StaticText*	m_label;
	CheckBox*	m_teamCheckBox;
	InputBox*	m_inputBox;
	Panel*		m_panel;
	Panel*		m_subPanel;

	bool m_teamChat;

private:
	void onCheckChanged(Button*) { m_teamChat = m_teamCheckBox->isChecked(); }
	void onInputEntered(TextBox*);
	void onEscaped(InputBox *) { Escaped(this); }

public:
	ChatDialog(Container* parent, Vec2i pos, Vec2i size);
//	static ChatDialog* showDialog(Vec2i pos, Vec2i size, bool teamOnly);

	string getInput() const		{ return m_inputBox->getText(); }
	bool isTeamOnly() const		{ return m_teamCheckBox->isChecked(); }

	void setTeamOnly(bool v)	{ m_teamCheckBox->setChecked(v); }
	void toggleTeamOnly()		{ m_teamCheckBox->setChecked(!m_teamCheckBox->isChecked()); }

	void clearText()			{ m_inputBox->setText(""); }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual void setVisible(bool vis);
	virtual string desc() { return string("[ChatDialog: ") + descPosDim() + "]"; }
};

}}//end namespace

#endif

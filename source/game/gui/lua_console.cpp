// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "lua_console.h"
#include "core_data.h"
#include "script_manager.h"
#include "widget_window.h"
#include "user_interface.h"

#include "leak_dumper.h"

namespace Glest { namespace Gui {

using Global::CoreData;

// =====================================================
// class LuaInputBox
// =====================================================

LuaInputBox::LuaInputBox(LuaConsole *console, Container *parent)
		: TextBox(parent)
		, m_historyPoint(-1)
		, m_console(console) {
}

LuaInputBox::LuaInputBox(LuaConsole *console, Container *parent, Vec2i pos, Vec2i size)
		: TextBox(parent, pos, size)
		, m_historyPoint(-1)
		, m_console(console) {
}

bool LuaInputBox::keyDown(Key key) {
	KeyCode code = key.getCode();
	switch (code) {
		case KeyCode::ARROW_UP:
			recallCommand();
			return true;
		case KeyCode::ARROW_DOWN:
			recallCommand(true);
			return true;
		case KeyCode::RETURN:
			InputEntered(this);
			storeCommand();
			return true;
		case KeyCode::ESCAPE:
			m_console->setVisible(false);
			return true;
	}
	return TextBox::keyDown(key);
}

void LuaInputBox::storeCommand() {
	Strings::iterator it = std::find(m_commandHistory.begin(), m_commandHistory.end(), getText());
	if (it != m_commandHistory.end()) {
		m_commandHistory.erase(it);
	}
	m_commandHistory.push_front(getText());
	setText("");
	m_historyPoint = -1;
}

void LuaInputBox::recallCommand(bool reverse) {
	if (reverse) {
		if (m_historyPoint - 1 >= -1) {
			--m_historyPoint;
			if (m_historyPoint != -1) {
				setText(m_commandHistory[m_historyPoint]);
			} else {
				setText("");
			}
		}
	} else {
		if (m_historyPoint + 1 < m_commandHistory.size()) {
			++m_historyPoint;
			setText(m_commandHistory[m_historyPoint]);
		}
	}
}

// =====================================================
// class LuaConsole
// =====================================================

LuaConsole::LuaConsole(UserInterface *ui, Container* parent, Vec2i pos, Vec2i sz)
		: BasicDialog(parent, pos, sz)
		, m_ui(ui) {
	init(pos, sz, "Lua-Console", "Close", "");
	m_panel = new Panel(this);
	setContent(m_panel);

	Vec2i size = m_panel->getSize();

	Vec2i ibSize;
	ibSize.x = size.x - 10;
	Font *font = g_widgetConfig.getGameFont()[FontSize::NORMAL];
	ibSize.y = int(font->getMetrics()->getHeight() + 6);
	m_inputBox = new LuaInputBox(this, m_panel, Vec2i(0), ibSize);
	m_inputBox->setTextParams("", Vec4f(1.f), font, false);
	m_inputBox->setTextPos(Vec2i(3, 3));
	m_inputBox->InputEntered.connect(this, &LuaConsole::onLineEntered);

	Vec2i obSize;
	obSize.x = ibSize.x;
	obSize.y = size.y - ibSize.y - 25;
	m_outputBox = new ScrollText(m_panel, Vec2i(0), obSize);
	m_outputBox->setTextParams("", Vec4f(1.f), font, false);

	m_panel->setLayoutParams(true, Orientation::VERTICAL);
	m_panel->setPaddingParams(10, 5);
	m_panel->layoutChildren();
}

LuaConsole::~LuaConsole() {
}

void LuaConsole::setVisible(bool vis) {
	if (vis != isVisible()) {
		if (vis) {
			m_inputBox->gainFocus();
		} else {
			g_widgetWindow.releaseKeyboardFocus(m_inputBox);
		}
		Widget::setVisible(vis);
	}
}

void LuaConsole::onLineEntered(TextBox*) {
	addOutput(string("> ") + m_inputBox->getText());
	Script::ScriptManager::doSomeLua(m_inputBox->getText());
}

void LuaConsole::addOutput(const std::string &v) {
	string::size_type offset = 0;
	string::size_type n;
	do {
		n = v.find_first_of('\n', offset);
		m_output.push_back(v.substr(offset, n));
		offset = n + 1;
	} while (n != string::npos);
	while (m_output.size() > maxOutputLines) {
		m_output.pop_front();
	}
	stringstream ss;
	foreach (Strings, it, m_output) {
		if (it != m_output.begin()) {
			ss << "\n";
		}
		ss << *it;
	}
	m_outputBox->setText(ss.str(), true);
}

bool LuaConsole::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		if (!m_ui->isSelectingPos()) {
			BasicDialog::mouseDown(btn, pos);
			m_inputBox->gainFocus();
			return true;
		}
	}
	return false;
}

}}

// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_GUI_LUA_CONSOLE_H_
#define _GLEST_GAME_GUI_LUA_CONSOLE_H_

#include <string>

#include "util.h"
#include "game_util.h"
#include "metrics.h"
#include "framed_widgets.h"
#include "forward_decs.h"

using std::string;

using Glest::Global::Metrics;

namespace Glest { namespace Gui {

using namespace Widgets;

class LuaConsole;
class UserInterface;

// =====================================================
// class LuaInputBox
// =====================================================

class LuaInputBox : public TextBox {
private:
	typedef deque<string> Strings;

private:
	LuaConsole *m_console;
	Strings m_commandHistory;
	int m_historyPoint;

	void storeCommand();
	void recallCommand(bool reverse = false);

public:
	LuaInputBox(LuaConsole *console, Container *parent);
//	LuaInputBox(LuaConsole *console, Container *parent, Vec2i pos, Vec2i size);

	virtual bool keyDown(Key key);
	virtual void setStyle() override { setWidgetStyle(WidgetType::CODE_EDIT); }
	virtual string descType() const override { return "LuaInputBox"; }
};

// =====================================================
// class LuaConsole
// =====================================================

class LuaConsole : public BasicDialog {
private:
	typedef deque<string> Strings;
	static const int maxOutputLines = 16;

private:
	class CodeBox : public ScrollText {
	public:
		CodeBox(Container *parent) : ScrollText(parent) {
			setWidgetStyle(WidgetType::CODE_VIEW);
			m_staticText->setTextFont(textStyle().m_fontIndex);
		}
		virtual void setStyle() override { setWidgetStyle(WidgetType::CODE_VIEW); }
	};

private:
	UserInterface *m_ui;
	Strings m_output;

	LuaInputBox* m_inputBox;
	CodeBox*  m_outputBox;

	void onLineEntered(Widget*);

public:
	LuaConsole(UserInterface *ui, Container* parent);
	~LuaConsole();

	void addOutput(const string &v);

	virtual void setVisible(bool vis) override;	
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual string descType() const override { return "LuaConsole"; }
};

}}

#endif

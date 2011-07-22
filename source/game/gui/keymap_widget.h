// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_GUI__KEYMAP_WIDGET_H_
#define _GLEST_GAME_GUI__KEYMAP_WIDGET_H_

#include <string>
#include "framed_widgets.h"
#include "keymap.h"

namespace Glest { namespace Gui {

using namespace Widgets;

class HotKeyInputBox : public InputBox {
private:
	HotKey         m_hotKey;

public:
	HotKeyInputBox(Container *parent);

	virtual bool keyDown(Key key) override;
	virtual bool keyUp(Key key) override;
	virtual bool keyPress(char c) override;

	void setHotKey(HotKey hotKey);
	HotKey getHotKey() const { return m_hotKey; }

	sigslot::signal<Widget*> Changed;
};

class KeyEntryWidget : public CellStrip, public sigslot::has_slots {
private:
	StaticText     *m_label;
	HotKeyInputBox *m_inputBox1;
	HotKeyInputBox *m_inputBox2;
	MessageDialog  *m_msgBox;

	Keymap         &m_keymap;
	UserCommand    m_userCommand;

	bool       m_firstEntry;

	HotKey     m_prevValue;
	HotKey     m_nextValue;

	// event handlers
	void onHotKeyChanged(Widget*);
	void onConfirmHotKeyChange(Widget*);
	void onCancelHotKeyChange(Widget*);

	// bound assignment changed
	void onHotKeyAssignmentChanged(HotKeyAssignment *assignment);

public:
	KeyEntryWidget(Container *parent, Keymap &keymap, UserCommand uc);

};

class KeymapWidget : public CellStrip, public MouseWidget, public sigslot::has_slots {
private:
	ScrollBar      *m_scrollBar;
	KeyEntryWidget *m_keyEntryWidgets[UserCommand::COUNT - 1];
	int             m_kewStartOffsets[UserCommand::COUNT - 1];

	void onScroll(ScrollBar*);

public:
	KeymapWidget(Container *parent);

	virtual void setSize(const Vec2i &sz) override;

	virtual bool mouseWheel(Vec2i pos, int z) override { m_scrollBar->scrollLine(z > 0); return true; }
};

}}

#endif
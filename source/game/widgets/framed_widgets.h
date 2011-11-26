// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS__FRAMED_WIDGETS_H_
#define _GLEST_WIDGETS__FRAMED_WIDGETS_H_

#include "list_widgets.h"
#include "misc_widgets.h"
#include "ticker_tape.h" // Actions

namespace Glest { namespace Widgets {

class ButtonFlags {
private:
	int flags;

public:
	static const int numButtons = 4;
	enum E { CLOSE = 1, ROLL_UPDOWN = 2, EXPAND = 4, SHRINK = 8 };
	
	ButtonFlags() : flags(0) {}
	ButtonFlags(int f) : flags(f) {}

	bool isSet(E e) { return (flags & e); }

	int getCount() const {
		int n = 0;
		for (int i=0; i < numButtons; ++i) {
			if (flags & (1 << i)) {
				++n;
			}
		}
		return n;
	}
};

class TitleBar : public CellStrip, public sigslot::has_slots {
private:
	///@todo class TitlebarButton : Button { ... } just store a WidgetType...
#	define BUTTON_CLASS(X, WT)                      \
		class X : public Button {                   \
		public:                                     \
			X(Container *parent) : Button(parent) { \
				setWidgetStyle(WT);                 \
			};                                      \
			virtual void setStyle() override {      \
				setWidgetStyle(WT);                 \
			};                                      \
		};
	BUTTON_CLASS(CloseButton, WidgetType::TITLE_BAR_CLOSE);
	BUTTON_CLASS(RollUpButton, WidgetType::TITLE_BAR_ROLL_UP);
	BUTTON_CLASS(RollDownButton, WidgetType::TITLE_BAR_ROLL_DOWN);
	BUTTON_CLASS(ExpandButton, WidgetType::TITLE_BAR_EXPAND);
	BUTTON_CLASS(ShrinkButton, WidgetType::TITLE_BAR_SHRINK);

private:
	ButtonFlags      m_flags;
	StaticText      *m_titleText;
	CloseButton     *m_closeButton;
	RollUpButton    *m_rollUpButton;
	RollDownButton  *m_rollDownButton;
	ExpandButton    *m_expandButton;
	ShrinkButton    *m_shrinkButton;

	void init(ButtonFlags flags);
	void setSizeHints();

	// slots
	void onButtonClicked(Widget *btn);

public:
	TitleBar(Container* parent, ButtonFlags flags);
	//TitleBar(Container* parent, ButtonFlags flags, Vec2i pos, Vec2i size, string title);

	const string& getText() const { return m_titleText->getText(); }
	void setText(const string &txt) { m_titleText->setText(txt); }

	// Widget overrides
	virtual void setSize(const Vec2i &sz) override { CellStrip::setSize(sz); setSizeHints(); }
	virtual string descType() const override { return "TitleBar"; }

	void enableShrinkExpand(bool shrink, bool expand) {
		m_shrinkButton->setEnabled(shrink);
		m_expandButton->setEnabled(expand);
	}
	void showShrinkExpand(bool v) {
		m_shrinkButton->setVisible(v);
		m_expandButton->setVisible(v);
	}

	void swapRollUpDown() {
		if (m_rollUpButton->isVisible()) {
			m_rollUpButton->setVisible(false);
			m_rollDownButton->setVisible(true);
			m_rollDownButton->setEnabled(true);
		} else {
			m_rollUpButton->setVisible(true);
			m_rollUpButton->setEnabled(true);
			m_rollDownButton->setVisible(false);
		}
	}

	void disableRollUpDown() {
		m_rollDownButton->setEnabled(false);
		m_rollUpButton->setEnabled(false);
	}

	// signals
	sigslot::signal<Widget*> RollUp;
	sigslot::signal<Widget*> RollDown;
	sigslot::signal<Widget*> Expand;
	sigslot::signal<Widget*> Shrink;
	sigslot::signal<Widget*> Close;
};

class Frame : public CellStrip, public MouseWidget, public sigslot::has_slots {
protected:
	TitleBar*   m_titleBar;

	/// drag inf
	bool        m_pressed;
	Vec2i       m_lastPos;
	
	// rolling up/down stuff
	bool        m_rollingUp;
	bool        m_rollingDown;
	bool        m_rolledUp;
	Vec2i       m_origSize;
	bool        m_pinned;

	ResizeWidgetAction *m_resizerAction;

protected:
	Frame(WidgetWindow*, ButtonFlags flags);
	Frame(Container*, ButtonFlags flags);
	Frame(Container*, ButtonFlags flags, Vec2i pos, Vec2i sz);

	void init(ButtonFlags flags);

public:
	void setTitleText(const string &text);
	const string& getTitleText() const { return m_titleBar->getText(); }

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	virtual void update() override;

	void setPinned(bool v) { m_pinned = v; }
	bool getPinned() const { return m_pinned; }

	void setTitleBarSize(int sz) { CellStrip::setSizeHint(0, SizeHint(-1, sz)); }
	void enableShrinkExpand(bool shrink, bool expand) { m_titleBar->enableShrinkExpand(shrink, expand); }

	// signals
	sigslot::signal<Widget*>  Close;
	//sigslot::signal<Widget*>  RollUp;
	//sigslot::signal<Widget*>  RollDown;
	sigslot::signal<Widget*>  Shrink;
	sigslot::signal<Widget*>  Expand;

private:
	void onClose(Widget*)    { Close(this);    }
	void onRollUp(Widget*);//   { RollUp(this);   }
	void onRollDown(Widget*);// { RollDown(this); }
	void onShrink(Widget*)   { Shrink(this);   }
	void onExpand(Widget*)   { Expand(this);   }
};

class BasicDialog : public Frame {
private:
	Widget	  *m_content;
	Button	  *m_button1,
			  *m_button2;
	int		   m_buttonCount;
	CellStrip *m_btnPnl;

protected:
	BasicDialog(WidgetWindow*);
	BasicDialog(WidgetWindow*, ButtonFlags btnFlags);
	BasicDialog(Container*);//, Vec2i pos, Vec2i sz);
	BasicDialog(Container*, ButtonFlags btnFlags);
	void onButtonClicked(Widget*);

	void init();

protected:
	void setContent(Widget* content); //REMOVE: use cell 1
	void init(Vec2i pos, Vec2i size, const string &title, const string &btn1, const string &btn2);

public:
	void setButtonText(const string &btn1Text, const string &btn2Text = "");

	// signals
	sigslot::signal<Widget*>  Button1Clicked;
	sigslot::signal<Widget*>  Button2Clicked;
	sigslot::signal<Widget*>  Escaped;

	virtual string descType() const override { return "BasicDialog"; }
};

class MessageDialog : public BasicDialog {
private:
	ScrollText* m_scrollText;

private:
	MessageDialog(WidgetWindow*);

protected:
	MessageDialog(Container*);

public:
	static MessageDialog* showDialog(Vec2i pos, Vec2i size, const string &title,
					const string &msg, const string &btn1Text, const string &btn2Text);

	virtual ~MessageDialog();

	virtual void setMessageText(const string &text, ScrollAction action);
	const string& getMessageText() const { return m_scrollText->getText(); }

	virtual string descType() const override { return "MessageDialog"; }
};

// =====================================================
// class InputDialog
// =====================================================

class InputDialog : public BasicDialog {
private:
	StaticText*	m_label;
	InputBox*	m_inputBox;

private:
	InputDialog(WidgetWindow*);

	void onInputEntered(Widget*);
	void onEscaped(Widget*) { Escaped(this); }

public:
	static InputDialog* showDialog(Vec2i pos, Vec2i size, const string &title,
					const string &msg, const string &btn1Text, const string &btn2Text);

	void setMessageText(const string &text);
	void setInputMask(const string &allowMask) { m_inputBox->setInputMask(allowMask); }

	const string& getMessageText() const { return m_label->getText(); }
	string getInput() const { return m_inputBox->getText(); }

	virtual string descType() const override { return "InputDialog"; }
};

}} // namespace Glest::Widgets

#endif

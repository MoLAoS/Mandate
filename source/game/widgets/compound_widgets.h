// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_COMPOUND_WIDGETS_INCLUDED_
#define _GLEST_COMPOUND_WIDGETS_INCLUDED_

#include "complex_widgets.h"

namespace Glest { namespace Widgets {

class OptionContainer : public Container {
private:
	StaticText* m_label;
	Widget*		m_widget;

	bool	m_abosulteLabelSize;
	int		m_labelSize;

public:
	OptionContainer(Container* parent, Vec2i pos, Vec2i size, const string &labelText);

	void setLabelWidth(int value, bool absolute);

	///@todo deprecate, over addChild, assume second child is contained widget
	void setWidget(Widget* widget);
	Widget* getWidget() { return m_widget; }

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	virtual string descType() const override { return "OptionBox"; }
};

class ScrollText : public CellStrip, public TextWidget, public sigslot::has_slots {
private:
	ScrollBar  *m_scrollBar;
	StaticText *m_staticText;
	string      m_origString;

private:
	void init();

public:
	ScrollText(Container* parent);
	ScrollText(Container* parent, Vec2i pos, Vec2i size);

	void recalc();
	void onScroll(int offset);
	void setText(const string &txt, bool scrollToBottom = false);

	virtual void setSize(const Vec2i &sz) override;
	virtual void render() override;
};

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
	void onButtonClicked(Button *btn);

public:
	TitleBar(Container* parent, ButtonFlags flags);
	//TitleBar(Container* parent, ButtonFlags flags, Vec2i pos, Vec2i size, string title);

	const string& getText() const { return m_titleText->getText(); }
	void setText(const string &txt) { m_titleText->setText(txt); }

	// Widget overrides
	virtual void setSize(const Vec2i &sz) override { CellStrip::setSize(sz); setSizeHints(); }
	virtual string descType() const override { return "TitleBar"; }

	// signals
	sigslot::signal<TitleBar*> RollUp;
	sigslot::signal<TitleBar*> RollDown;
	sigslot::signal<TitleBar*> Expand;
	sigslot::signal<TitleBar*> Shrink;
	sigslot::signal<TitleBar*> Close;
};

class Frame : public CellStrip, public MouseWidget {
protected:
	TitleBar*   m_titleBar;
	bool        m_pressed;
	Vec2i       m_lastPos;

protected:
	Frame(WidgetWindow*, ButtonFlags flags);
	Frame(Container*, ButtonFlags flags);
	Frame(Container*, ButtonFlags flags, Vec2i pos, Vec2i sz);

	void init(ButtonFlags flags);

public:
	void setTitleText(const string &text);
	const string& getTitleText() const { return m_titleBar->getText(); }

	bool mouseDown(MouseButton btn, Vec2i pos) override;
	bool mouseMove(Vec2i pos) override;
	bool mouseUp(MouseButton btn, Vec2i pos) override;
};

class BasicDialog : public Frame, public sigslot::has_slots {
private:
	Widget	  *m_content;
	Button	  *m_button1,
			  *m_button2;
	int		   m_buttonCount;
	CellStrip *m_btnPnl;

protected:
	BasicDialog(WidgetWindow*);
	BasicDialog(Container*);//, Vec2i pos, Vec2i sz);
	void onButtonClicked(Button*);

	void init();

protected:
	void setContent(Widget* content); //REMOVE: use cell 1
	void init(Vec2i pos, Vec2i size, const string &title, const string &btn1, const string &btn2);

public:
	void setButtonText(const string &btn1Text, const string &btn2Text = "");

	sigslot::signal<BasicDialog*>	Button1Clicked,
									Button2Clicked,
									Escaped;

	virtual string descType() const override { return "BasicDialog"; }
};

class MessageDialog : public BasicDialog {
private:
	ScrollText* m_scrollText;

private:
	MessageDialog(WidgetWindow*);

public:
	static MessageDialog* showDialog(Vec2i pos, Vec2i size, const string &title,
					const string &msg, const string &btn1Text, const string &btn2Text);

	virtual ~MessageDialog();

	void setMessageText(const string &text);
	const string& getMessageText() const { return m_scrollText->getText(); }

	virtual string descType() const override { return "MessageDialog"; }
};

// =====================================================
// class InputBox
// =====================================================

class InputBox : public TextBox {
public:
	InputBox(Container *parent);
//	InputBox(Container *parent, Vec2i pos, Vec2i size);

	virtual bool keyDown(Key key) override;
	sigslot::signal<InputBox*> Escaped;
	virtual string descType() const override { return "InputBox"; }
};

// =====================================================
// class InputDialog
// =====================================================

class InputDialog : public BasicDialog {
private:
	StaticText*	m_label;
	InputBox*	m_inputBox;
	Panel*		m_panel;

private:
	InputDialog(WidgetWindow*);

	void onInputEntered(TextBox*);
	void onEscaped(InputBox*) { Escaped(this); }

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

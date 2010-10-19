// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_COMPOUND_WIDGETS_INCLUDED_
#define _GLEST_COMPOUND_WIDGETS_INCLUDED_

#include "widgets.h"

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

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual string desc() { return string("[OptionBox: ") + descPosDim() + "]"; }
};

class ScrollText : public Panel, public MouseWidget, public TextWidget, public sigslot::has_slots {
private:
	VerticalScrollBar* m_scrollBar;
	int m_textBase;

public:
	ScrollText(Container* parent);
	ScrollText(Container* parent, Vec2i pos, Vec2i size);

	void recalc();
	void init();
	void onScroll(VerticalScrollBar*);
	void setText(const string &txt, bool scrollToBottom = false);

	void render();
};

class TitleBar : public Container, public TextWidget {
private:
	string		m_title;
	Button* m_closeButton;

public:
	TitleBar(Container* parent);
	TitleBar(Container* parent, Vec2i pos, Vec2i size, string title, bool closeBtn);

	void render();

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual string desc() { return string("[TitleBar: ") + descPosDim() + "]"; }
};

class Frame : public Container, public MouseWidget {
protected:
	TitleBar*	m_titleBar;
	bool			m_pressed;
	Vec2i			m_lastPos;

protected:
	Frame(WidgetWindow*);
	Frame(Container*);
	Frame(Container*, Vec2i pos, Vec2i sz);

public:
	void init(Vec2i pos, Vec2i size, const string &title);
	void setTitleText(const string &text);
	const string& getTitleText() const { return m_titleBar->getText(); }

	bool mouseDown(MouseButton btn, Vec2i pos);
	bool mouseMove(Vec2i pos);
	bool mouseUp(MouseButton btn, Vec2i pos);

	virtual void render();
	virtual void setSize(Vec2i size);
};

class BasicDialog : public Frame, public sigslot::has_slots {
private:
	//TitleBar*	m_titleBar;
	Widget	*m_content;
	Button	*m_button1,
			*m_button2;

	int		 m_buttonCount;

protected:
	BasicDialog(WidgetWindow*);
	BasicDialog(Container*, Vec2i pos, Vec2i sz);
	void onButtonClicked(Button*);

protected:
	void setContent(Widget* content);
	void init(Vec2i pos, Vec2i size, const string &title, const string &btn1, const string &btn2);

public:
	void setButtonText(const string &btn1Text, const string &btn2Text = "");

	sigslot::signal<BasicDialog*>	Button1Clicked,
									Button2Clicked,
									Escaped;

	void render();
	virtual Vec2i getPrefSize() const { return Vec2i(-1); }
	virtual Vec2i getMinSize() const { return Vec2i(-1); }
	virtual string desc() { return string("[BasicDialog: ") + descPosDim() + "]"; }
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

	virtual string desc() { return string("[MessageDialog: ") + descPosDim() + "]"; }
};

// =====================================================
// class InputBox
// =====================================================

class InputBox : public TextBox {
public:
	InputBox(Container *parent);
	InputBox(Container *parent, Vec2i pos, Vec2i size);

	virtual bool keyDown(Key key);
	sigslot::signal<InputBox*> Escaped;
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

	virtual string desc() { return string("[InputDialog: ") + descPosDim() + "]"; }
};

}} // namespace Glest::Widgets

#endif

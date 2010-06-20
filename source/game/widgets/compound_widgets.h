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
#include "game_constants.h"

namespace Glest { namespace Widgets {
using Sim::ControlType;

class PlayerSlotWidget : public Panel, public sigslot::has_slots {
public:
	typedef PlayerSlotWidget* Ptr;
private:
	StaticText::Ptr m_label;
	DropList::Ptr	m_controlList;
	DropList::Ptr	m_factionList;
	DropList::Ptr	m_teamList;

public:
	PlayerSlotWidget(Container::Ptr parent, Vec2i pos, Vec2i size);

	void setNameText(const string &name) { m_label->setText(name); }
	
	void setFactionItems(const vector<string> &items) {
		m_factionList->clearItems();
		m_factionList->addItems(items);
	}

	void setSelectedControl(ControlType ct) {
		m_controlList->setSelected(ct);
	}
	void setSelectedFaction(int ndx) {
		m_factionList->setSelected(ndx);
	}

	void setSelectedTeam(int team) {
		assert (team >= -1 && team < GameConstants::maxPlayers);
		m_teamList->setSelected(team);
	}

	ControlType getControlType() const { return ControlType(m_controlList->getSelectedIndex()); }
	int getSelectedFactionIndex() const { return m_factionList->getSelectedIndex(); }
	int getSelectedTeamIndex() const { return m_teamList->getSelectedIndex(); }

	sigslot::signal<Ptr> ControlChanged;
	sigslot::signal<Ptr> FactionChanged;
	sigslot::signal<Ptr> TeamChanged;

private: // re-route signals from DropLists
	void onControlChanged(ListBase::Ptr) {
		ControlChanged(this);
		if (m_controlList->getSelectedIndex() == ControlType::CLOSED) {
			m_factionList->setEnabled(false);
			m_teamList->setEnabled(false);
		} else {
			m_factionList->setEnabled(true);
			m_teamList->setEnabled(true);
		}
	}
	void onFactionChanged(ListBase::Ptr) { FactionChanged(this); }
	void onTeamChanged(ListBase::Ptr)	 { TeamChanged(this);	 }
};

class OptionContainer : public Container {
public:
	typedef OptionContainer* Ptr;

private:
	StaticText::Ptr m_label;
	Widget::Ptr		m_widget;

	bool	m_abosulteLabelSize;
	int		m_labelSize;

public:
	OptionContainer(Container::Ptr parent, Vec2i pos, Vec2i size, const string &labelText);

	void setLabelWidth(int value, bool absolute);

	///@todo deprecate, over addChild, assume second child is contained widget
	void setWidget(Widget::Ptr widget);
	Widget::Ptr getWidget() { return m_widget; }

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual string desc() { return string("[OptionBox: ") + descPosDim() + "]"; }
};

class ScrollText : public Panel, public MouseWidget, public TextWidget, public sigslot::has_slots {
public:
	typedef ScrollText* Ptr;

private:
	VerticalScrollBar::Ptr m_scrollBar;
	int m_textBase;

public:
	ScrollText(Container::Ptr parent);
	ScrollText(Container::Ptr parent, Vec2i pos, Vec2i size);

	void init();
	void onScroll(VerticalScrollBar::Ptr);

	void render();
};

class TitleBar : public Container, public TextWidget {
public:
	typedef TitleBar* Ptr;

private:
	string		m_title;
	Button::Ptr m_closeButton;

public:
	TitleBar(Container::Ptr parent);
	TitleBar(Container::Ptr parent, Vec2i pos, Vec2i size, string title, bool closeBtn);

	void render();

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual string desc() { return string("[TitleBar: ") + descPosDim() + "]"; }
};

class MessageDialog : public Container, public MouseWidget, public sigslot::has_slots {
public:
	typedef MessageDialog* Ptr;

private:
	TitleBar::Ptr	m_titleBar;
	ScrollText::Ptr	m_scrollText;
	Button::Ptr		m_button1,
					m_button2;

	int				m_buttonCount;
	bool			m_pressed;
	Vec2i			m_lastPos;

	void onButtonClicked(Button::Ptr);
	void init();

public:
	MessageDialog(WidgetWindow*);
	MessageDialog(Container::Ptr parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz);

	void setTitleText(const string &text);
	void setMessageText(const string &text);
	void setButtonText(const string &btn1Text, const string &btn2Text = "");

	sigslot::signal<Ptr> Button1Clicked,
						 Button2Clicked;

	bool EW_mouseDown(MouseButton btn, Vec2i pos);
	bool EW_mouseMove(Vec2i pos);
	bool EW_mouseUp(MouseButton btn, Vec2i pos);

	void render();

	virtual Vec2i getPrefSize() const { return Vec2i(-1); }
	virtual Vec2i getMinSize() const { return Vec2i(-1); }

	virtual string desc() { return string("[MessageDialog: ") + descPosDim() + "]"; }
};

}} // namespace Glest::Widgets

#endif

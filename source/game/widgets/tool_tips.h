// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_TOOL_TIPS_INCLUDED_
#define _GLEST_WIDGETS_TOOL_TIPS_INCLUDED_

#include "widgets.h"
#include "element_type.h"
#include "config.h"
#include "command_type.h"
#include "trade_command.h"
#include "formations.h"

namespace Glest { namespace Widgets {

using namespace Global;
using ProtoTypes::DisplayableType;
using ProtoTypes::CmdDescriptor;
using Gui_Mandate::TradeDescriptor;
using Hierarchy::FormationDescriptor;

class CommandTipHeader : public StaticText {
public:
	CommandTipHeader(Container *parent) : StaticText(parent) {
		setWidgetStyle(WidgetType::TOOLTIP_HEADER);
		TextWidget::setText("");
		TextWidget::setTextPos(Vec2i(getBorderLeft(), getBorderTop()));
	}

	void setHeaderText(const string &txt) {
		int max_w = g_config.getDisplayWidth() / 5;
		string wrapped = txt;
		getFont()->getMetrics()->wrapText(wrapped, max_w);
		TextWidget::setText(wrapped);
	}

	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP_HEADER); }
};

class CommandTipMain : public StaticText {
public:
	CommandTipMain(Container *parent) : StaticText(parent) {
		setWidgetStyle(WidgetType::TOOLTIP_MAIN);
		TextWidget::setText("");
		TextWidget::setTextPos(Vec2i(getBorderLeft(), getBorderTop()));
	}

	void setTipText(const string &txt) {
		int max_w = g_config.getDisplayWidth() / 5;
		string wrapped = txt;
		getFont()->getMetrics()->wrapText(wrapped, max_w);
		TextWidget::setText(wrapped);
	}

	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP_MAIN); }
};

class CommandTipElement : public StaticText {
public:
	CommandTipElement(Container *parent, const string &msg);
	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP_ITEM); }
};

class CommandTipItem : public CommandTipElement, public ImageWidget {
public:
	CommandTipItem(Container *parent, const DisplayableType *dt, const string &msg);

	virtual Vec2i getPrefSize() const override;
	virtual void render() override;
};

class CommandTipReq : public CommandTipItem {
private:
	bool    m_reqMet;

public:
	CommandTipReq(Container *parent, const DisplayableType *dt, bool ok, const string &msg);

	virtual void setStyle() override;
};

// =====================================================
//  class CommandTip
// =====================================================

class CommandTip : public Container, public CmdDescriptor {
private:
	typedef vector<CommandTipElement*> Elements;
private:
	CommandTipHeader  *m_header;
	CommandTipMain    *m_tip;
	Elements           m_items;

	string             m_commandName;
	string             m_tipText;

	void layout();

public:
	CommandTip(Container *parent);

	virtual void setHeader(const string &header) override;
	virtual void setTipText(const string &mainText) override;

	virtual void addElement(const string &msg) override;
	virtual void addItem(const DisplayableType *dt, const string &msg) override;
	virtual void addReq(const DisplayableType *dt, bool ok, const string &msg) override;

	void clearItems();

	bool isEmpty() const { return m_header->getText().empty() &&  m_tip->getText().empty(); }

	// Widget overrides
	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP); }
	virtual string descType() const override { return "Tooltip"; }
};

// =====================================================
//  class WidgetTip
// =====================================================

class WidgetTip : public Container {
private:
	StaticText  *m_tipLabel;
	Widget      *m_myWidget; // the widget I am a tip for
	void init();

public:
	WidgetTip(WidgetWindow *window, Widget* widget, const string &text);
//	WidgetTip(WidgetWindow *window, Widget* widget, Vec2i pos, Vec2i size);

	void setText(const string &txt);

	// Widget overrides
	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP); }
	virtual string descType() const override { return "Tooltip"; }
};

// =====================================================
//  Items For Trade Tip
// =====================================================

class TradeTipHeader : public StaticText {
public:
	TradeTipHeader(Container *parent) : StaticText(parent) {
		setWidgetStyle(WidgetType::TOOLTIP_HEADER);
		TextWidget::setText("");
		TextWidget::setTextPos(Vec2i(getBorderLeft(), getBorderTop()));
	}

	void setHeaderText(const string &txt) {
		int max_w = g_config.getDisplayWidth() / 5;
		string wrapped = txt;
		getFont()->getMetrics()->wrapText(wrapped, max_w);
		TextWidget::setText(wrapped);
	}

	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP_HEADER); }
};

class TradeTipMain : public StaticText {
public:
	TradeTipMain(Container *parent) : StaticText(parent) {
		setWidgetStyle(WidgetType::TOOLTIP_MAIN);
		TextWidget::setText("");
		TextWidget::setTextPos(Vec2i(getBorderLeft(), getBorderTop()));
	}

	void setTipText(const string &txt) {
		int max_w = g_config.getDisplayWidth() / 5;
		string wrapped = txt;
		getFont()->getMetrics()->wrapText(wrapped, max_w);
		TextWidget::setText(wrapped);
	}

	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP_MAIN); }
};

class TradeTipElement : public StaticText {
public:
	TradeTipElement(Container *parent, const string &msg);
	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP_ITEM); }
};

class TradeTipItem : public TradeTipElement, public ImageWidget {
public:
	TradeTipItem(Container *parent, const DisplayableType *dt, const string &msg);

	virtual Vec2i getPrefSize() const override;
	virtual void render() override;
};

class TradeTipReq : public TradeTipItem {
private:
	bool    m_reqMet;

public:
	TradeTipReq(Container *parent, bool ok, const DisplayableType *dt, const string &msg);

	virtual void setStyle() override;
};

// =====================================================
//  class TradeTip
// =====================================================

class TradeTip : public Container, public TradeDescriptor {
private:
	typedef vector<TradeTipElement*> Elements;
private:
	TradeTipHeader     *m_header;
	TradeTipMain       *m_tip;
	Elements           m_items;

	string             m_tradeName;
	string             m_tipText;

	void layout();

public:
	TradeTip(Container *parent);

	virtual void setHeader(const string &header) override;
	virtual void setTipText(const string &mainText) override;
	virtual void addElement(const string &msg) override;
	virtual void addItem(const DisplayableType *dt, const string &msg) override;
	virtual void addReq(bool ok, const DisplayableType *dt, const string &msg) override;

	void clearItems();

	bool isEmpty() const { return m_header->getText().empty() &&  m_tip->getText().empty(); }

	// Widget overrides
	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP); }
	virtual string descType() const override { return "Tooltip"; }
};

// =====================================================
//  class FormationTip
// =====================================================

class FormationTip : public Container, public FormationDescriptor {
private:
	typedef vector<TradeTipElement*> Elements;
private:
	TradeTipHeader     *m_header;
	TradeTipMain       *m_tip;
	Elements           m_items;

	string             m_tradeName;
	string             m_tipText;

	void layout();

public:
	FormationTip(Container *parent);

	virtual void setHeader(const string &header) override;
	virtual void setTipText(const string &mainText) override;
	virtual void addElement(const string &msg) override;
	virtual void addItem(const DisplayableType *dt, const string &msg) override;
	virtual void addReq(bool ok, const DisplayableType *dt, const string &msg) override;

	void clearItems();

	bool isEmpty() const { return m_header->getText().empty() &&  m_tip->getText().empty(); }

	// Widget overrides
	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOLTIP); }
	virtual string descType() const override { return "Tooltip"; }
};

}}

#endif

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

namespace Glest { namespace Widgets {

using namespace Global;
using ProtoTypes::DisplayableType;

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

class CommandTipReq : public StaticText, public ImageWidget {
private:
	bool    m_reqMet;

public:
	CommandTipReq(Container *parent, const DisplayableType *dt, bool ok, const string &msg);

	virtual Vec2i getPrefSize() const override;
	virtual void setStyle() override;
	virtual void render() override;
};

// =====================================================
//  class CommandTip
// =====================================================

class CommandTip : public Container {
private:
	CommandTipHeader      *m_header;
	CommandTipMain        *m_tip;
	vector<CommandTipReq*> m_reqs;

	string                 m_commandName;
	string                 m_tipText;

	void layout();

public:
	CommandTip(Container *parent);

	void setHeader(const string &header);
	void setTipText(const string &mainText);
	void addReq(const DisplayableType *dt, bool ok, const string &msg);

	void clearReqs();

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

}}

#endif

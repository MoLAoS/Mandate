// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_SLIDER_INCLUDED_
#define _GLEST_WIDGETS_SLIDER_INCLUDED_

#include "widgets.h"
#include "widget_config.h"

namespace Glest { namespace Widgets {

// =====================================================
//  class SliderThumb
// =====================================================

class SliderThumb : public Button {
private:
	WidgetType m_type;
	Vec2i      m_downPos; // 'mouse down' pos

public:
	SliderThumb(Container *parent, bool vert);

	virtual void setStyle() override { setWidgetStyle(m_type); }

	virtual void setPos(const Vec2i &pos) override;
	virtual void setSize(const Vec2i &sz) override;

	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	sigslot::signal<int> Moved;
	virtual string descType() const override { return "SliderThumb"; }
};

// =====================================================
//  class SliderShaft
// =====================================================

class SliderShaft : public Container, public MouseWidget, public sigslot::has_slots {
private:
	WidgetType   m_type;
	SliderThumb *m_thumb;
	int          m_range;
	int          m_value;
	Vec2i        m_dragPos;
	bool         m_dragging;

private:
	static const int thumbSize;

private:
	void recalc();
	void onThumbDragged(int diff);

public:
	SliderShaft(Container *parent, bool vert);

	virtual void setPos(const Vec2i &pos) override;
	virtual void setSize(const Vec2i &sz) override;
	virtual void setStyle() override { setWidgetStyle(m_type); }

	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	void setRange(int range);
	void setValue(int value);

	int getRange() const { return m_range; }
	int getValue() const { return m_value; }

	sigslot::signal<SliderShaft*> ThumbMoved;

	virtual string descType() const override { return "SliderShaft"; }
};

// =====================================================
//  class Slider
// =====================================================

class Slider2 : public Container, public sigslot::has_slots {
private:
	SliderShaft *m_shaft;

private:
	void onValueChanged(SliderShaft*) { ValueChanged(this); }

public:
	Slider2(Container *parent, bool vert);

	virtual void setStyle() override { setWidgetStyle(WidgetType::SLIDER); }
	virtual void setPos(const Vec2i &pos) override;
	virtual void setSize(const Vec2i &sz) override;

	void setRange(int range) { m_shaft->setRange(range); }
	void setValue(int value) { m_shaft->setValue(value); }

	int getRange() const { return m_shaft->getRange(); }
	int getValue() const { return m_shaft->getValue(); }

	sigslot::signal<Slider2*> ValueChanged;

	virtual string descType() const override { return "Slider"; }
};

}}

#endif

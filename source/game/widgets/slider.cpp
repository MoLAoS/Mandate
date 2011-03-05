// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "slider.h"
#include "widget_window.h"

#include "leak_dumper.h"

using Shared::Graphics::Texture;
using namespace Shared::Graphics::Gl;

namespace Glest { namespace Widgets {

// =====================================================
//  class SliderThumb
// =====================================================

SliderThumb::SliderThumb(Container *parent, bool vert)
		: Button(parent) {
	if (vert) {
		m_type = WidgetType::SLIDER_VERT_THUMB;
	} else {
		m_type = WidgetType::SLIDER_HORIZ_THUMB;
	}
	setWidgetStyle(m_type);
}

void SliderThumb::setPos(const Vec2i &pos) {
	if (isFocused()) {
		if (m_type == WidgetType::SLIDER_VERT_THUMB) {
			int diff = getPos().y - pos.y;
			m_downPos.y -= diff;
		} else {
			int diff = getPos().x - pos.x;
			m_downPos.x -= diff;
		}
	}
	Widget::setPos(pos);
}

void SliderThumb::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
}

bool SliderThumb::mouseMove(Vec2i pos) {
	Button::mouseMove(pos);
	if (isFocused()) {
		int diff;
		if (m_type == WidgetType::SLIDER_VERT_THUMB) {
			diff = m_downPos.y - pos.y;
		} else {
			diff = m_downPos.x - pos.x;
		}
		Moved(diff);
	}
	return true;
}

bool SliderThumb::mouseDown(MouseButton btn, Vec2i pos) {
	Button::mouseDown(btn, pos);
	if (btn == MouseButton::LEFT) {
		m_downPos = pos;
	}
	return true;
}

bool SliderThumb::mouseUp(MouseButton btn, Vec2i pos) {
	Button::mouseUp(btn, pos);
	if (btn == MouseButton::LEFT) {
		m_downPos = Vec2i(-1);
	}
	return true;
}

// =====================================================
//  class SliderShaft
// =====================================================

SliderShaft::SliderShaft(Container *parent, bool vert)
		: Container(parent)
		, MouseWidget(this)
		, m_range(-1), m_value(-1)
		, m_dragging(false) {
	if (vert) {
		m_type = WidgetType::SLIDER_VERT_SHAFT;
	} else {
		m_type = WidgetType::SLIDER_HORIZ_SHAFT;
	}
	m_thumb = new SliderThumb(this, vert);
	m_thumb->Moved.connect(this, &SliderShaft::onThumbDragged);
	setWidgetStyle(m_type);
}

void SliderShaft::recalc() {
	if (m_range != -1 && m_value != -1 && getSize() != Vec2i(0)) {
		int space;
		if (m_type == WidgetType::SLIDER_VERT_SHAFT) {
			space = getHeight() - getBordersVert() - thumbSize;
		} else {
			space = getWidth() - getBordersHoriz() - thumbSize;
		}
		float val_ratio = m_value / float(m_range);
		int pos = int(val_ratio * space);
		Vec2i thumbPos, thumbSz;
		if (m_type == WidgetType::SLIDER_VERT_SHAFT) {
			thumbPos = Vec2i(getBorderLeft(), pos + getBorderTop() /*- thumbSize / 2*/);
			thumbSz = Vec2i(getWidth() - getBordersHoriz(), thumbSize);
		} else {
			thumbPos = Vec2i(pos + getBorderLeft() /*- thumbSize / 2*/, getBorderTop());
			thumbSz = Vec2i(thumbSize, getHeight() - getBordersVert());
		}
		m_thumb->setPos(thumbPos);
		m_thumb->setSize(thumbSz);		
	}
}

void SliderShaft::onThumbDragged(int diff) {
	int space, currOffset;
	Vec2i thumbPos = m_thumb->getPos();
	if (m_type == WidgetType::SLIDER_VERT_SHAFT) {
		space = getHeight() - getBordersVert() - thumbSize;
		currOffset = thumbPos.y - getBorderTop();
	} else {
		space = getWidth() - getBordersHoriz() - thumbSize;
		currOffset = thumbPos.x - getBorderLeft();
	}
	int newOffset = clamp(currOffset - diff, 0, space);
	if (m_type == WidgetType::SLIDER_VERT_SHAFT) {
		thumbPos.y = getBorderTop() + newOffset;
	} else {
		thumbPos.x = getBorderLeft() + newOffset;
	}
	m_thumb->setPos(thumbPos);
	float ratio = newOffset / float(space);
	int newValue = clamp(int(ratio * m_range), 0, m_range);
	if (m_value != newValue) {
		m_value = newValue;
		ThumbMoved(this);
	}
}

void SliderShaft::setPos(const Vec2i &pos) {
	Container::setPos(pos);
}

void SliderShaft::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	recalc();
}

void SliderShaft::setRange(int range) {
	m_range = range;
	recalc();
}

void SliderShaft::setValue(int value) {
	m_value = value;
	recalc();
}

bool SliderShaft::mouseDown(MouseButton btn, Vec2i pos) {
	Vec2i p = pos - getScreenPos();
	if (btn == MouseButton::LEFT) {
		if (p.x >= getBorderLeft() && p.x < getWidth() - getBorderRight()
		&& p.y >= getBorderTop() && p.y < getHeight() - getBorderBottom()) {
			Vec2i thumbPos = m_thumb->getScreenPos();
			int diff;
			if (m_type == WidgetType::SLIDER_VERT_SHAFT) {
				diff = pos.y - (thumbPos.y + thumbSize / 2);
			} else {
				diff = pos.x - (thumbPos.x + thumbSize / 2);
			}
			onThumbDragged(-diff);
			m_dragPos = pos;
			m_dragging = true;
		}
	}
	return true;
}

bool SliderShaft::mouseMove(Vec2i pos) {
	if (m_dragging) {
		int diff;
		if (m_type == WidgetType::SLIDER_VERT_SHAFT) {
			diff = pos.y - m_dragPos.y;
		} else {
			diff = pos.x - m_dragPos.x;
		}
		onThumbDragged(-diff);
		m_dragPos = pos;
	}
	return true;
}

bool SliderShaft::mouseUp(MouseButton btn, Vec2i pos) {
	if (m_dragging) {
		m_dragging = false;
	}
	return true;
}

// =====================================================
//  class Slider
// =====================================================

Slider2::Slider2(Container *parent, bool vert)
		: WidgetCell(parent) {
	m_shaft = new SliderShaft(this, vert);
	Anchors anchors(Anchor(AnchorType::RIGID, getBorderLeft()), Anchor(AnchorType::RIGID, getBorderTop()),
		Anchor(AnchorType::RIGID, getBorderRight()), Anchor(AnchorType::RIGID, getBorderBottom()));
	m_shaft->setAnchors(anchors);
	m_shaft->ThumbMoved.connect(this, &Slider2::onValueChanged);
	setWidgetStyle(WidgetType::SLIDER);
}

void Slider2::setPos(const Vec2i &pos) {
	WidgetCell::setPos(pos);
}

void Slider2::setSize(const Vec2i &sz) {
	WidgetCell::setSize(sz);
}

}}

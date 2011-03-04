// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "scroll_bar.h"
#include "widget_window.h"

#include "leak_dumper.h"

using Shared::Graphics::Texture;
using namespace Shared::Graphics::Gl;

namespace Glest { namespace Widgets {

// =====================================================
//  class ScrollBarButton
// =====================================================

ScrollBarButton::ScrollBarButton(Container *parent, Direction dir)
		: Button(parent)
		, m_down(false)
		, m_counter(0)
		, m_fireOnUp(false) {
	switch (dir) {
		case Direction::LEFT: m_type = WidgetType::SCROLLBAR_BTN_LEFT; break;
		case Direction::UP: m_type = WidgetType::SCROLLBAR_BTN_UP; break;
		case Direction::RIGHT: m_type = WidgetType::SCROLLBAR_BTN_RIGHT; break;
		case Direction::DOWN: m_type = WidgetType::SCROLLBAR_BTN_DOWN; break;
		default: throw runtime_error("ScrollBarButton::ScrollBarButton() : bad direction");
	}
	setWidgetStyle(m_type);
}

ScrollBarButton::ScrollBarButton(Container *parent, Vec2i pos, Vec2i size, Direction dir)
		: Button(parent, pos, size)
		, m_down(false)
		, m_counter(0)
		, m_fireOnUp(false) {
	switch (dir) {
		case Direction::LEFT: m_type = WidgetType::SCROLLBAR_BTN_LEFT; break;
		case Direction::UP: m_type = WidgetType::SCROLLBAR_BTN_UP; break;
		case Direction::RIGHT: m_type = WidgetType::SCROLLBAR_BTN_RIGHT; break;
		case Direction::DOWN: m_type = WidgetType::SCROLLBAR_BTN_DOWN; break;
		default: throw runtime_error("ScrollBarButton::ScrollBarButton() : bad direction");
	}
	setWidgetStyle(m_type);
}

bool ScrollBarButton::mouseMove(Vec2i pos) {
	WIDGET_LOG( descLong() << " : ScrollBarButton::mouseMove( " << pos << " )");
	Button::mouseMove(pos);
	if (m_down) {
		if (!isHovered()) {
			m_rootWindow->unregisterUpdate(this);
			m_down = false;
		}
	} else { // !m_down
		if (isHovered() && isFocused()) {
			m_rootWindow->registerUpdate(this);
			m_counter = 0;
			m_down = true;
		}
	}
	return true;
}

bool ScrollBarButton::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( descLong() << " : ScrollBarButton::mouseDown( " << MouseButtonNames[btn] << ", " << pos << " )");
	Button::mouseDown(btn, pos);
	if (isFocused() && !m_down) {
		m_rootWindow->registerUpdate(this);
		m_counter = 0;
		m_down = true;
		m_fireOnUp = true;
	}
	return true;
}

bool ScrollBarButton::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( descLong() << " : ScrollBarButton::mouseUp( " << MouseButtonNames[btn] << ", " << pos << " )");
	if (isEnabled() && btn == MouseButton::LEFT) {
		if (isFocused() && isHovered() && m_fireOnUp) {
			Fire(this);
		}
		setFocus(false);
		if (m_down) {
			m_rootWindow->unregisterUpdate(this);
			m_down = false;
		}
	}
	return true;
}

void ScrollBarButton::update() {
	++m_counter;
	if (m_counter % 5 == 0) {
		Fire(this);
		m_fireOnUp = false;
	}
}

// =====================================================
//  class ScrollBarThumb
// =====================================================

ScrollBarThumb::ScrollBarThumb(ScrollBarShaft *parent, bool vert)
		: Button(parent), m_downPos(-1) {
	if (vert) {
		m_type = WidgetType::SCROLLBAR_VERT_THUMB;
	} else {
		m_type = WidgetType::SCROLLBAR_HORIZ_THUMB;
	}
	setWidgetStyle(m_type);
}

ScrollBarThumb::ScrollBarThumb(ScrollBarShaft *parent, Vec2i pos, Vec2i size, bool vert)
		: Button(parent, pos, size), m_downPos(-1) {
	if (vert) {
		m_type = WidgetType::SCROLLBAR_VERT_THUMB;
	} else {
		m_type = WidgetType::SCROLLBAR_HORIZ_THUMB;
	}
	setWidgetStyle(m_type);
}

void ScrollBarThumb::setPos(const Vec2i &pos) {
	if (isFocused()) {
		if (m_type == WidgetType::SCROLLBAR_VERT_THUMB) {
			int diff = getPos().y - pos.y;
			m_downPos.y -= diff;
		} else {
			int diff = getPos().x - pos.x;
			m_downPos.x -= diff;
		}
	}
	Widget::setPos(pos);
}

bool ScrollBarThumb::mouseMove(Vec2i pos) {
	Button::mouseMove(pos);
	if (isFocused()) {
		int diff;
		if (m_type == WidgetType::SCROLLBAR_VERT_THUMB) {
			diff = m_downPos.y - pos.y;
		} else {
			diff = m_downPos.x - pos.x;
		}
		Moved(diff);
	}
	return true;
}

bool ScrollBarThumb::mouseDown(MouseButton btn, Vec2i pos) {
	Button::mouseDown(btn, pos);
	if (btn == MouseButton::LEFT) {
		m_downPos = pos;
	}
	return true;
}

bool ScrollBarThumb::mouseUp(MouseButton btn, Vec2i pos) {
	Button::mouseUp(btn, pos);
	if (btn == MouseButton::LEFT) {
		m_downPos = Vec2i(-1);
	}
	return true;
}

// =====================================================
//  class ScrollBarShaft
// =====================================================

ScrollBarShaft::ScrollBarShaft(Container *parent, bool vert)
		: Container(parent)
		, MouseWidget(this) {
	init(vert);
}

ScrollBarShaft::ScrollBarShaft(Container *parent, Vec2i pos, Vec2i sz, bool vert)
		: Container(parent, pos, sz)
		, MouseWidget(this) {
	init(vert);
	recalc();
}

void ScrollBarShaft::init(bool vert) {
	if (vert) {
		m_type = WidgetType::SCROLLBAR_VERT_SHAFT;
	} else {
		m_type = WidgetType::SCROLLBAR_HORIZ_SHAFT;
	}
	m_totalRange = 100;
	m_availRange = 10;
	m_thumb = new ScrollBarThumb(this, vert);
	m_thumb->Moved.connect(this, &ScrollBarShaft::onThumbMoved);
	setWidgetStyle(m_type);
}

void ScrollBarShaft::recalc() {
	if (!m_totalRange) {
		return;
	}
	Vec2i size = getSize() - getBordersAll();
	m_availRatio = clamp(m_availRange / float(m_totalRange), 0.05f, 1.f);
	if (isVertical()) {
		m_thumbSize = int(m_availRatio * size.h);
		m_maxOffset = size.h - m_thumbSize;
		m_thumb->setPos(Vec2i(getBorderLeft(), getBorderTop()));
		m_thumb->setSize(Vec2i(size.w, m_thumbSize));
	} else {
		m_thumbSize = int(m_availRatio * size.w);
		m_maxOffset = size.w - m_thumbSize;
		m_thumb->setPos(Vec2i(getBorderLeft(), getBorderTop()));
		m_thumb->setSize(Vec2i(m_thumbSize, size.h));
	}
	m_pageSize = m_thumbSize;
}

void ScrollBarShaft::setSize(const Vec2i &sz) {
	Container::setSize(sz);
	recalc();
}

bool ScrollBarShaft::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		pos -= getScreenPos();
		if (isVertical()) {
			if (pos.y < m_thumb->getPos().y) {
				onThumbMoved(m_pageSize);
			} else if (pos.y > m_thumb->getPos().y + m_thumb->getSize().h) {
				onThumbMoved(-m_pageSize);
			}
		} else {
			if (pos.x < m_thumb->getPos().x) {
				onThumbMoved(m_pageSize);
			} else if (pos.x > m_thumb->getPos().x + m_thumb->getSize().w) {
				onThumbMoved(-m_pageSize);
			}
		}
	}
	return true;
}

void ScrollBarShaft::setRanges(int total, int avail) {
	m_totalRange = total;
	m_availRange = avail;
	recalc();
}

void ScrollBarShaft::onThumbMoved(int diff) {
	// thumb drag
	Vec2i size = getSize() - getBordersAll();
	int thumbOffset, borderOffset, domSize;
	Vec2i thumbPos, thumbSize;
	if (isVertical()) {
		domSize = size.h;
		thumbOffset = m_thumb->getPos().y - diff;
		borderOffset = getBorderTop();
		thumbOffset = clamp(thumbOffset, 0, m_maxOffset);
		thumbPos = Vec2i(getBorderLeft(), thumbOffset + borderOffset);
		thumbSize = Vec2i(size.w, m_thumbSize);
	} else {
		domSize = size.w;
		thumbOffset = m_thumb->getPos().x - diff;
		borderOffset = getBorderLeft();
		thumbOffset = clamp(thumbOffset, 0, m_maxOffset);
		thumbPos = Vec2i(thumbOffset + borderOffset, getBorderTop());
		thumbSize = Vec2i(m_thumbSize, size.h);
	}
	if (m_thumb->getPos() != thumbPos || m_thumb->getSize() != thumbSize) {
		m_thumb->setPos(thumbPos);
		m_thumb->setSize(thumbSize);		
		ThumbMoved(int(thumbOffset / float(domSize) * m_totalRange));
	}
}

int ScrollBarShaft::getThumbOffset() const {
	Vec2i size = getSize() - getBordersAll();
	int thumbOffset, domSize;
	if (isVertical()) {
		domSize = size.h;
		thumbOffset = m_thumb->getPos().y - getBorderTop();
	} else {
		domSize = size.w;
		thumbOffset = m_thumb->getPos().x - getBorderLeft();
	}
	return int(thumbOffset / float(domSize) * m_totalRange);
}

void ScrollBarShaft::setOffsetPercent(int v) {
	int offset = clamp(int(float(v) / 100.f * m_maxOffset), 0, m_maxOffset);
	Vec2i size = getSize() - getBordersAll();
	int borderOffset, domSize;
	Vec2i thumbPos;
	if (isVertical()) {
		domSize = size.h;
		borderOffset = getBorderTop();
		thumbPos = Vec2i(getBorderLeft(), offset + borderOffset);
	} else {
		domSize = size.w;
		borderOffset = getBorderLeft();
		thumbPos = Vec2i(offset + borderOffset, getBorderTop());
	}
	if (m_thumb->getPos() != thumbPos) {
		m_thumb->setPos(thumbPos);
		ThumbMoved(int(offset / float(domSize) * m_totalRange));
	}
}

// =====================================================
//  class ScrollBar
// =====================================================

ScrollBar::ScrollBar(Container *parent, bool vert, int lineSize)
		: CellStrip(parent, vert ? Orientation::VERTICAL : Orientation::HORIZONTAL,
                            vert ? Origin::FROM_TOP : Origin::FROM_LEFT, 3)
		, m_vertical(vert), m_lineSize(lineSize) {
	init();
}

ScrollBar::ScrollBar(Container *parent, Vec2i pos, Vec2i sz, bool vert, int lineSize)
		: CellStrip(parent, pos, sz, vert ? Orientation::VERTICAL : Orientation::HORIZONTAL,
                                     vert ? Origin::FROM_TOP : Origin::FROM_LEFT, 3)
		, m_vertical(vert), m_lineSize(lineSize) {
	init();
	setSize(sz); // trigger layout logic
}

void ScrollBar::init() {
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	m_btnOne = new ScrollBarButton(getCell(0), m_vertical ? Direction::UP : Direction::LEFT);
	m_btnOne->setAnchors(anchors);
	m_btnOne->Fire.connect(this, &ScrollBar::onScrollBtnFired);
	m_btnTwo = new ScrollBarButton(getCell(2), m_vertical ? Direction::DOWN : Direction::RIGHT);
	m_btnTwo->setAnchors(anchors);
	m_btnTwo->Fire.connect(this, &ScrollBar::onScrollBtnFired);
	m_shaft = new ScrollBarShaft(getCell(1), m_vertical);
	m_shaft->setAnchors(anchors);
	m_shaft->ThumbMoved.connect(this, &ScrollBar::onThumbMoved);
}

void ScrollBar::setSize(const Vec2i &sz) {
	CellStrip::setSize(sz);
}

void ScrollBar::layoutCells() {
	Vec2i size = getSize() - getBordersAll();
	int btnSize = m_vertical ? size.w : size.h;
	getCell(0)->setSizeHint(SizeHint(-1, btnSize));
	getCell(1)->setSizeHint(SizeHint(100));
	getCell(2)->setSizeHint(SizeHint(-1, btnSize));
	CellStrip::layoutCells();
}

void ScrollBar::onScrollBtnFired(ScrollBarButton *btn) {
	if (btn == m_btnOne) {
		m_shaft->onThumbMoved(m_lineSize);
	} else if (btn == m_btnTwo) {
		m_shaft->onThumbMoved(-m_lineSize);
	} else {
		assert(false);
	}
}

void ScrollBar::scrollLine(bool increase) {
	m_shaft->onThumbMoved(increase ? m_lineSize : -m_lineSize);
}

}}

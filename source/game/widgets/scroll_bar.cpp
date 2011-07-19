// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
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

ScrollBarButton::~ScrollBarButton() {
	m_rootWindow->unregisterUpdate(this);
}

bool ScrollBarButton::mouseMove(Vec2i pos) {
	WIDGET_LOG( descLong() << " : ScrollBarButton::mouseMove( " << pos << " )");
	Button::mouseMove(pos);
	if (m_down) {
		if (!isHovered()) {
			WIDGET_LOG( descId() << " : ScrollBarButton::mouseMove() :: m_down && !isHovered(), unregistering for update()." );
			m_rootWindow->unregisterUpdate(this);
			m_down = false;
		}
	} else { // !m_down
		if (isHovered() && isFocused()) {
			WIDGET_LOG( descId() << " : ScrollBarButton::mouseMove() :: !m_down && isHovered() && isFocused(), registering for update()." );
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
	if (btn == MouseButton::LEFT && isFocused() && !m_down) {
		WIDGET_LOG( descId() << " : ScrollBarButton::mouseDown() :: registering for update()." );
		m_rootWindow->registerUpdate(this);
		m_counter = 0;
		m_down = true;
		m_fireOnUp = true;
	}
	return true;
}

bool ScrollBarButton::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( descLong() << " : ScrollBarButton::mouseUp( " << MouseButtonNames[btn] << ", " << pos << " )");
	if (btn == MouseButton::LEFT) {
		if (isFocused() && isHovered() && m_fireOnUp) {
			Fire(this);
		}
		setFocus(false);
		if (m_down) {
			WIDGET_LOG( descId() << " : ScrollBarButton::mouseUp() :: unregistering for update()." );
			m_rootWindow->unregisterUpdate(this);
			m_down = false;
		}
	}
	return true;
}

void ScrollBarButton::update() {
	++m_counter;
	if (m_counter % 20 == 0) {
		WIDGET_LOG( descId() << " : ScrollBarButton::update() :: Fire(this)" );
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
	m_availRatio = m_shaftRatio = 0.f;
	m_maxOffset = 0;
	m_thumb = new ScrollBarThumb(this, vert);
	m_thumb->setPos(Vec2i(getBorderLeft(), getBorderTop()));
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
		m_shaftRatio = clamp(size.h / float(m_totalRange), 0.05f, 1.f);
		m_maxOffset = size.h - m_thumbSize;
		m_thumb->setPos(Vec2i(getBorderLeft(), getBorderTop()));
		m_thumb->setSize(Vec2i(size.w, m_thumbSize));
	} else {
		m_thumbSize = int(m_availRatio * size.w);
		m_shaftRatio = clamp(size.w / float(m_totalRange), 0.05f, 1.f);
		m_maxOffset = size.w - m_thumbSize;
		m_thumb->setPos(Vec2i(getBorderLeft(), getBorderTop()));
		m_thumb->setSize(Vec2i(m_thumbSize, size.h));
	}
	if (m_maxOffset < 0) {
		m_maxOffset = 0;
	}
	//WIDGET_LOG(
	//	"ScrollBarShaft::recalc() : shaftSize: " << dominantSize() 
	//	<< ", maxOffset: " << m_maxOffset << ", shaftRatio: " 
	//	<< m_shaftRatio << ", availRatio: " << m_availRatio
	//);
	m_pageSize = m_thumbSize;
}

void ScrollBarShaft::setSize(const Vec2i &sz) {
	if (getSize() != sz) {
		Container::setSize(sz);
		recalc();
	}
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

void ScrollBarShaft::setRanges(float total, float avail) {
	m_totalRange = total;
	m_availRange = avail;
	recalc();
}

void ScrollBarShaft::onThumbMoved(int diff) {
	// thumb drag
	Vec2i size = getSize() - getBordersAll();
	int thumbOffset;
	Vec2i thumbPos;
	if (isVertical()) {
		thumbOffset = clamp(m_thumb->getPos().y - diff, 0, m_maxOffset);
		thumbPos = Vec2i(getBorderLeft(), getBorderTop() + thumbOffset);
	} else {
		thumbOffset = clamp(m_thumb->getPos().x - diff, 0, m_maxOffset);
		thumbPos = Vec2i(getBorderLeft() + thumbOffset, getBorderTop());
	}
	if (m_thumb->getPos() != thumbPos) {
		m_thumb->setPos(thumbPos);
		ThumbMoved(this);
	}
	//WIDGET_LOG(
	//	"ScrollBarShaft::onThumbMoved() : " 
	//	<< " thumbOffset: " << thumbOffset << ", thumbOffset(): " << this->thumbOffset() 
	//	<< ", thumbPos (range units): " << getThumbPos()
	//);
}

void ScrollBarShaft::setThumbPosPixels(int v) {
	v = clamp(v, 0, m_maxOffset);
	Vec2i thumbPos(getBorderLeft(), getBorderTop());
	thumbPos += isVertical() ? Vec2i(0, v) : Vec2i(v, 0);
	if (m_thumb->getPos() != thumbPos) {
		m_thumb->setPos(thumbPos);
		ThumbMoved(this);
	}
	//WIDGET_LOG( 
	//	"ScrollBarShaft::setThumbPosPixels() : " << " thumbOffset: " << thumbOffset() 
	//	<< ", thumbPos (range units): " << getThumbPos()
	//);
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
	m_btnOne = new ScrollBarButton(this, m_vertical ? Direction::UP : Direction::LEFT);
	m_btnOne->setCell(0);
	m_btnOne->setAnchors(anchors);
	m_btnOne->Fire.connect(this, &ScrollBar::onScrollBtnFired);
	m_btnTwo = new ScrollBarButton(this, m_vertical ? Direction::DOWN : Direction::RIGHT);
	m_btnTwo->setCell(2);
	m_btnTwo->setAnchors(anchors);
	m_btnTwo->Fire.connect(this, &ScrollBar::onScrollBtnFired);
	m_shaft = new ScrollBarShaft(this, m_vertical);
	m_shaft->setCell(1);
	m_shaft->setAnchors(anchors);
	m_shaft->ThumbMoved.connect(this, &ScrollBar::onThumbMoved);
}

void ScrollBar::setSize(const Vec2i &sz) {
	CellStrip::setSize(sz);
}

void ScrollBar::layoutCells() {
	Vec2i size = getSize() - getBordersAll();
	int btnSize = m_vertical ? size.w : size.h;
	setSizeHint(0, SizeHint(-1, btnSize));
	setSizeHint(1, SizeHint(100));
	setSizeHint(2, SizeHint(-1, btnSize));
	CellStrip::layoutCells();
}

void ScrollBar::onScrollBtnFired(Widget *source) {
	ScrollBarButton *btn = static_cast<ScrollBarButton*>(source);
	if (btn == m_btnOne) {
		m_shaft->onThumbMoved(m_lineSize);
	} else if (btn == m_btnTwo) {
		m_shaft->onThumbMoved(-m_lineSize);
	} else {
		assert(false);
	}
}

void ScrollBar::scrollLine(bool increase) {
	m_shaft->onThumbMoved(increase ? m_shaft->getPageSize() : -m_shaft->getPageSize() );
}

// =====================================================
//  class ScrollCell
// =====================================================

void ScrollCell::addChild(Widget* child) {
	Container::addChild(child);
	m_childOffsets[child] = Vec2i(0);
}

void ScrollCell::remChild(Widget* child) {
	Container::remChild(child);
	m_childOffsets.erase(child);
}

void ScrollCell::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	Resized(sz);
}

void ScrollCell::render() {
	assert(glIsEnabled(GL_BLEND));
	// clip
	Vec2i pos = getScreenPos() + Vec2i(getBorderLeft(), getBorderTop());
	Vec2i size = getSize() - m_borderStyle.getBorderDims();
	m_rootWindow->pushClipRect(pos, size);
		Container::render();
	m_rootWindow->popClipRect();
}

// =====================================================
//  class ScrollPane
// =====================================================

ScrollPane::ScrollPane(Container *parent)
		: CellStrip(parent, Orientation::HORIZONTAL, Origin::CENTRE, 2) {
	init();
	setPos(Vec2i(0));
}

ScrollPane::ScrollPane(Container *parent, Vec2i pos, Vec2i sz)
		: CellStrip(parent, pos, sz, Orientation::HORIZONTAL, Origin::CENTRE, 2) {
	init();
	setPos(pos);
}

void ScrollPane::init() {
	setWidgetStyle(WidgetType::TEXT_BOX);
	Anchors anchors;
	anchors.set(Edge::COUNT, 0, false); // fill

	CellStrip *bigStrip = new CellStrip(this, Orientation::VERTICAL, Origin::CENTRE, 2);
	bigStrip->setCell(0);
	bigStrip->setAnchors(anchors);
	
	CellStrip *littleStrip = new CellStrip(this, Orientation::VERTICAL, Origin::CENTRE, 2);
	littleStrip->setCell(1);
	littleStrip->setAnchors(anchors);
	
	m_scrollCell = new ScrollCell(bigStrip);
	m_scrollCell->setCell(0);
	m_scrollCell->setAnchors(anchors);
	m_scrollCell->Resized.connect(this, &ScrollPane::onScrollCellResized);

	int barSize = m_rootWindow->getConfig()->getDefaultItemHeight();

	setSizeHint(0, SizeHint(100));
	setSizeHint(1, SizeHint(-1, barSize));

	bigStrip->setSizeHint(0, SizeHint(100));
	bigStrip->setSizeHint(1, SizeHint(-1, barSize));
	littleStrip->setSizeHint(0, SizeHint(100));
	littleStrip->setSizeHint(1, SizeHint(-1, barSize));

	m_vertBar = new ScrollBar(littleStrip, true, 10);
	m_vertBar->setCell(0);
	m_vertBar->setAnchors(anchors);
	m_vertBar->ThumbMoved.connect(this, &ScrollPane::onVerticalScroll);
	m_horizBar = new ScrollBar(bigStrip, false, 10);
	m_horizBar->setCell(1);
	m_horizBar->setAnchors(anchors);
	m_horizBar->ThumbMoved.connect(this, &ScrollPane::onHorizontalScroll);

	m_offset = Vec2i(0);
}

void ScrollPane::setOffset(Vec2i offset) {
	m_scrollCell->setOffset(offset);
}

void ScrollPane::onVerticalScroll(ScrollBar*) {
	m_offset.y = -round(m_vertBar->getThumbPos());//-diff;
	m_scrollCell->setOffset(m_offset);
}

void ScrollPane::onHorizontalScroll(ScrollBar*) {
	m_offset.x = -round(m_horizBar->getThumbPos());//-diff;
	m_scrollCell->setOffset(m_offset);
}

void ScrollPane::onScrollCellResized(Vec2i avail) {
	WIDGET_LOG( descShort() << " ScrollPane::onScrollCellResized() setting available scroll range to " << avail );
	m_vertBar->setRanges(m_totalRange.h, avail.h);
	m_horizBar->setRanges(m_totalRange.w, avail.w);
	setOffset(m_offset);
}

void ScrollPane::setTotalRange(Vec2i total) {
	m_totalRange = total;
	Vec2i avail = m_scrollCell->getSize();
	WIDGET_LOG( descShort() << " ScrollPane::setTotalRange() setting available scroll range to " << avail );
	m_vertBar->setRanges(m_totalRange.h, avail.h);
	m_horizBar->setRanges(m_totalRange.w, avail.w);
	setOffset(m_offset);
}

}}

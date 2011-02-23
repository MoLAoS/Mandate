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
//  class VerticalScrollBar
// =====================================================

VerticalScrollBar::VerticalScrollBar(Container* parent)
		: Widget(parent)
		, ImageWidget(this)
		, MouseWidget(this)
		, hoverPart(0), pressedPart(0)
		//, fullThumb(false), smallThumb(false)
		, shaftOffset(0), shaftHeight(0)
		, thumbOffset(0), thumbSize(0)
		, totalRange(0), availRange(0), lineSize(0)
		, timeCounter(0) {
	init();
}

VerticalScrollBar::VerticalScrollBar(Container* parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, ImageWidget(this)
		, MouseWidget(this)
		, hoverPart(0), pressedPart(0)
		//, fullThumb(false), smallThumb(false)
		, shaftOffset(0), shaftHeight(0)
		, thumbOffset(0), thumbSize(0)
		, totalRange(0), availRange(0), lineSize(0)
		, timeCounter(0) {
	init();
	recalc();
}

VerticalScrollBar::~VerticalScrollBar() {
	getRootWindow()->unregisterUpdate(this);
}

void VerticalScrollBar::init() {
	m_borderStyle = m_rootWindow->getConfig()->getBorderStyle(WidgetType::SCROLL_BAR);
	m_backgroundStyle = m_rootWindow->getConfig()->getBackgroundStyle(WidgetType::SCROLL_BAR);
	setPadding(0);
	//addImage(g_coreData.getVertScrollUpTexture());
	//addImage(g_coreData.getVertScrollUpHoverTex());
	//addImage(g_coreData.getVertScrollDownTexture());
	//addImage(g_coreData.getVertScrollDownHoverTex());
	//m_shaftStyle.setEmbed(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
	//m_shaftStyle.setSizes(1);
	//m_thumbStyle.setRaise(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
	//m_thumbStyle.setSizes(1);
}

void VerticalScrollBar::recalc() {
	Vec2i size = getSize();
	Vec2i imgSize = Vec2i(size.x);
	Vec2i upPos(0, 0);
	Vec2i downPos(0, size.y - imgSize.y);
	//setImageX(0, 0, upPos, imgSize);
	//setImageX(0, 1, upPos, imgSize);
	//setImageX(0, 2, downPos, imgSize);
	//setImageX(0, 3, downPos, imgSize);
	shaftOffset = imgSize.y;
	shaftHeight = size.y - imgSize.y * 2;
	topOffset = imgSize.y + 1;
	thumbOffset = 0;
	float availRatio = availRange / float(totalRange);
	thumbSize = int(availRatio * (shaftHeight - 2));
}

void VerticalScrollBar::setRanges(int total, int avail, int line) {
	if (total < avail) {
		total = avail;
	}
	totalRange = total;
	availRange = avail;
	lineSize = line;
	recalc();
}

void VerticalScrollBar::setOffset(float percent) {
	const int min = 0;
	const int max = (shaftHeight - 2) - thumbSize;

	thumbOffset = clamp(int(max - percent * max / 100.f), min, max);
	ThumbMoved(this);
}

bool VerticalScrollBar::mouseDown(MouseButton btn, Vec2i pos) {
	Vec2i localPos = pos - getScreenPos();
	pressedPart = hoverPart;
	if (pressedPart == Part::UPPER_SHAFT || pressedPart == Part::LOWER_SHAFT) {
		thumbOffset = clamp(localPos.y - topOffset - thumbSize / 2, 0, (shaftHeight - 2) - thumbSize);
		ThumbMoved(this);
		pressedPart = hoverPart = Part::THUMB;
	} else if (pressedPart == Part::UP_BUTTON || pressedPart == Part::DOWN_BUTTON) {
		getRootWindow()->registerUpdate(this);
		timeCounter = 0;
		moveOnMouseUp = true;
	}
	return true;
}

bool VerticalScrollBar::mouseUp(MouseButton btn, Vec2i pos) {
	Vec2i localPos = pos - getScreenPos();
	Part upPart = partAt(localPos);

	if (pressedPart == Part::UP_BUTTON || pressedPart == Part::DOWN_BUTTON) {
		getRootWindow()->unregisterUpdate(this);
	}
	if (pressedPart == upPart && moveOnMouseUp) {
		if (pressedPart == Part::UP_BUTTON) {
			scrollLine(true);
		} else if (pressedPart == Part::DOWN_BUTTON) {
			scrollLine(false);
		}
	}
	pressedPart = Part::NONE;
	return true;
}

void VerticalScrollBar::scrollLine(bool i_up) {
	thumbOffset += i_up ? -lineSize : lineSize;
	thumbOffset = clamp(thumbOffset, 0, (shaftHeight - 2) - thumbSize);
	ThumbMoved(this);
}

void VerticalScrollBar::update() {
	++timeCounter;
	if (timeCounter % 7 != 0) {
		return;
	}
	scrollLine(pressedPart == Part::UP_BUTTON);
	moveOnMouseUp = false;
}

VerticalScrollBar::Part VerticalScrollBar::partAt(const Vec2i &pos) {
	if (pos.y < shaftOffset) {
		return Part::UP_BUTTON;
	} else if (pos.y > shaftOffset + shaftHeight) {
		return Part::DOWN_BUTTON;
	} else if (pos.y < shaftOffset + thumbOffset) {
		return Part::UPPER_SHAFT;
	} else if (pos.y > shaftOffset + thumbOffset + thumbSize) {
		return Part::LOWER_SHAFT;
	} else {
		return Part::THUMB;
	}
}

bool VerticalScrollBar::mouseMove(Vec2i pos) {
	Vec2i localPos = pos - getScreenPos();
	if (pressedPart == Part::NONE) {
		hoverPart = partAt(localPos);
		//cout << "Part: " << hoverPart << endl;
	} else {
		if (pressedPart == Part::DOWN_BUTTON) {
			if (hoverPart == Part::DOWN_BUTTON && localPos.y >= shaftOffset) {
				hoverPart = Part::NONE;
			} else if (hoverPart == Part::NONE && localPos.y < shaftOffset) {
				hoverPart = Part::DOWN_BUTTON;
			}
		} else if (pressedPart == Part::UP_BUTTON) {
			if (hoverPart == Part::UP_BUTTON && localPos.y <= shaftOffset + shaftHeight) {
				hoverPart = Part::NONE;
			} else if (hoverPart == Part::NONE && localPos.y > shaftOffset + shaftHeight) {
				hoverPart = Part::UP_BUTTON;
			}
		} else if (pressedPart == Part::THUMB) {
			thumbOffset = clamp(localPos.y - topOffset - thumbSize / 2, 0, (shaftHeight - 2) - thumbSize);
			ThumbMoved(this);
		} else {
			// don't care for shaft clicked here
		}
	}
	return true;
}

Vec2i VerticalScrollBar::getPrefSize() const {return Vec2i(-1);}
Vec2i VerticalScrollBar::getMinSize() const {return Vec2i(-1);}

void VerticalScrollBar::render() {
	Widget::renderBackground();
	// buttons
	renderImage((pressedPart == Part::UP_BUTTON) ? 1 : 0);	// up arrow
	renderImage((pressedPart == Part::DOWN_BUTTON) ? 3 : 2); // down arrow
	
	if (hoverPart == Part::UP_BUTTON || hoverPart == Part::DOWN_BUTTON) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;

		Vec2i pos(0, hoverPart == Part::UP_BUTTON ? 0 : shaftOffset + shaftHeight);
		Vec2i size(getSize().x);
		int ndx = m_rootWindow->getConfig()->getColourIndex(Vec3f(1.f));
		renderHighLight(ndx, centreAlpha, borderAlpha, pos, size);
	}

	// shaft
	Vec2i shaftPos(0, shaftOffset);
	Vec2i shaftSize(shaftOffset, shaftHeight);
	renderBorders(m_shaftStyle, shaftPos, shaftSize);
	renderBackground(m_backgroundStyle, shaftPos, shaftSize);

	// thumb
	Vec2i thumbPos(1, topOffset + thumbOffset);
	Vec2i thumbSizev(shaftOffset - 2, thumbSize);
	renderBorders(m_thumbStyle, thumbPos, thumbSizev);
	if (hoverPart == Part::THUMB) {
		int ndx = m_rootWindow->getConfig()->getColourIndex(Vec3f(1.f));
		renderHighLight(ndx, 0.2f, 0.5f, thumbPos, thumbSizev);
	}
	Widget::renderForeground();
}

}}

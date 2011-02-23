// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_SCROLL_BAR_INCLUDED_
#define _GLEST_WIDGETS_SCROLL_BAR_INCLUDED_

#include <stack>

#include "widgets_base.h"
#include "widget_config.h"

namespace Glest { namespace Widgets {

// =====================================================
//  class VerticalScrollBar
// =====================================================

class VerticalScrollBar : public Widget, public ImageWidget, public MouseWidget {
private:
	WRAPPED_ENUM( Part, NONE, UP_BUTTON, DOWN_BUTTON, THUMB, UPPER_SHAFT, LOWER_SHAFT );

private:
	Part hoverPart, pressedPart;
	// special case conditions? thumb fills shaft and thumb is so small we need to render it bigger.
	//bool fullThumb, smallThumb;
	int shaftOffset, shaftHeight;
	int thumbOffset, thumbSize;
	int totalRange, availRange, lineSize;
	int timeCounter;
	bool moveOnMouseUp;
	int topOffset;

	BorderStyle m_shaftStyle;
	BorderStyle m_thumbStyle;

	void init();
	void recalc();
	Part partAt(const Vec2i &pos);

public:
	VerticalScrollBar(Container* parent);
	VerticalScrollBar(Container* parent, Vec2i pos, Vec2i size);
	~VerticalScrollBar();

	void setRanges(int total, int avail, int line = 60);
	void setTotalRange(int max) { totalRange = max; }
	void setActualRane(int avail) { availRange = avail; recalc(); }
	void setLineSize(int line) { lineSize = line; }

	int getRangeOffset() const {
		return clamp(int(thumbOffset / float(shaftHeight - 2) * totalRange), 0, totalRange - availRange);
	}
	void setOffset(float percent);

	void scrollLine(bool i_up);

//	int getTotalRange() const { return totalRange; }
//	int getActualRange() const { return actualRange; }

	virtual void setSize(const Vec2i &sz) { Widget::setSize(sz); recalc(); }

	virtual void update();
	virtual void mouseOut() { hoverPart = Part::NONE; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void setStyle() { setWidgetStyle(WidgetType::SCROLL_BAR); }
	virtual void render();
	virtual string desc() { return string("[VerticalScrollBar: ") + descPosDim() + "]"; }

	sigslot::signal<VerticalScrollBar*> ThumbMoved;
};

}}

#endif

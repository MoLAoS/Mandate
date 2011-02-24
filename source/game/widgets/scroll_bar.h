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

#include "widgets.h"
#include "widget_config.h"

namespace Glest { namespace Widgets {

class ScrollBarShaft;
class ScrollBar;

// =====================================================
//  class ScrollBarButton
// =====================================================

class ScrollBarButton : public Button {
private:
	WidgetType m_type;
	bool       m_down;
	int        m_counter;
	bool       m_fireOnUp;

public:
	ScrollBarButton(Container *parent, Direction dir);
	ScrollBarButton(Container *parent, Vec2i pos, Vec2i size, Direction dir);

	virtual void setStyle() override { setWidgetStyle(m_type); }
	virtual string desc() override { return string("[ScrollButton: ") + descPosDim() + "]"; }

	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	virtual void update() override;

	sigslot::signal<ScrollBarButton*> Fire;
};

// =====================================================
//  class ScrollBarThumb
// =====================================================

class ScrollBarThumb : public Button {
private:
	WidgetType m_type;
	Vec2i      m_downPos; // 'mouse down' pos

public:
	ScrollBarThumb(ScrollBarShaft *parent, bool vert);
	ScrollBarThumb(ScrollBarShaft *parent, Vec2i pos, Vec2i size, bool vert);

	virtual void setStyle() override { setWidgetStyle(m_type); }

	virtual void setPos(const Vec2i &pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	sigslot::signal<int> Moved;
	virtual string desc() override { return string("[ScrollThumb: ") + descPosDim() + "]"; }
};

// =====================================================
//  class ScrollBarShaft
// =====================================================

class ScrollBarShaft : public Container, public sigslot::has_slots {
private:
	ScrollBarThumb *m_thumb;
	int             m_totalRange;
	int             m_availRange;
	float           m_availRatio;
	int             m_maxOffset;
	int             m_thumbSize;
	WidgetType      m_type;

private:
	void init(bool vert);
	void recalc();

public:
	ScrollBarShaft(Container *parent, bool vert);
	ScrollBarShaft(Container *parent, Vec2i pos, Vec2i size, bool vert);

	virtual void setStyle() override { setWidgetStyle(m_type); }
	virtual void setSize(const Vec2i &sz) override;

	void setRanges(int total, int avail);
	void setTotalRange(int max) { m_totalRange = max; }
	void setActualRange(int avail) { m_availRange = avail; recalc(); }

	int getThumbOffset() const;
	void setOffsetPercent(int v);

	bool isVertical() const { return m_type == WidgetType::SCROLLBAR_VERT_SHAFT; }

	void onThumbMoved(int diff);
	virtual string desc() override { return string("[ScrollShaft: ") + descPosDim() + "]"; }

	sigslot::signal<int>  ThumbMoved;
};

// =====================================================
//  class ScrollBar
// =====================================================

class ScrollBar : public CellStrip, public sigslot::has_slots {
private:
	ScrollBarButton *m_btnOne;
	ScrollBarButton *m_btnTwo;
	ScrollBarShaft  *m_shaft;
	bool             m_vertical;
	int              m_lineSize;

private:
	void init();

	void onScrollBtnFired(ScrollBarButton*);
	void onThumbMoved(int diff) { ThumbMoved(diff); }

public:
	ScrollBar(Container *parent, bool vert, int lineSize);
	ScrollBar(Container *parent, Vec2i pos, Vec2i sz, bool vert, int lineSize);

	void setLineSize(int sz) { m_lineSize = sz; }
	void setRanges(int total, int avail) { m_shaft->setRanges(total, avail); }

	void scrollLine(bool increase);
	void setOffsetPercent(int v) { m_shaft->setOffsetPercent(v); }
	int getThumbOffset() const { return m_shaft->getThumbOffset(); }

	virtual void setStyle() override { setWidgetStyle(WidgetType::SCROLL_BAR); }
	virtual void setSize(const Vec2i &sz) override;
	virtual string desc() override { return string("[ScrollBar: ") + descPosDim() + "]"; }

	sigslot::signal<int>  ThumbMoved;
};

}}

#endif

// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
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
	~ScrollBarButton();

	virtual void setStyle() override { setWidgetStyle(m_type); }
	virtual string descType() const override { return "ScrollButton"; }

	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	virtual void update() override;

	sigslot::signal<Widget*> Fire;
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
	virtual string descType() const override { return "ScrollThumb"; }
};

// =====================================================
//  class ScrollBarShaft
// =====================================================

class ScrollBarShaft : public Container, public MouseWidget, public sigslot::has_slots {
private:
	ScrollBarThumb *m_thumb;
	float           m_totalRange; // in 'range' units
	float           m_availRange; // in 'range' units

	float           m_availRatio;/* m_thumbRatio */ // cache: m_availRange / m_totalRange
	float           m_shaftRatio;/* m_scaleRatio */ // cache: dominantSize / m_totalRange
	
	int             m_maxOffset; // in pixels, cache: dominantSize - m_thumbSize
	int             m_thumbSize; // in pixels, cache: m_availRatio * dominantSize
	int             m_pageSize;  // in pixels
	WidgetType      m_type;

private:
	void init(bool vert);
	void recalc();

	bool isVertical() const { return m_type == WidgetType::SCROLLBAR_VERT_SHAFT; }
	int dominantSize() const {
		return isVertical() ? getHeight() - getBordersVert() : getWidth() - getBordersHoriz();
	}
	int thumbOffset() const {
		return isVertical() ? m_thumb->getPos().y - getBorderTop() : m_thumb->getPos().x - getBorderLeft();
	}
	int borderOffset() const {
		return isVertical() ? getBorderTop() : getBorderLeft();
	}

public:
	ScrollBarShaft(Container *parent, bool vert);
	ScrollBarShaft(Container *parent, Vec2i pos, Vec2i size, bool vert);

	virtual void setStyle() override { setWidgetStyle(m_type); }
	virtual void setSize(const Vec2i &sz) override;

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;

	/** sets the total space to scroll over and the available area
	  * @param total total space the widget we are scrolling requires (constitutes the 'range units')
	  * @param avail the available space we have to display part of the widget in */
	void setRanges(float total, float avail);

	/*/* sets just the total range requirement of the widget we are scrolling */
	//void setTotalRange(int max) { m_totalRange = max; recalc(); }

	/*/* sets the available space to show the scrolled widget in */
	//void setActualRange(int avail) { m_availRange = avail; recalc(); }

	int getPageSize() const { return m_pageSize; }
	void setPageSize(int v) { m_pageSize = v; }

	/** @returns the offset to the top or left of thumb, in pixels */
	int getThumbPosPixels() const { return thumbOffset(); }
	/** @returns the size of the thumb, in pixels */
	int getThumbSizePixels() const { return m_thumbSize; }

	/** @returns the offset to the top or left of thumb, as a ratio of shaft size */
	float getThumbPosRatio() const { return thumbOffset() / float(dominantSize()); }
	/** @returns the size of the thumb, as a ratio of shaft size */
	float getThumbSizeRatio() const { return m_availRatio; }

	/** @returns the offset to the top or left of thumb, in 'range units' */
	float getThumbPos() const { return getThumbPosRatio() * m_totalRange; }
	/** @returns the size of the thumb, in 'range units' */
	float getThumbSize() const { return m_availRange; }

	/** @param v the offset from top or left, in pixels to position the thumb at */
	void setThumbPosPixels(int v);

	/** @param v the offset from top or left, as a ratio of shaft size to position 
	  * the thumb at.  Will be clamped to 0.f -> (1.f - thumbRatio) */
	void setThumbPosRatio(float v) { setThumbPosPixels(round(v * dominantSize())); }

	/** @param v the offset from top or left, in 'range units' to position the thumb at,
	  * will be clamped to 0 -> (totalRange - thumbSize) */
	void setThumbPos(float v) { setThumbPosPixels(round(v * m_shaftRatio)); }

	/** @param v the offset from top or left, as a percentage of available space,
	  * to position the thumb at (taking into account thumb size) */
	void setThumbPosPercent(int v) { setThumbPosPixels(round(v / 100.f * m_maxOffset)); }

	void onThumbMoved(int diff);
	virtual string descType() const override { return "ScrollShaft"; }

	sigslot::signal<ScrollBarShaft*>  ThumbMoved;
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

	void onScrollBtnFired(Widget*);
	void onThumbMoved(ScrollBarShaft*) { ThumbMoved(this); }

public:
	ScrollBar(Container *parent, bool vert, int lineSize);
	ScrollBar(Container *parent, Vec2i pos, Vec2i sz, bool vert, int lineSize);

	float getThumbPos() const { return m_shaft->getThumbPos(); }

	void setThumbPos(float v) { m_shaft->setThumbPos(float(v)); }
	void setThumbPosPercent(int v) { m_shaft->setThumbPosPercent(v); }

	void setLineSize(int sz) { m_lineSize = sz; }
	void setRanges(int total, int avail) { m_shaft->setRanges(float(total), float(avail)); }
	
	void scrollLine(bool increase);
	void scrollPage(bool increase);

	// CellStrip overrids
	virtual void layoutCells() override;

	// Widget overrides
	virtual void setStyle() override { setWidgetStyle(WidgetType::SCROLL_BAR); }
	virtual void setSize(const Vec2i &sz) override;
	virtual string descType() const override { return "ScrollBar"; }

	// signals
	sigslot::signal<ScrollBar*>  ThumbMoved;
};

// =====================================================
//  class ScrollCell
// =====================================================

class ScrollCell : public Container {
private:
	typedef map<Widget*, Vec2i>   OffsetMap;

private:
	OffsetMap  m_childOffsets;

public:
	ScrollCell(Container *parent) : Container(parent) {}
	ScrollCell(Container *parent, Vec2i pos, Vec2i sz) : Container(parent, pos, sz) {}

	void setOffset(Vec2i offset) {
		foreach (WidgetList, it, m_children) {
			Widget *child = *it;
			child->setPos(m_childOffsets[child] + offset);
		}
	}

	// Container overrides
	virtual void addChild(Widget* child) override;
	virtual void remChild(Widget* child) override;

	// Widget overrides
	virtual void setSize(const Vec2i &sz);
	virtual void render() override;
	virtual string descType() const override { return "ScrollCell"; }

	// signals
	sigslot::signal<Vec2i> Resized;
};

// =====================================================
//  class ScrollPane
// =====================================================

class ScrollPane : public CellStrip, public sigslot::has_slots {
private:
	ScrollBar  *m_vertBar;
	ScrollBar  *m_horizBar;
	ScrollCell *m_scrollCell;
	Vec2i       m_offset;
	Vec2i       m_totalRange;

	void init();
	void setOffset(Vec2i offset = Vec2i(0));
	void onVerticalScroll(ScrollBar*);
	void onHorizontalScroll(ScrollBar*);

public:
	ScrollPane(Container *parent);
	ScrollPane(Container *parent, Vec2i pos, Vec2i sz);

	void setTotalRange(Vec2i total);
	ScrollCell* getScrollCell() { return m_scrollCell; }

	// Widget overrides
	virtual string descType() const override { return "ScrollPane"; }

	// slots (event handlers)
	void onScrollCellResized(Vec2i avail);
};

}}

#endif

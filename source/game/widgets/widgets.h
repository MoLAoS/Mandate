// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_INCLUDED_
#define _GLEST_WIDGETS_INCLUDED_

#include <stack>

#include "sigslot.h"

#include "widgets_base.h"
#include "widget_config.h"

namespace Glest { namespace Widgets {

// =====================================================
// class StaticImage
// =====================================================

class StaticImage : public Widget, public ImageWidget {
public:
	typedef StaticImage* Ptr;

private:
	Texture2D *texture;

public:
	StaticImage(Container::Ptr parent)
			: Widget(parent)
			, ImageWidget(this) {}

	StaticImage(Container::Ptr parent, Vec2i pos, Vec2i size) 
			: Widget(parent, pos, size)
			, ImageWidget(this) {}

	StaticImage(Container::Ptr parent, Vec2i pos, Vec2i size, Texture2D *tex)
			: Widget(parent, pos, size)
			, ImageWidget(this, tex) {}

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render() { renderImage(); }
	virtual string desc() { return string("[StaticImage: ") + descPosDim() + "]"; }
};

// =====================================================
// class StaticText
// =====================================================

class StaticText : public Widget, public TextWidget {
public:
	typedef StaticText* Ptr;

public:
	StaticText(Container::Ptr parent)
			: Widget(parent) , TextWidget(this) {
		m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::STATIC_WIDGET);
	}

	StaticText(Container::Ptr parent, Vec2i pos, Vec2i size)
			: Widget(parent, pos, size) , TextWidget(this) {
		m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::STATIC_WIDGET);
	}

public:
	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[StaticText: ") + descPosDim() + "]"; }
};

// =====================================================
// class Button
// =====================================================

class Button : public Widget, public TextWidget, public ImageWidget, public MouseWidget {
public:
	typedef Button* Ptr;

protected:
	bool hover;
	bool pressed;

public:
	Button(Container::Ptr parent);
	Button(Container::Ptr parent, Vec2i pos, Vec2i size, bool defaultTex = true);

	virtual void setSize(const Vec2i &sz);

	virtual void EW_mouseIn() { hover = true; }
	virtual void EW_mouseOut() { hover = false; }

	virtual bool EW_mouseDown(MouseButton btn, Vec2i pos);
	virtual bool EW_mouseUp(MouseButton btn, Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[Button: ") + descPosDim() + "]"; }

	sigslot::signal<Button*> Clicked;
};

// =====================================================
// class CheckBox
// =====================================================

class CheckBox : public Button {
public:
	typedef CheckBox* Ptr;
protected:
	bool checked;

public:
	CheckBox(Container::Ptr parent);
	CheckBox(Container::Ptr parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz);
	void setChecked(bool v) { checked = v; }
	bool isChecked() const { return checked; }

	virtual bool EW_mouseDown(MouseButton btn, Vec2i pos);
	virtual bool EW_mouseUp(MouseButton btn, Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[CheckBox: ") + descPosDim() + "]"; }
};

// =====================================================
// class TextBox
// =====================================================

class TextBox : public Widget, public MouseWidget, public KeyboardWidget, public TextWidget {
public:
	typedef TextBox* Ptr;

private:
	bool hover;
	bool focus;
	bool changed;

public:
	TextBox(Container::Ptr parent);
	TextBox(Container::Ptr parent, Vec2i pos, Vec2i size);

	virtual void EW_mouseIn()	{ hover = true;	 }
	virtual void EW_mouseOut() { hover = false;	}

	virtual bool EW_mouseDown(MouseButton btn, Vec2i pos);
	virtual bool EW_mouseUp(MouseButton btn, Vec2i pos);

	virtual bool EW_keyDown(Key key);
	virtual bool EW_keyUp(Key key);
	virtual bool EW_keyPress(char c);
	virtual void EW_lostKeyboardFocus();

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[Button: ") + descPosDim() + "]"; }

	sigslot::signal<TextBox*> TextChanged;
};

// =====================================================
//  class Slider
// =====================================================

class Slider : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
public:
	typedef Slider* Ptr;

private:
	float	m_sliderValue;
	bool	m_thumbHover,
			m_thumbPressed,
			m_shaftHover;
	int		m_shaftOffset, 
			m_shaftSize,
			m_thumbCentre,
			m_valSize;
	Vec2i	m_thumbPos,
			m_thumbSize;
	Vec2i	m_titlePos,
			m_valuePos;
	string	m_title;

	BorderStyle m_shaftStyle;

	void recalc();

public:
	Slider(Container::Ptr parent, Vec2i pos, Vec2i size, const string &title);

	void setValue(float val) { m_sliderValue = val; recalc(); }
	float getValue() const { return m_sliderValue; }

	void setSize(const Vec2i &size) {
		Widget::setSize(size);
		recalc();
	}

	virtual void EW_mouseOut();

	virtual bool EW_mouseDown(MouseButton btn, Vec2i pos);
	virtual bool EW_mouseUp(MouseButton btn, Vec2i pos);
	virtual bool EW_mouseMove(Vec2i pos);

	void render();

	virtual Vec2i getPrefSize() const { return Vec2i(-1); }
	virtual Vec2i getMinSize() const { return Vec2i(300,32); }
	virtual string desc() { return string("[Slider: ") + descPosDim() + "]"; }

	sigslot::signal<Ptr> ValueChanged;
};

// =====================================================
//  class VerticalScrollBar
// =====================================================

class VerticalScrollBar : public Widget, public ImageWidget, public MouseWidget {
public:
	typedef VerticalScrollBar* Ptr;

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
	VerticalScrollBar(Container::Ptr parent);
	VerticalScrollBar(Container::Ptr parent, Vec2i pos, Vec2i size);

	void setRanges(int total, int avail, int line = 10) {
		if (total < avail) {
			total = avail;
		}
		totalRange = total;
		availRange = avail;
		lineSize = line;
		recalc();
	}

	void setTotalRange(int max) { totalRange = max; }
	void setActualRane(int avail) { availRange = avail; recalc(); }
	void setLineSize(int line) { lineSize = line; }

	int getRangeOffset() const { return int((thumbOffset - topOffset) / float(shaftHeight) * totalRange); }
	void setRangeOffset() {

	}

//	int getTotalRange() const { return totalRange; }
//	int getActualRange() const { return actualRange; }

	virtual void setSize(const Vec2i &sz) { Widget::setSize(sz); recalc(); }

	virtual void update();
	virtual void EW_mouseOut() { hoverPart = Part::NONE; }

	virtual bool EW_mouseDown(MouseButton btn, Vec2i pos);
	virtual bool EW_mouseUp(MouseButton btn, Vec2i pos);
	virtual bool EW_mouseMove(Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[VerticalScrollBar: ") + descPosDim() + "]"; }

	// not firing yet
	sigslot::signal<VerticalScrollBar*> ThumbMoved;
};

// =====================================================
// class Panel
// =====================================================

class Panel : public Container {
public:
	typedef Panel* Ptr;

public:
	WRAPPED_ENUM( LayoutDirection, VERTICAL, HORIZONTAL );
	WRAPPED_ENUM( LayoutOrigin, FROM_TOP, FROM_BOTTOM, CENTRE, FROM_LEFT, FROM_RIGHT );

protected:
	int		widgetPadding;	// padding between child widgets
	bool	autoLayout;
	LayoutDirection	layoutDirection;
	LayoutOrigin	layoutOrigin;

	Panel(WidgetWindow* window);

public:
	void setLayoutParams(bool autoLayout, LayoutDirection dir, LayoutOrigin origin = LayoutOrigin::CENTRE);
	
public:
	Panel(Container::Ptr parent);
	Panel(Container::Ptr parent, Vec2i pos, Vec2i size);

	void setAutoLayout(bool val) { autoLayout = val; }
	void setPaddingParams(int panelPad, int widgetPad);

	void addChild(Widget::Ptr child);
	void remChild(Widget::Ptr child);

	void setLayoutOrigin(LayoutOrigin lo) { layoutOrigin = lo; }
	virtual void layoutChildren();

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[Panel: ") + descPosDim() + "]"; }
};

// =====================================================
// class PicturePanel
// =====================================================

class PicturePanel : public Panel, public ImageWidget {
public:
	typedef PicturePanel* Ptr;

public:
	PicturePanel(Container::Ptr parent)
			: Panel(parent)
			, ImageWidget(this) {
		WIDGET_LOG( __FUNCTION__ );
	}

	PicturePanel(Container::Ptr parent, Vec2i pos, Vec2i size) 
			: Panel(parent, pos, size)
			, ImageWidget(this) {
		WIDGET_LOG( __FUNCTION__ );
	}

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render() {
		renderImage();
		Panel::render();
	}

	virtual string desc() { return string("[PicturePanel: ") + descPosDim() + "]"; }
};

class ListBoxItem;

// =====================================================
// class ListBase
// =====================================================

class ListBase : public Panel {
public:
	typedef ListBase* Ptr;

protected:
	ListBoxItem* selectedItem;
	Font *itemFont;
	int selectedIndex;
	vector<string> listItems;

	ListBase(WidgetWindow* window);

public:
	ListBase(Container::Ptr parent);
	ListBase(Container::Ptr parent, Vec2i pos, Vec2i size);

	virtual void addItems(const vector<string> &items) = 0;
	virtual void addItem(const string &item) = 0;
//	virtual void clearItems() = 0;

	virtual void setSelected(int index) = 0;

	int getSelectedIndex() { return selectedIndex; }
	ListBoxItem* getSelectedItem() { return selectedItem; }

	sigslot::signal<ListBase*> SelectionChanged;
};

// =====================================================
// class ListBox
// =====================================================

class ListBox : public ListBase, public sigslot::has_slots {
public:
	typedef ListBox* Ptr;

public:
	//WRAPPED_ENUM( ScrollSetting, NEVER, AUTO, ALWAYS );
private:
	vector<int> yPositions; // 'original' (non-scrolled) y coords of children (sans scrollBar)

protected:
	vector<ListBoxItem*> listBoxItems;

	// not in use
	VerticalScrollBar::Ptr scrollBar;
	//ScrollSetting scrollSetting;
	int scrollWidth;

public:
	ListBox(Container::Ptr parent);
	ListBox(Container::Ptr parent, Vec2i pos, Vec2i size);
	ListBox(WidgetWindow* window);

	virtual void addItems(const vector<string> &items);
	virtual void addItem(const string &item);
//	virtual void clearItems();

	virtual void setSelected(int index);
	//virtual void setSelected(ListBoxItem *item);
	void onSelected(ListBoxItem* item);
	
	virtual void setSize(const Vec2i &sz);
	void setScrollBarWidth(int width);

	virtual void layoutChildren();
//	void setScrollSetting(ScrollSetting setting);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	int getPrefHeight(int childCount = -1);

	virtual string desc() { return string("[ListBox: ") + descPosDim() + "]"; }

	void onScroll(VerticalScrollBar::Ptr);

	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

// =====================================================
// class ListBoxItem
// =====================================================

class ListBoxItem : public Widget, public TextWidget, public MouseWidget {
public:
	typedef ListBoxItem* Ptr;

private:
	bool selected;
	bool hover;
	bool pressed;

protected:

public:
	ListBoxItem(ListBase::Ptr parent);
	ListBoxItem(ListBase::Ptr parent, Vec2i pos, Vec2i sz);

	void setSelected(bool s) { selected = s; }

	virtual void EW_mouseIn();
	virtual void EW_mouseOut();
	virtual bool EW_mouseDown(MouseButton btn, Vec2i pos);
	virtual bool EW_mouseUp(MouseButton btn, Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[ListBoxItem: ") + descPosDim() + "]"; }

	sigslot::signal<ListBoxItem*> Selected;
	sigslot::signal<ListBoxItem*> Clicked;
	// inherited signals:
	//		Widget::Destoyed
};

// =====================================================
// class DropList
// =====================================================

class DropList : public ListBase, public sigslot::has_slots {
public:
	typedef DropList* Ptr;

private:
	ListBox::Ptr floatingList;
	Button::Ptr button;
	int dropBoxHeight;

	void expandList();
	void layout();

public:
	DropList(Container::Ptr parent);
	DropList(Container::Ptr parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz);
	void setDropBoxHeight(int h) { dropBoxHeight = h; }

	void addItems(const vector<string> &items);
	void addItem(const string &item);
	void clearItems();

	void setSelected(int index);
	void setSelected(const string &item);

	// event handlers
	void onBoxClicked(ListBoxItem::Ptr);
	void onExpandList(Button::Ptr);
	void onSelectionMade(ListBase::Ptr);
	void onListDisposed(Widget::Ptr);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual string desc() { return string("[DropList: ") + descPosDim() + "]"; }

	sigslot::signal<DropList*> ListExpanded;
	sigslot::signal<DropList*> ListCollapsed;
	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

}}

#endif

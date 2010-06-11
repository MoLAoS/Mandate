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

#include "window_gl.h"
#include "sigslot.h"

#include "widgets_base.h"

using Shared::Platform::WindowGl;

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
			: Widget(parent)
			, TextWidget(this) {}

	StaticText(Container::Ptr parent, Vec2i pos, Vec2i size)
			: Widget(parent, pos, size)
			, TextWidget(this) {}

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

	virtual void mouseIn()	{ Widget::mouseIn(); hover = true;		}
	virtual void mouseOut() { Widget::mouseOut(); hover = false;	}

	virtual void EW_mouseIn() { std::cout << "EW_mouseIn()\n"; }
	virtual void EW_mouseOut() { std::cout << "EW_mouseOut()\n"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[Button: ") + descPosDim() + "]"; }

	sigslot::signal<Button*> Clicked;
};

class StateButton : public Button {
protected:
	int state;
	int numStates;

public:
	StateButton(Container::Ptr parent);
	StateButton(Container::Ptr parent, Vec2i pos, Vec2i size);
};

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

	virtual void mouseIn()	{ Button::mouseIn(); }
	virtual void mouseOut() { Button::mouseOut();}

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[CheckBox: ") + descPosDim() + "]"; }
};

// =====================================================
// class TextBox
// =====================================================

class TextBox : public Widget, public TextWidget, public MouseWidget, public KeyboardWidget {
public:
	typedef TextBox* Ptr;

private:
	bool hover;
	bool focus;
	bool changed;

public:
	TextBox(Container::Ptr parent);
	TextBox(Container::Ptr parent, Vec2i pos, Vec2i size);

	virtual void mouseIn()	{ Widget::mouseIn(); hover = true;		}
	virtual void mouseOut() { Widget::mouseOut(); hover = false;	}

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);

	virtual bool keyDown(Key key);
	virtual bool keyUp(Key key);
	virtual bool keyPress(char c);
	virtual void lostKeyboardFocus();

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[Button: ") + descPosDim() + "]"; }

	sigslot::signal<TextBox*> TextChanged;
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

	void recalc();
	Part partAt(const Vec2i &pos);

public:
	VerticalScrollBar(Container::Ptr parent);
	VerticalScrollBar(Container::Ptr parent, Vec2i pos, Vec2i size);

	void setRanges(int total, int avail, int line = 10) { 
		totalRange = total;
		availRange = avail;
		lineSize = line;
		recalc();
	}

	void setTotalRange(int max) { totalRange = max; }
	void setActualRane(int avail) { availRange = avail; recalc(); }
	void setLineSize(int line) { lineSize = line; }

	int getRangeOffset() const { return int((thumbOffset - topOffset) / float(shaftHeight) * totalRange); }

//	int getTotalRange() const { return totalRange; }
//	int getActualRange() const { return actualRange; }

	virtual void setSize(const Vec2i &sz) { Widget::setSize(sz); recalc(); }

	virtual void update();
	virtual void mouseIn()	{ Widget::mouseIn(); }
	virtual void mouseOut() { Widget::mouseOut(); hoverPart = Part::NONE; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);

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
	WRAPPED_ENUM( LayoutOrigin, FROM_TOP, CENTRE, FROM_BOTTOM );

protected:
	//int panelPadding;	// padding between panel edges and child widgets
	int widgetPadding;	// padding between child widgets
	bool autoLayout;
	LayoutOrigin layoutOrigin;

public:
	void setLayoutParams(bool autoLayout, LayoutOrigin layoutOrigin = LayoutOrigin::CENTRE) {
		this->autoLayout = autoLayout;
		this->layoutOrigin = layoutOrigin;
	}
	
public:
	Panel(Container::Ptr parent);
	Panel(Container::Ptr parent, Vec2i pos, Vec2i size);

	void setAutoLayout(bool val) { autoLayout = val; }
	void setPaddingParams(int panelPad, int widgetPad);

	void addChild(Widget::Ptr child);

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

public:
	ListBase(Container::Ptr parent);
	ListBase(Container::Ptr parent, Vec2i pos, Vec2i size);

	virtual void addItems(const vector<string> &items) = 0;
	virtual void addItem(const string &item) = 0;

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

	virtual void mouseIn() { Widget::mouseIn(); setBorderColour(Vec3f(1.f)); hover = true; }
	virtual void mouseOut() { Widget::mouseOut(); setBorderColour(Vec3f(0.f)); hover = false; }
	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);

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

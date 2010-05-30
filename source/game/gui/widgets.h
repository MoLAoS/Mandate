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

class StaticImage : public ImageWidget {
private:
	Texture2D *texture;

public:
	StaticImage(ContainerPtr parent)
			: Widget(parent) {}

	StaticImage(ContainerPtr parent, Vec2i pos, Vec2i size) 
			: Widget(parent, pos, size) {}

	StaticImage(ContainerPtr parent, Vec2i pos, Vec2i size, Texture2D *tex)
			: Widget(parent, pos, size)
			, ImageWidget(tex) {}

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render() { renderImage(); }
	virtual string desc() { return string("[StaticImage: ") + descPosDim() + "]"; }
};

// =====================================================
// class StaticText
// =====================================================

class StaticText : public TextWidget {
public:
	StaticText(ContainerPtr parent)
			: Widget(parent) {}

	StaticText(ContainerPtr parent, Vec2i pos, Vec2i size)
			: Widget(parent, pos, size) {}

public:
	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[StaticText: ") + descPosDim() + "]"; }
};

// =====================================================
// class Button
// =====================================================

class Button : public TextWidget, public ImageWidget {
protected:
	bool hover;
	bool pressed;

protected:
	Button() : hover(false), pressed(false) {}

public:
	Button(ContainerPtr parent);
	Button(ContainerPtr parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz);

	virtual void mouseIn()	{ Widget::mouseIn(); hover = true;		}
	virtual void mouseOut() { Widget::mouseOut(); hover = false;	}

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[Button: ") + descPosDim() + "]"; }

	sigslot::signal<Button*> Clicked;
};

typedef Button* ButtonPtr;

class StateButton : public Button {
protected:
	int state;
	int numStates;

public:
	StateButton(ContainerPtr parent);
	StateButton(ContainerPtr parent, Vec2i pos, Vec2i size);
};

class CheckBox : public Button {
protected:
	bool checked;

public:
	CheckBox(ContainerPtr parent);
	CheckBox(ContainerPtr parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz);

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

class TextBox : public TextWidget {
private:
	bool hover;
	bool focus;
	bool changed;

public:
	TextBox(ContainerPtr parent);
	TextBox(ContainerPtr parent, Vec2i pos, Vec2i size);

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

typedef TextBox* TextBoxPtr;

// =====================================================
//  class VerticalScrollBar
// =====================================================

class VerticalScrollBar : public ImageWidget {
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

	void recalc();
	Part partAt(const Vec2i &pos);

public:
	VerticalScrollBar(ContainerPtr parent);
	VerticalScrollBar(ContainerPtr parent, Vec2i pos, Vec2i size);

	void setRanges(int total, int avail, int line = 10) { 
		totalRange = total;
		availRange = avail;
		lineSize = line;
		recalc();
	}

	void setTotalRange(int max) { totalRange = max; }
	void setActualRane(int avail) { availRange = avail; recalc(); }
	void setLineSize(int line) { lineSize = line; }

	int getRangeOffset() const { return int(thumbOffset / float(shaftHeight) * totalRange); }

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
	sigslot::signal<VerticalScrollBar*> ThmubMoved;
};

typedef VerticalScrollBar* VerticalScrollBarPtr;

// =====================================================
// class Panel
// =====================================================

class Panel : public Container {
public:
	WRAPPED_ENUM( LayoutOrigin, FROM_TOP, CENTRE, FROM_BOTTOM );

protected:
	//int panelPadding;	// padding between panel edges and child widgets
	int widgetPadding;	// padding between child widgets
	bool autoLayout;
	LayoutOrigin layoutOrigin;

protected:
	Panel() : autoLayout(true), layoutOrigin(LayoutOrigin::CENTRE) {
		WIDGET_LOG( __FUNCTION__ );
		setPaddingParams(10, 5);
	}
	
	Panel(bool autoLayout)
			: autoLayout(autoLayout)
			, layoutOrigin(LayoutOrigin::CENTRE) {
		WIDGET_LOG( __FUNCTION__ );
		setPaddingParams(10, 5);
	}

	Panel(int panelPad, int widgetPad, bool autoLayout=true)
			: widgetPadding(widgetPad)
			, autoLayout(autoLayout)
			, layoutOrigin(LayoutOrigin::CENTRE) {
		WIDGET_LOG( __FUNCTION__ );
		setPadding(panelPad);
	}

public:
	Panel(ContainerPtr parent);
	Panel(ContainerPtr parent, Vec2i pos, Vec2i size);

	void setAutoLayout(bool val) { autoLayout = val; }
	void setPaddingParams(int panelPad, int widgetPad);

	void addChild(WidgetPtr child);

	void setLayoutOrigin(LayoutOrigin lo) { layoutOrigin = lo; }
	void layoutChildren();

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
	PicturePanel(ContainerPtr parent)
			: Widget(parent) {
		WIDGET_LOG( __FUNCTION__ );
	}

	PicturePanel(ContainerPtr parent, Vec2i pos, Vec2i size) 
			: Widget(parent, pos, size) {
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
typedef ListBoxItem* ListBoxItemPtr;

// =====================================================
// class ListBase
// =====================================================

class ListBase : public Panel {
protected:
	ListBoxItemPtr selectedItem;
	Font *itemFont;
	int selectedIndex;
	vector<string> listItems;

public:
	ListBase();

	virtual void addItems(const vector<string> &items) = 0;
	virtual void addItem(const string &item) = 0;

	virtual void setSelected(int index) = 0;

	int getSelectedIndex() { return selectedIndex; }
	ListBoxItemPtr getSelectedItem() { return selectedItem; }

	sigslot::signal<ListBase*> SelectionChanged;
};

typedef ListBase* ListBasePtr;

// =====================================================
// class ListBox
// =====================================================

class ListBox : public ListBase, public sigslot::has_slots {
public:
	WRAPPED_ENUM( ScrollSetting, NEVER, AUTO, ALWAYS );

protected:
	vector<ListBoxItemPtr> listBoxItems;

	// not in use
	VerticalScrollBarPtr scrollBar;
	ScrollSetting scrollSetting;
	int scrollWidth;

public:
	ListBox(ContainerPtr parent);
	ListBox(ContainerPtr parent, Vec2i pos, Vec2i size);
	ListBox(WindowPtr window);

	virtual void addItems(const vector<string> &items);
	virtual void addItem(const string &item);

	virtual void setSelected(int index);
	//virtual void setSelected(ListBoxItem *item);
	void onSelected(ListBoxItemPtr item);
	
	virtual void setSize(const Vec2i &sz);
//	void setScrollBarWidth(int width);
//	void setScrollSetting(ScrollSetting setting);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	int getPrefHeight(int childCount = -1);

	virtual string desc() { return string("[ListBox: ") + descPosDim() + "]"; }

	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

typedef ListBox* ListBoxPtr;

// =====================================================
// class ListBoxItem
// =====================================================

class ListBoxItem : public TextWidget {
private:
	bool selected;
	bool hover;
	bool pressed;

protected:

public:
	ListBoxItem(ListBasePtr parent);
	ListBoxItem(ListBasePtr parent, Vec2i pos, Vec2i sz);

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
// class ComboBox
// =====================================================

class ComboBox : public ListBase, public sigslot::has_slots {
private:
	ListBoxPtr floatingList;
	ButtonPtr button;

	void expandList();

public:
	ComboBox(ContainerPtr parent);
	ComboBox(ContainerPtr parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz);

	void addItems(const vector<string> &items);
	void addItem(const string &item);

	void setSelected(int index);

	// event handlers
	void onBoxClicked(ListBoxItemPtr);
	void onExpandList(ButtonPtr);
	void onSelectionMade(ListBasePtr);
	void onListDisposed(WidgetPtr);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual string desc() { return string("[ComboBox: ") + descPosDim() + "]"; }

	sigslot::signal<ComboBox*> ListExpanded;
	sigslot::signal<ComboBox*> ListCollapsed;
	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

typedef ComboBox* ComboBoxPtr;

}}

#endif

// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_COMPLEX_WIDGETS_INCLUDED_
#define _GLEST_WIDGETS_COMPLEX_WIDGETS_INCLUDED_

#include "widgets.h"
#include "scroll_bar.h"
#include "widget_config.h"

namespace Glest { namespace Widgets {

class ListBoxItem;

// =====================================================
// class ListBase
// =====================================================

class ListBase : public Panel {
protected:
	ListBoxItem* selectedItem;
	//Font *itemFont;
	int selectedIndex;
	vector<string> listItems;

	ListBase(WidgetWindow* window);

public:
	ListBase(Container* parent);
	ListBase(Container* parent, Vec2i pos, Vec2i size);

	virtual void addItems(const vector<string> &items) = 0;
	virtual void addItem(const string &item) = 0;
//	virtual void clearItems() = 0;

	virtual void setSelected(int index) = 0;

	int getSelectedIndex() { return selectedIndex; }
	ListBoxItem* getSelectedItem() { return selectedItem; }

	unsigned getItemCount() const { return listItems.size(); }

	sigslot::signal<ListBase*> SelectionChanged;
	sigslot::signal<ListBase*> SameSelected;
};

// =====================================================
// class ListBox
// =====================================================

class ListBox : public ListBase, public MouseWidget, public sigslot::has_slots {
public:
	//WRAPPED_ENUM( ScrollSetting, NEVER, AUTO, ALWAYS );
private:
	vector<int> yPositions; // 'original' (non-scrolled) y coords of children (sans scrollBar)

protected:
	vector<ListBoxItem*> listBoxItems;
	ScrollBar* scrollBar;

	int scrollWidth;

public:
	ListBox(Container* parent);
	ListBox(Container* parent, Vec2i pos, Vec2i size);
	ListBox(WidgetWindow* window);

	virtual void addItems(const vector<string> &items);
	virtual void addItem(const string &item);
	void addColours(const vector<Vec3f> &colours);
//	virtual void clearItems();

	virtual void setSelected(int index);
	//virtual void setSelected(ListBoxItem *item);
	void onSelected(ListBoxItem* item);
	
	virtual void setSize(const Vec2i &sz);
	void setScrollBarWidth(int width);

	virtual void setStyle() { setWidgetStyle(WidgetType::LIST_BOX); }
	virtual void layoutChildren();
//	void setScrollSetting(ScrollSetting setting);

	bool mouseWheel(Vec2i pos, int z);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	int getPrefHeight(int childCount = -1);

	virtual string desc() { return string("[ListBox: ") + descPosDim() + "]"; }

	void onScroll(int);

	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

// =====================================================
// class ListBoxItem
// =====================================================

class ListBoxItem : public StaticText, public MouseWidget {
private:
	bool selected;
	bool pressed;

protected:

public:
	ListBoxItem(ListBase* parent);
	ListBoxItem(ListBase* parent, Vec2i pos, Vec2i sz);
	ListBoxItem(ListBase* parent, Vec2i pos, Vec2i sz, const Vec3f &bgColour);

	void setSelected(bool s) { selected = s; }
	void setBackgroundColour(const Vec3f &colour);

	virtual void mouseIn();
	virtual void mouseOut();
	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);

	virtual void setStyle() { setWidgetStyle(WidgetType::LIST_ITEM); }
	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	//virtual void render();
	virtual string desc() { return string("[ListBoxItem: ") + descPosDim() + "]"; }

	sigslot::signal<ListBoxItem*> Selected;
	sigslot::signal<ListBoxItem*> Clicked;
	// inherited signals:
	//		Widget::Destoyed
};

// =====================================================
// class DropList
// =====================================================

class DropList : public ListBase, public MouseWidget, public sigslot::has_slots {
private:
	ListBox* floatingList;
	Button* button;
	int dropBoxHeight;

	void expandList();
	void layout();

public:
	DropList(Container* parent);
	DropList(Container* parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz);
	void setDropBoxHeight(int h) { dropBoxHeight = h; }

	virtual void setStyle() { setWidgetStyle(WidgetType::DROP_LIST); }
	void addItems(const vector<string> &items);
	void addItem(const string &item);
	void clearItems();

	void setSelected(int index);
	void setSelected(const string &item);

	bool mouseWheel(Vec2i pos, int z) override;

	// event handlers
	void onBoxClicked(ListBoxItem*);
	void onExpandList(Button*);
	void onSelectionMade(ListBase*);
	void onSameSelected(ListBase*);
	void onListDisposed(Widget*);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;
	//virtual void setEnabled(bool v) {
	//}

	virtual string desc() { return string("[DropList: ") + descPosDim() + "]"; }

	sigslot::signal<DropList*> ListExpanded;
	sigslot::signal<DropList*> ListCollapsed;
	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

}}

#endif

// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS__LIST_WIDGETS_H_
#define _GLEST_WIDGETS__LIST_WIDGETS_H_

#include "widgets.h"
#include "scroll_bar.h"
#include "widget_config.h"

namespace Glest { namespace Widgets {

class ListBoxItem;

// =====================================================
// class ListBase
// =====================================================

class ListBase : public CellStrip {
protected:
	ListBoxItem*    m_selectedItem;
	int             m_selectedIndex;
	vector<string>  m_listItems;

	// Construct floating
	ListBase(WidgetWindow* window, Orientation ld, Origin orgn, int cells);

public:
	// Construct
	ListBase(Container* parent, Orientation ld, Origin orgn, int cells);
	ListBase(Container* parent, Vec2i pos, Vec2i size, Orientation ld, Origin orgn, int cells);

	// get selected
	int getSelectedIndex() { return m_selectedIndex; }
	ListBoxItem* getSelectedItem() { return m_selectedItem; }

	// get item count
	unsigned getItemCount() const { return m_listItems.size(); }

	// list interface
	virtual void addItems(const vector<string> &items) = 0;
	virtual void addItem(const string &item) = 0;
	virtual void clearItems() = 0;
	virtual void setSelected(int index) = 0;
	virtual void setSelected(const string &item) = 0;

	// Signals
	sigslot::signal<Widget*> SelectionChanged;
	sigslot::signal<Widget*> SameSelected;
};

// =====================================================
// class ListBox
// =====================================================

class ListBox : public ListBase, public MouseWidget, public sigslot::has_slots {
private:
	vector<int> m_yPositions; // 'original' (non-scrolled) y coords of children (sans scrollBar)

protected:
	vector<ListBoxItem*> m_listBoxItems;
	CellStrip           *m_listStrip;
	ScrollBar           *m_scrollBar;

private:
	void init();

public:
	// Construct
	ListBox(Container* parent);
	ListBox(Container* parent, Vec2i pos, Vec2i size);
	//ListBox(WidgetWindow* window);

	int getPrefHeight(int childCount = -1);

	// ListBase overrides
	virtual void addItems(const vector<string> &items) override;
	virtual void addItem(const string &item) override;
	virtual void clearItems() override;
	virtual void setSelected(int index) override;
	virtual void setSelected(const string &name) override;

	// CellStrip override
	virtual void layoutCells() override;

	// Widget overrides
	virtual void setSize(const Vec2i &sz) override;
	virtual void setStyle() override { setWidgetStyle(WidgetType::LIST_BOX); }
	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;
	virtual string descType() const override { return "ListBox"; }

	// MouseWidget overrides
	virtual bool mouseWheel(Vec2i pos, int z) override;

	// Slots (event handlers)
	void onScroll(ScrollBar *sb);
	void onSelected(Widget* item);

	// Signals
	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

// =====================================================
// class ListBoxItem
// =====================================================

class ListBoxItem : public StaticText, public MouseWidget {
private:
	bool m_selected;
	bool m_pressed;

protected:

public:
	// Construct
	ListBoxItem(Container* parent);
	ListBoxItem(Container* parent, Vec2i pos, Vec2i sz);
	ListBoxItem(Container* parent, Vec2i pos, Vec2i sz, const Vec3f &bgColour); /**< @deprecated do not use */

	// set 'selected' flag
	void setSelected(bool s);
	// obsolete
	void setBackgroundColour(const Vec3f &colour); /**< @deprecated do not use */

	// Widget overrides
	virtual void setStyle() override { setWidgetStyle(WidgetType::LIST_ITEM); }
	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;
	virtual string descType() const override { return "ListBoxItem"; }

	// MouseWidget overrides
	virtual void mouseIn() override;
	virtual void mouseOut() override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	// Signals
	//sigslot::signal<Widget*> Selected;
	sigslot::signal<Widget*> Clicked;
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
	void init();

public:
	// Construct
	DropList(Container* parent);
	DropList(Container* parent, Vec2i pos, Vec2i size);

	// set a maximum height for the expanded drop list
	void setDropBoxHeight(int h) { dropBoxHeight = h; }

	// ListBase overrides
	virtual void addItems(const vector<string> &items) override;
	virtual void addItem(const string &item) override;
	virtual void clearItems() override;
	virtual void setSelected(int index) override;
	virtual void setSelected(const string &item) override;

	// Widget overrides
	virtual void setStyle() override { setWidgetStyle(WidgetType::DROP_LIST); }
	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;
	virtual string descType() const override { return "DropList"; }

	// MouseWidget overrides
	virtual bool mouseWheel(Vec2i pos, int z) override;
	virtual void mouseIn() override;
	virtual void mouseOut() override;

	// Slots (event handlers)
	void onBoxClicked(Widget*);
	void onExpandList(Widget*);
	void onSelectionMade(Widget*);
	void onSameSelected(Widget*);
	void onListDisposed(Widget*);

	// Signals
	sigslot::signal<Widget*> ListExpanded;
	sigslot::signal<Widget*> ListCollapsed;
	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

}}

#endif

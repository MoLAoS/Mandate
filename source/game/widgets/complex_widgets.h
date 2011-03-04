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

class ListBase : public CellStrip {
protected:
	ListBoxItem*    m_selectedItem;
	int             m_selectedIndex;
	vector<string>  m_listItems;

	ListBase(WidgetWindow* window, Orientation ld, Origin orgn, int cells);

public:
	ListBase(Container* parent, Orientation ld, Origin orgn, int cells);
	ListBase(Container* parent, Vec2i pos, Vec2i size, Orientation ld, Origin orgn, int cells);

	virtual void addItems(const vector<string> &items) = 0;
	virtual void addItem(const string &item) = 0;
	virtual void clearItems() = 0;

	virtual void setSelected(int index) = 0;
	virtual void setSelected(const string &item) = 0;

	int getSelectedIndex() { return m_selectedIndex; }
	ListBoxItem* getSelectedItem() { return m_selectedItem; }

	unsigned getItemCount() const { return m_listItems.size(); }

	sigslot::signal<ListBase*> SelectionChanged;
	sigslot::signal<ListBase*> SameSelected;
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
	ListBox(Container* parent);
	ListBox(Container* parent, Vec2i pos, Vec2i size);
	//ListBox(WidgetWindow* window);

	virtual void addItems(const vector<string> &items) override;
	virtual void addItem(const string &item) override;
	void addColours(const vector<Vec3f> &colours);
	void clearItems() override;

	virtual void setSelected(int index) override;
	virtual void setSelected(const string &name) override;
	void onSelected(ListBoxItem* item);
	
	virtual void setSize(const Vec2i &sz) override;

	virtual void setStyle() override { setWidgetStyle(WidgetType::LIST_BOX); }
	virtual void layoutCells() override;

	bool mouseWheel(Vec2i pos, int z) override;

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	int getPrefHeight(int childCount = -1);

	virtual string descType() const override { return "ListBox"; }

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
	bool m_selected;
	bool m_pressed;

protected:

public:
	ListBoxItem(Container* parent);
	ListBoxItem(Container* parent, Vec2i pos, Vec2i sz);
	ListBoxItem(Container* parent, Vec2i pos, Vec2i sz, const Vec3f &bgColour);

	void setSelected(bool s);
	void setBackgroundColour(const Vec3f &colour);

	virtual void mouseIn() override;
	virtual void mouseOut() override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	virtual void setStyle() override { setWidgetStyle(WidgetType::LIST_ITEM); }
	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	//virtual void render();
	virtual string descType() const override { return "ListBoxItem"; }

	//sigslot::signal<ListBoxItem*> Selected;
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
	//void layout();
	void init();

public:
	DropList(Container* parent);
	DropList(Container* parent, Vec2i pos, Vec2i size);

	//virtual void setSize(const Vec2i &sz) override;
	void setDropBoxHeight(int h) { dropBoxHeight = h; }

	virtual void setStyle() override { setWidgetStyle(WidgetType::DROP_LIST); }
	void addItems(const vector<string> &items) override;
	void addItem(const string &item) override;
	void clearItems() override;

	void setSelected(int index) override;
	void setSelected(const string &item) override;

	bool mouseWheel(Vec2i pos, int z) override;

	// event handlers
	void onBoxClicked(ListBoxItem*);
	void onExpandList(Button*);
	void onSelectionMade(ListBase*);
	void onSameSelected(ListBase*);
	void onListDisposed(Widget*);

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;
	//virtual void setEnabled(bool v) {
	//}

	virtual string descType() const override { return "DropList"; }

	sigslot::signal<DropList*> ListExpanded;
	sigslot::signal<DropList*> ListCollapsed;
	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

}}

#endif

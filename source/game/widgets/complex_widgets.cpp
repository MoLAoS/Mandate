// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "complex_widgets.h"

#include "widget_window.h"
#include "leak_dumper.h"

using Shared::Graphics::Texture;
using namespace Shared::Graphics::Gl;

namespace Glest { namespace Widgets {

// =====================================================
//  class ListBase
// =====================================================

ListBase::ListBase(Container* parent, Orientation orient, Origin orgn, int cells)
		: CellStrip(parent, orient, orgn, cells)
		, m_selectedItem(0)
		, m_selectedIndex(-1) {
}

ListBase::ListBase(Container* parent, Vec2i pos, Vec2i size, Orientation orient, Origin orgn, int cells)
		: CellStrip(parent, pos, size, orient, orgn, cells)
		, m_selectedItem(0)
		, m_selectedIndex(-1) {
}

ListBase::ListBase(WidgetWindow* window, Orientation orient, Origin orgn, int cells)
		: CellStrip(window, orient, orgn, cells)
		, m_selectedItem(0)
		, m_selectedIndex(-1) {
}

// =====================================================
//  class ListBox
// =====================================================

ListBox::ListBox(Container* parent)
: ListBase(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2)
		, MouseWidget(this)
		, m_listStrip(0)
		, m_scrollBar(0){
	init();
}

ListBox::ListBox(Container* parent, Vec2i pos, Vec2i size) 
		: ListBase(parent, pos, size, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2)
		, MouseWidget(this)
		, m_listStrip(0)
		, m_scrollBar(0) {
	init();
}

//ListBox::ListBox(WidgetWindow* window)
//		: ListBase(window, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2)
//		, MouseWidget(this)
//		, m_listStrip(0)
//		, m_scrollBar(0) {
//	init();
//}

void ListBox::init() {
	Anchors anchors;
	anchors.set(Edge::COUNT, 0, false);
	setWidgetStyle(WidgetType::LIST_BOX);
	getCell(0)->setSizeHint(SizeHint(100));
	getCell(1)->setSizeHint(SizeHint(0));
	m_listStrip = new CellStrip(getCell(0), Orientation::VERTICAL, Origin::FROM_TOP, 0);
	m_listStrip->setAnchors(anchors);
	m_scrollBar = new ScrollBar(getCell(1), true, 10);
	m_scrollBar->setVisible(false);
	m_scrollBar->setFade(1.f);
	m_scrollBar->setAnchors(anchors);
	m_scrollBar->ThumbMoved.connect(this, &ListBox::onScroll);
}

void ListBox::onSelected(ListBoxItem* item) {
	if (m_selectedItem != item) {
		if (m_selectedItem) {
			m_selectedItem->setSelected(false);
		}
		m_selectedItem = item;
		m_selectedItem->setSelected(true);
		for (int i = 0; i < m_listBoxItems.size(); ++i) {
			if (item == m_listBoxItems[i]) {
				m_selectedIndex = i;
				SelectionChanged(this);
				return;
			}
		}
	} else {
		SameSelected(this);
	}
}

void ListBox::addItems(const vector<string> &items) {
	if (items.empty()) {
		return;
	}
	int ndx = m_listStrip->getCellCount();
	m_listStrip->addCells(items.size());
	Anchors anchors;
	anchors.set(Edge::COUNT, 0, false);
	int itemHeight = m_rootWindow->getConfig()->getDefaultItemHeight() + getBordersVert();

	foreach_const (vector<string>, it, items) {
		WidgetCell *cell = m_listStrip->getCell(ndx++);
		cell->setSizeHint(SizeHint(-1, itemHeight));
		ListBoxItem *nItem = new ListBoxItem(cell, Vec2i(0), Vec2i(0), Vec3f(0.25f));
		nItem->setAnchors(anchors);
		nItem->setText(*it);
		m_listBoxItems.push_back(nItem);
		nItem->Clicked.connect(this, &ListBox::onSelected);
	}
	layoutCells();
}

void ListBox::addItem(const string &item) {
	int ndx = m_listStrip->getCellCount();
	m_listStrip->addCells(1);	
	Anchors anchors;
	anchors.set(Edge::COUNT, 0, false);
	int itemHeight = m_rootWindow->getConfig()->getDefaultItemHeight() + getBordersVert();
	ListBoxItem *nItem = new ListBoxItem(m_listStrip->getCell(ndx), Vec2i(0), Vec2i(0), Vec3f(0.25f));
	m_listStrip->getCell(ndx)->setSizeHint(SizeHint(-1, itemHeight));
	nItem->setAnchors(anchors);
	nItem->setText(item);
	m_listBoxItems.push_back(nItem);
	nItem->Clicked.connect(this, &ListBox::onSelected);
	layoutCells();
}

void ListBox::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	layoutCells();
}

void ListBox::layoutCells() {
	// set hints
	int totalItemHeight = getPrefHeight();
	int clientHeight = getSize().y - getBordersVert();
	SizeHint hint(-1, m_rootWindow->getConfig()->getDefaultItemHeight());
	if (totalItemHeight > clientHeight) {
		getCell(1)->setSizeHint(hint);
		m_scrollBar->setVisible(true);
	} else {
		getCell(1)->setSizeHint(SizeHint(0));
		m_scrollBar->setVisible(false);
	}
	
	for (int i=0; i < m_listStrip->getCellCount(); ++i) {
		m_listStrip->getCell(i)->setSizeHint(hint);
	}
	// zap
	CellStrip::layoutCells();

	// the above call will cause m_listStrip to be laid out, but not until just prior to next render
	// we need to pull y coord values out, so we force it here
	m_listStrip->layoutCells();

	// set scroll-bar ranges
	if (totalItemHeight > clientHeight) {
		m_scrollBar->setRanges(totalItemHeight, clientHeight);
	}
	m_yPositions.clear();
	for (int i=0; i < m_listStrip->getCellCount(); ++i) {
		Vec2i pos = m_listStrip->getCell(i)->getPos();
		m_yPositions.push_back(pos.y);
	}
}

void ListBox::clearItems() {
	m_listItems.clear();
	deleteCells();
	m_selectedIndex = -1;
}

void ListBox::onScroll(int offset) {
	int ndx = 0;
	const int x = 0;//m_borderStyle.m_sizes[Border::LEFT];
	for (int i=0; i < m_listStrip->getCellCount(); ++i) {
		m_listStrip->getCell(i)->setPos(Vec2i(x, m_yPositions[i] - offset));
	}
}

bool ListBox::mouseWheel(Vec2i pos, int z) {
	if (m_scrollBar->isVisible()) {
		m_scrollBar->scrollLine(z > 0);
	}
	return true;
}

int ListBox::getPrefHeight(int childCount) {
	WidgetConfig &cfg = *m_rootWindow->getConfig();
	int itemSize = cfg.getDefaultItemHeight();
	if (childCount == -1) {
		childCount = m_listBoxItems.size();
	}
	return itemSize * childCount;
}

///@todo handle no m_children
Vec2i ListBox::getMinSize() const {
	Vec2i res(0);
	foreach_const (WidgetList, it, m_children) {
		Vec2i ips = (*it)->getPrefSize();
		if (ips.x > res.x) res.x = ips.x;
		if (ips.y > res.y) res.y = ips.y;
	}
	res.y *= 3;
	res += m_borderStyle.getBorderDims();
	return res;
}

Vec2i ListBox::getPrefSize() const {
	Vec2i res(0);
	foreach_const (WidgetList, it, m_children) {
		Vec2i ips = (*it)->getPrefSize();
		if (ips.x > res.x) res.x = ips.x;
		res.y += ips.y;
	}
	res += m_borderStyle.getBorderDims();
	return res;
}

void ListBox::setSelected(int index) {
	if (index < 0) {
		if (!m_selectedItem) {
			assert(m_selectedIndex == -1);
			return;
		}
		m_selectedIndex = -1;
		if (m_selectedItem) {
			m_selectedItem->setSelected(false);
			m_selectedItem = 0;
		}
	} else if (index < m_listBoxItems.size()) {
		if (m_selectedItem == m_listBoxItems[index]) {
			return;
		}
		if (m_selectedItem) {
			m_selectedItem->setSelected(false);
		}
		m_selectedItem = m_listBoxItems[index];
		m_selectedItem->setSelected(true);
		m_selectedIndex = index;
	}
	SelectionChanged(this);
}

void ListBox::setSelected(const string &item) {
	for (int i=0; i < m_listItems.size(); ++i) {
		if (m_listItems[i] == item) {
			setSelected(i);
			return;
		}
	}
}

// =====================================================
//  class ListBoxItem
// =====================================================

ListBoxItem::ListBoxItem(Container* parent)
		: StaticText(parent) 
		, MouseWidget(this)
		, m_pressed(false) {
	setWidgetStyle(WidgetType::LIST_ITEM);
}

ListBoxItem::ListBoxItem(Container* parent, Vec2i pos, Vec2i sz)
		: StaticText(parent, pos, sz)
		, MouseWidget(this)
		, m_pressed(false) {
	setWidgetStyle(WidgetType::LIST_ITEM);
}

ListBoxItem::ListBoxItem(Container* parent, Vec2i pos, Vec2i sz, const Vec3f &bgColour)
		: StaticText(parent, pos, sz)
		, MouseWidget(this)
		, m_pressed(false) {
	setWidgetStyle(WidgetType::LIST_ITEM);
}

Vec2i ListBoxItem::getMinSize() const {
	Vec2i dims = getTextDimensions();
	Vec2i xtra = m_borderStyle.getBorderDims();
	return dims + xtra;
}

Vec2i ListBoxItem::getPrefSize() const {
	return getMinSize();
}

void ListBoxItem::setBackgroundColour(const Vec3f &colour) {
	m_backgroundStyle.m_type = BackgroundType::COLOUR;
	m_backgroundStyle.m_colourIndices[0] = m_rootWindow->getConfig()->getColourIndex(colour);
}

void ListBoxItem::setSelected(bool s) {
	Widget::setSelected(s);
}

void ListBoxItem::mouseIn() {
	if (isEnabled()) {
		setHover(true);
		if (m_pressed) {
			setFocus(true);
		}
	}
}

void ListBoxItem::mouseOut() {
	setHover(false);
	setFocus(false);
}

bool ListBoxItem::mouseDown(MouseButton btn, Vec2i pos) {
	if (isEnabled() && btn == MouseButton::LEFT) {
		setFocus(true);
		m_pressed = true;
		return true;
	}
	return false;
}

bool ListBoxItem::mouseUp(MouseButton btn, Vec2i pos) {
	if (isEnabled() && btn == MouseButton::LEFT) {
		if (isHovered() && m_pressed) {
			Clicked(this);
		}
		m_pressed = false;
		setFocus(false);
		return true;
	}
	return false;
}

// =====================================================
//  class DropList
// =====================================================

DropList::DropList(Container* parent)
		: ListBase(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2)
		, MouseWidget(this)
		, floatingList(0)
		, dropBoxHeight(0) {
	init();
}

DropList::DropList(Container* parent, Vec2i pos, Vec2i size) 
		: ListBase(parent, pos, size, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2)
		, MouseWidget(this)
		, floatingList(0)
		, dropBoxHeight(0) {
	init();
}

void DropList::init() {
	setWidgetStyle(WidgetType::DROP_LIST);
	WidgetConfig &cfg = *m_rootWindow->getConfig();
	Font *itemFont = cfg.getFontSet(m_textStyle.m_fontIndex)[FontSize::NORMAL];

	Anchors anchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0));

	m_selectedItem = new ListBoxItem(getCell(0));
	m_selectedItem->setAnchors(anchors);
	m_selectedItem->setTextParams("", Vec4f(1.f), itemFont, true);
	m_selectedItem->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	m_selectedItem->Clicked.connect(this, &DropList::onBoxClicked);

	getCell(1)->setSizeHint(SizeHint(-1, cfg.getDefaultItemHeight()));

	button = new Button(getCell(1));
	button->setAnchors(anchors);
	button->Clicked.connect(this, &DropList::onExpandList);
}

Vec2i DropList::getPrefSize() const {
	Vec2i res = m_selectedItem->getPrefSize();
	res.x += res.y - getBordersVert();
	res += getBordersAll();
	return  res;
}

Vec2i DropList::getMinSize() const {
	Vec2i res = m_selectedItem->getMinSize();
	res.x += res.y - getBordersVert();
	res += getBordersAll();
	return  res;
}
//
//void DropList::setSize(const Vec2i &size) {
//	Widget::setSize(size);
//	layout();
//}

//void DropList::layout() {
////	const int borderSize = getBorderSize();
//	Vec2i size = getSize();
//	int btn_sz = size.y - getBordersVert();
//	button->setSize(Vec2i(btn_sz));
//	button->setPos(Vec2i(size.x - btn_sz - getBorderRight(), getBorderBottom()));
//	Vec2i liPos(getBorderLeft(), getBorderBottom());
//	Vec2i liSz(size.x - btn_sz - getBordersHoriz(), btn_sz);
//	m_selectedItem->setPos(liPos);
//	m_selectedItem->setSize(liSz);
//}

bool DropList::mouseWheel(Vec2i pos, int z) {
	if (m_selectedIndex == -1) {
		return true;
	}
	if (z > 0) {
		if (m_selectedIndex != 0) {
			setSelected(m_selectedIndex - 1);
		}
	} else {
		if (m_selectedIndex != m_listItems.size() - 1) {
			setSelected(m_selectedIndex + 1);
		}
	}
	return true;
}

void DropList::addItems(const vector<string> &items) {
	foreach_const (vector<string>, it, items) {
		m_listItems.push_back(*it);
	}
}

void DropList::addItem(const string &item) {
	m_listItems.push_back(item);
}

void DropList::clearItems() {
	m_listItems.clear();
	m_selectedItem->setText("");
	m_selectedIndex = -1;
}

void DropList::setSelected(int index) {
	if (index < 0 || index >= m_listItems.size()) {
		if (m_selectedIndex == -1) {
			return;
		}
		m_selectedItem->setText("");
		m_selectedItem->setSelected(false);
		m_selectedIndex = -1;
	} else {
		if (m_selectedIndex == index) {
			return;
		}
		m_selectedItem->setText(m_listItems[index]);
		m_selectedIndex = index;
		m_selectedItem->setSelected(true);
	}
	SelectionChanged(this);
}

void DropList::setSelected(const string &item) {
	int ndx = -1;
	for (int i=0; i < m_listItems.size(); ++i) {
		if (m_listItems[i] == item) {
			ndx = i;
			break;
		}
	}
	if (ndx != -1) {
		setSelected(ndx);
	}
}

void DropList::expandList() {
	floatingList = new ListBox(getRootWindow());
	getRootWindow()->setFloatingWidget(floatingList);
	const Vec2i &size = getSize();
	const Vec2i &screenPos = getScreenPos();
	int num = m_listItems.size();
	int ph = floatingList->getPrefHeight(num);
	int h = dropBoxHeight == 0 ? ph : ph > dropBoxHeight ? dropBoxHeight : ph;

	Vec2i sz(size.x, h);
	Vec2i pos(screenPos.x, screenPos.y);
	floatingList->setPos(pos);
	floatingList->addItems(m_listItems);
	floatingList->setSize(sz);

	floatingList->setSelected(m_selectedIndex);
	floatingList->Destroyed.connect(this, &DropList::onListDisposed);
	floatingList->SelectionChanged.connect(this, &DropList::onSelectionMade);
	floatingList->SameSelected.connect(this, &DropList::onSameSelected);
	setVisible(false);
	ListExpanded(this);
}

void DropList::onBoxClicked(ListBoxItem*) {
	if (isEnabled()) {
		expandList();
	}
}

void DropList::onExpandList(Button*) {
	if (isEnabled()) {
		expandList();
	}
}

void DropList::onSelectionMade(ListBase* lb) {
	assert(floatingList == lb);
	setSelected(lb->getSelectedIndex());
	floatingList->Destroyed.disconnect(this);
	onListDisposed(lb);
	getRootWindow()->removeFloatingWidget(lb);
}

void DropList::onSameSelected(ListBase* lb) {
	assert(floatingList == lb);
	floatingList->Destroyed.disconnect(this);
	onListDisposed(lb);
	getRootWindow()->removeFloatingWidget(lb);
}

void DropList::onListDisposed(Widget*) {
	setVisible(true);
	floatingList = 0;
	ListCollapsed(this);
}

}}

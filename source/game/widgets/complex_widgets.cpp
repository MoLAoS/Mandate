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

ListBase::ListBase(Container* parent)
		: Panel(parent)
		, selectedItem(0)
		, selectedIndex(-1)
		/*, itemFont(0) */{
	setPaddingParams(2, 0);
	setAutoLayout(false);
	//itemFont = CoreData::getInstance().getFTMenuFontNormal();
}

ListBase::ListBase(Container* parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size)
		, selectedItem(0)
		, selectedIndex(-1)
		/*, itemFont(0) */{
	setPaddingParams(2, 0);
	setAutoLayout(false);
	//itemFont = CoreData::getInstance().getFTMenuFontNormal();
}

ListBase::ListBase(WidgetWindow* window) 
		: Panel(window)
		, selectedItem(0)
		, selectedIndex(-1) {
	//itemFont = CoreData::getInstance().getFTMenuFontNormal();
}

// =====================================================
//  class ListBox
// =====================================================

ListBox::ListBox(Container* parent)
		: ListBase(parent)
		, MouseWidget(this)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	setWidgetStyle(WidgetType::LIST_BOX);
	setPadding(0);
	setAutoLayout(false);
}

ListBox::ListBox(Container* parent, Vec2i pos, Vec2i size) 
		: ListBase(parent, pos, size)
		, MouseWidget(this)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	setWidgetStyle(WidgetType::LIST_BOX);
	setPadding(0);
	setAutoLayout(false);
}

ListBox::ListBox(WidgetWindow* window)
		: ListBase(window)
		, MouseWidget(this)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	setWidgetStyle(WidgetType::LIST_BOX);
	setPadding(0);
	setAutoLayout(false);
}

void ListBox::onSelected(ListBoxItem* item) {
	if (selectedItem != item) {
		if (selectedItem) {
			selectedItem->setSelected(false);
		}
		selectedItem = item;
		selectedItem->setSelected(true);
		for (int i = 0; i < listBoxItems.size(); ++i) {
			if (item == listBoxItems[i]) {
				selectedIndex = i;
				SelectionChanged(this);
				return;
			}
		}
	} else {
		SameSelected(this);
	}
}

void ListBox::addItems(const vector<string> &items) {
	WidgetConfig &cfg = *m_rootWindow->getConfig();
	const TextStyle &textStyle = cfg.getWidgetStyle(WidgetType::LIST_ITEM).textStyle();
	Font *itemFont = cfg.getFontSet(textStyle.m_fontIndex)[FontSize::NORMAL];
	Vec2i sz(getSize().x - 4, int(itemFont->getMetrics()->getHeight()) + 4);
	foreach_const (vector<string>, it, items) {
		ListBoxItem *nItem = new ListBoxItem(this, Vec2i(0), sz, Vec3f(0.25f));
		nItem->setTextParams(*it, Vec4f(1.f), itemFont, true);
		nItem->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		listBoxItems.push_back(nItem);
		nItem->Selected.connect(this, &ListBox::onSelected);
	}
}

void ListBox::addItem(const string &item) {
	WidgetConfig &cfg = *m_rootWindow->getConfig();
	const TextStyle &textStyle = cfg.getWidgetStyle(WidgetType::LIST_ITEM).textStyle();
	Font *itemFont = cfg.getFontSet(textStyle.m_fontIndex)[FontSize::NORMAL];
	Vec2i sz(getSize().x - getBordersHoriz(), int(itemFont->getMetrics()->getHeight()) + 4);
	ListBoxItem *nItem = new ListBoxItem(this, Vec2i(getBorderLeft(), getBorderBottom()), sz, Vec3f(0.25f));
	nItem->setTextParams(item, Vec4f(1.f), itemFont, true);
	nItem->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	listBoxItems.push_back(nItem);
	nItem->Selected.connect(this, &ListBox::onSelected);
}

void ListBox::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	layoutChildren();
}

void ListBox::layoutChildren() {
	if (m_children.empty()) {
		return;
	}
	const int padHoriz = m_borderStyle.getHorizBorderDim() + getPadding() * 2;
	const int padVert = m_borderStyle.getVertBorderDim() + getPadding() * 2;
	Vec2i size = getSize();

	// scrollBar ?
	int totalItemHeight = getPrefHeight() - padVert;
	int clientHeight = size.y - padVert;

	if (totalItemHeight > clientHeight) {
		if (!scrollBar) {
			scrollBar = new VerticalScrollBar(this);
			scrollBar->ThumbMoved.connect(this, &ListBox::onScroll);
		}
		scrollBar->setSize(Vec2i(24, clientHeight));
		scrollBar->setPos(Vec2i(size.x - 24 - m_borderStyle.m_sizes[Border::RIGHT], 
								m_borderStyle.m_sizes[Border::BOTTOM]));
		scrollBar->setRanges(totalItemHeight, clientHeight, 2);
		//cout << "setting scroll ranges total = " << totalItemHeight << ", actual = " << clientHeight << endl;
		//int scrollOffset = scrollBar->getRangeOffset();
		//cout << "range offset = " << scrollOffset << endl;
	} else {
		if (scrollBar) {
			delete scrollBar;
			scrollBar = 0;
		}
	}

	vector<int> widgetYPos;
	int wh = 0;
	Vec2i room = size - m_borderStyle.getBorderDims() - Vec2i(getPadding() * 2);
	if (scrollBar) {
		room.x -= scrollBar->getWidth();
	}
	foreach (WidgetList, it, m_children) {
		if (*it == scrollBar) {
			continue;
		}
		widgetYPos.push_back(wh);
		wh += (*it)->getHeight() + widgetPadding;
	}
	
	Vec2i topLeft(m_borderStyle.m_sizes[Border::LEFT] + getPadding(),
				  m_borderStyle.m_sizes[Border::TOP] + getPadding());

	int ndx = 0;
	yPositions.clear();
	foreach (WidgetList, it, m_children) {
		if (*it == scrollBar) {
			continue;
		}
		int y = topLeft.y + widgetYPos[ndx++];
		(*it)->setPos(topLeft.x, y);
		(*it)->setSize(room.x, (*it)->getHeight());
		static_cast<ListBoxItem*>(*it)->centreText();
		yPositions.push_back(y);
	}
}

void ListBox::onScroll(VerticalScrollBar*) {
	int offset = scrollBar->getRangeOffset();
	//cout << "Scroll offset = " << offset << endl;

	int ndx = 0;
	const int x = m_borderStyle.m_sizes[Border::LEFT] + getPadding();
	foreach (WidgetList, it, m_children) {
		if (*it == scrollBar) {
			continue;
		}
		//Widget* widget = *it;
		(*it)->setPos(x, yPositions[ndx++] - offset);
	}
}

bool ListBox::mouseWheel(Vec2i pos, int z) {
	if (scrollBar) {
		scrollBar->scrollLine(z > 0);
	}
	return true;
}

int ListBox::getPrefHeight(int childCount) {
	WidgetConfig &cfg = *m_rootWindow->getConfig();
	const TextStyle &textStyle = cfg.getWidgetStyle(WidgetType::LIST_ITEM).textStyle();
	Font *itemFont = cfg.getFontSet(textStyle.m_fontIndex)[FontSize::NORMAL];

	int res = m_borderStyle.getVertBorderDim() + getPadding() * 2;
	int iSize = int(itemFont->getMetrics()->getHeight()) + 4;
	if (childCount == -1) {
		childCount = m_children.size();//listBoxItems.size();
	}
	res += iSize * childCount;
	if (childCount) {
		res += widgetPadding * (childCount - 1);
	}
	return res;
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
	res += m_borderStyle.getBorderDims() + Vec2i(getPadding() * 2);
	return res;
}

Vec2i ListBox::getPrefSize() const {
	Vec2i res(0);
	foreach_const (WidgetList, it, m_children) {
		Vec2i ips = (*it)->getPrefSize();
		if (ips.x > res.x) res.x = ips.x;
		res.y += ips.y;
	}
	res += m_borderStyle.getBorderDims() + Vec2i(getPadding() * 2);
	return res;
}

void ListBox::setSelected(int index) {
	if (index < 0) {
		if (!selectedItem) {
			assert(selectedIndex == -1);
			return;
		}
		selectedIndex = -1;
		if (selectedItem) {
			selectedItem->setSelected(false);
			selectedItem = 0;
		}
	} else if (index < listBoxItems.size()) {
		if (selectedItem == listBoxItems[index]) {
			return;
		}
		if (selectedItem) {
			selectedItem->setSelected(false);
		}
		selectedItem = listBoxItems[index];
		selectedItem->setSelected(true);
		selectedIndex = index;
	}
	SelectionChanged(this);
}

// =====================================================
//  class ListBoxItem
// =====================================================

ListBoxItem::ListBoxItem(ListBase* parent)
		: StaticText(parent) 
		, MouseWidget(this)
		, selected(false)
		, pressed(false) {
	setWidgetStyle(WidgetType::LIST_ITEM);
}

ListBoxItem::ListBoxItem(ListBase* parent, Vec2i pos, Vec2i sz)
		: StaticText(parent, pos, sz)
		, MouseWidget(this)
		, selected(false)
		, pressed(false) {
	setWidgetStyle(WidgetType::LIST_ITEM);
}

ListBoxItem::ListBoxItem(ListBase* parent, Vec2i pos, Vec2i sz, const Vec3f &bgColour)
		: StaticText(parent, pos, sz)
		, MouseWidget(this)
		, selected(false)
		, pressed(false) {
	setWidgetStyle(WidgetType::LIST_ITEM);
}

Vec2i ListBoxItem::getMinSize() const {
	Vec2i dims = getTextDimensions();
	Vec2i xtra = m_borderStyle.getBorderDims() + Vec2i(getPadding() * 2);
	return dims + xtra;
}

Vec2i ListBoxItem::getPrefSize() const {
	return getMinSize();
}

void ListBoxItem::setBackgroundColour(const Vec3f &colour) {
	m_backgroundStyle.m_type = BackgroundType::COLOUR;
	m_backgroundStyle.m_colourIndices[0] = m_rootWindow->getConfig()->getColourIndex(colour);
}

void ListBoxItem::mouseIn() {
	if (isEnabled()) {
		setHover(true);
		if (pressed) {
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
		pressed = true;
		return true;
	}
	return false;
}

bool ListBoxItem::mouseUp(MouseButton btn, Vec2i pos) {
	if (isEnabled() && btn == MouseButton::LEFT) {
		if (isHovered() && pressed) {
			Clicked(this);
		}
		pressed = false;
		setFocus(false);
		return true;
	}
	return false;
}

// =====================================================
//  class DropList
// =====================================================

DropList::DropList(Container* parent)
		: ListBase(parent)
		, MouseWidget(this)
		, floatingList(0)
		, dropBoxHeight(0) {
	setWidgetStyle(WidgetType::DROP_LIST);
	setAutoLayout(false);
	setPadding(0);
	button = new Button(this);
	button->Clicked.connect(this, &DropList::onExpandList);
	
	WidgetConfig &cfg = *m_rootWindow->getConfig();
	const TextStyle &textStyle = cfg.getWidgetStyle(WidgetType::LIST_ITEM).textStyle();
	Font *itemFont = cfg.getFontSet(textStyle.m_fontIndex)[FontSize::NORMAL];

	selectedItem = new ListBoxItem(this);
	selectedItem->setTextParams("", Vec4f(1.f), itemFont, true);
	selectedItem->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	selectedItem->Clicked.connect(this, &DropList::onBoxClicked);
}

DropList::DropList(Container* parent, Vec2i pos, Vec2i size) 
		: ListBase(parent, pos, size)
		, MouseWidget(this)
		, floatingList(0)
		, dropBoxHeight(0) {
	setWidgetStyle(WidgetType::DROP_LIST);
	setAutoLayout(false);
	setPadding(0);

	WidgetConfig &cfg = *m_rootWindow->getConfig();
	const TextStyle &textStyle = cfg.getWidgetStyle(WidgetType::LIST_ITEM).textStyle();
	Font *itemFont = cfg.getFontSet(textStyle.m_fontIndex)[FontSize::NORMAL];

	button = new Button(this);
	button->Clicked.connect(this, &DropList::onExpandList);
	selectedItem = new ListBoxItem(this);
	selectedItem->setTextParams("", Vec4f(1.f), itemFont, true);
	selectedItem->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	selectedItem->Clicked.connect(this, &DropList::onBoxClicked);
	layout();
}

Vec2i DropList::getPrefSize() const {
	Vec2i res = selectedItem->getPrefSize();
	res.x += res.y - getBordersVert();
	res += getBordersAll() + Vec2i(getPadding() * 2);
	return  res;
}

Vec2i DropList::getMinSize() const {
	Vec2i res = selectedItem->getMinSize();
	res.x += res.y - getBordersVert();
	res += getBordersAll() + Vec2i(getPadding() * 2);
	return  res;
}

void DropList::setSize(const Vec2i &size) {
	Widget::setSize(size);
	layout();
}

void DropList::layout() {
//	const int borderSize = getBorderSize();
	Vec2i size = getSize();
	int btn_sz = size.y - getBordersVert() - getPadding() * 2;
	button->setSize(Vec2i(btn_sz));
	button->setPos(Vec2i(size.x - btn_sz - getBorderRight() - getPadding(), getBorderBottom() + getPadding()));
	Vec2i liPos(getBorderLeft() + getPadding(), getBorderBottom() + getPadding());
	Vec2i liSz(size.x - btn_sz - getBordersHoriz() - getPadding() * 2, btn_sz);
	selectedItem->setPos(liPos);
	selectedItem->setSize(liSz);
}

bool DropList::mouseWheel(Vec2i pos, int z) {
	if (selectedIndex == -1) {
		return true;
	}
	if (z > 0) {
		if (selectedIndex != 0) {
			setSelected(selectedIndex - 1);
		}
	} else {
		if (selectedIndex != listItems.size() - 1) {
			setSelected(selectedIndex + 1);
		}
	}
	return true;
}

void DropList::addItems(const vector<string> &items) {
	foreach_const (vector<string>, it, items) {
		listItems.push_back(*it);
	}
}

void DropList::addItem(const string &item) {
	listItems.push_back(item);
}

void DropList::clearItems() {
	listItems.clear();
	selectedItem->setText("");
	selectedIndex = -1;
}

void DropList::setSelected(int index) {
	if (index < 0 || index >= listItems.size()) {
		if (selectedIndex == -1) {
			return;
		}
		selectedItem->setText("");
		selectedIndex = -1;
	} else {
		if (selectedIndex == index) {
			return;
		}
		selectedItem->setText(listItems[index]);
		selectedIndex = index;
	}
	SelectionChanged(this);
}

void DropList::setSelected(const string &item) {
	int ndx = -1;
	for (int i=0; i < listItems.size(); ++i) {
		if (listItems[i] == item) {
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
	floatingList->setPaddingParams(0, 0);
	const Vec2i &size = getSize();
	const Vec2i &screenPos = getScreenPos();
	int num = listItems.size();
	int ph = floatingList->getPrefHeight(num);
	int h = dropBoxHeight == 0 ? ph : ph > dropBoxHeight ? dropBoxHeight : ph;

	Vec2i sz(size.x, h);
	Vec2i pos(screenPos.x, screenPos.y);
	floatingList->setPos(pos);
	floatingList->addItems(listItems);
	floatingList->setSize(sz);

	floatingList->setSelected(selectedIndex);
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

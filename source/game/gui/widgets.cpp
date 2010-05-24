// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "widgets.h"

#include <sstream>

#include "metrics.h"
#include "util.h"
#include "renderer.h"
#include "texture_gl.h"
#include "game_constants.h"
#include "core_data.h"

#include "widget_window.h"

using Shared::Util::deleteValues;
using namespace Shared::Graphics::Gl;
using Glest::Graphics::Renderer;
using namespace Glest::Global;

namespace Glest { namespace Widgets {

// =====================================================
// class StaticText
// =====================================================

void StaticText::render() {
	/*
	Vec2f verts[4];
	verts[0] = Vec2f(screenPos);
	verts[1] = Vec2f(screenPos) + Vec2f(0.f, float(size.y));
	verts[2] = Vec2f(screenPos) + Vec2f(size);
	verts[3] = Vec2f(screenPos) + Vec2f(float(size.x), 0.f);

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4f(1.f, 1.f, 1.f, 0.2f);
	glBegin(GL_QUAD_STRIP);
		glVertex2fv(verts[0].ptr());
		glVertex2fv(verts[1].ptr());
		glVertex2fv(verts[2].ptr());
		glVertex2fv(verts[3].ptr());
		glVertex2fv(verts[0].ptr());
		glVertex2fv(verts[1].ptr());
	glEnd();
	glPopAttrib();
	*/
	renderText();
}

// =====================================================
// class Button
// =====================================================

Button::Button(ContainerPtr parent)
		: Widget(parent)
		, hover(false), pressed(false) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr)" );
}

Button::Button(ContainerPtr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, hover(false), pressed(false) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr, Vec2i, Vec2i)" );
	// background texture
	CoreData &coreData = CoreData::getInstance();
	Texture2D *tex = size.x > 3 * size.y / 2 
		? coreData.getButtonBigTexture() : coreData.getButtonSmallTexture();
	setImage(tex);
}

void Button::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	CoreData &coreData = CoreData::getInstance();
	Texture2D *tex = size.x > 3 * size.y / 2 
		? coreData.getButtonBigTexture() : coreData.getButtonSmallTexture();
	setImage(tex);
}

bool Button::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		pressed = true;
		return true;
	}
	return parent->mouseDown(btn, pos);
}

bool Button::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		if (pressed && hover) {
			Clicked(this);
		}
		pressed = false;
		return true;
	}
	return parent->mouseDown(btn, pos);
}

void Button::render() {
	// render background
	ImageWidget::renderImage();

	// render hilight
	if (hover) {
		float anim = rootWindow->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;
		Widget::renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha);
	}

	// render label
	TextWidget::renderText();
}

// =====================================================
// class CheckBox
// =====================================================

CheckBox::CheckBox(ContainerPtr parent)
		: Widget(parent), checked(false) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr)" );
	CoreData &coreData = CoreData::getInstance();
	addImageX(coreData.getCheckBoxCrossTexture(), Vec2i(0), Vec2i(32));
	addImageX(coreData.getCheckBoxTickTexture(), Vec2i(0), Vec2i(32));
	setTextParams("No", Vec4f(1.f), coreData.getfreeTypeMenuFont(), true, false);
	addText("Yes");
}

CheckBox::CheckBox(ContainerPtr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size), checked(false) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr, Vec2i, Vec2i)" );
	CoreData &coreData = CoreData::getInstance();
	addImageX(coreData.getCheckBoxCrossTexture(), Vec2i(0), Vec2i(32));
	addImageX(coreData.getCheckBoxTickTexture(), Vec2i(0), Vec2i(32));
	setTextParams("No", Vec4f(1.f), coreData.getfreeTypeMenuFont(), true, false);
	addText("Yes");
	int y = int((size.y - getTextFont()->getMetrics()->getHeight()) / 2);
	setTextPos(Vec2i(40, y));
}

void CheckBox::setSize(const Vec2i &sz) {
	// bypass Button::setSize()
	Widget::setSize(sz);
	int y = int((sz.y - getTextFont()->getMetrics()->getHeight()) / 2);
	setTextPos(Vec2i(40, y));
}

Vec2i CheckBox::getPrefSize() {
	Vec2i dim = getTextDimensions();
	dim.x += 40;
	if (dim.y < 32) {
		dim.y = 32;
	}
	return dim;
}

bool CheckBox::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		pressed = true;
		return true;
	}
	return parent->mouseDown(btn, pos);
}

bool CheckBox::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		if (pressed && hover) {
			checked = !checked;
			Clicked(this);
		}
		pressed = false;
		return true;
	}
	return parent->mouseDown(btn, pos);
}

void CheckBox::render() {
	// render background
	ImageWidget::renderImage(checked ? 1 : 0);

	// render hilight
	if (hover) {
		float anim = rootWindow->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;
		Widget::renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha, Vec2i(0), Vec2i(32));
	}

	// render label
	TextWidget::renderText(checked ? 1 : 0);
}

// =====================================================
// class TextBox
// =====================================================

TextBox::TextBox(ContainerPtr parent)
		: Widget(parent)
		, hover(false)
		, focus(false)
		, changed(false) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr)" );
	borderStyle = BorderStyle::EMBED;
	setBorderSize(2);
}
		
TextBox::TextBox(ContainerPtr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, hover(false)
		, focus(false)
		, changed(false) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr, Vec2i, Vec2i)" );
	borderStyle = BorderStyle::EMBED;
	setBorderSize(2);
}

bool TextBox::mouseDown(MouseButton btn, Vec2i pos) {
	if (enabled) {
		focus = true;
		rootWindow->aquireKeyboardFocus(this);
		return true;
	}
	return Widget::mouseDown(btn, pos);
}

bool TextBox::mouseUp(MouseButton btn, Vec2i pos) {
	return Widget::mouseUp(btn, pos);
}

bool TextBox::keyDown(Key key) {
	KeyCode code = key.getCode();
	switch (code) {
		case KeyCode::BACK_SPACE: {
			const string &txt = getText();
			if (!txt.empty()) {
				setText(txt.substr(0, txt.size() - 1));
				changed = true;
			}
			return true;
		}
		case KeyCode::RETURN:
			rootWindow->releaseKeyboardFocus(this);
			if (changed) {
				TextChanged(this);
				changed = false;
			}
			return true;
		/*
		case KeyCode::DELETE_:
		case KeyCode::ESCAPE:
		case KeyCode::ARROW_LEFT:
		case KeyCode::ARROW_RIGHT:
		case KeyCode::HOME:
		case KeyCode::END:
		case KeyCode::TAB:
			cout << "KeyDown: [" << KeyCodeNames[code] << "]\n";
			return true;
		*/
		default:
			break;
	}
	return Widget::keyDown(key);
}

bool TextBox::keyUp(Key key) {
	return Widget::keyUp(key);
}

bool TextBox::keyPress(char c) {
	if (c >= 32 && c <= 126) { // 'space' -> 'tilde' [printable ascii char]
		cout << "KeyPress: ASCII=='" << c << "' [" << int(c) << "]\n";
		string s(getText());
		cout << "Current text: " << s << ", Setting text: " << (s + c) << endl;
		setText(s + c);
		changed = true;
		return true;
	}
	return Widget::keyPress(c);
}

void TextBox::lostKeyboardFocus() {
	focus = false;
	if (changed) {
		TextChanged(this);
		changed = false;
	}
}

void TextBox::render() {
	Widget::renderBgAndBorders();
	TextWidget::renderText();
}

// =====================================================
// class Panel
// =====================================================

Panel::Panel(ContainerPtr parent)
		: Widget(parent)
		, autoLayout(true) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr)" );
	setPaddingParams(10, 5);
	borderSize = 2;
}

Panel::Panel(ContainerPtr parent, Vec2i pos, Vec2i sz)
		: Widget(parent, pos, sz)
		, autoLayout(true) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr, Vec2i, Vec2i)" );
	setPaddingParams(10, 5);
	borderSize = 2;
}

void Panel::setPaddingParams(int panelPad, int widgetPad) {
	panelPadding = panelPad;
	widgetPadding = widgetPad;
}

void Panel::layoutChildren() {
	if (children.empty()) {
		return;
	}
	vector<int> widgetYPos;
	int wh = 0;
	int yPos = size.y - panelPadding - borderSize;
	int ySpace = size.y - (panelPadding * 2 + borderSize * 2);
	foreach (WidgetList, it, children) {
		yPos -= (*it)->getHeight();
		wh += (*it)->getHeight();
		widgetYPos.push_back(yPos);
		yPos -= widgetPadding;
	}
	wh += widgetPadding * (children.size() - 1);
	const int offset = (ySpace - wh) / 2;
	int ndx = 0;
	foreach (WidgetList, it, children) {
		int ww = (*it)->getWidth();
		int x = (size.x - ww) / 2;
		(*it)->setPos(x, widgetYPos[ndx++] - offset);
	}
}

void Panel::addChild(WidgetPtr child) {
	Container::addChild(child);
	if (!autoLayout) {
		return;
	}
	Vec2i sz = child->getSize();
	Vec2i space = size - Vec2i(panelPadding * 2 + borderSize * 2);
	if (sz.x > space.x) {
		child->setSize(space.x, sz.y);
	}
	layoutChildren();
}

void Panel::render() {
	Widget::renderBgAndBorders();
	Container::render();
}

// =====================================================
// class ListBase
// =====================================================

ListBase::ListBase()
		: Panel(2, 0)
		, selectedItem(0)
		, selectedIndex(-1)
		, itemFont(0) {
	WIDGET_LOG( __FUNCTION__ << "()" );
	itemFont = CoreData::getInstance().getfreeTypeMenuFont();
}

// =====================================================
// class ListBox
// =====================================================

ListBox::ListBox(ContainerPtr parent)
		: Widget(parent) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr)" );
	borderStyle = BorderStyle::EMBED;
	setBorderSize(2);
}

ListBox::ListBox(ContainerPtr parent, Vec2i pos, Vec2i size) 
		: Widget(parent, pos, size) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr, Vec2i, Vec2i)" );
	borderStyle = BorderStyle::EMBED;
	setBorderSize(2);
}

ListBox::ListBox(WindowPtr window)
		: Widget(window) {
	WIDGET_LOG( __FUNCTION__ << "(WindowPtr)" );
	borderStyle = BorderStyle::EMBED;
	setBorderSize(2);
}

void ListBox::onSelected(ListBoxItemPtr item) {
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
	}
}

void ListBox::addItems(const vector<string> &items) {
	Vec2i sz(size.x - 4, int(itemFont->getMetrics()->getHeight()) + 4);
	foreach_const (vector<string>, it, items) {
		ListBoxItem *nItem = new ListBoxItem(this, Vec2i(0), sz);
		nItem->setTextParams(*it, Vec4f(1.f), itemFont, true, true);
		listBoxItems.push_back(nItem);
		nItem->Selected.connect(this, &ListBox::onSelected);
	}
}

void ListBox::addItem(const string &item) {
	Vec2i sz(size.x - 4, int(itemFont->getMetrics()->getHeight()) + 4);
	ListBoxItem *nItem = new ListBoxItem(this, Vec2i(0), sz);
	nItem->setTextParams(item, Vec4f(1.f), itemFont, true, true);
	listBoxItems.push_back(nItem);
	nItem->Selected.connect(this, &ListBox::onSelected);
}

void ListBox::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	Panel::layoutChildren();
}

int ListBox::getPrefHeight(int childCount) {
	int res = borderSize * 2 + panelPadding * 2;
	int iSize = int(CoreData::getInstance().getfreeTypeMenuFont()->getMetrics()->getHeight()) + 4;
	if (childCount == -1) {
		childCount = listBoxItems.size();
	}
	res += iSize * childCount;
	if (childCount) {
		res += widgetPadding * (childCount - 1);
	}
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
// class ListBoxItem
// =====================================================

ListBoxItem::ListBoxItem(ListBasePtr parent)
		: Widget(parent) 
		, selected(false)
		, hover(false)
		, pressed(false) {
	WIDGET_LOG( __FUNCTION__ << "(ListBasePtr)" );
	borderStyle = BorderStyle::SOLID;
	setBgAlphaValue(0.4f);
	setBorderColour(Vec3f(0.f));
	setBorderSize(2);
}

ListBoxItem::ListBoxItem(ListBasePtr parent, Vec2i pos, Vec2i sz)
		: Widget(parent, pos, sz)
		, selected(false)
		, hover(false)
		, pressed(false) {
	WIDGET_LOG( __FUNCTION__ << "(ListBasePtr, Vec2i, Vec2i)" );
	borderStyle = BorderStyle::SOLID;
	setBgAlphaValue(0.4f);
	setBorderColour(Vec3f(0.f));
	setBorderSize(2);
}

void ListBoxItem::render() {
	Widget::renderBgAndBorders();
	TextWidget::renderTextShadowed();
	if (selected) {
		Widget::renderHighLight(Vec3f(1.f), 0.1f, 0.3f);
	}
}

bool ListBoxItem::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		pressed = true;
		return true;
	}
	return Widget::mouseDown(btn, pos);
}

bool ListBoxItem::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		if (hover && !selected) {
			Selected(this);
			//static_cast<ListBase*>(parent)->setSelected(this);
		}
		if (hover && pressed) {
			Clicked(this);
		}
		pressed = false;
		return true;
	}
	return Widget::mouseUp(btn, pos);
}

// =====================================================
// class ComboBox
// =====================================================

ComboBox::ComboBox(ContainerPtr parent)
		: Widget(parent)
		, floatingList(0) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr)" );
	setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.6f);
	setAutoLayout(false);
	button = new Button(this);
	button->Clicked.connect(this, &ComboBox::onExpandList);
	selectedItem = new ListBoxItem(this);
	selectedItem->Clicked.connect(this, &ComboBox::onBoxClicked);
}

ComboBox::ComboBox(ContainerPtr parent, Vec2i pos, Vec2i size) 
		: Widget(parent, pos, size)
		, floatingList(0) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr, Vec2i, Vec2i)" );
	setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.6f);
	setAutoLayout(false);
	int btn_sz = size.y - borderSize * 2;
	button = new Button(this, Vec2i(size.x - btn_sz - borderSize, borderSize), Vec2i(btn_sz));
	button->Clicked.connect(this, &ComboBox::onExpandList);
	Vec2i liPos(borderSize);
	Vec2i liSz(size.x - btn_sz - borderSize * 3, btn_sz);
	selectedItem = new ListBoxItem(this, liPos, liSz);
	selectedItem->setTextParams("", Vec4f(1.f), itemFont, true, true);
	selectedItem->Clicked.connect(this, &ComboBox::onBoxClicked);
}

void ComboBox::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	int btn_sz = size.y - borderSize * 2;
	button->setSize(Vec2i(btn_sz));
	button->setPos(Vec2i(size.x - btn_sz - borderSize, borderSize));
	Vec2i liPos(borderSize);
	Vec2i liSz(size.x - btn_sz - borderSize * 3, btn_sz);
	selectedItem->setPos(liPos);
	selectedItem->setSize(liSz);
}

void ComboBox::addItems(const vector<string> &items) {
	foreach_const (vector<string>, it, items) {
		listItems.push_back(*it);
	}
}

void ComboBox::addItem(const string &item) {
	listItems.push_back(item);
}

void ComboBox::setSelected(int index) {
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

void ComboBox::expandList() {
	floatingList = new ListBox(rootWindow);
	rootWindow->setFloatingWidget(floatingList);
	floatingList->setPaddingParams(0, 0);
	Vec2i sz(size.x, floatingList->getPrefHeight(listItems.size()));
	Vec2i pos(screenPos.x, screenPos.y - sz.y + size.y);
	floatingList->setPos(pos);
	floatingList->setSize(sz);
	floatingList->addItems(listItems);
	floatingList->setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.6f);
	floatingList->setSelected(selectedIndex);
	floatingList->Destroyed.connect(this, &ComboBox::onListDisposed);
	floatingList->SelectionChanged.connect(this, &ComboBox::onSelectionMade);
	setVisible(false);
	//rootWindow->setFade(0.5f);
	ListExpanded(this);
}

void ComboBox::onBoxClicked(ListBoxItemPtr) {
	expandList();
}

void ComboBox::onExpandList(ButtonPtr) {
	expandList();
}

void ComboBox::onSelectionMade(ListBasePtr lb) {
	assert(floatingList == lb);
	setSelected(lb->getSelectedIndex());
	floatingList->Destroyed.disconnect(this);
	onListDisposed(lb);
	rootWindow->removeFloatingWidget(lb);
}

void ComboBox::onListDisposed(WidgetPtr) {
	setVisible(true);
	floatingList = 0;
	//rootWindow->setFade(1.f);
	ListCollapsed(this);
}

}}


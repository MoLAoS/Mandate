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
//  class StaticText
// =====================================================

void StaticText::render() {
/*
	Vec2i screenPos = getScreenPos();
	Vec2i size = getSize();
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

Vec2i StaticText::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	int xtra = getBorderSize() * 2 + getPadding() + 2;
	return txtDim + Vec2i(xtra);
}

Vec2i StaticText::getPrefSize() const {
	return getMinSize();
}

// =====================================================
//  class StaticImage
// =====================================================

Vec2i StaticImage::getMinSize() const {
	const Pixmap2D *pixmap = getImage()->getPixmap();
	Vec2i imgDim = Vec2i(pixmap->getW(), pixmap->getH());
	int xtra = getBorderSize() * 2 + getPadding() + 2;
	return imgDim + Vec2i(xtra);
}

Vec2i StaticImage::getPrefSize() const {
	return getMinSize();
}

// =====================================================
//  class Button
// =====================================================

Button::Button(Container::Ptr parent)
		: Widget(parent)
		, MouseWidget(this)
		, hover(false), pressed(false) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr)" );
}

Button::Button(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, hover(false), pressed(false) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr, Vec2i, Vec2i)" );
	// background texture
	CoreData &coreData = CoreData::getInstance();
	Texture2D *tex = size.x > 3 * size.y / 2 
		? coreData.getButtonBigTexture() : coreData.getButtonSmallTexture();
	setImage(tex);
}

Vec2i Button::getPrefSize() const {
	const Pixmap2D *pixmap = ImageWidget::getImage()->getPixmap();
	Vec2i imgSize(pixmap->getW(), pixmap->getH());
	Vec2i txtSize(getMinSize());
	Vec2i res(
		imgSize.x > txtSize.x ? imgSize.x : txtSize.x,
		imgSize.y > txtSize.y ? imgSize.y : txtSize.y);
	return res;
}

Vec2i Button::getMinSize() const {
	Vec2i res = TextWidget::getTextDimensions();
	res += Vec2i(getBorderSize() * 2 + getPadding() * 2);
	return res;
}

//Vec2i Button::getMaxSize() const {
//	return Vec2i(-1);
//}

void Button::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	CoreData &coreData = CoreData::getInstance();
	Vec2i size = getSize();
	Texture2D *tex = size.x > 3 * size.y / 2 
		? coreData.getButtonBigTexture() : coreData.getButtonSmallTexture();
	setImage(tex);
}

bool Button::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		pressed = true;
		return true;
	}
	return getParent()->mouseDown(btn, pos);
}

bool Button::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		if (pressed && hover) {
			Clicked(this);
		}
		pressed = false;
		return true;
	}
	return getParent()->mouseDown(btn, pos);
}

void Button::render() {
	// render background
	ImageWidget::renderImage();

	// render hilight
	if (hover) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;
		Widget::renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha);
	}

	// render label
	if (hasText()) {
		TextWidget::renderText();
	}
}

// =====================================================
//  class CheckBox
// =====================================================

CheckBox::CheckBox(Container::Ptr parent)
		: Widget(parent), checked(false) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr)" );
	CoreData &coreData = CoreData::getInstance();
	addImageX(coreData.getCheckBoxCrossTexture(), Vec2i(0), Vec2i(32));
	addImageX(coreData.getCheckBoxTickTexture(), Vec2i(0), Vec2i(32));
	setTextParams("No", Vec4f(1.f), coreData.getfreeTypeMenuFont(), true, false);
	addText("Yes");
}

CheckBox::CheckBox(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size), checked(false) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr, Vec2i, Vec2i)" );
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

Vec2i CheckBox::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	int xtra = getBorderSize() * 2 + getPadding() + 2;
	Vec2i res(txtDim.x + txtDim.y + 2 + xtra, txtDim.y + xtra);
	return res;
}

Vec2i CheckBox::getPrefSize() const {
	Vec2i dim = getTextDimensions();
	int xtra = getBorderSize() * 2 + getPadding() + 2;
	dim.x += 40;
	if (dim.y < 32) {
		dim.y = 32;
	}
	dim += Vec2i(xtra);
	return dim;
}

//Vec2i CheckBox::getMaxSize() const {
//}

bool CheckBox::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		pressed = true;
		return true;
	}
	return getParent()->mouseDown(btn, pos);
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
	return getParent()->mouseDown(btn, pos);
}

void CheckBox::render() {
	// render background
	ImageWidget::renderImage(checked ? 1 : 0);

	// render hilight
	if (hover) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;
		int offset = getBorderSize() + getPadding();
		Widget::renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha, Vec2i(offset), Vec2i(32));
	}

	// render label
	TextWidget::renderText(checked ? 1 : 0);
}

// =====================================================
//  class TextBox
// =====================================================

TextBox::TextBox(Container::Ptr parent)
		: Widget(parent)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, hover(false)
		, focus(false)
		, changed(false) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr)" );
	setBorderStyle(BorderStyle::EMBED);
	setBorderSize(2);
}
		
TextBox::TextBox(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, hover(false)
		, focus(false)
		, changed(false) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr, Vec2i, Vec2i)" );
	setBorderStyle(BorderStyle::EMBED);
	setBorderSize(2);
}

bool TextBox::mouseDown(MouseButton btn, Vec2i pos) {
	if (isEnabled()) {
		focus = true;
		getRootWindow()->aquireKeyboardFocus(this);
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
			getRootWindow()->releaseKeyboardFocus(this);
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

Vec2i TextBox::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	int xtra = getBorderSize() * 2 + getPadding() + 2;
	return Vec2i(200, txtDim.y + xtra);
}

Vec2i TextBox::getPrefSize() const {
	Vec2i dim = getTextDimensions();
	int xtra = getBorderSize() * 2 + getPadding() + 2;
	return Vec2i(400, dim.y + xtra);
}

// =====================================================
//  class VerticalScrollBar
// =====================================================

VerticalScrollBar::VerticalScrollBar(Container::Ptr parent)
		: Widget(parent) 
		, MouseWidget(this)
		, hoverPart(0), pressedPart(0)
		//, fullThumb(false), smallThumb(false)
		, shaftOffset(0), shaftHeight(0)
		, thumbOffset(0), thumbSize(0)
		, totalRange(0), availRange(0), lineSize(0)
		, timeCounter(0) {
	setBorderSize(0);
	setPadding(0);
	setBorderParams(BorderStyle::NONE, 0, Vec3f(0.5f), 0.6f);
	CoreData &coreData = CoreData::getInstance();
	addImage(coreData.getVertScrollUpTexture());
	addImage(coreData.getVertScrollUpHoverTex());
	addImage(coreData.getVertScrollDownTexture());
	addImage(coreData.getVertScrollDownHoverTex());
}

VerticalScrollBar::VerticalScrollBar(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, hoverPart(0), pressedPart(0)
		//, fullThumb(false), smallThumb(false)
		, shaftOffset(0), shaftHeight(0)
		, thumbOffset(0), thumbSize(0)
		, totalRange(0), availRange(0), lineSize(0)
		, timeCounter(0) {
	setBorderSize(0);
	setPadding(0);
	setBorderParams(BorderStyle::NONE, 0, Vec3f(0.5f), 0.6f);
	Vec2i imgSize(size.x);
	Vec2i downPos(0,0);
	Vec2i upPos(0, size.y - imgSize.y);
	CoreData &coreData = CoreData::getInstance();
	addImage(coreData.getVertScrollUpTexture());
	addImage(coreData.getVertScrollUpHoverTex());
	addImage(coreData.getVertScrollDownTexture());
	addImage(coreData.getVertScrollDownHoverTex());
	recalc();
}

void VerticalScrollBar::recalc() {
	Vec2i size = getSize();
	Vec2i imgSize = Vec2i(size.x);
	Vec2i downPos(0,0);
	Vec2i upPos(0, size.y - imgSize.y);
	setImageX(0, 0, upPos, imgSize);
	setImageX(0, 1, upPos, imgSize);
	setImageX(0, 2, downPos, imgSize);
	setImageX(0, 3, downPos, imgSize);
	shaftOffset = imgSize.y;
	shaftHeight = size.y - imgSize.y * 2;
	topOffset = thumbOffset = size.y - imgSize.y;
	float availRatio = availRange / float(totalRange);
	thumbSize = int(availRatio * shaftHeight);
}

bool VerticalScrollBar::mouseDown(MouseButton btn, Vec2i pos) {
	Vec2i localPos = pos - getScreenPos();
	pressedPart = hoverPart;
	if (pressedPart == Part::UPPER_SHAFT || pressedPart == Part::LOWER_SHAFT) {
		thumbOffset = clamp(localPos.y + thumbSize / 2, shaftOffset + thumbSize, shaftOffset + shaftHeight);
		ThumbMoved(this);
		pressedPart = hoverPart = Part::THUMB;
	} else if (pressedPart == Part::UP_BUTTON || pressedPart == Part::DOWN_BUTTON) {
		getRootWindow()->registerUpdate(this);
		timeCounter = 0;
		moveOnMouseUp = true;
	}
	return getParent()->mouseDown(btn, pos);
}

bool VerticalScrollBar::mouseUp(MouseButton btn, Vec2i pos) {
	Vec2i localPos = pos - getScreenPos();
	Part upPart = partAt(localPos);

	if (pressedPart == Part::UP_BUTTON || pressedPart == Part::DOWN_BUTTON) {
		getRootWindow()->unregisterUpdate(this);
	}
	if (pressedPart == upPart && moveOnMouseUp) {
		if (pressedPart == Part::UP_BUTTON) {
			thumbOffset += lineSize;
			thumbOffset = clamp(thumbOffset, shaftOffset + thumbSize, shaftOffset + shaftHeight);
			ThumbMoved(this);
		} else if (pressedPart == Part::DOWN_BUTTON) {
			thumbOffset -= lineSize;
			thumbOffset = clamp(thumbOffset, shaftOffset + thumbSize, shaftOffset + shaftHeight);
			ThumbMoved(this);
		}
	}
	pressedPart = Part::NONE;
	return getParent()->mouseUp(btn, pos);
}

void VerticalScrollBar::update() {
	++timeCounter;
	if (timeCounter % 7 != 0) {
		return;
	}
	if (pressedPart == Part::UP_BUTTON) {
		if (hoverPart == Part::UP_BUTTON) {
			thumbOffset += lineSize;
			thumbOffset = clamp(thumbOffset, shaftOffset + thumbSize, shaftOffset + shaftHeight);
			ThumbMoved(this);
		}
	} else if (pressedPart == Part::DOWN_BUTTON) {
		if (hoverPart == Part::DOWN_BUTTON) {
			thumbOffset -= lineSize;
			thumbOffset = clamp(thumbOffset, shaftOffset + thumbSize, shaftOffset + shaftHeight);
			ThumbMoved(this);
		}
	} else {
		assert(false);
	}
	moveOnMouseUp = false;
}

VerticalScrollBar::Part VerticalScrollBar::partAt(const Vec2i &pos) {
	if (pos.y < shaftOffset) {
		return Part::DOWN_BUTTON;
	} else if (pos.y > shaftOffset + shaftHeight) {
		return Part::UP_BUTTON;
	} else if (pos.y < thumbOffset - thumbSize) {
		return Part::LOWER_SHAFT;
	} else if (pos.y > thumbOffset) {
		return Part::UPPER_SHAFT;
	} else {
		return Part::THUMB;
	}
}

bool VerticalScrollBar::mouseMove(Vec2i pos) {
	Vec2i localPos = pos - getScreenPos();
	if (pressedPart == Part::NONE) {
		hoverPart = partAt(localPos);
	} else {
		if (pressedPart == Part::DOWN_BUTTON) {
			if (hoverPart == Part::DOWN_BUTTON && localPos.y >= shaftOffset) {
				hoverPart = Part::NONE;
			} else if (hoverPart == Part::NONE && localPos.y < shaftOffset) {
				hoverPart = Part::DOWN_BUTTON;
			}
		} else if (pressedPart == Part::UP_BUTTON) {
			if (hoverPart == Part::UP_BUTTON && localPos.y <= shaftOffset + shaftHeight) {
				hoverPart = Part::NONE;
			} else if (hoverPart == Part::NONE && localPos.y > shaftOffset + shaftHeight) {
				hoverPart = Part::UP_BUTTON;
			}
		} else if (pressedPart == Part::THUMB) {
			thumbOffset = clamp(localPos.y + thumbSize / 2, shaftOffset + thumbSize, shaftOffset + shaftHeight);
			ThumbMoved(this);
		} else {
			// don't care for shaft clicked here
		}
	}
	return getParent()->mouseMove(pos);
}

Vec2i VerticalScrollBar::getPrefSize() const {return Vec2i(-1);}
Vec2i VerticalScrollBar::getMinSize() const {return Vec2i(-1);}

void VerticalScrollBar::render() {
	// buttons
	renderImage((pressedPart == Part::UP_BUTTON) ? 1 : 0);	// up arrow
	renderImage((pressedPart == Part::DOWN_BUTTON) ? 3 : 2); // down arrow
	
	if (hoverPart == Part::UP_BUTTON || hoverPart == Part::DOWN_BUTTON) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;

		Vec2i pos(0, hoverPart == Part::UP_BUTTON ? shaftOffset + shaftHeight: 0);
		Vec2i size(getSize().x);
		renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha, pos, size);
	}

	// shaft
	Vec2i shaftPos(0, shaftOffset);
	Vec2i shaftSize(shaftOffset, shaftHeight);
	renderBorders(BorderStyle::EMBED, shaftPos, shaftSize, 1);

	// thumb
	Vec2i thumbPos(1, thumbOffset - thumbSize);
	Vec2i thumbSizev(shaftOffset - 2, thumbSize);
	renderBorders(BorderStyle::RAISE, thumbPos, thumbSizev, 1);
	if (hoverPart == Part::THUMB) {
		renderHighLight(Vec3f(1.f), 0.2f, 0.5f, thumbPos, thumbSizev);
	}
}

// =====================================================
//  class Panel
// =====================================================

Panel::Panel(Container::Ptr parent)
		: Widget(parent)
		, autoLayout(true)
		, layoutOrigin(LayoutOrigin::CENTRE) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr)" );
	setPaddingParams(10, 5);
	setBorderSize(2);
}

Panel::Panel(Container::Ptr parent, Vec2i pos, Vec2i sz)
		: Widget(parent, pos, sz)
		, autoLayout(true)
		, layoutOrigin(LayoutOrigin::CENTRE) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr, Vec2i, Vec2i)" );
	setPaddingParams(10, 5);
	setBorderSize(2);
}

void Panel::setPaddingParams(int panelPad, int widgetPad) {
	setPadding(panelPad);
	widgetPadding = widgetPad;
}

void Panel::layoutChildren() {
	if (!autoLayout || children.empty()) {
		return;
	}
	vector<int> widgetYPos;
	const int borderPlusPad = getBorderSize() + getPadding();
	int wh = 0;
	Vec2i size = getSize();
	Vec2i room = size - Vec2i(borderPlusPad * 2);
	foreach (WidgetList, it, children) {
		wh += (*it)->getHeight();
		widgetYPos.push_back(wh);
		wh += widgetPadding;
	}
	wh -= widgetPadding;
	
	Vec2i topLeft(borderPlusPad, size.y - borderPlusPad);
	int offset;
	switch (layoutOrigin) {
		case LayoutOrigin::FROM_TOP: offset = borderPlusPad; break;
		case LayoutOrigin::CENTRE: offset = (size.y - wh) / 2; break;
		case LayoutOrigin::FROM_BOTTOM: offset = size.y - wh - borderPlusPad; break;
	}
	int offset_y = size.y - offset;
	int ndx = 0;
	foreach (WidgetList, it, children) {
		int ww = (*it)->getWidth();
		int x = topLeft.x + (room.x - ww) / 2;
		int y = offset_y - widgetYPos[ndx++];
		(*it)->setPos(x, y);
	}
}

Vec2i Panel::getMinSize() const {
	///TODO Compositor...
	return Vec2i(-1);
}

Vec2i Panel::getPrefSize() const {
	return Vec2i(-1);
}

void Panel::addChild(Widget::Ptr child) {
	Container::addChild(child);
	if (!autoLayout) {
		return;
	}
	Vec2i sz = child->getSize();
	Vec2i space = getSize() - Vec2i(getPadding() * 2 + getBorderSize() * 2);
	if (sz.x > space.x) {
		child->setSize(space.x, sz.y);
	}
	layoutChildren();
}

void Panel::render() {
	Widget::renderBgAndBorders();
	int brdrPad = getBorderSize() + getPadding();
	Vec2i pos = getScreenPos() + Vec2i(brdrPad);
	Vec2i size = getSize() - Vec2i(brdrPad) * 2;
	glEnable(GL_SCISSOR_TEST);
	glScissor(pos.x, pos.y, size.x, size.y);
	Container::render();
	glDisable(GL_SCISSOR_TEST);
}

// =====================================================
//  class PicturePanel
// =====================================================

Vec2i PicturePanel::getMinSize() const {
	const Pixmap2D *pixmap = getImage()->getPixmap();
	Vec2i imgDim = Vec2i(pixmap->getW(), pixmap->getH());
	int xtra = getBorderSize() * 2 + getPadding() + 2;
	return imgDim + Vec2i(xtra);
}

Vec2i PicturePanel::getPrefSize() const {
	return Vec2i(-1);
}

// =====================================================
//  class ListBase
// =====================================================

ListBase::ListBase()
		: Panel(2, 0, false)
		, selectedItem(0)
		, selectedIndex(-1)
		, itemFont(0) {
	WIDGET_LOG( __FUNCTION__ << "()" );
	itemFont = CoreData::getInstance().getfreeTypeMenuFont();
}

// =====================================================
//  class ListBox
// =====================================================

ListBox::ListBox(Container::Ptr parent)
		: Widget(parent)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr)" );
	setBorderStyle(BorderStyle::EMBED);
	setBorderSize(2);
	setAutoLayout(false);
}

ListBox::ListBox(Container::Ptr parent, Vec2i pos, Vec2i size) 
		: Widget(parent, pos, size)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr, Vec2i, Vec2i)" );
	setBorderStyle(BorderStyle::EMBED);
	setBorderSize(2);
	setAutoLayout(false);
}

ListBox::ListBox(WidgetWindow::Ptr window)
		: Widget(window)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	WIDGET_LOG( __FUNCTION__ << "(WidgetWindow::Ptr)" );
	setBorderStyle(BorderStyle::EMBED);
	setBorderSize(2);
	setAutoLayout(false);
}

void ListBox::onSelected(ListBoxItem::Ptr item) {
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
	Vec2i sz(getSize().x - 4, int(itemFont->getMetrics()->getHeight()) + 4);
	foreach_const (vector<string>, it, items) {
		ListBoxItem *nItem = new ListBoxItem(this, Vec2i(0), sz);
		nItem->setTextParams(*it, Vec4f(1.f), itemFont, true, true);
		listBoxItems.push_back(nItem);
		nItem->Selected.connect(this, &ListBox::onSelected);
	}

}

void ListBox::addItem(const string &item) {
	Vec2i sz(getSize().x - 4, int(itemFont->getMetrics()->getHeight()) + 4);
	ListBoxItem *nItem = new ListBoxItem(this, Vec2i(0), sz);
	nItem->setTextParams(item, Vec4f(1.f), itemFont, true, true);
	listBoxItems.push_back(nItem);
	nItem->Selected.connect(this, &ListBox::onSelected);
}

void ListBox::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	layoutChildren();
}

void ListBox::layoutChildren() {
	if (children.empty()) {
		return;
	}
	const int borderPlusPad = getBorderSize() + getPadding();
	Vec2i size = getSize();

	// scrollBar ?
	int totalItemHeight = getPrefHeight() - 2 * borderPlusPad;
	int clientHeight = size.y - 2 * borderPlusPad;

	if (totalItemHeight > clientHeight) {
		if (!scrollBar) {
			scrollBar = new VerticalScrollBar(this);
			scrollBar->ThumbMoved.connect(this, &ListBox::onScroll);
		}
		scrollBar->setSize(Vec2i(24, clientHeight));
		scrollBar->setPos(Vec2i(size.x - 24 - borderPlusPad, borderPlusPad));
		scrollBar->setRanges(totalItemHeight, clientHeight, 2);
		//cout << "setting scroll ranges total = " << totalItemHeight << ", actual = " << clientHeight << endl;
		int scrollOffset = scrollBar->getRangeOffset();
		//cout << "range offset = " << scrollOffset << endl;
	} else {
		if (scrollBar) {
			delete scrollBar;
			scrollBar = 0;
		}
	}

	vector<int> widgetYPos;
	int wh = 0;
	Vec2i room = size - Vec2i(borderPlusPad * 2);
	if (scrollBar) {
		room.x -= scrollBar->getWidth();
	}
	foreach (WidgetList, it, children) {
		if (*it == scrollBar) {
			continue;
		}
		wh += (*it)->getHeight();
		widgetYPos.push_back(wh);
		wh += widgetPadding;
	}
	wh -= widgetPadding;
	
	Vec2i topLeft(borderPlusPad, size.y - borderPlusPad);
	int offset_y = size.y - borderPlusPad;
	int ndx = 0;
	yPositions.clear();
	foreach (WidgetList, it, children) {
		if (*it == scrollBar) {
			continue;
		}
		int y = offset_y - widgetYPos[ndx++];
		(*it)->setPos(topLeft.x, y);
		(*it)->setSize(room.x, (*it)->getHeight());
		yPositions.push_back(y);
	}
}

void ListBox::onScroll(VerticalScrollBar::Ptr) {
	int offset = scrollBar->getRangeOffset();
	//cout << "Scroll offset = " << offset << endl;

	int ndx = 0;
	const int borderPlusPad = getBorderSize() + getPadding();
	foreach (WidgetList, it, children) {
		if (*it == scrollBar) {
			continue;
		}
		//Widget::Ptr widget = *it;
		(*it)->setPos(borderPlusPad, yPositions[ndx++] - offset);
	}
}

int ListBox::getPrefHeight(int childCount) {
	int res = getBorderSize() * 2 + getPadding() * 2;
	int iSize = int(CoreData::getInstance().getfreeTypeMenuFont()->getMetrics()->getHeight()) + 4;
	if (childCount == -1) {
		childCount = children.size();//listBoxItems.size();
	}
	res += iSize * childCount;
	if (childCount) {
		res += widgetPadding * (childCount - 1);
	}
	return res;
}

Vec2i ListBox::getMinSize() const {
	Vec2i itemPref(0);
	foreach_const (WidgetList, it, children) {
		Vec2i ips = (*it)->getPrefSize();
		if (ips.x > itemPref.x) itemPref.x = ips.x;
		if (ips.y > itemPref.y) itemPref.y = ips.y;
	}
	itemPref.y *= 3;
	int xtra = getBorderSize() * 2 + getPadding() * 2;
	return itemPref + Vec2i(xtra);	
}

Vec2i ListBox::getPrefSize() const {
	Vec2i itemPref(0);
	foreach_const (WidgetList, it, children) {
		Vec2i ips = (*it)->getPrefSize();
		if (ips.x > itemPref.x) itemPref.x = ips.x;
		itemPref.y += ips.y;
	}
	int xtra = getBorderSize() * 2 + getPadding() * 2;
	return itemPref + Vec2i(xtra);
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

ListBoxItem::ListBoxItem(ListBase::Ptr parent)
		: Widget(parent) 
		, MouseWidget(this)
		, selected(false)
		, hover(false)
		, pressed(false) {
	WIDGET_LOG( __FUNCTION__ << "(ListBase::Ptr)" );
	setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.4f);
}

ListBoxItem::ListBoxItem(ListBase::Ptr parent, Vec2i pos, Vec2i sz)
		: Widget(parent, pos, sz)
		, MouseWidget(this)
		, selected(false)
		, hover(false)
		, pressed(false) {
	WIDGET_LOG( __FUNCTION__ << "(ListBase::Ptr, Vec2i, Vec2i)" );
	setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.4f);
}


Vec2i ListBoxItem::getMinSize() const {
	Vec2i dims = getTextDimensions();
	int xtra = getBorderSize() * 2 + getPadding() * 2;
	return dims + Vec2i(xtra);
}

Vec2i ListBoxItem::getPrefSize() const {
	return getMinSize();
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
//  class DropList
// =====================================================

DropList::DropList(Container::Ptr parent)
		: Widget(parent)
		, floatingList(0)
		, dropBoxHeight(0) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr)" );
	setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.6f);
	setPadding(0);
	setAutoLayout(false);
	button = new Button(this);
	button->Clicked.connect(this, &DropList::onExpandList);
	selectedItem = new ListBoxItem(this);
	selectedItem->Clicked.connect(this, &DropList::onBoxClicked);
}

DropList::DropList(Container::Ptr parent, Vec2i pos, Vec2i size) 
		: Widget(parent, pos, size)
		, floatingList(0)
		, dropBoxHeight(0) {
	WIDGET_LOG( __FUNCTION__ << "(Container::Ptr, Vec2i, Vec2i)" );
	setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.6f);
	setPadding(0);
	setAutoLayout(false);
	const int borderSize = getBorderSize();
	int btn_sz = size.y - borderSize * 2;
	button = new Button(this, Vec2i(size.x - btn_sz - borderSize, borderSize), Vec2i(btn_sz));
	button->Clicked.connect(this, &DropList::onExpandList);
	Vec2i liPos(borderSize + getPadding());
	Vec2i liSz(size.x - btn_sz - borderSize * 2 - getPadding(), btn_sz);
	selectedItem = new ListBoxItem(this, liPos, liSz);
	selectedItem->setTextParams("", Vec4f(1.f), itemFont, true, true);
	selectedItem->Clicked.connect(this, &DropList::onBoxClicked);
}

Vec2i DropList::getPrefSize() const {
	Vec2i res = selectedItem->getPrefSize();
	int xtra = getBorderSize() * 2 + getPadding() * 2;
	int btn_sz = res.y - getBorderSize() * 2;
	res.x += btn_sz;
	return  res + Vec2i(xtra); 
}

Vec2i DropList::getMinSize() const {
	Vec2i res = selectedItem->getMinSize();
	int xtra = getBorderSize() * 2 + getPadding() * 2;
	int btn_sz = res.y - getBorderSize() * 2;
	res.x += btn_sz;
	return  res + Vec2i(xtra); 
}

void DropList::setSize(const Vec2i &size) {
	Widget::setSize(size);
	layout();
}

void DropList::layout() {
	const int borderSize = getBorderSize();
	Vec2i size = getSize();
	int btn_sz = size.y - borderSize * 2;
	button->setSize(Vec2i(btn_sz));
	button->setPos(Vec2i(size.x - btn_sz - borderSize, borderSize));
	Vec2i liPos(borderSize);
	Vec2i liSz(size.x - btn_sz - borderSize * 3, btn_sz);
	selectedItem->setPos(liPos);
	selectedItem->setSize(liSz);
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

void DropList::expandList() {
	floatingList = new ListBox(getRootWindow());
	getRootWindow()->setFloatingWidget(floatingList);
	floatingList->setPaddingParams(0, 0);
	const Vec2i &size = getSize();
	const Vec2i &screenPos = getScreenPos();
	int h = dropBoxHeight == 0 ? floatingList->getPrefHeight(listItems.size()) : dropBoxHeight;
	Vec2i sz(size.x, h);
	Vec2i pos(screenPos.x, screenPos.y - sz.y + size.y);
	floatingList->setPos(pos);
	floatingList->setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.6f);
	floatingList->addItems(listItems);
	floatingList->setSize(sz);

	floatingList->setSelected(selectedIndex);
	floatingList->Destroyed.connect(this, &DropList::onListDisposed);
	floatingList->SelectionChanged.connect(this, &DropList::onSelectionMade);
	setVisible(false);
	ListExpanded(this);
}

void DropList::onBoxClicked(ListBoxItem::Ptr) {
	expandList();
}

void DropList::onExpandList(Button::Ptr) {
	expandList();
}

void DropList::onSelectionMade(ListBase::Ptr lb) {
	assert(floatingList == lb);
	setSelected(lb->getSelectedIndex());
	floatingList->Destroyed.disconnect(this);
	onListDisposed(lb);
	getRootWindow()->removeFloatingWidget(lb);
}

void DropList::onListDisposed(Widget::Ptr) {
	setVisible(true);
	floatingList = 0;
	//rootWindow->setFade(1.f);
	ListCollapsed(this);
}

}}


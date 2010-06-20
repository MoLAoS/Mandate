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
	Widget::renderBgAndBorders();
	renderText();
}

Vec2i StaticText::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
	xtra += Vec2i(getPadding());
	return txtDim + xtra;
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
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
	xtra += Vec2i(getPadding());
	return imgDim + xtra;
}

Vec2i StaticImage::getPrefSize() const {
	return getMinSize();
}

// =====================================================
//  class Button
// =====================================================

Button::Button(Container::Ptr parent)
		: Widget(parent)
		, TextWidget(this)
		, ImageWidget(this)
		, MouseWidget(this)
		, hover(false)
		, pressed(false) {
}

Button::Button(Container::Ptr parent, Vec2i pos, Vec2i size, bool defaultTexture)
		: Widget(parent, pos, size)
		, TextWidget(this)
		, ImageWidget(this)
		, MouseWidget(this)
		, hover(false), pressed(false) {
	// background texture
	if (defaultTexture) {
		CoreData &coreData = CoreData::getInstance();
		Texture2D *tex = size.x > 3 * size.y / 2 
			? coreData.getButtonBigTexture() : coreData.getButtonSmallTexture();
		setImage(tex);
	}
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
	Vec2i txt = TextWidget::getTextDimensions();
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
	xtra += Vec2i(getPadding());
	return txt + xtra;
}

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
	return true;
}

bool Button::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		if (pressed && hover) {
			Clicked(this);
		}
		pressed = false;
		return true;
	}
	return false;
}

void Button::render() {
	// render background
	ImageWidget::renderImage();

	// render hilight
	if (hover && isEnabled()) {
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
		: Button(parent), checked(false) {
	CoreData &coreData = CoreData::getInstance();
	addImageX(coreData.getCheckBoxCrossTexture(), Vec2i(0), Vec2i(32));
	addImageX(coreData.getCheckBoxTickTexture(), Vec2i(0), Vec2i(32));
	setTextParams("No", Vec4f(1.f), coreData.getfreeTypeMenuFont(), false);
	addText("Yes");
}

CheckBox::CheckBox(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Button(parent, pos, size, false), checked(false) {
	CoreData &coreData = CoreData::getInstance();
	addImageX(coreData.getCheckBoxCrossTexture(), Vec2i(0), Vec2i(32));
	addImageX(coreData.getCheckBoxTickTexture(), Vec2i(0), Vec2i(32));
	setTextParams("No", Vec4f(1.f), coreData.getfreeTypeMenuFont(), false);
	addText("Yes");
	int y = int((size.y - getTextFont()->getMetrics()->getHeight()) / 2);
	setTextPos(Vec2i(40, y), 0);
	setTextPos(Vec2i(40, y), 1);
}

void CheckBox::setSize(const Vec2i &sz) {
	// bypass Button::setSize()
	Widget::setSize(sz);
	int y = int((sz.y - getTextFont()->getMetrics()->getHeight()) / 2);
	setTextPos(Vec2i(40, y), 0);
	setTextPos(Vec2i(40, y), 1);
}

Vec2i CheckBox::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
	xtra += Vec2i(getPadding());
	Vec2i res = txtDim + xtra + Vec2i(txtDim.y + 2, 0);
	return res;
}

Vec2i CheckBox::getPrefSize() const {
	Vec2i dim = getTextDimensions();
	dim.x += 40;
	if (dim.y < 32) {
		dim.y = 32;
	}
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
	xtra += Vec2i(getPadding());
	return dim + xtra;
}

bool CheckBox::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		pressed = true;
		return true;
	}
	return false;
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
	return false;
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
		Vec2i offset(m_borderStyle.m_sizes[Border::LEFT], m_borderStyle.m_sizes[Border::BOTTOM]);
		offset += Vec2i(getPadding());
		Widget::renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha, offset, Vec2i(32));
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
		, TextWidget(this)
		, hover(false)
		, focus(false)
		, changed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::TEXT_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::TEXT_BOX);
}
		
TextBox::TextBox(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, TextWidget(this)
		, hover(false)
		, focus(false)
		, changed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::TEXT_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::TEXT_BOX);
}

bool TextBox::mouseDown(MouseButton btn, Vec2i pos) {
	if (isEnabled()) {
		focus = true;
		getRootWindow()->aquireKeyboardFocus(this);
		return true;
	}
	return false;
}

bool TextBox::mouseUp(MouseButton btn, Vec2i pos) {
	return true;
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
	return false;
}

bool TextBox::keyUp(Key key) {
	return false;
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
	return false;
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
	return Vec2i(200, txtDim.y) + m_borderStyle.getBorderDims();
}

Vec2i TextBox::getPrefSize() const {
	Vec2i dim = getTextDimensions();
	return dim + m_borderStyle.getBorderDims();
}

// =====================================================
//  class Slider
// =====================================================

Slider::Slider(Container::Ptr parent, Vec2i pos, Vec2i size, const string &title)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_sliderValue(0.f)
		, m_thumbHover(false)
		, m_thumbPressed(false)
		, m_shaftHover(false)
		, m_title(title) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::SLIDER);

	const CoreData &coreData = CoreData::getInstance();
	Font *font = coreData.getfreeTypeMenuFont();
	addImage(coreData.getButtonSmallTexture());
	setTextParams(m_title, Vec4f(1.f), font, false);
	addText("0 %");
	
	string maxVal = "100 %";
	Vec2f dims = getTextFont()->getMetrics()->getTextDiminsions(maxVal);
	m_valSize = int(dims.x + 5.f);
	m_shaftStyle.setRaise(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
	m_shaftStyle.setSizes(2);

	recalc();
}

void Slider::recalc() {
	Vec2i size = getSize();	

	int space = size.x - m_valSize;
	m_shaftOffset = int(space * 0.35f);
	m_shaftSize = int(space * 0.60f);

	m_thumbCentre = m_shaftOffset + 5 + int(m_sliderValue * (m_shaftSize - 10));

	space = size.y - (m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]);
	m_thumbPos = Vec2i(m_thumbCentre - space / 4 + m_borderStyle.m_sizes[Border::LEFT], 
						m_borderStyle.m_sizes[Border::BOTTOM]);
	m_thumbSize = Vec2i(space / 2, space);
	setImageX(0, 0, m_thumbPos, m_thumbSize);

	Vec2f dims = getTextFont()->getMetrics()->getTextDiminsions(m_title);
	m_titlePos = Vec2i(m_shaftOffset / 2 - int(dims.x / 2), size.y / 2 - int(dims.y / 2));

	string sliderString = Conversion::toStr(int(m_sliderValue * 100.f)) + " %";
	setText(sliderString, 1);
	dims = getTextFont()->getMetrics()->getTextDiminsions(sliderString);
	int valWidth = size.x - m_shaftOffset - m_shaftSize;
	m_valuePos = Vec2i(m_shaftOffset + m_shaftSize + (valWidth / 2 - int(dims.x / 2)), size.y / 2 - int(dims.y / 2));
	setTextPos(m_titlePos, 0);
	setTextPos(m_valuePos, 1);
}

bool Slider::mouseMove(Vec2i pos) {
	pos -= getScreenPos();
	if (m_thumbPressed) {
		int x_pos = clamp(pos.x - m_shaftOffset - 5, 0, m_shaftSize - 10);
		float oldVal = m_sliderValue;
		m_sliderValue = x_pos / float(m_shaftSize - 10);
		recalc();
		if (oldVal != m_sliderValue) {
			ValueChanged(this);
		}
		return true;
	}
	if (Vec2i::isInside(pos, m_thumbPos, m_thumbSize)) {
		m_thumbHover = true;
	} else {
		m_thumbHover = false;
	}
	if (!m_thumbHover) {
		if (pos.x >= m_shaftOffset + 3 && pos.x < m_shaftOffset + m_shaftSize - 3) {
			m_shaftHover = true;
		} else {
			m_shaftHover = false;
		}
	} else {
		m_shaftHover = true;
	}
	return true;
}

void Slider::mouseOut() {
	if (!m_thumbPressed) {
		m_shaftHover = m_thumbHover = false;
	}
}

bool Slider::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn != MouseButton::LEFT) {
		return false;
	}
	pos -= getScreenPos();
	if (m_thumbHover || m_shaftHover) {
		m_thumbHover = true;
		m_shaftHover = false;
		m_thumbPressed = true;

		int x_pos = clamp(pos.x - m_shaftOffset - 5, 0, m_shaftSize - 10);
		float oldVal = m_sliderValue;
		m_sliderValue = x_pos / float(m_shaftSize - 10);
		recalc();
		if (oldVal != m_sliderValue) {
			ValueChanged(this);
		}
	}
	return true;
}

bool Slider::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn != MouseButton::LEFT) {
		return false;
	}
	if (m_thumbPressed) {
		m_thumbPressed = false;
	}
	if (isInside(pos)) {
		pos -= getScreenPos();
		if (Vec2i::isInside(pos, m_thumbPos, m_thumbSize)) {
			m_thumbHover = true;
		} else {
			m_thumbHover = false;
		}
		if (!m_thumbHover) {
			if (pos.x >= m_shaftOffset + 3 && pos.x < m_shaftOffset + m_shaftSize - 3) {
				m_shaftHover = true;
			} else {
				m_shaftHover = false;
			}
		} else {
			m_shaftHover = false;
		}

	} else {
		m_shaftHover = m_thumbHover = false;
	}
	return true;
}

void Slider::render() {
	Widget::renderBgAndBorders();
	TextWidget::renderText(0); // label
	TextWidget::renderText(1); // value %

	// slide bar
	Vec2i size = getSize();
	int cy = size.y / 2;
	Vec2i pos(m_shaftOffset + 5, cy - 3);
	Vec2i sz(m_shaftSize - 10, 6);

	Widget::renderBorders(m_shaftStyle, pos, sz);

	// slider thumb
	ImageWidget::renderImage();
	if (m_thumbHover || m_shaftHover) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;
		//int offset = getBorderSize() + getPadding();
		Widget::renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha, m_thumbPos, m_thumbSize);
	}
}

// =====================================================
//  class VerticalScrollBar
// =====================================================

VerticalScrollBar::VerticalScrollBar(Container::Ptr parent)
		: Widget(parent)
		, ImageWidget(this)
		, MouseWidget(this)
		, hoverPart(0), pressedPart(0)
		//, fullThumb(false), smallThumb(false)
		, shaftOffset(0), shaftHeight(0)
		, thumbOffset(0), thumbSize(0)
		, totalRange(0), availRange(0), lineSize(0)
		, timeCounter(0) {
	init();
}

VerticalScrollBar::VerticalScrollBar(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, ImageWidget(this)
		, MouseWidget(this)
		, hoverPart(0), pressedPart(0)
		//, fullThumb(false), smallThumb(false)
		, shaftOffset(0), shaftHeight(0)
		, thumbOffset(0), thumbSize(0)
		, totalRange(0), availRange(0), lineSize(0)
		, timeCounter(0) {
	init();
	recalc();
}

void VerticalScrollBar::init() {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::SCROLL_BAR);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::SCROLL_BAR);
	setPadding(0);
	addImage(g_coreData.getVertScrollUpTexture());
	addImage(g_coreData.getVertScrollUpHoverTex());
	addImage(g_coreData.getVertScrollDownTexture());
	addImage(g_coreData.getVertScrollDownHoverTex());
	m_shaftStyle.setEmbed(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
	m_shaftStyle.setSizes(1);
	m_thumbStyle.setRaise(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
	m_thumbStyle.setSizes(1);
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
	return true;
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
	return true;
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
	return true;
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
	renderBorders(m_shaftStyle, shaftPos, shaftSize);
	renderBackground(m_backgroundStyle, shaftPos, shaftSize);

	// thumb
	Vec2i thumbPos(1, thumbOffset - thumbSize);
	Vec2i thumbSizev(shaftOffset - 2, thumbSize);
	renderBorders(m_thumbStyle, thumbPos, thumbSizev);
	if (hoverPart == Part::THUMB) {
		renderHighLight(Vec3f(1.f), 0.2f, 0.5f, thumbPos, thumbSizev);
	}
}

// =====================================================
//  class Panel
// =====================================================

Panel::Panel(Container::Ptr parent)
		: Container(parent)
		, autoLayout(true)
		, layoutOrigin(LayoutOrigin::CENTRE) {
	setPaddingParams(10, 5);
	m_borderStyle.setNone();

}

Panel::Panel(Container::Ptr parent, Vec2i pos, Vec2i sz)
		: Container(parent, pos, sz)
		, autoLayout(true)
		, layoutOrigin(LayoutOrigin::CENTRE) {
	setPaddingParams(10, 5);
	m_borderStyle.setNone();
}

Panel::Panel(WidgetWindow::Ptr window)
		: Container(window) {
}

void Panel::setPaddingParams(int panelPad, int widgetPad) {
	setPadding(panelPad);
	widgetPadding = widgetPad;
}

void Panel::setLayoutParams(bool autoLayout, LayoutDirection dir, LayoutOrigin origin) {
	assert(
		origin == LayoutOrigin::CENTRE
		|| ((origin == LayoutOrigin::FROM_BOTTOM || origin == LayoutOrigin::FROM_TOP)
			&& dir == LayoutDirection::VERTICAL)
		|| ((origin == LayoutOrigin::FROM_LEFT || origin == LayoutOrigin::FROM_RIGHT)
			&& dir == LayoutDirection::HORIZONTAL)
	);
	this->layoutDirection = dir;
	this->layoutOrigin = origin;		
	this->autoLayout = autoLayout;
}

void Panel::layoutChildren() {
	if (!autoLayout || children.empty()) {
		return;
	}
	vector<int> widgetYPos;
//	const int borderPlusPad = getBorderSize() + getPadding();
	int wh = 0;
	Vec2i size = getSize();
	Vec2i room = size - m_borderStyle.getBorderDims();
	foreach (WidgetList, it, children) {
		wh += (*it)->getHeight();
		widgetYPos.push_back(wh);
		wh += widgetPadding;
	}
	wh -= widgetPadding;
	
	Vec2i topLeft(m_borderStyle.m_sizes[Border::LEFT] + getPadding(),
				size.y - m_borderStyle.m_sizes[Border::TOP] - getPadding());
	
	int offset;
	switch (layoutOrigin) {
		case LayoutOrigin::FROM_TOP: offset = m_borderStyle.m_sizes[Border::TOP]; break;
		case LayoutOrigin::CENTRE: offset = (size.y - wh) / 2; break;
		case LayoutOrigin::FROM_BOTTOM: offset = size.y - wh - m_borderStyle.m_sizes[Border::TOP]; break;
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
	int space_x = getWidth() - m_borderStyle.getHorizBorderDim() - getPadding() * 2;
	if (sz.x > space_x) {
		child->setSize(space_x, sz.y);
	}
	layoutChildren();
}

void Panel::remChild(Widget::Ptr child) {
	Container::remChild(child);
}

void Panel::render() {
	Widget::renderBgAndBorders();
	Vec2i offset(m_borderStyle.m_sizes[Border::LEFT] + getPadding(),
				m_borderStyle.m_sizes[Border::BOTTOM] + getPadding());
	Vec2i pos = getScreenPos() + offset;
	Vec2i size = getSize() - m_borderStyle.getBorderDims() - Vec2i(getPadding() * 2);
	glPushAttrib(GL_SCISSOR_BIT);
		glEnable(GL_SCISSOR_TEST);
		glScissor(pos.x, pos.y, size.x, size.y);
		Container::render();
		glDisable(GL_SCISSOR_TEST);
	glPopAttrib();
}

// =====================================================
//  class PicturePanel
// =====================================================

Vec2i PicturePanel::getMinSize() const {
	const Pixmap2D *pixmap = getImage()->getPixmap();
	Vec2i imgDim = Vec2i(pixmap->getW(), pixmap->getH());
	Vec2i xtra = m_borderStyle.getBorderDims() + Vec2i(getPadding() * 2);
	return imgDim + xtra;
}

Vec2i PicturePanel::getPrefSize() const {
	return Vec2i(-1);
}

// =====================================================
//  class ListBase
// =====================================================

ListBase::ListBase(Container::Ptr parent)
		: Panel(parent)
		, selectedItem(0)
		, selectedIndex(-1)
		, itemFont(0) {
	setPaddingParams(2, 0);
	setAutoLayout(false);
	itemFont = CoreData::getInstance().getfreeTypeMenuFont();
}

ListBase::ListBase(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size)
		, selectedItem(0)
		, selectedIndex(-1)
		, itemFont(0) {
	setPaddingParams(2, 0);
	setAutoLayout(false);
	itemFont = CoreData::getInstance().getfreeTypeMenuFont();
}

ListBase::ListBase(WidgetWindow* window) 
		: Panel(window)
		, selectedItem(0)
		, selectedIndex(-1) {
	itemFont = CoreData::getInstance().getfreeTypeMenuFont();
}

// =====================================================
//  class ListBox
// =====================================================

ListBox::ListBox(Container::Ptr parent)
		: ListBase(parent)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_BOX);
	setAutoLayout(false);
}

ListBox::ListBox(Container::Ptr parent, Vec2i pos, Vec2i size) 
		: ListBase(parent, pos, size)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_BOX);
	setAutoLayout(false);
}

ListBox::ListBox(WidgetWindow::Ptr window)
		: ListBase(window)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_BOX);
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
		nItem->setTextParams(*it, Vec4f(1.f), itemFont, true);
		listBoxItems.push_back(nItem);
		nItem->Selected.connect(this, &ListBox::onSelected);
	}

}

void ListBox::addItem(const string &item) {
	Vec2i sz(getSize().x - getBordersHoriz(), int(itemFont->getMetrics()->getHeight()) + 4);
	ListBoxItem *nItem = new ListBoxItem(this, Vec2i(getBorderLeft(), getBorderBottom()), sz);
	nItem->setTextParams(item, Vec4f(1.f), itemFont, true);
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
	Vec2i room = size - m_borderStyle.getBorderDims() - Vec2i(getPadding() * 2);
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
	
	Vec2i topLeft(m_borderStyle.m_sizes[Border::LEFT], 
		size.y - m_borderStyle.m_sizes[Border::TOP] - getPadding());

	int ndx = 0;
	yPositions.clear();
	foreach (WidgetList, it, children) {
		if (*it == scrollBar) {
			continue;
		}
		int y = topLeft.y - widgetYPos[ndx++];
		(*it)->setPos(topLeft.x, y);
		(*it)->setSize(room.x, (*it)->getHeight());
		static_cast<ListBoxItem::Ptr>(*it)->centreText();
		yPositions.push_back(y);
	}
}

void ListBox::onScroll(VerticalScrollBar::Ptr) {
	int offset = scrollBar->getRangeOffset();
	//cout << "Scroll offset = " << offset << endl;

	int ndx = 0;
	const int x = m_borderStyle.m_sizes[Border::LEFT] + getPadding();
	foreach (WidgetList, it, children) {
		if (*it == scrollBar) {
			continue;
		}
		//Widget::Ptr widget = *it;
		(*it)->setPos(x, yPositions[ndx++] - offset);
	}
}

int ListBox::getPrefHeight(int childCount) {
	int res = m_borderStyle.getVertBorderDim() + getPadding() * 2;
	int iSize = int(g_widgetConfig.getFont(WidgetFont::MENU_NORMAL)->getMetrics()->getHeight()) + 4;
	if (childCount == -1) {
		childCount = children.size();//listBoxItems.size();
	}
	res += iSize * childCount;
	if (childCount) {
		res += widgetPadding * (childCount - 1);
	}
	return res;
}

///@todo handle no children
Vec2i ListBox::getMinSize() const {
	Vec2i res(0);
	foreach_const (WidgetList, it, children) {
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
	foreach_const (WidgetList, it, children) {
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

ListBoxItem::ListBoxItem(ListBase::Ptr parent)
		: Widget(parent) 
		, TextWidget(this)
		, MouseWidget(this)
		, selected(false)
		, hover(false)
		, pressed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_ITEM);
	m_borderStyle.m_colourIndices[1] = WidgetColour::DARK_BORDER;
	m_borderStyle.m_colourIndices[2] = WidgetColour::LIGHT_BORDER;
	m_borderStyle.m_colourIndices[0] = m_borderStyle.m_colourIndices[1];

	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::LIST_ITEM);
}

ListBoxItem::ListBoxItem(ListBase::Ptr parent, Vec2i pos, Vec2i sz)
		: Widget(parent, pos, sz)
		, TextWidget(this)
		, MouseWidget(this)
		, selected(false)
		, hover(false)
		, pressed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_ITEM);
	m_borderStyle.m_colourIndices[1] = WidgetColour::DARK_BORDER;
	m_borderStyle.m_colourIndices[2] = WidgetColour::LIGHT_BORDER;
	m_borderStyle.m_colourIndices[0] = m_borderStyle.m_colourIndices[1];

	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::LIST_ITEM);
}


Vec2i ListBoxItem::getMinSize() const {
	Vec2i dims = getTextDimensions();
	Vec2i xtra = m_borderStyle.getBorderDims() + Vec2i(getPadding() * 2);
	return dims + xtra;
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

void ListBoxItem::mouseIn() {
	if (isEnabled()) {
		m_borderStyle.m_colourIndices[0] = m_borderStyle.m_colourIndices[2];
		hover = true;
	}
}

void ListBoxItem::mouseOut() {
	m_borderStyle.m_colourIndices[0] = m_borderStyle.m_colourIndices[1];
	hover = false;
}

bool ListBoxItem::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		pressed = true;
		return true;
	}
	return false;
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
	return false;
}

// =====================================================
//  class DropList
// =====================================================

DropList::DropList(Container::Ptr parent)
		: ListBase(parent)
		, floatingList(0)
		, dropBoxHeight(0) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::DROP_LIST);
	setAutoLayout(false);
	setPadding(0);
	button = new Button(this);
	button->Clicked.connect(this, &DropList::onExpandList);
	selectedItem = new ListBoxItem(this);
	selectedItem->setTextParams("", Vec4f(1.f), itemFont, true);
	selectedItem->Clicked.connect(this, &DropList::onBoxClicked);
}

DropList::DropList(Container::Ptr parent, Vec2i pos, Vec2i size) 
		: ListBase(parent, pos, size)
		, floatingList(0)
		, dropBoxHeight(0) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::DROP_LIST);
	setAutoLayout(false);
	setPadding(0);
	button = new Button(this);
	button->Clicked.connect(this, &DropList::onExpandList);
	selectedItem = new ListBoxItem(this);
	selectedItem->setTextParams("", Vec4f(1.f), itemFont, true);
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
	int ph = floatingList->getPrefHeight(listItems.size());
	int h = dropBoxHeight == 0 ? ph : ph > dropBoxHeight ? dropBoxHeight : ph;

	Vec2i sz(size.x, h);
	Vec2i pos(screenPos.x, screenPos.y - sz.y + size.y);
	floatingList->setPos(pos);
	floatingList->addItems(listItems);
	floatingList->setSize(sz);

	floatingList->setSelected(selectedIndex);
	floatingList->Destroyed.connect(this, &DropList::onListDisposed);
	floatingList->SelectionChanged.connect(this, &DropList::onSelectionMade);
	setVisible(false);
	ListExpanded(this);
}

void DropList::onBoxClicked(ListBoxItem::Ptr) {
	if (isEnabled()) {
		expandList();
	}
}

void DropList::onExpandList(Button::Ptr) {
	if (isEnabled()) {
		expandList();
	}
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
	ListCollapsed(this);
}

}}


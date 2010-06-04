// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "widgets_base.h"
#include "widget_window.h"

#include "metrics.h"
#include "texture_gl.h"
#include "game_constants.h"
#include "core_data.h"
#include "renderer.h"

using Shared::Util::deleteValues;
using namespace Shared::Graphics::Gl;
using Glest::Graphics::Renderer;
using namespace Glest::Global;

namespace Glest { namespace Widgets {

MouseWidget::MouseWidget(WidgetPtr widget) {
	me = widget;
	WidgetWindow::getInstance()->registerMouseWidget(me, this);
}

MouseWidget::~MouseWidget() {
	WidgetWindow::getInstance()->unregisterMouseWidget(me);
}

KeyboardWidget::KeyboardWidget(WidgetPtr widget) {
	me = widget;
	WidgetWindow::getInstance()->registerKeyboardWidget(me, this);
}

KeyboardWidget::~KeyboardWidget() {
	WidgetWindow::getInstance()->unregisterKeyboardWidget(me);
}

// =====================================================
// class Widget
// =====================================================

Widget::Widget(ContainerPtr parent)
		: parent(parent)
		, pos(0)
		, screenPos(0)
		, size(0)
		, visible(true)
		, enabled(true)
		, fade(1.f)
		, borderStyle(BorderStyle::NONE)
		, borderColour(0.f)
		, borderSize(0)
		, bgAlpha(0.2f)
		, padding(0) {
	rootWindow = parent->getRootWindow();
	parent->addChild(this);
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr)" );
}

Widget::Widget(ContainerPtr parent, Vec2i pos, Vec2i size)
		: parent(parent)
		, pos(pos)
		, size(size)
		, visible(true)
		, enabled(true)
		, fade(1.f)
		, borderStyle(BorderStyle::NONE)
		, borderColour(0.f)
		, borderSize(0)
		, bgAlpha(0.2f)
		, padding(0) {
	WIDGET_LOG( __FUNCTION__ << "(ContainerPtr, Vec2i, Vec2i)" );
	screenPos = parent->getScreenPos() + pos;
	rootWindow = parent->getRootWindow();
	parent->addChild(this);
}

Widget::Widget(WindowPtr window)
		: parent(0)
		, pos(0)
		, screenPos(0)
		, size(0)
		, visible(true)
		, enabled(true)
		, fade(1.f)
		, borderStyle(BorderStyle::NONE)
		, borderColour(0.f)
		, borderSize(0)
		, bgAlpha(0.2f)
		, padding(0) {
	WIDGET_LOG( __FUNCTION__ << "(WindowPtr)" );
	rootWindow = window;
}

Widget::~Widget() {
	WIDGET_LOG( __FUNCTION__ );
	Destroyed(this);
	if (rootWindow != this) {
		parent->remChild(this);
	}
}

WidgetPtr Widget::getWidgetAt(const Vec2i &pos) {
	WIDGET_LOG( __FUNCTION__ );
	assert(isInside(pos));
	return this;
}

string Widget::descPosDim() {
	std::stringstream ss;
	ss << "Pos: " << screenPos << ", Size: " << size;
	return ss.str();
}

bool Widget::mouseDown(MouseButton btn, Vec2i pos) {
	return parent->mouseDown(btn, pos);
}

bool Widget::mouseUp(MouseButton btn, Vec2i pos) {
	return parent->mouseUp(btn, pos);
}

bool Widget::mouseMove(Vec2i pos) {
	return parent->mouseMove(pos);
}

bool Widget::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	return parent->mouseDoubleClick(btn, pos);
}

bool Widget::mouseWheel(Vec2i pos, int z) {
	return parent->mouseWheel(pos, z);
}

void Widget::mouseIn() {
	WIDGET_LOG( "mouseIn() : " << desc() );
}

void Widget::mouseOut() {
	WIDGET_LOG( "mouseOut() : " << desc() );
}

bool Widget::keyDown(Key key) {
	return parent->keyDown(key);
}

bool Widget::keyUp(Key key) {
	return parent->keyUp(key);
}

bool Widget::keyPress(char c) {
	return parent->keyPress(c);
}

void Widget::setSize(const Vec2i &sz) {
	size = sz;
}

void Widget::setPos(const Vec2i &p) {
	pos = p;
	screenPos = parent->getScreenPos() + pos;
}

void Widget::setBorderParams(BorderStyle st, int sz, Vec3f col, float alph) {
	borderStyle = st;
	borderSize = sz;
	borderColour = col;
	bgAlpha = alph;
}

void Widget::renderBorders(BorderStyle style, const Vec2i &offset, const Vec2i &size, int borderSize) {
	if (style != BorderStyle::NONE && borderSize > 0) {
		Vec4f colourBackground(0.5f, 0.5f, 0.5f, bgAlpha * fade);
		Vec4f colourTopLeft;
		Vec4f colourBottomRight;
		switch (style) {
			case BorderStyle::RAISE:
				colourTopLeft = Vec4f(1.f, 1.f, 1.f, bgAlpha * fade);
				colourBottomRight = Vec4f(0.f, 0.f, 0.f, bgAlpha * fade);
				break;
			case BorderStyle::EMBED:
				colourTopLeft = Vec4f(0.f, 0.f, 0.f, bgAlpha * fade);
				colourBottomRight = Vec4f(1.f, 1.f, 1.f, bgAlpha * fade);
				break;
			case BorderStyle::SOLID:
				colourTopLeft = Vec4f(borderColour, bgAlpha * fade);
				colourBottomRight = Vec4f(borderColour, bgAlpha * fade);
				break;
		}
		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		float border = float(borderSize);
		Vec2f verts[8];
		verts[0] = Vec2f(screenPos + offset);
		verts[1] = verts[0] + Vec2f(border, border);
		verts[2] = verts[0] + Vec2f(0.f, float(size.y));
		verts[3] = verts[2] + Vec2f(border, -border);
		verts[4] = verts[0] + Vec2f(size);
		verts[5] = verts[4] + Vec2f(-border, -border);
		verts[6] = verts[0] + Vec2f(float(size.x), 0.f);
		verts[7] = verts[6] + Vec2f(-border, border);

		glColor4fv(colourTopLeft.ptr());
		glBegin(GL_QUAD_STRIP);
			glVertex2fv(verts[0].ptr());
			glVertex2fv(verts[1].ptr());
			glVertex2fv(verts[2].ptr());
			glVertex2fv(verts[3].ptr());
			glVertex2fv(verts[4].ptr());
			glVertex2fv(verts[5].ptr());
		glEnd();

		glColor4fv(colourBottomRight.ptr());
		glBegin(GL_QUAD_STRIP);
			glVertex2fv(verts[4].ptr());
			glVertex2fv(verts[5].ptr());
			glVertex2fv(verts[6].ptr());
			glVertex2fv(verts[7].ptr());
			glVertex2fv(verts[0].ptr());
			glVertex2fv(verts[1].ptr());
		glEnd();

		glColor4fv(colourBackground.ptr());
		glBegin(GL_TRIANGLE_FAN);
			glVertex2fv(verts[1].ptr());
			glVertex2fv(verts[3].ptr());
			glVertex2fv(verts[5].ptr());
			glVertex2fv(verts[7].ptr());
		glEnd();
		glPopAttrib();
	}
}

void Widget::renderBgAndBorders() {
	renderBorders(borderStyle, Vec2i(0), size, borderSize);
}

void Widget::renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha, Vec2i offset, Vec2i size) {
	const Vec4f borderColour = Vec4f(colour, borderAlpha * fade);
	const Vec4f centreColour = Vec4f(colour, centreAlpha * fade);
	float x1 = float(screenPos.x + offset.x);
	float y1 = float(screenPos.y + offset.y);
	float x2 = x1 + float(size.x);
	float y2 = y1 + float(size.y);
	
	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
	glEnable(GL_BLEND);

	glBegin(GL_TRIANGLE_FAN);
		// centre
		glColor4fv(centreColour.ptr());
		glVertex2f((x1 + x2) / 2.f, (y1 + y2) / 2.f);
		// corners
		glColor4fv(borderColour.ptr());
		glVertex2f(x1, y1);
		glVertex2f(x2, y1);
		glVertex2f(x2, y2);
		glVertex2f(x1, y2);
		glVertex2f(x1, y1);
	glEnd();
	glPopAttrib();
}

void Widget::renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha) {
	Vec2i offset(borderSize);
	Vec2i sz = Vec2i(size) - (offset * 2);
	renderHighLight(colour, centreAlpha, borderAlpha, offset, sz);
}

// =====================================================
// class ImageWidget
// =====================================================

ImageWidget::ImageWidget() {
	WIDGET_LOG( __FUNCTION__ );
}

ImageWidget::ImageWidget(Texture2D *tex) {
	WIDGET_LOG( __FUNCTION__ << "(Texture2D*)" );
	textures.push_back(tex);
	imageInfo.push_back(ImageRenderInfo());
}

void ImageWidget::renderImage(int ndx) {
	ASSERT_RANGE(ndx, textures.size());

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	int x1, x2, y1, y2;
	Vec2i pos = getScreenPos();
	x1 = pos.x;
	y1 = pos.y;
	if (imageInfo[ndx].hasOffset) {
		x1 += imageInfo[ndx].offset.x;
		y1 += imageInfo[ndx].offset.y;
	}
	if (imageInfo[ndx].hasCustomSize) {
		x2 = x1 + imageInfo[ndx].size.x;
		y2 = y1 + imageInfo[ndx].size.y;
	} else {
		Vec2i sz = getSize();
		x2 = x1 + sz.x;
		y2 = y1 + sz.y;
	}
	glColor4f(1.f, 1.f, 1.f, getFade());
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(textures[ndx])->getHandle());
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2i(0, 1);
		glVertex2i(x1, y2);
		glTexCoord2i(0, 0);
		glVertex2i(x1, y1);
		glTexCoord2i(1, 1);
		glVertex2i(x2, y2);
		glTexCoord2i(1, 0);
		glVertex2i(x2, y1);
	glEnd();

	glPopAttrib();
	assertGl();
}

int ImageWidget::addImage(Texture2D *tex) {
	textures.push_back(tex);
	imageInfo.push_back(ImageRenderInfo());
	return textures.size() - 1;
}

void ImageWidget::setImage(Texture2D *tex, int ndx) {
	if (textures.empty() && !ndx) {
		textures.push_back(tex);
		imageInfo.push_back(ImageRenderInfo());
	}
	ASSERT_RANGE(ndx, textures.size());
	textures[ndx] = tex;
	imageInfo[ndx] = ImageRenderInfo();
}

int ImageWidget::addImageX(Texture2D *tex, Vec2i offset, Vec2i sz) {
	textures.push_back(tex);
	bool hasOffset = offset != Vec2i(0);
	bool hasSize = sz != Vec2i(0);
	ImageRenderInfo info(hasOffset, offset, hasSize, sz);
	imageInfo.push_back(info);
	return textures.size() - 1;
}

void ImageWidget::setImageX(Texture2D *tex, int ndx, Vec2i offset, Vec2i sz) {
	if (textures.empty() && !ndx) {
		assert(tex);
		textures.push_back(tex);
		imageInfo.push_back(ImageRenderInfo());
	}
	ASSERT_RANGE(ndx, textures.size());
	if (tex) {
		textures[ndx] = tex;
	}
	ImageRenderInfo info(offset != Vec2i(0), offset, sz != Vec2i(0), sz);
	imageInfo[ndx] = info;
}

// =====================================================
// class TextWidget
// =====================================================

TextWidget::TextWidget() 
		: txtColour(1.f)
		, txtShadowColour(0.f)
		, txtPos(0)
		, font(0)
		, isFreeTypeFont(false)
		, centre(true) {
	WIDGET_LOG( __FUNCTION__ );
}

void TextWidget::centreText(int ndx) {
	ASSERT_RANGE(ndx, texts.size());
	const Metrics &metrics = Metrics::getInstance();
	Vec2f txtDims = font->getMetrics()->getTextDiminsions(texts[ndx]);
	Vec2i centre = getScreenPos() + (getSize() / 2);
	txtPos = Vec2i(centre.x - int(txtDims.x / 2.f), centre.y - int(txtDims.y / 2.f));
	txtPos -= getScreenPos();
}

void TextWidget::renderText(const string &txt, int x, int y, const Vec4f &colour, const Font *font) {
	if (!font) {
		font = this->font;
	}
	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
	glEnable(GL_BLEND);
	glColor4fv(colour.ptr());
	TextRenderer *tr = getRootWindow()->getTextRenderer(isFreeTypeFont);
	tr->begin(font);
	tr->render(txt, x, y);
	tr->end();
	glPopAttrib();
}

void TextWidget::renderText(int ndx) {
	ASSERT_RANGE(ndx, texts.size());
	Vec2i pos = getScreenPos() + txtPos;
	txtColour.a = getFade();
	renderText(texts[ndx], pos.x, pos.y, txtColour);
}

void TextWidget::renderTextShadowed(int ndx) {
	ASSERT_RANGE(ndx, texts.size());
	Vec2i pos = getScreenPos() + txtPos;
	Vec2i sPos = pos + Vec2i(2, -2);
	txtShadowColour.a = txtColour.a = getFade();
	renderText(texts[ndx], sPos.x, sPos.y, txtShadowColour);
	renderText(texts[ndx], pos.x, pos.y, txtColour);
}

Vec2i TextWidget::getTextDimensions() const {
	Vec2i max = Vec2i(0);
	foreach_const (vector<string>, it, texts) {
		Vec2i tSz = Vec2i(font->getMetrics()->getTextDiminsions(*it) + Vec2f(1.f));
		if (max.x < tSz.x) {
			max.x = tSz.x;
		}
		if (max.y < tSz.y) {
			max.y = tSz.y;
		}

	}
	return max;
}

void TextWidget::setTextParams(const string &txt, const Vec4f colour, const Font *font, bool freeType, bool centre) {
	if (texts.empty()) {
		texts.push_back(txt);
	} else {
		texts[0] = txt;
	}
	txtColour = colour;
	this->font = font;
	isFreeTypeFont = freeType;
	this->centre = centre;
	if (centre) {
		centreText();
	}
}

int TextWidget::addText(const string &txt) {
	texts.push_back(txt);
	if (centre && texts.size() == 1) {
		centreText();
	}
	return texts.size() - 1;
}

void TextWidget::setText(const string &txt, int ndx) {
	if (texts.empty() && !ndx) {
		texts.push_back(txt);
	} else {
		ASSERT_RANGE(ndx, texts.size());
		texts[ndx] = txt;
	}
	if (centre && texts.size() == 1) {
		centreText();
	}
}

void TextWidget::setTextFont(const Font *f) {
	font = f;
	if (centre && texts.size() == 1) {
		centreText();
	}
}

void TextWidget::setTextPos(const Vec2i &pos) { 
	txtPos = pos;
}

void TextWidget::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	if (centre && texts.size() == 1) {
		centreText();
	}
}

void TextWidget::setPos(const Vec2i &p) {
	Widget::setPos(p);
	if (centre && texts.size() == 1) {
		centreText();
	}
}

// =====================================================
// class Container
// =====================================================

Container::Container() {
	WIDGET_LOG( __FUNCTION__ );
}

Container::~Container() {
	WIDGET_LOG( __FUNCTION__ );
	clear();
}

void Container::addChild(WidgetPtr child) {
	WIDGET_LOG( __FUNCTION__ );
	children.push_back(child);
}

void Container::remChild(WidgetPtr child) {
	WIDGET_LOG( __FUNCTION__ );
	WidgetList::iterator it = std::find(children.begin(), children.end(), child);
	if (it != children.end()) {
		children.erase(it);
	}
}

void Container::clear() {
	WIDGET_LOG( __FUNCTION__ );
	deleteValues(children);
	children.clear();
}

void Container::setEnabled(bool v) {
	Widget::setEnabled(v);
	foreach (WidgetList, it, children) {
		(*it)->setEnabled(v);
	}
}

void Container::setFade(float v) {
	Widget::setFade(v);
	foreach (WidgetList, it, children) {
		(*it)->setFade(v);
	}
}

WidgetPtr Container::getWidgetAt(const Vec2i &pos) {
	WIDGET_LOG( __FUNCTION__ );
	assert(isInside(pos));
	foreach (WidgetList, it, children) {
		WidgetPtr widget = *it;
		if (widget->isVisible() && widget->isInside(pos)) {
			return widget->getWidgetAt(pos);
		}
	}
	return this;
}

void Container::render() {
	foreach (WidgetList, it, children) {
		WidgetPtr widget = *it;
		if (widget->isVisible()) {
			widget->render();
		}
	}
}

}}

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
#include "widget_config.h"

#include "metrics.h"
#include "texture_gl.h"
#include "game_constants.h"
#include "core_data.h"
#include "renderer.h"

#include "leak_dumper.h"

using Shared::Util::deleteValues;
using namespace Shared::Graphics::Gl;
using Glest::Graphics::Renderer;
using namespace Glest::Global;

namespace Glest { namespace Widgets {

MouseWidget::MouseWidget(Widget* widget) {
	me = widget;
	me->setMouseWidget(this);
}

KeyboardWidget::KeyboardWidget(Widget* widget) {
	me = widget;
	me->setKeyboardWidget(this);
}

// =====================================================
// class Widget
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(Widget)

Widget::Widget(Container* parent)
		: parent(parent) {
	init(Vec2i(0), Vec2i(0));
	rootWindow = parent->getRootWindow();
	parent->addChild(this);
}

Widget::Widget(Container* parent, Vec2i pos, Vec2i size)
		: parent(parent) {
	init(pos, size);
	screenPos = parent->getScreenPos() + pos;
	rootWindow = parent->getRootWindow();
	parent->addChild(this);
}

Widget::Widget(WidgetWindow* window)
		: parent(window)
		, screenPos(0) {
	init(Vec2i(0), Vec2i(0));
	rootWindow = window;
}

Widget::~Widget() {
	Destroyed(this);
	if (rootWindow != this) {
		parent->remChild(this);
	}
}

void Widget::init(const Vec2i &pos, const Vec2i &size) {
	this->pos = pos;
	this->size = size;
	visible = true;
	m_enabled = true;
	fade = 1.f;

	padding = 0;
	mouseWidget = 0;
	keyboardWidget = 0;
	textWidget = 0;
}

Widget* Widget::getWidgetAt(const Vec2i &pos) {
	assert(isInside(pos));
	return this;
}

string Widget::descPosDim() {
	std::stringstream ss;
	ss << "Pos: " << screenPos << ", Size: " << size;
	return ss.str();
}

void Widget::setSize(const Vec2i &sz) {
	size = sz;
	if (textWidget) {
		textWidget->widgetReSized();
	}
}

void Widget::setPos(const Vec2i &p) {
	pos = p;
	screenPos = parent->getScreenPos() + pos;
}

inline Colour calcColour(const Colour &c, float fade) {
	Colour res(c);
	res.a = clamp(unsigned(float(c.a) * fade), 0u, 255u);
	return res;
}

void Widget::setBorderStyle(const BorderStyle &style) {
	m_borderStyle = style;
}

void Widget::setBackgroundStyle(const BackgroundStyle &style) {
	m_backgroundStyle = style;
}


void Widget::renderBorders(const BorderStyle &style, const Vec2i &offset, const Vec2i &size) {
	assert(style.m_type != BorderType::NONE);
	assert(size.x >= style.m_sizes[Border::LEFT] + style.m_sizes[Border::RIGHT]);
	assert(size.y >= style.m_sizes[Border::TOP] + style.m_sizes[Border::BOTTOM]);

	enum { OBL, OTL, OTR, OBR, IBL, ITL, ITR, IBR };
	Vec2i verts[8];
	verts[OBL] = screenPos + offset;
	verts[OTL] = verts[OBL] + Vec2i(0, size.y);
	verts[OTR] = verts[OBL] + size;
	verts[OBR] = verts[OBL] + Vec2i(size.x, 0);

	verts[IBL] = verts[OBL] + Vec2i(style.m_sizes[Border::LEFT], style.m_sizes[Border::BOTTOM]);
	verts[ITL] = verts[OTL] + Vec2i(style.m_sizes[Border::LEFT], -style.m_sizes[Border::TOP]);
	verts[ITR] = verts[OTR] + Vec2i(-style.m_sizes[Border::RIGHT], -style.m_sizes[Border::TOP]);
	verts[IBR] = verts[OBR] + Vec2i(-style.m_sizes[Border::RIGHT], style.m_sizes[Border::BOTTOM]);

	assertGl();
	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	bool raised = false;

	Colour colour;
	switch (style.m_type) {
		case BorderType::SOLID:
			// alpha ?
			colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[0]), getFade());
			glColor4ubv(colour.ptr());
			glBegin(GL_QUAD_STRIP);
				glVertex2iv(verts[OBL].ptr());
				glVertex2iv(verts[IBL].ptr());
				glVertex2iv(verts[OTL].ptr());
				glVertex2iv(verts[ITL].ptr());
				glVertex2iv(verts[OTR].ptr());
				glVertex2iv(verts[ITR].ptr());
				glVertex2iv(verts[OBR].ptr());
				glVertex2iv(verts[IBR].ptr());
				glVertex2iv(verts[OBL].ptr());
				glVertex2iv(verts[IBL].ptr());
			glEnd();

			break;

		case BorderType::RAISE:
			raised = true;
		case BorderType::EMBED: {
				colour = calcColour(
					g_widgetConfig.getColour(style.m_colourIndices[raised ? 0 : 1]), getFade());
				glBegin(GL_QUAD_STRIP);
					glColor4ubv(colour.ptr());
					glVertex2iv(verts[OBL].ptr());
					glVertex2iv(verts[IBL].ptr());
					glVertex2iv(verts[OTL].ptr());
					glVertex2iv(verts[ITL].ptr());
					glVertex2iv(verts[OTR].ptr());
					glVertex2iv(verts[ITR].ptr());
				glEnd();
				colour = calcColour(
					g_widgetConfig.getColour(style.m_colourIndices[raised ? 1 : 0]), getFade());
				glBegin(GL_QUAD_STRIP);
					glColor4ubv(colour.ptr());
					glVertex2iv(verts[OTR].ptr());
					glVertex2iv(verts[ITR].ptr());
					glVertex2iv(verts[OBR].ptr());
					glVertex2iv(verts[IBR].ptr());
					glVertex2iv(verts[OBL].ptr());
					glVertex2iv(verts[IBL].ptr());
				glEnd();
			}
			break;

		case BorderType::CUSTOM_SIDES:
			glBegin(GL_QUADS);
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[0]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OBL].ptr());
				glVertex2iv(verts[IBL].ptr());
				glVertex2iv(verts[OTL].ptr());
				glVertex2iv(verts[ITL].ptr());
			glEnd();
			glBegin(GL_QUADS);
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[1]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTL].ptr());
				glVertex2iv(verts[ITL].ptr());
				glVertex2iv(verts[OTR].ptr());
				glVertex2iv(verts[ITR].ptr());
			glEnd();
			glBegin(GL_QUADS);
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[2]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTR].ptr());
				glVertex2iv(verts[ITR].ptr());
				glVertex2iv(verts[OBR].ptr());
				glVertex2iv(verts[IBR].ptr());
			glEnd();
			glBegin(GL_QUADS);
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[3]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OBR].ptr());
				glVertex2iv(verts[IBR].ptr());
				glVertex2iv(verts[OBL].ptr());
				glVertex2iv(verts[IBL].ptr());
			glEnd();

			break;

		case BorderType::CUSTOM_CORNERS:
			glBegin(GL_QUAD_STRIP);
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[0]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTL].ptr());
				glVertex2iv(verts[ITL].ptr());
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[1]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTR].ptr());
				glVertex2iv(verts[ITR].ptr());
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[2]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OBR].ptr());
				glVertex2iv(verts[IBR].ptr());
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[3]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OBL].ptr());
				glVertex2iv(verts[IBL].ptr());
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[0]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTL].ptr());
				glVertex2iv(verts[ITL].ptr());
			glEnd();

			break;
	}
	glPopAttrib();
	assertGl();
}

void Widget::renderBackground(const BackgroundStyle &style, const Vec2i &offset, const Vec2i &size) {
	assert(style.m_type != BackgroundType::NONE);

	Vec2i verts[4];
	verts[0] = screenPos + offset;
	verts[1] = verts[0] + Vec2i(0, size.y);
	verts[2] = verts[0] + size;
	verts[3] = verts[0] + Vec2i(size.x, 0);

	assertGl();
	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);

	Colour colour;
	switch (style.m_type) {
		case BackgroundType::COLOUR:
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_TRIANGLE_FAN);
				// alpha ?
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[0]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[0].ptr());
				glVertex2iv(verts[1].ptr());
				glVertex2iv(verts[2].ptr());
				glVertex2iv(verts[3].ptr());
			glEnd();

			break;

		case BackgroundType::CUSTOM_COLOURS:
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_TRIANGLE_FAN);
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[0]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[0].ptr());
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[1]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[1].ptr());
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[2]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[2].ptr());
				colour = calcColour(g_widgetConfig.getColour(style.m_colourIndices[3]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[3].ptr());
			glEnd();

			break;

		case BackgroundType::TEXTURE:
			glEnable(GL_TEXTURE_2D);
			const Texture2D *tex = g_widgetConfig.getTexture(style.m_imageIndex);
			glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(tex)->getHandle());
			glBegin(GL_TRIANGLE_FAN);
			glColor4f(1.f, 1.f, 1.f, getFade());
				glTexCoord2i(0, 0);
				glVertex2iv(verts[0].ptr());
				glTexCoord2i(0, 1);
				glVertex2iv(verts[1].ptr());
				glTexCoord2i(1, 0);
				glVertex2iv(verts[2].ptr());
				glTexCoord2i(1, 1);
				glVertex2iv(verts[3].ptr());
			glEnd();

			break;
	}
	glPopAttrib();
	assertGl();
}

void Widget::renderBgAndBorders(bool bg) {
	if (m_backgroundStyle.m_type) {
		renderBackground(m_backgroundStyle, Vec2i(0), size);
	}
	if (m_borderStyle.m_type) {
		renderBorders(m_borderStyle, Vec2i(0), size);
	}
}

void Widget::renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha, Vec2i offset, Vec2i size) {
	const Vec4f borderColour = Vec4f(colour, borderAlpha * fade);
	const Vec4f centreColour = Vec4f(colour, centreAlpha * fade);
	float x1 = float(screenPos.x + offset.x);
	float y1 = float(screenPos.y + offset.y);
	float x2 = x1 + float(size.x);
	float y2 = y1 + float(size.y);
	
	assertGl();
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
	assertGl();
}

void Widget::renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha) {
	Vec2i offset(m_borderStyle.m_sizes[Border::LEFT], m_borderStyle.m_sizes[Border::BOTTOM]);
	Vec2i inset(m_borderStyle.m_sizes[Border::RIGHT], m_borderStyle.m_sizes[Border::TOP]);
	Vec2i sz = Vec2i(size) - offset - inset;
	renderHighLight(colour, centreAlpha, borderAlpha, offset, sz);
}

// =====================================================
// class ImageWidget
// =====================================================

ImageWidget::ImageWidget(Widget* me) 
		: me(me) {
	batchRender = false;
}

ImageWidget::ImageWidget(Widget* me, Texture2D *tex)
		: me(me) {
	textures.push_back(tex);
	imageInfo.push_back(ImageRenderInfo());
	batchRender = false;
}

void ImageWidget::renderImage(int ndx) {
	Vec4f colour(1.f, 1.f, 1.f, me->getFade());
	renderImage(ndx, colour);
}

void ImageWidget::startBatch() {
	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	batchRender = true;
}

void ImageWidget::endBatch() {
	glPopAttrib();
	assertGl();
	batchRender = false;
}

void ImageWidget::renderImage(int ndx, const Vec4f &colour) {
	ASSERT_RANGE(ndx, textures.size());
	assertGl();

	if (!batchRender) {
		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
	}

	int x1, x2, y1, y2;
	Vec2i pos = me->getScreenPos();
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
		Vec2i sz = me->getSize();
		x2 = x1 + sz.x;
		y2 = y1 + sz.y;
	}
	glColor4fv(colour.ptr());
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

	if (!batchRender) {
		glPopAttrib();
		assertGl();
	}
}

int ImageWidget::addImage(const Texture2D *tex) {
	textures.push_back(tex);
	imageInfo.push_back(ImageRenderInfo());
	return textures.size() - 1;
}

void ImageWidget::setImage(const Texture2D *tex, int ndx) {
	if (textures.empty() && !ndx) {
		textures.push_back(tex);
		imageInfo.push_back(ImageRenderInfo());
	}
	ASSERT_RANGE(ndx, textures.size());
	textures[ndx] = tex;
	//imageInfo[ndx] = ImageRenderInfo();
}

int ImageWidget::addImageX(const Texture2D *tex, Vec2i offset, Vec2i sz) {
	textures.push_back(tex);
	bool hasOffset = offset != Vec2i(0);
	bool hasSize = sz != Vec2i(0);
	ImageRenderInfo info(hasOffset, offset, hasSize, sz);
	imageInfo.push_back(info);
	return textures.size() - 1;
}

void ImageWidget::setImageX(const Texture2D *tex, int ndx, Vec2i offset, Vec2i sz) {
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

TextWidget::TextWidget(Widget* me)
		: me(me)
		, txtColour(1.f)
		, txtShadowColour(0.f)
		, font(0)
		, centre(true)
		, m_batchRender(false) {
	me->textWidget = this;
}

void TextWidget::centreText(int ndx) {
	if (texts.empty()) {
		return;
	}
	ASSERT_RANGE(ndx, texts.size());
	const Metrics &metrics = Metrics::getInstance();
	Vec2f txtDims = font->getMetrics()->getTextDiminsions(texts[ndx]);
	Vec2i halfText = Vec2i(txtDims) / 2;
	Vec2i centre = me->getSize() / 2;
	txtPositions[ndx] = centre - halfText;
}

void TextWidget::widgetReSized() {
	if (centre) {
		centreText();
	}
}

void TextWidget::startBatch(const Font *font) {
	if (!font) {
		font = this->font;
	}
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_BLEND);
	m_textRenderer = g_widgetWindow.getTextRenderer();
	m_textRenderer->begin(font);
	m_batchRender = true;
}

void TextWidget::endBatch() {
	m_textRenderer->end();
	glPopAttrib();
	m_batchRender = false;
}


void TextWidget::renderText(const string &txt, int x, int y, const Vec4f &colour, const Font *font) {
	assertGl();
	if (!m_batchRender) {
		if (!font) {
			font = this->font;
		}
		glPushAttrib(GL_ENABLE_BIT);
		glEnable(GL_BLEND);
		m_textRenderer = g_widgetWindow.getTextRenderer();
		m_textRenderer->begin(font);
	} 
	glColor4fv(colour.ptr());
	m_textRenderer->render(txt, x, y);
	if (!m_batchRender) {
		m_textRenderer->end();
		glPopAttrib();
	}
	assertGl();
}

void TextWidget::renderText(int ndx) {
	ASSERT_RANGE(ndx, texts.size());
	Vec2i pos = me->getScreenPos() + txtPositions[ndx];
	txtColour.a = me->getFade();
	renderText(texts[ndx], pos.x, pos.y, txtColour);
}

void TextWidget::renderTextShadowed(int ndx, int offset) {
	ASSERT_RANGE(ndx, texts.size());
	Vec2i pos = me->getScreenPos() + txtPositions[ndx];
	Vec2i sPos = pos + Vec2i(offset, -offset);
	txtShadowColour.a = txtColour.a = me->getFade();
	renderText(texts[ndx], sPos.x, sPos.y, txtShadowColour);
	renderText(texts[ndx], pos.x, pos.y, txtColour);
}

void TextWidget::renderTextDoubleShadowed(int ndx, int offset) {
	ASSERT_RANGE(ndx, texts.size());
	Vec2i pos = me->getScreenPos() + txtPositions[ndx];
	Vec2i sPos1 = pos + Vec2i(offset, -offset);
	Vec2i sPos2 = sPos1 + Vec2i(offset, -offset);
	txtShadowColour2.a = txtShadowColour.a = txtColour.a = me->getFade();
	renderText(texts[ndx], sPos2.x, sPos2.y, txtShadowColour2);
	renderText(texts[ndx], sPos1.x, sPos1.y, txtShadowColour);
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

Vec2i TextWidget::getTextDimensions(int ndx) const {
	return Vec2i(font->getMetrics()->getTextDiminsions(texts[ndx]) + Vec2f(1.f));
}

void TextWidget::setTextParams(const string &txt, const Vec4f colour, const Font *font, bool cntr) {
	if (texts.empty()) {
		texts.push_back(txt);
		txtPositions.push_back(Vec2i(0));
	} else {
		texts[0] = txt;
	}
	txtColour = colour;
	this->font = font;
	centre = cntr;
	if (centre) {
		centreText();
	}
}

int TextWidget::addText(const string &txt) {
	texts.push_back(txt);
	txtPositions.push_back(Vec2i(0));
	return texts.size() - 1;
}

void TextWidget::setText(const string &txt, int ndx) {
	if (texts.empty() && !ndx) {
		texts.push_back(txt);
		txtPositions.push_back(Vec2i(0));
	} else {
		ASSERT_RANGE(ndx, texts.size());
		texts[ndx] = txt;
	}
	if (centre && ndx == 0) {
		centreText();
	}
}

void TextWidget::setTextFont(const Font *f) {
	font = f;
}

void TextWidget::setTextPos(const Vec2i &pos, int ndx) {
	ASSERT_RANGE(ndx, texts.size());
	txtPositions[ndx] = pos;
}

// =====================================================
// class Container
// =====================================================

Container::Container(Container* parent) 
		: Widget(parent) {
}

Container::Container(Container* parent, Vec2i pos, Vec2i size) 
		: Widget(parent, pos, size) {
}

Container::Container(WidgetWindow* window)
		: Widget(window) {
}

Container::~Container() {
	clear();
}

void Container::addChild(Widget* child) {
	assert(std::find(children.begin(), children.end(), child) == children.end());
	children.push_back(child);
	child->setFade(getFade());
}

void Container::remChild(Widget* child) {
	WidgetList::iterator it = std::find(children.begin(), children.end(), child);
	if (it != children.end()) {
		children.erase(it);
	}
}

void Container::clear() {
	deleteValues(children);
	children.clear();
}

void Container::setPos(const Vec2i &p) {
	Widget::setPos(p);
	foreach (WidgetList, it, children) {
		(*it)->setPos((*it)->getPos());
	}
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

Widget* Container::getWidgetAt(const Vec2i &pos) {
	foreach_rev (WidgetList, it, children) {
		Widget* widget = *it;
		if (widget->isVisible() && widget->isInside(pos)) {
			return widget->getWidgetAt(pos);
		}
	}
	return this;
}

void Container::render() {
	assertGl();
	foreach (WidgetList, it, children) {
		Widget* widget = *it;
		if (widget->isVisible()) {
			widget->render();
		}
	}
	assertGl();
}

}}

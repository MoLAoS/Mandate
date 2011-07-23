// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
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

// =====================================================
//  struct Anchors
// =====================================================

Anchors::Anchors()
		: left(AnchorType::NONE, 0), top(AnchorType::NONE, 0)
		, right(AnchorType::NONE, 0), bottom(AnchorType::NONE, 0)
		, centreVertical(false), centreHorizontal(false) {
}

Anchors::Anchors(Anchor all)
		: left(all), top(all), right(all), bottom(all)
		, centreVertical(false), centreHorizontal(false) {
}

Anchors::Anchors(Anchor leftRight, Anchor topBottom)
		: left(leftRight), top(topBottom), right(leftRight), bottom(topBottom)
		, centreVertical(false), centreHorizontal(false) {
}

Anchors::Anchors(Anchor left, Anchor top, Anchor right, Anchor bottom)
		: left(left), top(top), right(right), bottom(bottom)
		, centreVertical(false), centreHorizontal(false) {
}

Anchor Anchors::get(Edge e) const {
	if (e == Edge::LEFT) {
		return left;
	} else if (e == Edge::TOP) {
		return top;
	} else if (e == Edge::RIGHT) {
		return right;
	} else if (e == Edge::BOTTOM) {
		return bottom;
	} else {
		assert(false);
		throw runtime_error("invalid edge enum passed to Anchors::get()");
	}
}

void Anchors::set(Edge e, Anchor a) {
	if (e == Edge::LEFT) {
		left = a;
	} else if (e == Edge::TOP) {
		top = a;
	} else if (e == Edge::RIGHT) {
		right = a;
	} else if (e == Edge::BOTTOM) {
		bottom = a;
	} else { // e == Edge::COUNT
		left = top = right = bottom = a;
	}
	if (a.getType() != AnchorType::NONE) {
		centreVertical = centreHorizontal = false;
	}
}

void Anchors::setCentre(bool vert, bool horiz) {
	centreVertical = vert;
	if (vert) {
		clear(Edge::TOP);
		clear(Edge::BOTTOM);
	}
	centreHorizontal = horiz;
	if (horiz) {
		clear(Edge::LEFT);
		clear(Edge::RIGHT);
	}
}

ostream& operator<<(ostream &stream, const Anchor &anchor) {
	if (anchor.getType() == AnchorType::NONE) {
		stream << "None";
	} else if (anchor.getType() == AnchorType::RIGID) {
		stream << "Rigid " << anchor.getValue() << "px";
	} else if (anchor.getType() == AnchorType::SPRINGY) {
		stream << "Springy " << anchor.getValue() << "%";
	}
	return stream;
}

ostream& operator<<(ostream &stream, const Anchors &anchors) {
	stream << "(Left: " << anchors[Edge::LEFT] << ", Top: " << anchors[Edge::TOP]
		<< ", Right: " << anchors[Edge::RIGHT] << ", Bottom: " << anchors[Edge::BOTTOM] << ")";
	return stream;
}

// =====================================================
// class Widget
// =====================================================

int  Widget_idCounter = 0;

MEMORY_CHECK_IMPLEMENTATION(Widget)

Widget::Widget(Container* parent)
		: m_parent(parent) {
	m_id = Widget_idCounter++;
	WIDGET_LOG( descId() << " : Widget::Widget( parent:" << m_parent->getId() << " )" );
	m_rootWindow = m_parent->getRootWindow();
	m_parent->addChild(this);
	init(Vec2i(0), Vec2i(0));
}

Widget::Widget(Container* parent, Vec2i pos, Vec2i size)
		: m_parent(parent) {
	m_id = Widget_idCounter++;
	WIDGET_LOG( descId() << " : Widget::Widget( parent:" << m_parent->getId() << ", "
		<< pos << ", " << size << " )" );
	m_screenPos = m_parent->getScreenPos() + pos;
	m_rootWindow = m_parent->getRootWindow();
	m_parent->addChild(this);
	init(pos, size);
}

Widget::Widget(WidgetWindow* window)
		: m_parent(window)
		, m_screenPos(0) {
	m_id = Widget_idCounter++;
	WIDGET_LOG( descId() << " : Widget::Widget( window:" << m_parent->getId() << " )" );
	m_rootWindow = window;
	init(Vec2i(0), Vec2i(0));
}

Widget::~Widget() {
	WIDGET_LOG( descId() << " : Widget::~Widget()" );
	Destroyed(this);
	if (m_rootWindow != this) {
		m_parent->remChild(this);
	}
}

void Widget::init(const Vec2i &pos, const Vec2i &size) {
	m_hover = false;
	m_focus = false;
	m_visible = true;
	m_selected = false;
	m_enabled = true;
	m_permanent = false;
	m_pos = pos;
	m_size = size;
	if (m_rootWindow != this) {
		m_screenPos = m_parent->getScreenPos() + m_pos;
	} else {
		assert(m_pos == Vec2i(0));
		m_screenPos = Vec2i(0);
	}

	m_fade = 1.f;

	m_mouseWidget = 0;
	m_keyboardWidget = 0;
	m_textWidget = 0;
	m_cell = 0;
}

string Widget::descState() {
	if (!m_enabled) { // disabled ?
		return "disabled";
	} else if (m_selected) { // else priority to selected flag
		return "selected";
	} else if (m_focus) { // followed by focus flag
		return "focus";
	} else if (m_hover) { // then hover
		return "hover";
	} else { // or normal
		return "normal";
	}
}

void Widget::setWidgetStyle(WidgetType type) {
	if (!m_enabled) { // disabled ?
		setStyle(g_widgetConfig.getWidgetStyle(type, WidgetState::DISABLED));
	} else if (m_selected) { // else priority to selected flag
		setStyle(g_widgetConfig.getWidgetStyle(type, WidgetState::SELECTED));
	} else if (m_focus) { // followed by focus flag
		setStyle(g_widgetConfig.getWidgetStyle(type, WidgetState::FOCUS));
	} else if (m_hover) { // then hover
		setStyle(g_widgetConfig.getWidgetStyle(type, WidgetState::HOVER));
	} else { // or normal
		setStyle(g_widgetConfig.getWidgetStyle(type, WidgetState::NORMAL));
	}
}

void Widget::setHover(bool v) {
	m_hover = v;
	setStyle();
}

void Widget::setFocus(bool v) {
	m_focus = v;
	setStyle();
}

void Widget::setEnabled(bool v) {
	m_enabled = v;
	setStyle();
}

void Widget::setSelected(bool v) {
	m_selected = v;
	setStyle();
}

Widget* Widget::getWidgetAt(const Vec2i &pos) {
	assert(isInside(pos));
	return this;
}

FontPtr Widget::getFont(int ndx) const {
	RUNTIME_CHECK(ndx >= 0);
	return m_rootWindow->getConfig()->getFont(ndx);
}

TexPtr Widget::getTexture(int ndx) const {
	RUNTIME_CHECK(ndx >= 0);
	return m_rootWindow->getConfig()->getTexture(ndx);
}

Colour  Widget::getColour(int ndx) const {
	RUNTIME_CHECK(ndx >= 0);
	return m_rootWindow->getConfig()->getColour(ndx);
}

string Widget::descId() {
	std::stringstream ss;
	ss << "[Widget Id: " << m_id << "]";
	return ss.str();
}

string Widget::descShort() {
	std::stringstream ss;
	ss << "[" << descType() <<  " Id: " << m_id << "]";
	return ss.str();
}

string Widget::descLong() {
	std::stringstream ss;
	ss << "[" << descType() <<  " Id: " << m_id << ", Pos: " << m_screenPos << ", Size: " << m_size << "]";
	return ss.str();
}

void Widget::setSize(const Vec2i &sz) {
	//WIDGET_LOG( descShort() << " : Widget::setSize( " << sz << " )" );
	m_size = sz;
	if (m_textWidget) {
		m_textWidget->widgetReSized();
	}
	//WIDGET_LOG( descLong() );
}

void Widget::setPos(const Vec2i &p) {
	m_pos = p;
	m_screenPos = m_parent->getScreenPos() + m_pos;
	//WIDGET_LOG( descShort() << " : Widget::setPos( " << p << " ) => ScreenPos now: " << m_screenPos );
}

bool Widget::isInsideBorders(const Vec2i &pos) const {
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (pos.x >= myPos.x + getBorderLeft() && pos.y >= myPos.y + getBorderTop()
	&& pos.x < myPos.x + mySize.x - getBorderRight()
	&& pos.y < myPos.y + mySize.y - getBorderBottom()) {
		return true;
	}
	return false;
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

void Widget::renderOverlay(int ndx, Vec2i pos, Vec2i size) {
	assert(glIsEnabled(GL_BLEND));
	const Texture2D *tex = m_rootWindow->getConfig()->getTexture(ndx);
	if (!tex) {
		assert(false);
		return;
	}
	Vec2i verts[4];
	verts[0] = m_screenPos + pos;
	verts[1] = verts[0] + Vec2i(size.x, 0);
	verts[2] = verts[0] + size;
	verts[3] = verts[0] + Vec2i(0, size.y);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(tex)->getHandle());
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.f, 1.f, 1.f, getFade());
		glTexCoord2i(0, 1);
		glVertex2iv(verts[0].ptr());
		glTexCoord2i(1, 1);
		glVertex2iv(verts[1].ptr());
		glTexCoord2i(1, 0);
		glVertex2iv(verts[2].ptr());
		glTexCoord2i(0, 0);
		glVertex2iv(verts[3].ptr());
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}

Vec2f invertTexCoord(Vec2f in) {
	Vec2f res(in);
	res.y = 1.f - in.y;
	return res;
}

void buildArrayQuad(Vec2f pos, Vec2f sz, Vec2f uv, Vec2f step, Vec2f *posBase, Vec2f *uvBase) {
	posBase[0] = Vec2f(pos.x, pos.y);
	uvBase[0] = invertTexCoord(Vec2f(uv.u, uv.v));
	posBase[1] = Vec2f(pos.x + sz.w, pos.y);
	uvBase[1] = invertTexCoord(Vec2f(uv.u + step.u, uv.v));
	posBase[2] = Vec2f(pos.x + sz.w, pos.y + sz.h);
	uvBase[2] = invertTexCoord(Vec2f(uv.u + step.u, uv.v + step.v));
	posBase[3] = Vec2f(pos.x, pos.y + sz.h);
	uvBase[3] = invertTexCoord(Vec2f(uv.u, uv.v + step.v));
}

void Widget::renderBordersFromTexture(const BorderStyle &style, const Vec2i &offset, const Vec2i &size) {
	const int &borderSize = style.m_sizes[0];
	const int &cornerSize = style.m_cornerSize;
	const int &imgNdx = style.m_imageNdx;

	if (style.m_cornerSize * 2 > size.w || style.m_cornerSize * 2 > size.h) {
		return;
	}

	assert(imgNdx >= 0);
	const Texture2DGl *tex = static_cast<const Texture2DGl*>(m_rootWindow->getConfig()->getTexture(imgNdx));
	const Vec2f uvStepBorder(borderSize / float(tex->getPixmap()->getW()),
		borderSize / float(tex->getPixmap()->getH()));
	const Vec2f uvStepCorner(cornerSize / float(tex->getPixmap()->getW()),
		cornerSize / float(tex->getPixmap()->getH()));

	// assert sanity
	assert(glIsEnabled(GL_BLEND));
	assert(glIsTexture(tex->getHandle()));

	// 8 quads
	Vec2f verts[32];
	Vec2f uvCoords[32];

	Vec2f pos = Vec2f(getScreenPos() + offset);
	Vec2f sz = Vec2f(size);

	// 4 corners,
	Vec2f cnrSize;
	cnrSize.x = cnrSize.y = float(cornerSize);
	buildArrayQuad(pos, cnrSize, Vec2f(0.f), uvStepCorner, &verts[0], &uvCoords[0]);
	buildArrayQuad(pos + Vec2f(sz.w - float(cornerSize), 0.f),
		cnrSize, Vec2f(1.f - uvStepCorner.u, 0.f), uvStepCorner, &verts[4], &uvCoords[4]);
	buildArrayQuad(pos + Vec2f(sz.w - float(cornerSize), sz.h - float(cornerSize)),
		cnrSize, Vec2f(1.f) - uvStepCorner, uvStepCorner, &verts[8], &uvCoords[8]);
	buildArrayQuad(pos + Vec2f(0.f, sz.h - float(cornerSize)),
		cnrSize, Vec2f(0.f, 1.f - uvStepCorner.v), uvStepCorner, &verts[12], &uvCoords[12]);

	// and 4 sides,
	// top & bottom
	Vec2f tbSize(sz.w - float(cornerSize) * 2.f, float(borderSize));
	Vec2f tUvOffset(uvStepCorner.u, 0.f);
	Vec2f bUvOffset(uvStepCorner.u, 1.f - uvStepBorder.v);
	Vec2f tbUvStep(1.f - 2.f * uvStepCorner.u, uvStepBorder.v);
	buildArrayQuad(pos + Vec2f(float(cornerSize), 0.f), tbSize, tUvOffset, tbUvStep, &verts[16], &uvCoords[16]);
	buildArrayQuad(pos + Vec2f(float(cornerSize), sz.h - float(borderSize)), tbSize, bUvOffset, tbUvStep, &verts[20], &uvCoords[20]);

	// left & right
	Vec2f lrSize(float(borderSize), sz.h - float(cornerSize) * 2.f);
	Vec2f lUvOffset(0.f, uvStepCorner.v);
	Vec2f rUvOffset(1.f - uvStepBorder.u, uvStepCorner.v);
	Vec2f lrUvStep(uvStepBorder.u, 1.f - 2.f * uvStepCorner.v);
	buildArrayQuad(pos + Vec2f(0.f, float(cornerSize)), lrSize, lUvOffset, lrUvStep, &verts[24], &uvCoords[24]);
	buildArrayQuad(pos + Vec2f(sz.w - float(borderSize), float(cornerSize)), lrSize, rUvOffset, lrUvStep, &verts[28], &uvCoords[28]);

	static GLushort indices[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
		12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
		22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	};

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex->getHandle());
	assertGl();

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, &verts[0]);
	glTexCoordPointer(2, GL_FLOAT, 0, &uvCoords[0]);
	assertGl();
	glColor4f(1.f, 1.f, 1.f, m_fade);

	glDrawElements(GL_QUADS, 32, GL_UNSIGNED_SHORT, indices);
	assertGl();

	// restore GL state
	glPopClientAttrib();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	assertGl();
}

void Widget::renderBorders(const BorderStyle &style, const Vec2i &offset, const Vec2i &size) {
	assert(glIsEnabled(GL_BLEND));
	assert(style.m_type != BorderType::NONE);

	if (style.m_type == BorderType::INVISIBLE) {
		return;
	}

	if (size.x < style.m_sizes[Border::LEFT] + style.m_sizes[Border::RIGHT]
	|| size.y < style.m_sizes[Border::TOP] + style.m_sizes[Border::BOTTOM]) {
		return;
	}

	if (style.m_type == BorderType::TEXTURE) {
		renderBordersFromTexture(style, offset, size);
		return;
	}

	// O = Outside, I = Inside, L = Left, T = Top, R = Right, B = Bottom
	enum { OBL, OTL, OTR, OBR, IBL, ITL, ITR, IBR };
	Vec2i verts[8];
	verts[OTL] = m_screenPos + offset;
	verts[OBL] = verts[OTL] + Vec2i(0, size.y);
	verts[OBR] = verts[OTL] + size;
	verts[OTR] = verts[OTL] + Vec2i(size.x, 0);

	verts[IBL] = verts[OBL] + Vec2i(style.m_sizes[Border::LEFT], -style.m_sizes[Border::BOTTOM]);
	verts[ITL] = verts[OTL] + Vec2i(style.m_sizes[Border::LEFT], style.m_sizes[Border::TOP]);
	verts[ITR] = verts[OTR] + Vec2i(-style.m_sizes[Border::RIGHT], style.m_sizes[Border::TOP]);
	verts[IBR] = verts[OBR] + Vec2i(-style.m_sizes[Border::RIGHT], -style.m_sizes[Border::BOTTOM]);

	assertGl();

	bool raised = false;

	Colour colour;
	switch (style.m_type) {
		case BorderType::SOLID:
			// alpha ?
			colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[0]), getFade());
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
					m_rootWindow->getConfig()->getColour(style.m_colourIndices[raised ? 0 : 1]), getFade());
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
					m_rootWindow->getConfig()->getColour(style.m_colourIndices[raised ? 1 : 0]), getFade());
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
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[0]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OBL].ptr());
				glVertex2iv(verts[IBL].ptr());
				glVertex2iv(verts[OTL].ptr());
				glVertex2iv(verts[ITL].ptr());
			glEnd();
			glBegin(GL_QUADS);
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[1]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTL].ptr());
				glVertex2iv(verts[ITL].ptr());
				glVertex2iv(verts[OTR].ptr());
				glVertex2iv(verts[ITR].ptr());
			glEnd();
			glBegin(GL_QUADS);
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[2]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTR].ptr());
				glVertex2iv(verts[ITR].ptr());
				glVertex2iv(verts[OBR].ptr());
				glVertex2iv(verts[IBR].ptr());
			glEnd();
			glBegin(GL_QUADS);
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[3]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OBR].ptr());
				glVertex2iv(verts[IBR].ptr());
				glVertex2iv(verts[OBL].ptr());
				glVertex2iv(verts[IBL].ptr());
			glEnd();

			break;

		case BorderType::CUSTOM_CORNERS:
			glBegin(GL_QUAD_STRIP);
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[0]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTL].ptr());
				glVertex2iv(verts[ITL].ptr());
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[1]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTR].ptr());
				glVertex2iv(verts[ITR].ptr());
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[2]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OBR].ptr());
				glVertex2iv(verts[IBR].ptr());
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[3]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OBL].ptr());
				glVertex2iv(verts[IBL].ptr());
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[0]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[OTL].ptr());
				glVertex2iv(verts[ITL].ptr());
			glEnd();

			break;
	}
	assertGl();
}

void Widget::renderBackground(const BackgroundStyle &style, const Vec2i &offset, const Vec2i &size) {
	assert(glIsEnabled(GL_BLEND));
	assert(style.m_type != BackgroundType::NONE);

	Vec2i verts[4];
	verts[0] = m_screenPos + offset;
	verts[1] = verts[0] + Vec2i(size.x, 0);
	verts[2] = verts[0] + size;
	verts[3] = verts[0] + Vec2i(0, size.y);

	assertGl();

	Colour colour;
	switch (style.m_type) {
		case BackgroundType::COLOUR:
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_TRIANGLE_FAN);
				// alpha ?
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[0]), getFade());
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
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[0]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[0].ptr());
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[1]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[1].ptr());
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[2]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[2].ptr());
				colour = calcColour(m_rootWindow->getConfig()->getColour(style.m_colourIndices[3]), getFade());
				glColor4ubv(colour.ptr());
				glVertex2iv(verts[3].ptr());
			glEnd();

			break;

		case BackgroundType::TEXTURE:
			glEnable(GL_TEXTURE_2D);
			const Texture2D *tex = m_rootWindow->getConfig()->getTexture(style.m_imageIndex);
			glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(tex)->getHandle());
			glBegin(GL_TRIANGLE_FAN);
			glColor4f(1.f, 1.f, 1.f, getFade());
				glTexCoord2i(0, 1);
				glVertex2iv(verts[0].ptr());
				glTexCoord2i(1, 1);
				glVertex2iv(verts[1].ptr());
				glTexCoord2i(1, 0);
				glVertex2iv(verts[2].ptr());
				glTexCoord2i(0, 0);
				glVertex2iv(verts[3].ptr());
			glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);

			break;
	}
	assertGl();
}

void Widget::renderBackground() {
	if (m_backgroundStyle.m_type) {
		Vec2i pos, size;
		if (m_backgroundStyle.m_insideBorders) {
			pos = Vec2i(getBorderLeft(), getBorderTop());
			size = m_size - getBordersAll();
		} else {
			pos = Vec2i(0);
			size = m_size;
		}
		renderBackground(m_backgroundStyle, pos, size);
	}
	if (m_borderStyle.m_type) {
		renderBorders(m_borderStyle, Vec2i(0), m_size);
	}
}

void Widget::renderForeground() {
	if (m_overlayStyle.m_tex != -1) {
		Vec2i pos, size;
		if (m_overlayStyle.m_insideBorders) {
			pos = Vec2i(getBorderLeft(), getBorderTop());
			size = m_size - getBordersAll();
		} else {
			pos = Vec2i(0);
			size = m_size;
		}
		renderOverlay(m_overlayStyle.m_tex, pos, size);
	}
	if (m_highlightStyle.m_type == HighLightType::FIXED) {
		renderHighLight(m_highlightStyle.m_colourIndex, 0.8f, 0.35f);
	} else if (m_highlightStyle.m_type == HighLightType::OSCILLATE) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;
		renderHighLight(m_highlightStyle.m_colourIndex, centreAlpha, borderAlpha);
	}
}

void Widget::renderHighLight(int colour, float centreAlpha, float borderAlpha, Vec2i offset, Vec2i size) {
	assert(glIsEnabled(GL_BLEND));
	Colour c = m_rootWindow->getConfig()->getColour(colour);
	const Colour borderColour = Colour(c.r, c.g, c.b, clamp(unsigned(c.a * borderAlpha * m_fade), 0u, 255u));
	const Colour centreColour = Colour(c.r, c.g, c.b, clamp(unsigned(c.a * centreAlpha * m_fade), 0u, 255u));
	float x1 = float(m_screenPos.x + offset.x);
	float y1 = float(m_screenPos.y + offset.y);
	float x2 = x1 + float(size.x);
	float y2 = y1 + float(size.y);
	
	assertGl();
	glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLE_FAN);
		// centre
		glColor4ubv(centreColour.ptr());
		glVertex2f((x1 + x2) / 2.f, (y1 + y2) / 2.f);
		// corners
		glColor4ubv(borderColour.ptr());
		glVertex2f(x1, y1);
		glVertex2f(x2, y1);
		glVertex2f(x2, y2);
		glVertex2f(x1, y2);
		glVertex2f(x1, y1);
	glEnd();
	assertGl();
}

void Widget::renderHighLight(int colour, float centreAlpha, float borderAlpha) {
	Vec2i offset(m_borderStyle.m_sizes[Border::LEFT], m_borderStyle.m_sizes[Border::BOTTOM]);
	Vec2i inset(m_borderStyle.m_sizes[Border::RIGHT], m_borderStyle.m_sizes[Border::TOP]);
	Vec2i sz = Vec2i(m_size) - offset - inset;
	renderHighLight(colour, centreAlpha, borderAlpha, offset, sz);
}

void Widget::render() {
	renderBackground();
	renderForeground();
}

void Widget::anchor() {
	// cell pos and size
	Rect2i cellArea = m_parent->getCellArea(m_cell);
	Vec2i  cellPos = cellArea.p[0],
		   cellSize = cellArea.p[1] - cellArea.p[0];

	if (cellSize == Vec2i(0)) {
		//WIDGET_LOG( descLong() << " : Widget::anchor() ... aborting (cellSize == 0,0)" );
		return;
	}
	WIDGET_LOG( descLong() << " : Widget::anchor()" );

	// result pos & size
	Vec2i pos = this->getPos();//cellPos;
	Vec2i size = this->getSize();

	// flags (so we don't apply anchors after centering)
	bool anchorVertical = true,
		 anchorHorizontal = true;

	// 1. apply centre anchors
	if (m_anchors.isCentreHorizontal() || m_anchors.isCentreVertical()) {
		Vec2i offset = (cellSize - m_size) / 2;
		if (m_anchors.isCentreHorizontal()) {
			pos.x = offset.x;
			anchorHorizontal = false;
		}
		if (m_anchors.isCentreVertical()) {
			pos.y = offset.y;
			anchorVertical = false;
		}
	}

	// 2. convert percentages to absolute values
	int absolute[Edge::COUNT];
	foreach_enum (Edge, e) {
		if (m_anchors[e].isRigid()) {
			absolute[e] = m_anchors[e].getValue();
		} else if (m_anchors[e].isSpringy()) {
			int avail = (e == Edge::LEFT || e == Edge::RIGHT) ? cellSize.w : cellSize.h;
			absolute[e] = int(float(avail) * (m_anchors[e].getValue() / 100.f));
		} else {
			absolute[e] = -1;
		}
	}

	// 3. apply absolute anchors (if not centering)
	if (anchorHorizontal) {
		if (absolute[Edge::LEFT] != -1) { // anchor left
			pos.x = absolute[Edge::LEFT];
			if (absolute[Edge::RIGHT] != -1) { // stretch horizontally
				size.w = cellSize.w - absolute[Edge::LEFT] - absolute[Edge::RIGHT];
			}
		} else if (absolute[Edge::RIGHT] != -1) { // anchor right
			pos.x = cellSize.w - absolute[Edge::RIGHT] - size.w;
		}
	}
	if (anchorVertical) {
		if (absolute[Edge::TOP] != -1) { // anchor top
			pos.y = absolute[Edge::TOP];
			if (absolute[Edge::BOTTOM] != -1) { // stretch vertically
				size.h = cellSize.h - absolute[Edge::TOP] - absolute[Edge::BOTTOM];
			}
		} else if (absolute[Edge::BOTTOM] != -1) { // anchor bottom
			pos.y = cellSize.h - absolute[Edge::BOTTOM] - size.h;
		}
	}

	// 4. set position and size
	WIDGET_LOG( descShort() << " : setting pos: " << pos << " & size: " << size );
	this->setPos(cellPos + pos);
	this->setSize(size);
}

// =====================================================
// class MouseWidget
// =====================================================

MouseWidget::MouseWidget(Widget* widget) {
	me = widget;
	me->setMouseWidget(this);
}

KeyboardWidget::KeyboardWidget(Widget* widget) {
	me = widget;
	me->setKeyboardWidget(this);
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
	glEnable(GL_TEXTURE_2D);
	batchRender = true;
}

void ImageWidget::endBatch() {
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	assertGl();
	batchRender = false;
}

void ImageWidget::renderImage(int ndx, const Vec4f &colour) {
	ASSERT_RANGE(ndx, textures.size());
	assertGl();

	if (!batchRender) {
		glEnable(GL_TEXTURE_2D);
	}

	int x1, x2, y1, y2;
	Vec2i pos = me->getScreenPos();
	x1 = pos.x;
	y2 = pos.y;
	if (imageInfo[ndx].hasOffset) {
		x1 += imageInfo[ndx].offset.x;
		y2 += imageInfo[ndx].offset.y;
	}
	if (imageInfo[ndx].hasCustomSize) {
		x2 = x1 + imageInfo[ndx].size.x;
		y1 = y2 + imageInfo[ndx].size.y;
	} else {
		Vec2i sz = me->getSize();
		x2 = x1 + sz.x;
		y1 = y2 + sz.y;
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
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
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
//  struct TextRenderInfo
// =====================================================

TextRenderInfo::TextRenderInfo(const string &txt, int font, int colour, const Vec2i &pos)
		: m_text(txt), m_pos(pos), m_colour(colour), m_font(font), m_fade(1.f) {
	WidgetConfig &cfg = g_widgetConfig;
	Colour c = cfg.getColour(colour);
	Colour black(0u, 0u, 0u, 255u);
	Colour shadow = (c + black * 2u) / 3u;
	m_shadowColour = cfg.getColourIndex(shadow);
	m_shadowColour2 = cfg.getColourIndex(black);
}

// =====================================================
// class TextWidget
// =====================================================

TextWidget::TextWidget(Widget* me)
		: me(me)
		, m_align(Alignment::CENTERED)
		, m_batchRender(false)
		, m_textRenderer(0) {
	me->m_textWidget = this;
}

void TextWidget::setTextColour(const Vec4f &col, int ndx) {
	ASSERT_RANGE(ndx, m_texts.size());
	m_texts[ndx].m_colour = g_widgetConfig.getColourIndex(col);
}

void TextWidget::setTextShadowColour(const Vec4f &col, int ndx) {
	ASSERT_RANGE(ndx, m_texts.size());
	m_texts[ndx].m_shadowColour = g_widgetConfig.getColourIndex(col);
}

void TextWidget::setTextShadowColour2(const Vec4f &col, int ndx) {
	ASSERT_RANGE(ndx, m_texts.size());
	m_texts[ndx].m_shadowColour2 = g_widgetConfig.getColourIndex(col);
}

void TextWidget::setTextShadowOffset(const Vec2i &offset, int ndx) {
	ASSERT_RANGE(ndx, m_texts.size());
	m_texts[ndx].m_shadowOffset = offset;
}


void TextWidget::setTextFade(float alpha, int ndx) {
	m_texts[ndx].m_fade = alpha;
}

void TextWidget::alignText(int ndx) {
	if (m_texts.empty() || m_align == Alignment::NONE) {
		return;
	}
	ASSERT_RANGE(ndx, m_texts.size());
	Vec2i mySize = me->getSize();
	const FontMetrics *fm;
	if (m_texts[ndx].m_font != -1) {
		fm = g_widgetConfig.getFont(m_texts[ndx].m_font)->getMetrics();
	} else {
		fm = me->getFont()->getMetrics();
	}
	Vec2f txtDims = fm->getTextDiminsions(m_texts[ndx].m_text);

	int y = (mySize.h - int(txtDims.h)) / 2;
	y = y > 0 ? y : 0;
	switch (m_align) {
		case Alignment::CENTERED:
			m_texts[ndx].m_pos = Vec2i((mySize.w - int(txtDims.w)) / 2, y);
			break;
		case Alignment::FLUSH_LEFT:
			m_texts[ndx].m_pos = Vec2i(me->getBorderLeft(), y);
			break;
		case Alignment::FLUSH_RIGHT:
			m_texts[ndx].m_pos = Vec2i(mySize.w - int(txtDims.w) - me->getBorderRight(), y);
			break;
		default:
			throw runtime_error("TextWidget::alignText() bad Alignment in,\n\t" + me->descLong());
	}
}

void TextWidget::widgetReSized() {
	if (m_align != Alignment::NONE) {
		alignText();
	}
}

void TextWidget::startBatch(const Font *font) {
	if (!font) {
		font = me->getFont(me->textStyle().m_fontIndex);
	}
	m_textRenderer = g_widgetWindow.getTextRenderer();
	m_textRenderer->begin(font);
	m_batchRender = true;
}

void TextWidget::endBatch() {
	m_textRenderer->end();
	m_batchRender = false;
}

void TextWidget::renderText(const string &txt, int x, int y, const Colour &colour, const Font *font) {
	assertGl();
	if (!m_batchRender) {
		if (!font) {
			font = me->getFont(me->textStyle().m_fontIndex);
		}
		m_textRenderer = g_widgetWindow.getTextRenderer();
		m_textRenderer->begin(font);
	} 
	glColor4ubv(colour.ptr());
	m_textRenderer->render(txt, x, y + int(font->getMetrics()->getMaxAscent()));
	if (!m_batchRender) {
		m_textRenderer->end();
	}
	assertGl();
}

void TextWidget::renderText(const string &txt, int x, int y, const Colour &colour, int fontNdx) {
	const Font *font = g_widgetConfig.getFont(fontNdx);
	renderText(txt, x, y, colour, font);
}

void TextWidget::renderText(int ndx) {
	ASSERT_RANGE(ndx, m_texts.size());
	Vec2i pos = me->getScreenPos() + m_texts[ndx].m_pos;
	Colour colour = g_widgetConfig.getColour(m_texts[ndx].m_colour);
	colour.a = uint8(clamp(unsigned(colour.a * me->getFade() * m_texts[ndx].m_fade), 0u, 255u));
	renderText(m_texts[ndx].m_text, pos.x, pos.y, colour, m_texts[ndx].m_font);
}

void TextWidget::renderTextShadowed(int ndx) {
	ASSERT_RANGE(ndx, m_texts.size());
	WidgetConfig &cfg = g_widgetConfig;
	Vec2i pos = me->getScreenPos() + m_texts[ndx].m_pos;
	Vec2i sPos = pos + m_texts[ndx].m_shadowOffset;
	Colour colour = cfg.getColour(m_texts[ndx].m_colour);
	Colour shadowColour = cfg.getColour(m_texts[ndx].m_shadowColour);
	colour.a = uint8(clamp(unsigned(colour.a * me->getFade() * m_texts[ndx].m_fade), 0u, 255u));
	shadowColour.a = uint8(clamp(unsigned(shadowColour.a * me->getFade() * m_texts[ndx].m_fade), 0u, 255u));
	renderText(m_texts[ndx].m_text, sPos.x, sPos.y, shadowColour);
	renderText(m_texts[ndx].m_text, pos.x, pos.y, colour);
}

void TextWidget::renderTextDoubleShadowed(int ndx) {
	ASSERT_RANGE(ndx, m_texts.size());
	WidgetConfig &cfg = g_widgetConfig;
	Vec2i pos = me->getScreenPos() + m_texts[ndx].m_pos;
	Vec2i sPos1 = pos + m_texts[ndx].m_shadowOffset;
	Vec2i sPos2 = sPos1 + m_texts[ndx].m_shadowOffset;
	Colour colour = cfg.getColour(m_texts[ndx].m_colour);
	Colour shadowColour = cfg.getColour(m_texts[ndx].m_shadowColour);
	Colour shadowColour2 = cfg.getColour(m_texts[ndx].m_shadowColour2);
	colour.a = uint8(clamp(unsigned(colour.a * me->getFade() * m_texts[ndx].m_fade), 0u, 255u));
	shadowColour.a = uint8(clamp(unsigned(shadowColour.a * me->getFade() * m_texts[ndx].m_fade), 0u, 255u));
	shadowColour2.a = uint8(clamp(unsigned(shadowColour2.a * me->getFade() * m_texts[ndx].m_fade), 0u, 255u));
	renderText(m_texts[ndx].m_text, sPos2.x, sPos2.y, shadowColour2);
	renderText(m_texts[ndx].m_text, sPos1.x, sPos1.y, shadowColour);
	renderText(m_texts[ndx].m_text, pos.x, pos.y, colour);
}

Vec2i TextWidget::getTextDimensions() const {
	WidgetConfig &cfg = g_widgetConfig;
	Vec2i max = Vec2i(0);
	foreach_const (vector<TextRenderInfo>, it, m_texts) {
		const FontMetrics *fm = cfg.getFont(it->m_font)->getMetrics();
		Vec2i tSz = Vec2i(fm->getTextDiminsions(it->m_text) + Vec2f(1.f));
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
	ASSERT_RANGE(ndx, m_texts.size());
	const FontMetrics *fm = g_widgetConfig.getFont(m_texts[ndx].m_font)->getMetrics();
	return Vec2i(fm->getTextDiminsions(m_texts[ndx].m_text) + Vec2f(1.f));
}

//void TextWidget::setTextParams(const string &txt, int colour, int font, bool cntr) {
//	if (m_texts.empty()) {
//		m_texts.push_back(TextRenderInfo(txt, font, colour, Vec2i(0)));
//	} else {
//		m_texts[0] = TextRenderInfo(txt, font, colour, Vec2i(0));
//	}
//	m_centreText = cntr;
//	if (m_centreText) {
//		centreText();
//	}
//}

 int TextWidget::addText(const string &txt) {
	TextStyle &style = me->textStyle();
	m_texts.push_back(TextRenderInfo(txt, style.m_fontIndex, style.m_colourIndex, Vec2i(0)));
	return m_texts.size() - 1;
}

void TextWidget::setText(const string &txt, int ndx) {
	if (m_texts.empty() && !ndx) {
		TextStyle &style = me->textStyle();
		m_texts.push_back(TextRenderInfo(txt, style.m_fontIndex, style.m_colourIndex, Vec2i(0)));
	} else {
		ASSERT_RANGE(ndx, m_texts.size());
		m_texts[ndx].m_text = txt;
	}
	if (m_align != Alignment::NONE) {
		alignText(ndx);
	}
}

//void TextWidget::setTextFont(const Font *f) {
//	m_defaultFont = f;
//}

void TextWidget::setTextPos(const Vec2i &pos, int ndx) {
	ASSERT_RANGE(ndx, m_texts.size());
	m_texts[ndx].m_pos = pos;
}

// =====================================================
// class MouseCursor
// =====================================================

void MouseCursor::renderTex(const Texture2D *tex) {
	Vec2i pos = getScreenPos() + Vec2i(10, 10);
	int x1 = pos.x + 1;
	int y1 = pos.y + 1;
	int x2 = x1 + 32;
	int y2 = y1 + 32;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(tex)->getHandle());
	glColor4f(1.f, 1.f, 1.f, 0.5f);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2i(0, 0);
		glVertex2i(x1, y2);
		glTexCoord2i(0, 1);
		glVertex2i(x1, y1);
		glTexCoord2i(1, 0);
		glVertex2i(x2, y2);
		glTexCoord2i(1, 1);
		glVertex2i(x2, y1);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
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
	//WIDGET_LOG( descShort() << " : Container::addChild( child:" << child->getId() << " )" );
	assert(std::find(m_children.begin(), m_children.end(), child) == m_children.end());
	m_children.push_back(child);
	child->setFade(getFade());
}

void Container::remChild(Widget* child) {
	//WIDGET_LOG( descId() <<  " : Container::remChild( child:" << child->getId() << " )" );
	WidgetList::iterator it = std::find(m_children.begin(), m_children.end(), child);
	if (it != m_children.end()) {
		m_children.erase(it);
		//WIDGET_LOG( "[Widget Id: " << m_id << " : child:" << child->getId() << " removed." );
	}/* else {
		WIDGET_LOG( descShort() << " : child:" << child->getId() << " not found!" );
	}*/
}

void Container::delChild(Widget* child) {
	RUNTIME_CHECK(std::find(m_children.begin(), m_children.end(), child) != m_children.end());
	//WIDGET_LOG( descId() << " : Container::delChild( child:" << child->getId() << " )" );
	delete child;
}

void Container::clear() {
	WIDGET_LOG( descId() << " : Container::clear()" );
	deleteValues(m_children);
	assert(m_children.empty());
}

void Container::setPos(const Vec2i &p) {
	Widget::setPos(p);
	foreach (WidgetList, it, m_children) {
		(*it)->setPos((*it)->getPos());
	}
}

void Container::setEnabled(bool v) {
	Widget::setEnabled(v);
	foreach (WidgetList, it, m_children) {
		(*it)->setEnabled(v);
	}
}

void Container::setFade(float v) {
	Widget::setFade(v);
	foreach (WidgetList, it, m_children) {
		(*it)->setFade(v);
	}
}

Widget* Container::getWidgetAt(const Vec2i &pos) {
	foreach_rev (WidgetList, it, m_children) {
		Widget* widget = *it;
		if (widget->isVisible() && widget->isInside(pos)) {
			return widget->getWidgetAt(pos);
		}
	}
	return this;
}

void Container::render() {
	assertGl();
	Widget::render();
	foreach (WidgetList, it, m_children) {
		Widget* widget = *it;
		if (widget->isVisible()) {
			widget->render();
		}
	}
	assertGl();
}

// =====================================================
// class WidgetCell
// =====================================================

//WidgetCell::WidgetCell(Container *parent, Vec2i pos, Vec2i size)
//		: Container(parent, pos, size) {
//}
//
//WidgetCell::WidgetCell(Container *parent) : Container(parent) {
//}
//
//void WidgetCell::anchorWidgets() {
//	if (getSize() == Vec2i(0)) {
//		//WIDGET_LOG( descLong() << " : WidgetCell::anchorWidgets() ... aborting (Size == 0,0)" );
//		return;
//	}
//	WIDGET_LOG( descLong() << " : WidgetCell::anchorWidgets()" );
//	foreach (WidgetList, it, m_children) {
//		Widget *child = *it;
//		Anchors anchors = child->getAnchors();
//		WIDGET_LOG( descShort() << " : anchoring child " << child->descShort()
//			<< " with anchors: " << anchors );
//		if (anchors.isCentreHorizontal() && anchors.isCentreVertical()) {
//			Vec2i offset = (getSize() - child->getSize()) / 2;
//			child->setPos(offset);
//			continue;
//		}
//		///@todo FIX ... for only centred one way
//
//		Vec2i pos = getPos();
//		Vec2i cellSize = getSize();
//		Vec2i size = child->getSize();
//
//		// 1. convert percentages to absolute values
//		int absolute[Edge::COUNT];
//		foreach_enum (Edge, e) {
//			if (anchors[e].isRigid()) {
//				absolute[e] = anchors[e].getValue();
//			} else if (anchors[e].isSpringy()) {
//				int avail;
//				if (e == Edge::LEFT || e == Edge::RIGHT) {
//					avail = cellSize.w;
//				} else {
//					avail = cellSize.h;
//				}
//				absolute[e] = int(float(avail) * (anchors[e].getValue() / 100.f));
//			} else {
//				absolute[e] = -1;
//			}
//		}
//
//		// 2. apply absolute anchors
//		if (absolute[Edge::LEFT] != -1) { // anchor left
//			pos.x = absolute[Edge::LEFT];
//			if (absolute[Edge::RIGHT] != -1) { // stretch horizontally
//				size.w = cellSize.w - absolute[Edge::LEFT] - absolute[Edge::RIGHT];
//			}
//		} else if (absolute[Edge::RIGHT] != -1) { // anchor right
//			pos.x = cellSize.w - absolute[Edge::RIGHT] - size.w;
//		}
//		if (absolute[Edge::TOP] != -1) { // anchor top
//			pos.y = absolute[Edge::TOP];
//			if (absolute[Edge::BOTTOM] != -1) { // stretch vertically
//				size.h = cellSize.h - absolute[Edge::TOP] - absolute[Edge::BOTTOM];
//			}
//		} else if (absolute[Edge::BOTTOM] != -1) { // anchor bottom
//			pos.y = cellSize.h - absolute[Edge::BOTTOM] - size.h;
//		}
//		WIDGET_LOG( descShort() << " : setting child " << child->descShort()
//			<< " to pos: " << pos << " & size: " << size);
//		child->setPos(pos);
//		child->setSize(size);
//	}
//}

}}

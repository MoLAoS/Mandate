// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011	Nathan Turner <hailstone3, sf.net>
//						James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "mouse_cursor.h"
#include "widget_window.h"
#include "renderer.h"
#include "core_data.h"
#include "platform_util.h"

namespace Glest { namespace Widgets {

// =====================================================
// class CodeMouseCursor
// =====================================================

OSMouseCursor::OSMouseCursor(WidgetWindow *window) 
		: MouseCursor(window) {
	showCursor(true);
}

// =====================================================
// class CodeMouseCursor
// =====================================================

CodeMouseCursor::CodeMouseCursor(WidgetWindow *window)
		: MouseCursor(window)
		, m_app(MouseAppearance::DEFAULT), m_tex(0) {
	DEBUG_HOOK();
	showCursor(false);
}

CodeMouseCursor::~CodeMouseCursor() {
	DEBUG_HOOK();
}

void CodeMouseCursor::setAppearance(MouseAppearance ma, const Texture2D *tex) {
	if (ma == MouseAppearance::CMD_ICON) {
		RUNTIME_CHECK(tex != 0);
		m_tex = tex;
		m_app = MouseAppearance::CMD_ICON;
	} else {
		m_tex = 0;
		m_app = MouseAppearance::DEFAULT;
	}
}

void CodeMouseCursor::render() {
	if (!Widget::isVisible()) {
		return;
	}

	float color1, color2;
	Vec2i points[4];
	points[0] = getScreenPos();
	points[1] = points[0] + Vec2i(20, 10);
	points[2] = points[0] + Vec2i(10, 20);

	int numPoints;
	if (m_tex) {
		numPoints = 4;
		points[3] = points[2];
		points[2] = points[0] + Vec2i(10, 10);
	} else {
		numPoints = 3;
	}

	int mAnim = int(getRootWindow()->getSlowAnim() * 200) - 100;
	color2 = float(abs(mAnim)) / 100.f / 2.f + 0.4f;
	color1 = float(abs(mAnim)) / 100.f / 2.f + 0.8f;

	glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	// inside
	glColor4f(0.4f, 0.2f, 0.2f, 0.5f);
	glBegin(GL_TRIANGLE_FAN);
	for (int i=0; i < numPoints; ++i) {
		glVertex2iv(points[i].ptr());
	}
	glEnd();

	// border
	glLineWidth(2);
	glBegin(GL_LINE_LOOP);
		glColor4f(1.f, 0.2f, 0, color1);
		glVertex2iv(points[0].ptr());
		glColor4f(1.f, 0.4f, 0, color2);
		for (int i=1; i < numPoints; ++i) {
			glVertex2iv(points[i].ptr());
		}
	glEnd();

	// command icon ?
	if (m_tex) {
		MouseCursor::renderTex(m_tex);
	}
	glPopAttrib();
}

// =====================================================
// class ImageSetMouseCursor
// =====================================================

ImageSetMouseCursor::ImageSetMouseCursor(WidgetWindow *window)
		: MouseCursor(window)
		, m_app(MouseAppearance::DEFAULT)
		, m_icon(0), m_mouseMain(0), m_mouseTexture(0) {
	DEBUG_HOOK();
	showCursor(false);
}

ImageSetMouseCursor::~ImageSetMouseCursor() {
	DEBUG_HOOK();
	delete m_mouseMain;
}

void ImageSetMouseCursor::setAppearance(MouseAppearance ma, const Texture2D *tex) {
	RUNTIME_CHECK(ma < MouseAppearance::COUNT);

	if (ma == MouseAppearance::CMD_ICON) {
		RUNTIME_CHECK(tex != 0);
		m_icon = tex;
		m_app = MouseAppearance::CMD_ICON;
	} else {
		m_icon = 0;
		m_app = ma;
	}
	m_mouseMain->setActive(m_app);
}

void ImageSetMouseCursor::render() {
	if (!Widget::isVisible()) {
		return;
	}

	float color1, color2;
	Vec2i points[4];
	points[0] = getScreenPos();
	points[1] = points[0] + Vec2i(20, 10);
	points[2] = points[0] + Vec2i(10, 20);

	int numPoints;
	if (m_icon) {
		numPoints = 4;
		points[3] = points[2];
		points[2] = points[0] + Vec2i(10, 10);
	} else {
		numPoints = 3;
	}

	int mAnim = int(getRootWindow()->getSlowAnim() * 200) - 100;
	color2 = float(abs(mAnim)) / 100.f / 2.f + 0.4f;
	color1 = float(abs(mAnim)) / 100.f / 2.f + 0.8f;

	glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	// hardcoded if texture doesn't exist
	if (!m_mouseMain) {
		// inside
		glColor4f(0.4f, 0.2f, 0.2f, 0.5f);
		glBegin(GL_TRIANGLE_FAN);
		for (int i=0; i < numPoints; ++i) {
			glVertex2iv(points[i].ptr());
		}
		glEnd();

		// border
		glLineWidth(2);
		glBegin(GL_LINE_LOOP);
			glColor4f(1.f, 0.2f, 0, color1);
			glVertex2iv(points[0].ptr());
			glColor4f(1.f, 0.4f, 0, color2);
			for (int i=1; i < numPoints; ++i) {
				glVertex2iv(points[i].ptr());
			}
		glEnd();
	} else {
		m_mouseMain->setPos(getScreenPos());
		m_mouseMain->setFade(getFade());
		m_mouseMain->render();
	}

	// command icon ?
	if (m_icon) {
		MouseCursor::renderTex(m_icon);
	}
	glPopAttrib();
}

/** Load the mouse texture from dir
  * @return true on success
  */
bool ImageSetMouseCursor::loadMouse(const string &dir) {
	string path;
	const string name = "mouse";

	// check each file type (there must be a better way?)
	if (fileExists(dir + "/" + name + ".png")) {
		path = dir + "/" + name + ".png";
	} else if (fileExists(dir + "/" + name + ".jpg")) {
		path = dir + "/" + name + ".jpg";
	} else if (fileExists(dir + "/" + name + ".bmp")) {
		path = dir + "/" + name + ".bmp";
	} else if (fileExists(dir + "/" + name + ".tga")) {
		path = dir + "/" + name + ".tga";
	} else {
		return false;
	}
	
	Texture2D *tex = g_renderer.newTexture2D(ResourceScope::GLOBAL);
	tex->setMipmap(false);
	tex->load(path);
	try{
		tex->init();
		m_mouseTexture = tex;
	} catch (...) {
	}

	initMouse();

	return true;
}

void ImageSetMouseCursor::initMouse() {
	if (!m_mouseTexture) {
		m_mouseTexture = g_coreData.getMouseTexture();
	}

	delete m_mouseMain;

	m_mouseMain = new Imageset(m_mouseTexture, 32, 32);
	m_mouseMain->setSize(32, 32);
	m_mouseTexture = 0; // custom mouse is only applied once

	//mouseAnimations = new Animset(this, mouseMain, 30);
}

}} // end namesace Glest::Widgets
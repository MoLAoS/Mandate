// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011	Nathan Turner <hailstone3, sf.net>
//						James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_MOUSE_CURSOR_INCLUDED_
#define _GLEST_WIDGETS_MOUSE_CURSOR_INCLUDED_

#include "imageset.h"
#include "widgets_base.h"

namespace Glest { namespace Widgets {

// =====================================================
// class OSMouseCursor
// =====================================================

class OSMouseCursor : public MouseCursor {
public:
	OSMouseCursor(WidgetWindow *window);
	virtual ~OSMouseCursor() {}

	virtual void setAppearance(MouseAppearance ma, const Texture2D *tex = 0) override {};
	virtual void render() override {}
	virtual string descType() const override { return "OSMouseCursor"; }
};

// =====================================================
// class CodeMouseCursor
// =====================================================

class CodeMouseCursor : public MouseCursor {
private:
	MouseAppearance  m_app;
	const Texture2D *m_tex;

public:
	CodeMouseCursor(WidgetWindow *window);
	virtual ~CodeMouseCursor();

	virtual void setAppearance(MouseAppearance ma, const Texture2D *tex = 0) override;

	virtual Vec2i getPrefSize() const override { return Vec2i(32); }
	virtual Vec2i getMinSize() const override { return Vec2i(32); }

	virtual void render() override;
	virtual string descType() const override { return "CodeMouseCursor"; }
};

// =====================================================
// class ImageSetMouseCursor
// =====================================================

using namespace Glest::Graphics;

class ImageSetMouseCursor : public MouseCursor {
private:
	MouseAppearance  m_app;
	const Texture2D *m_icon;
	Imageset *m_mouseMain;
	Texture2D *m_mouseTexture;

public:
	ImageSetMouseCursor(WidgetWindow *window);
	virtual ~ImageSetMouseCursor();

	virtual void setAppearance(MouseAppearance ma, const Texture2D *tex = 0) override;

	virtual Vec2i getPrefSize() const override { return Vec2i(32); }
	virtual Vec2i getMinSize() const override { return Vec2i(32); }

	virtual void render() override;
	virtual string descType() const override { return "ImageSetMouseCursor"; }

	bool loadMouse(const string &dir);
	virtual void initMouse() override;
};

}} // end namesace Glest::Widgets

#endif
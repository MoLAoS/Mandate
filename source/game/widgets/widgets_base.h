// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_WIDGET_BASE_INCLUDED_
#define _GLEST_WIDGETS_WIDGET_BASE_INCLUDED_

#include <string>
#include <vector>
#include <string>

#include "math_util.h"
#include "font.h"
#include "input.h"
#include "texture.h"
#include "text_renderer.h"
#include "sigslot.h"
#include "logger.h"

#include "widget_style.h"

using namespace Shared::Math;
using namespace Shared::Graphics;
using Glest::Util::Logger;

namespace Glest { namespace Widgets {

class Widget;
class Container;
class WidgetWindow;

using std::string;

class MouseWidget;
class KeyboardWidget;
class TextWidget;

// =====================================================
// class Widget
// =====================================================

class Widget {
	friend class WidgetWindow;
	friend class MouseWidget;
	friend class KeyboardWidget;
	friend class TextWidget;

public:
	MEMORY_CHECK_DECLARATIONS(Widget)

private:
	//int id;
	Container* parent;
	WidgetWindow* rootWindow;
	Vec2i	pos,
			screenPos,
			size;
	bool	visible;

	bool	m_enabled;
	float	fade;

	int padding;

	MouseWidget *mouseWidget;
	KeyboardWidget *keyboardWidget;
	TextWidget *textWidget;

protected:
	BorderStyle		m_borderStyle;
	BackgroundStyle m_backgroundStyle;

	int	getBorderLeft() const	{ return m_borderStyle.m_sizes[Border::LEFT]; }
	int	getBorderRight() const	{ return m_borderStyle.m_sizes[Border::RIGHT]; }
	int	getBorderTop() const	{ return m_borderStyle.m_sizes[Border::TOP]; }
	int	getBorderBottom() const { return m_borderStyle.m_sizes[Border::BOTTOM]; }
	int	getBordersHoriz() const {
		return m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT];
	}
	int	getBordersVert() const	{
		return m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM];
	}
	Vec2i getBordersAll() const	{ return Vec2i(getBordersHoriz(), getBordersVert()); }

private:
	void setMouseWidget(MouseWidget *mw) { mouseWidget = mw; }
	void setKeyboardWidget(KeyboardWidget *kw) { keyboardWidget = kw; }

	// as
	MouseWidget * asMouseWidget() const			{ return mouseWidget; }
	KeyboardWidget * asKeyboardWidget() const		{ return keyboardWidget; }

	void init(const Vec2i &pos, const Vec2i &size);

protected:
	Widget(WidgetWindow* window);
	Widget(Container* parent);
	Widget(Container* parent, Vec2i pos, Vec2i size);

public:
	virtual ~Widget();

	// de-virtualise ??
	// get/is
	virtual Vec2i getScreenPos() const { return screenPos; }
	virtual Vec2i getPos() const { return pos; }

	virtual Container* getParent() const { return parent; }
	virtual WidgetWindow* getRootWindow() const { return rootWindow; }
	virtual Widget* getWidgetAt(const Vec2i &pos);

	virtual Vec2i getSize() const		{ return size;		 }
	virtual int   getWidth() const		{ return size.x;	 }
	virtual int   getHeight() const		{ return size.y;	 }
	virtual float getFade() const		{ return fade;		 }
//	virtual int	  getBorderSize() const	{ return borderSize; }
	virtual int	  getPadding() const	{ return padding;	 }

	// layout helpers
	virtual Vec2i getPrefSize() const {return Vec2i(-1); } // may return (-1,-1) to indicate 'as big as possible'
	virtual Vec2i getMinSize() const  {return Vec2i(-1); } // should not return (-1,-1)
	virtual Vec2i getMaxSize() const  {return Vec2i(-1); } // return (-1,-1) to indicate 'no maximum size'

	virtual bool isVisible() const		{ return visible; }
	virtual bool isInside(const Vec2i &pos) const {
		return pos.southEastOf(screenPos) && pos.northWestOf(screenPos + size);
	}

	virtual bool isEnabled() const	{ return m_enabled;	}

	// set
	virtual void setEnabled(bool v) { m_enabled = v;	}
	virtual void setSize(const Vec2i &sz);
	virtual void setPos(const Vec2i &p);
	void setSize(const int x, const int y) { setSize(Vec2i(x,y)); }
	void setPos(const int x, const int y) { setPos(Vec2i(x,y)); }
	virtual void setVisible(bool vis) { visible = vis; }
	virtual void setFade(float v) { fade = v; }
	virtual void setParent(Container* p) { parent = p; }

	void setBorderStyle(const BorderStyle &style);
	void setBackgroundStyle(const BackgroundStyle &style);

	void setPadding(int pad) { padding = pad; }

	virtual void update() {} // must 'register' with WidgetWindow to receive

	virtual void render() = 0;

	void renderBorders(const BorderStyle &style, const Vec2i &offset, const Vec2i &size);
	void renderBackground(const BackgroundStyle &style, const Vec2i &offset, const Vec2i &size);

	//void renderBorders(BorderType type, const Vec2i &offset, const Vec2i &size, int borderSize, bool bg = true);
	void renderBgAndBorders(bool bg = true);
	void renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha, Vec2i offset, Vec2i size);
	void renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha);

	virtual string descPosDim();
	virtual string desc() = 0;

	sigslot::signal<Widget*> Destroyed;

};

class MouseWidget {
	friend class WidgetWindow;
private:
	Widget* me;

public:
	MouseWidget(Widget* widget);
	virtual ~MouseWidget() {}

private:
	virtual bool mouseDown(MouseButton btn, Vec2i pos)			{ return false; }
	virtual bool mouseUp(MouseButton btn, Vec2i pos)			{ return false; }
	virtual bool mouseMove(Vec2i pos)							{ return false; }
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos)	{ return false; }
	virtual bool mouseWheel(Vec2i pos, int z)					{ return false; }

	virtual void mouseIn() {}
	virtual void mouseOut() {}
};

class KeyboardWidget {
	friend class WidgetWindow;

private:
	Widget* me;

public:
	KeyboardWidget(Widget* widget);
	virtual ~KeyboardWidget() {}

private:
	virtual bool keyDown(Key key)	{ return false; }
	virtual bool keyUp(Key key)		{ return false; }
	virtual bool keyPress(char c)	{ return false; }

	virtual void lostKeyboardFocus() {}
};

struct ImageRenderInfo {
	bool hasOffset, hasCustomSize;
	Vec2i offset, size;

	ImageRenderInfo() : hasOffset(false), hasCustomSize(false) {}

	ImageRenderInfo(bool hasOffset, Vec2i offset, bool hasCustomSize, Vec2i size)
		: hasOffset(hasOffset), hasCustomSize(hasCustomSize), offset(offset), size(size) {}
};

// =====================================================
// class ImageWidget
// =====================================================

class ImageWidget {
private:
 	typedef vector<const Texture2D*> Textures;

	Widget* me;
	Textures textures;
	vector<ImageRenderInfo> imageInfo;
	bool batchRender;

protected:
	void renderImage(int ndx = 0);
	void renderImage(int ndx, const Vec4f &colour);

	void startBatch();
	void endBatch();

public:
	ImageWidget(Widget* me);
	ImageWidget(Widget* me, Texture2D *tex);
	virtual ~ImageWidget() {}

	int addImage(const Texture2D *tex);
	void setImage(const Texture2D *tex, int ndx = 0);
	int addImageX(const Texture2D *tex, Vec2i offset, Vec2i sz);
	void setImageX(const Texture2D *tex, int ndx, Vec2i offset, Vec2i sz);

	const Texture2D* getImage(int ndx=0) const {
		ASSERT_RANGE(ndx, textures.size());
		return textures[ndx];
	}

	Vec2i getImagePos(int ndx) const { return imageInfo[ndx].offset; }
	Vec2i getImageSize(int ndx) const { return imageInfo[ndx].size; }
	size_t getNumImages() const { return textures.size(); }

	bool hasImage() const { return !textures.empty(); }
	bool hasImage(int ndx) const { return ndx < textures.size(); }
};

typedef const Font* FontPtr;

struct TextRenderInfo {
	string  m_text;
	Vec2i   m_pos;
	Vec4f   m_colour;
	Vec4f   m_shadowColour;
	Vec4f   m_shadowColour2;
	FontPtr m_font;

	TextRenderInfo(const string &txt, FontPtr font, const Vec4f &colour, const Vec2i &pos)
			: m_text(txt), m_pos(pos), m_colour(colour), m_font(font) {
		m_shadowColour = m_shadowColour2 = m_colour;
	}
};

// =====================================================
// class TextWidget
// =====================================================

class TextWidget {
private:
	Widget* me;
	vector<TextRenderInfo> m_texts;
	Vec4f m_defaultColour;
	Vec4f m_shadowColour;
	Vec4f m_shadowColour2;
	const Font *m_defaultFont;
	bool centre;
	bool m_batchRender;
	TextRenderer *m_textRenderer;

protected:
	void renderText(const string &txt, int x, int y, const Vec4f &colour, const Font *font = 0);
	void renderText(int ndx = 0);
	void renderTextShadowed(int ndx = 0, int offset = 2);
	void renderTextDoubleShadowed(int ndx = 0, int offset = 2);

	void startBatch(const Font *font);
	void endBatch();

public:
	TextWidget(Widget* me);
	virtual ~TextWidget() {}

	// set
	void setCentre(bool val)	{ centre = val; }
	void setTextParams(const string&, const Vec4f, const Font*, bool cntr=true);
	int addText(const string &txt);
	void setText(const string &txt, int ndx = 0);
	void setTextColour(const Vec4f &col) { m_defaultColour = col;	 }
	void setTextShadowColour(const Vec4f &col) { m_shadowColour = col;	 }
	void setTextShadowColour2(const Vec4f &col) { m_shadowColour2 = col;	 }
	void setTextShadowColours(const Vec4f &col1, const Vec4f &col2) {
		m_shadowColour = col1;
		m_shadowColour2 = col2;
	}
	void setTextCentre(bool v)	{ centre = v; }
	void setTextPos(const Vec2i &pos, int ndx=0);
	void setTextFont(const Font *f);

	void centreText(int ndx = 0);
	void widgetReSized();

	// get
	const string& getText(int ndx=0) const	{ ASSERT_RANGE(ndx, m_texts.size()); return m_texts[ndx].m_text; }
	const Vec4f& getTextColour() const	 { return m_defaultColour; }
	const Vec2i& getTextPos(int ndx=0) const { ASSERT_RANGE(ndx, m_texts.size()); return m_texts[ndx].m_pos; }
	const Font* getTextFont() const { return m_defaultFont; }
	Vec2i getTextDimensions() const;
	Vec2i getTextDimensions(int ndx) const;
	bool hasText() const { return !m_texts.empty(); }
	int  numSnippets() const { return m_texts.size(); }
};

// =====================================================
// class Container
// =====================================================

class Container : public Widget {
public:
	typedef vector<Widget*> WidgetList;

protected:
	WidgetList children;

public:
	Container(Container* parent);
	Container(Container* parent, Vec2i pos, Vec2i size);
	Container(WidgetWindow* window);
	virtual ~Container();

	virtual Widget* getWidgetAt(const Vec2i &pos);

	virtual void setPos(const Vec2i &p);

	virtual void addChild(Widget* child);
	virtual void remChild(Widget* child);
	virtual void clear();
	virtual void setEnabled(bool v);
	virtual void setFade(float v);
	virtual void render();
};

}}

#endif

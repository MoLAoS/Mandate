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

using namespace Shared::Math;
using namespace Shared::Graphics;
using Glest::Util::Logger;

namespace Glest { namespace Widgets {

class Widget;
class Container;
class WidgetWindow;

typedef Widget* WidgetPtr;
typedef Container* ContainerPtr;
typedef WidgetWindow* WindowPtr;

using std::string;

#ifndef WIDGET_LOGGING
#	define WIDGET_LOGGING 0
#endif
#if WIDGET_LOGGING
#	define WIDGET_LOG(x) STREAM_LOG(x)
#else
#	define WIDGET_LOG(x)
#endif

// =====================================================
// class WidgetBase
// =====================================================
/// dataless widget abstraction
/*
class WidgetBase {
	friend class WidgetWindow;

public:
	virtual ~WidgetBase();

	// get/is
	virtual ContainerPtr getParent() const = 0;
	virtual WindowPtr getRootWindow() const = 0;
	virtual WidgetBase* getWidgetAt(const Vec2i &pos) = 0;

	virtual Vec2i getScreenPos() const = 0;
	virtual Vec2i getSize() const = 0;
	virtual int   getWidth() const = 0;
	virtual int   getHeight() const = 0;
	virtual float getFade() const = 0;

	virtual bool isVisible() const = 0;
	virtual bool isInside(const Vec2i &pos) const = 0;
	virtual bool isEnabled() const = 0;

	// set
	virtual void setEnabled(bool v) = 0;
	virtual void setSize(const Vec2i &sz) = 0;
	virtual void setPos(const Vec2i &p) = 0;
	virtual void setSize(const int x, const int y) { setSize(Vec2i(x,y)); }
	virtual void setPos(const int x, const int y) { setPos(Vec2i(x,y)); }
	virtual void setVisible(bool vis) = 0;
	virtual void setFade(float v) = 0;
	virtual void setParent(ContainerPtr p) = 0;

	virtual bool mouseDown(MouseButton btn, Vec2i pos) = 0;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) = 0;
	virtual bool mouseMove(Vec2i pos) = 0;
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos) = 0;
	virtual bool mouseWheel(Vec2i pos, int z) = 0;
	virtual void mouseIn() = 0;
	virtual void mouseOut() = 0;
	virtual bool keyDown(Key key) = 0;
	virtual bool keyUp(Key key) = 0;
	virtual bool keyPress(char c) = 0;

	virtual void lostKeyboardFocus() = 0;

	virtual void render() = 0;
	virtual string descPosDim() = 0;
	virtual string desc() = 0;

	sigslot::signal<WidgetBase*> Destroyed;
};
*/

// =====================================================
// enum BackgroundStyle
// =====================================================

WRAPPED_ENUM( BackgroundStyle,
	NONE,
	SOLID_COLOUR,
	ALPHA_COLOUR
);

// =====================================================
// enum BorderStyle - for '3d' borders
// =====================================================

WRAPPED_ENUM( BorderStyle,
	NONE,		/**< Draw nothing */
	RAISE,		/**< Draw a raised widget */
	EMBED,		/**< Draw a lowered widget */
	SOLID//,		/**< Draw a solid border */
	//EMBOSS,		/**< Draw a raised border */
	//ETCH		/**< Draw an etched (lowered) border */
);

// =====================================================
// class Widget
// =====================================================

class Widget /*: public WidgetBase */{
	friend class WidgetWindow;
protected:
	//int id;
	ContainerPtr parent;
	WindowPtr rootWindow;
	Vec2i	pos, 
			screenPos, 
			size;
	bool	visible;
	bool	enabled;
	float	fade;

	BorderStyle borderStyle;
	Vec3f borderColour;
	int borderSize;
	float bgAlpha;

protected:
	Widget(WindowPtr window);
	Widget() {} // dangerous perhaps, but needed to avoid virtual inheritence problems (double inits)

public:
	Widget(ContainerPtr parent);
	Widget(ContainerPtr parent, Vec2i pos, Vec2i size);
	virtual ~Widget();

	// get/is
	virtual Vec2i getScreenPos() const { return screenPos; }
	virtual ContainerPtr getParent() const { return parent; }
	virtual WindowPtr getRootWindow() const { return rootWindow; }
	virtual WidgetPtr getWidgetAt(const Vec2i &pos);

	virtual Vec2i getSize() const		{ return size;		 }
	virtual int   getWidth() const		{ return size.x;	 }
	virtual int   getHeight() const		{ return size.y;	 }
	virtual float getFade() const		{ return fade;		 }
	virtual int	  getBorderSize() const	{ return borderSize; }

	virtual bool isVisible() const		{ return visible; }
	virtual bool isInside(const Vec2i &pos) const { return pos >= screenPos && pos < screenPos + size; }
	virtual bool isEnabled() const	{ return enabled;	}

	// set
	virtual void setEnabled(bool v) { enabled = v;	}
	virtual void setSize(const Vec2i &sz);
	virtual void setPos(const Vec2i &p);
	virtual void setSize(const int x, const int y) { setSize(Vec2i(x,y)); }
	virtual void setPos(const int x, const int y) { setPos(Vec2i(x,y)); }
	virtual void setVisible(bool vis) { visible = vis; }
	virtual void setFade(float v) { fade = v; }
	virtual void setParent(ContainerPtr p) { parent = p; }

	// to decorator
	void setBorderSize(int sz) { borderSize = sz; }
	void setBorderStyle(BorderStyle style) { borderStyle = style; }
	void setBgAlphaValue(float v) { bgAlpha = v; }
	void setBorderColour(Vec3f colour) { borderColour = colour; }

	void setBorderParams(BorderStyle st, int sz, Vec3f col, float alp);

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos);
	virtual bool mouseWheel(Vec2i pos, int z);
	virtual void mouseIn();
	virtual void mouseOut();
	virtual bool keyDown(Key key);
	virtual bool keyUp(Key key);
	virtual bool keyPress(char c);

	virtual void lostKeyboardFocus() {}

	virtual void render() = 0;

	void renderBgAndBorders();
	void renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha, Vec2i offset, Vec2i size);
	void renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha);

	virtual string descPosDim();
	virtual string desc() = 0;

	sigslot::signal<WidgetPtr> Destroyed;

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

class ImageWidget : public virtual Widget {
private:
	typedef vector<Texture2D*> Textures;
	Textures textures;

	vector<ImageRenderInfo> imageInfo;
	//Texture2D *texture;

protected:
	void renderImage(int ndx = 0);

public:
	ImageWidget();
	ImageWidget(Texture2D *tex);

	int addImage(Texture2D *tex);
	void setImage(Texture2D *tex, int ndx = 0);
	int addImageX(Texture2D *tex, Vec2i offset, Vec2i sz);
	void setImageX(Texture2D *tex, int ndx, Vec2i offset, Vec2i sz);
};

// =====================================================
// class TextWidget
// =====================================================

class TextWidget : public virtual Widget {
private:
	vector<string> texts;
	//string text;
	Vec4f txtColour;
	Vec4f txtShadowColour;
	Vec2i txtPos;
	const Font *font;
	bool isFreeTypeFont;
	bool centre;

	void renderText(const string &txt, int x, int y, const Vec4f &colour, const Font *font = 0);

protected:
	void centreText(int ndx = 0);
	void renderText(int ndx = 0);
	void renderTextShadowed(int ndx = 0);

public:
	TextWidget();

	// set
	void setCentre(bool val)	{ centre = val; }
	void setTextParams(const string&, const Vec4f, const Font*, bool ft=false, bool cntr=true);
	int addText(const string &txt);
	void setText(const string &txt, int ndx = 0);
	void setTextColour(const Vec4f &col) { txtColour = col;	 }
	void setTextPos(const Vec2i &pos);
	void setTextFont(const Font *f);

	// virtual set
	virtual void setSize(const Vec2i &sz);
	virtual void setPos(const Vec2i &p);

	// get
	const string& getText(int ndx=0) const	{ return texts[ndx];	}
	const Vec4f& getTextColour() const	 { return txtColour; }
	const Vec2i& getTextPos() const	  { return txtPos; }
	const Font* getTextFont() const { return font; }
	Vec2i getTextDimensions() const;
};

// =====================================================
// class Container
// =====================================================

class Container : public virtual Widget {
public:
	typedef vector<WidgetPtr> WidgetList;

protected:
	WidgetList children;

public:
	Container();
	virtual ~Container();

	virtual WidgetPtr getWidgetAt(const Vec2i &pos);

	virtual void addChild(WidgetPtr child);
	virtual void remChild(WidgetPtr child);
	virtual void clear();
	virtual void setEnabled(bool v);
	virtual void setFade(float v);
	virtual void render();
};

// =====================================================
// class Layer
// =====================================================

class Layer : public Container {
private:
	const string name;
	const int id;

public:
	Layer(WidgetWindow *window, const string &name, int id)
			: Widget(window)
			, name(name), id(id) {
		WIDGET_LOG( __FUNCTION__ << endl );
	}

	~Layer() {
		WIDGET_LOG( __FUNCTION__ << endl );
	}

	int getId() const { return id; }
	const string& getName() const { return name; }

	virtual string desc() { return string("[Layer '") + name + "':" + descPosDim() + "]"; }
};

typedef Layer* LayerPtr;

}}

#endif

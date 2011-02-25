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

#include "widget_config.h"

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
//  enums Edge & AnchorType
// =====================================================

WRAPPED_ENUM( Direction, LEFT, UP, RIGHT, DOWN );
WRAPPED_ENUM( Edge, LEFT, TOP, RIGHT, BOTTOM );
WRAPPED_ENUM( AnchorType, NONE, RIGID, SPRINGY );

// =====================================================
//  class Anchor
// =====================================================

class Anchor {
	uint32	m_type  :  2;
	uint32	m_value : 30;

public:
	Anchor(AnchorType type, int val) : m_type(type), m_value(val) {}

	AnchorType getType() const { return AnchorType(m_type); }
	bool isRigid() const { return m_type == AnchorType::RIGID; }
	bool isSpringy() const { return m_type == AnchorType::SPRINGY; }
	void setType(AnchorType t) { m_type = t; }

	int getValue() const { return m_value; }
	void setValue(int v) { m_value = v; }
};

// =====================================================
//  class Anchors
// =====================================================

class Anchors {
private:
	Anchor left, top, right, bottom;
	bool centreVertical, centreHorizontal;

public:
	Anchors();
	Anchors(Anchor all);
	Anchors(Anchor leftRight, Anchor topBottom);
	Anchors(Anchor left, Anchor top, Anchor right, Anchor bottom);
	
	Anchor get(Edge e) const;
	Anchor operator[](Edge e) const { return get(e); }
	bool isCentreVertical() const { return centreVertical; }
	bool isCentreHorizontal() const { return centreHorizontal; }

	void set(Edge e, Anchor a);
	void set(Edge e, int val, bool spring) {
		set(e, Anchor(spring ? AnchorType::SPRINGY : AnchorType::RIGID, val));
	}
	void clear(Edge e) {
		set(e, Anchor(AnchorType::NONE, 0));
	}
	void setCentre(bool vert, bool horiz);
	void setCentre(bool v) { setCentre(v, v); }
};

// =====================================================
//  class Widget
// =====================================================

class Widget : public WidgetStyle {
	friend class WidgetWindow;
	friend class MouseWidget;
	friend class KeyboardWidget;
	friend class TextWidget;

public:
	MEMORY_CHECK_DECLARATIONS(Widget)

private:
	//int id;
	// position and size
	Vec2i m_pos,       // relative to parent
	      m_screenPos, // cache [m_parent->getScreenPos() + m_pos]
	      m_size;

	// state flags
	bool  m_hover;
	bool  m_focus;
	bool  m_enabled;

	// visibility
	bool  m_visible;
	float m_fade;

	// attachments
	MouseWidget    *m_mouseWidget;
	KeyboardWidget *m_keyboardWidget;
	TextWidget     *m_textWidget;

protected:
	// ancestors (parent and 'adam')
	Container    *m_parent;
	WidgetWindow *m_rootWindow;

	// 'cell' anchors
	Anchors      m_anchors;
	// styles inherited

protected: // flag setters
	void setHover(bool v);
	void setFocus(bool v);

	virtual void setStyle() {}
	void setWidgetStyle(WidgetType type);

protected: // get border sizes
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
	void setMouseWidget(MouseWidget *mw) { m_mouseWidget = mw; }
	void setKeyboardWidget(KeyboardWidget *kw) { m_keyboardWidget = kw; }

	// as
	MouseWidget * asMouseWidget() const			{ return m_mouseWidget; }
	KeyboardWidget * asKeyboardWidget() const		{ return m_keyboardWidget; }

	void init(const Vec2i &pos, const Vec2i &size);

protected:
	Widget(WidgetWindow* window); // for floating widgets and mouse cursors only!
	Widget(Container* parent, bool addToParent = true);
	Widget(Container* parent, Vec2i pos, Vec2i size, bool addToParent = true);

public:
	virtual ~Widget();

	// layout helpers .. Remove... bound for CellWidget...
	virtual Vec2i getPrefSize() const {return Vec2i(-1); } // may return (-1,-1) to indicate 'as big as possible'
	virtual Vec2i getMinSize() const {return Vec2i(-1); }  // should not return (-1,-1)
	virtual Vec2i getMaxSize() const  {return Vec2i(-1); } // return (-1,-1) to indicate 'no maximum size'

	// get / is
	Vec2i getScreenPos() const { return m_screenPos; }
	Vec2i getPos() const { return m_pos; }

	Container* getParent() const { return m_parent; }
	WidgetWindow* getRootWindow() const { return m_rootWindow; }
	virtual Widget* getWidgetAt(const Vec2i &pos);

	Vec2i getSize() const    { return m_size;    }
	int   getWidth() const   { return m_size.x;  }
	int   getHeight() const  { return m_size.y;  }
	float getFade() const    { return m_fade;    }
	bool isVisible() const   { return m_visible; }
	Anchors  getAnchors() const  { return m_anchors;  }

	bool isInside(const Vec2i &pos) const {
		const Vec2i &p1 = m_screenPos;
		const Vec2i  p2(m_screenPos + m_size);
		return pos.x >= p1.x && pos.y >= p1.y && pos.x < p2.x && pos.y < p2.y;
	}

	bool isEnabled() const { return m_enabled;	}
	bool isHovered() const { return m_enabled && m_hover; }
	bool isFocused() const { return m_enabled && m_focus; }

	// set
	virtual void setEnabled(bool v) { m_enabled = v; }
	virtual void setPos(const Vec2i &p);
	virtual void setSize(const Vec2i &sz);

	virtual void setVisible(bool vis) { m_visible = vis; }
	virtual void setFade(float v) { m_fade = v; }

	void setSize(const int x, const int y) { setSize(Vec2i(x,y)); }
	void setPos(const int x, const int y) { setPos(Vec2i(x,y)); }
	void setAnchors(Anchors a)    { m_anchors = a;   }
	void setStyle(const WidgetStyle &style) {
		*static_cast<WidgetStyle*>(this) = style;
	}
	
	//virtual void setParent(Container* p) { m_parent = p; }

	void setBorderStyle(const BorderStyle &style);
	void setBackgroundStyle(const BackgroundStyle &style);

	virtual void update() {} // must 'register' with WidgetWindow to receive

	virtual void render();

	void renderBordersFromTexture(const BorderStyle &style, const Vec2i &offset, const Vec2i &size);
	void renderBorders(const BorderStyle &style, const Vec2i &offset, const Vec2i &size);
	void renderBackground(const BackgroundStyle &style, const Vec2i &offset, const Vec2i &size);
	void renderOverlay(int ndx, Vec2i offset, Vec2i size);

	void renderBackground();
	void renderForeground();

//	void renderBgAndBorders(bool bg = true);
	void renderHighLight(int colour, float centreAlpha, float borderAlpha, Vec2i offset, Vec2i size);
	void renderHighLight(int colour, float centreAlpha, float borderAlpha);

	//void renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha, Vec2i offset, Vec2i size);
	//void renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha);

	virtual string descPosDim();
	virtual string desc() = 0;

	sigslot::signal<Widget*> Destroyed;

};

// =====================================================
//  class MouseWidget
// =====================================================

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

// =====================================================
//  class KeyboardWidget
// =====================================================

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

// =====================================================
//  struct ImageRenderInfo
// =====================================================

struct ImageRenderInfo {
	bool hasOffset, hasCustomSize;
	Vec2i offset, size;

	ImageRenderInfo() : hasOffset(false), hasCustomSize(false) {}

	ImageRenderInfo(bool hasOffset, Vec2i offset, bool hasCustomSize, Vec2i size)
		: hasOffset(hasOffset), hasCustomSize(hasCustomSize), offset(offset), size(size) {}
};

// =====================================================
//  class ImageWidget
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

// =====================================================
//  struct TextRenderInfo
// =====================================================

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
		Vec4f black(0.f, 0.f, 0.f, 1.f);
		m_shadowColour = (m_colour + black * 2.f) / 3.f;
		m_shadowColour2 = black;

	}
};

// =====================================================
//  class TextWidget
// =====================================================

class TextWidget {
private:
	typedef vector<TextRenderInfo> Texts;

private:
	Widget*       me;
	Texts         m_texts;
	bool          m_centreText;
	bool          m_batchRender;
	FontPtr       m_defaultFont;
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
	void setCentre(bool val)	{ m_centreText = val; }
	void setTextParams(const string&, const Vec4f, const Font*, bool cntr=true);
	int addText(const string &txt);
	void setText(const string &txt, int ndx = 0);
	void setTextColour(const Vec4f &col, int ndx = 0) {
		ASSERT_RANGE(ndx, m_texts.size());
		m_texts[ndx].m_colour = col;
	}
	void setTextShadowColour(const Vec4f &col, int ndx = 0) {
		ASSERT_RANGE(ndx, m_texts.size());
		m_texts[ndx].m_shadowColour = col;
	}
	void setTextShadowColour2(const Vec4f &col, int ndx = 0) {
		ASSERT_RANGE(ndx, m_texts.size());
		m_texts[ndx].m_shadowColour2 = col;
	}
	void setTextShadowColours(const Vec4f &col1, const Vec4f &col2, int ndx = 0) {
		setTextShadowColour(col1, ndx);
		setTextShadowColour2(col2, ndx);
	}
	void setTextCentre(bool v)	{ m_centreText = v; }
	void setTextPos(const Vec2i &pos, int ndx=0);
	void setTextFont(const Font *f);

	void centreText(int ndx = 0);
	void widgetReSized();

	// get
	const string& getText(int ndx=0) const	{ ASSERT_RANGE(ndx, m_texts.size()); return m_texts[ndx].m_text; }
	const Vec2i& getTextPos(int ndx=0) const { ASSERT_RANGE(ndx, m_texts.size()); return m_texts[ndx].m_pos; }
	const Font* getTextFont() const { return m_defaultFont; }
	Vec2i getTextDimensions() const;
	Vec2i getTextDimensions(int ndx) const;
	bool hasText() const { return !m_texts.empty(); }
	int  numSnippets() const { return m_texts.size(); }
};

// =====================================================
//  enum MouseAppearance
// =====================================================

WRAPPED_ENUM( MouseAppearance, 
	DEFAULT,
	CMD_ICON,
	MOVE_FREE/*,
	MOVE_UP,
	MOVE_RIGHT_UP,
	MOVE_RIGHT,
	MOVE_RIGHT_DOWN,
	MOVE_DOWN,
	MOVE_LEFT_DOWN,
	MOVE_LEFT,
	MOVE_LEFT_UP,
	ROTATE*/
)

// =====================================================
//  class MouseCursor
// =====================================================

class MouseCursor : public Widget {
protected:
	void renderTex(const Texture2D *tex);

public:
	MouseCursor(WidgetWindow *window) : Widget(window) {}
	virtual ~MouseCursor() {}

	virtual void setAppearance(MouseAppearance ma, const Texture2D *tex = 0) = 0;
};

// =====================================================
//  class Container
// =====================================================

class Container : public Widget {
public:
	typedef vector<Widget*> WidgetList;

protected:
	WidgetList  m_children;

	virtual void delChild(Widget* child);
	virtual void clear();

public:
	Container(Container* parent);
	Container(Container* parent, Vec2i pos, Vec2i size);
	Container(WidgetWindow* window);
	virtual ~Container();

	virtual Widget* getWidgetAt(const Vec2i &pos);

	virtual void addChild(Widget* child);
	virtual void remChild(Widget* child);

	virtual void setPos(const Vec2i &p);
	virtual void setEnabled(bool v);
	virtual void setFade(float v);
	virtual void render();
};

// =====================================================
//  enums Orientation & Origin
// =====================================================

WRAPPED_ENUM( Orientation, VERTICAL, HORIZONTAL );
WRAPPED_ENUM( Origin, FROM_TOP, FROM_BOTTOM, CENTRE, FROM_LEFT, FROM_RIGHT );

// =====================================================
//  class SizeHint
// =====================================================

class SizeHint {
private:
	int m_percentage;
	int m_absolute;

public:
	SizeHint(int percentage = -1, int absolute = -1) 
			: m_percentage(percentage), m_absolute(absolute) {}

	SizeHint(const SizeHint &rhs)
			: m_percentage(rhs.m_percentage), m_absolute(rhs.m_absolute) {}

	bool isPercentage() const   { return m_absolute == -1; }
	int  getPercentage() const  { return m_percentage; }
	int  getAbsolute() const    { return m_absolute; }
};

// =====================================================
// class WidgetCell
// =====================================================

class WidgetCell : public Container {
private:
	SizeHint   m_sizeHint;       // => WidgetCell

protected:
	void anchorWidgets();

public:
	WidgetCell(Container *parent);
	WidgetCell(Container *parent, Vec2i pos, Vec2i size);

	void setSizeHint(SizeHint sh) { m_sizeHint = sh; }

	virtual void setSize(const Vec2i &sz) {
		Widget::setSize(sz);
		anchorWidgets();
	}
	virtual void setPos(const Vec2i &p) {
		Widget::setPos(p); // skip Container, we reset children ourselves...
		anchorWidgets();
	}

	void clear() { Container::clear(); }

	SizeHint getSizeHint() const { return m_sizeHint; }

	virtual string desc() override { return string("[WidgetCell: ") + descPosDim() + "]"; }
};

}}

#endif

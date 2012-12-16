// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_WIDGET_STYLE_INCLUDED_
#define _GLEST_WIDGETS_WIDGET_STYLE_INCLUDED_

#include <string>
#include <vector>
#include <string>
#include "font.h"

#include "math_util.h"

using namespace Shared::Math;
using Shared::Graphics::Font;

namespace Glest { namespace Widgets {

WRAPPED_ENUM( Border, TOP, RIGHT, BOTTOM, LEFT );
WRAPPED_ENUM( Corner, TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT );

// =====================================================
//  BorderStyle
// =====================================================

STRINGY_ENUM( BorderType, NONE, INVISIBLE, RAISE, EMBED, SOLID, CUSTOM_SIDES, CUSTOM_CORNERS, TEXTURE );

struct BorderStyle {
	BorderType	m_type;
	int			m_colourIndices[Corner::COUNT]; // 4x4 => 16
	int			m_sizes[Border::COUNT];         // 4x4 => 16
	int         m_imageNdx;   // for type == TEXTURE
	int         m_cornerSize; // for type == TEXTURE

	BorderStyle();
	BorderStyle(const BorderStyle &style);

	void setSizes(int left, int top, int right, int bottom);
	void setSizes(int all) { setSizes(all, all, all, all); }
	void setSizes(Vec4i v) { setSizes(v.x, v.y, v.z, v.w); }

	void setNone();
	void setRaise(int lightColourIndex, int darkColourIndex);
	void setEmbed(int lightColourIndex, int darkColourIndex);
	void setSolid(int colourIndex);
	void setImage(int imageIndex, int borderSize, int cornerSize);

	Vec2i getBorderDims() const {
		return Vec2i(m_sizes[Border::LEFT] + m_sizes[Border::RIGHT],
					 m_sizes[Border::TOP] + m_sizes[Border::BOTTOM]);
	}

	int getHorizBorderDim() const {
		return m_sizes[Border::LEFT] + m_sizes[Border::RIGHT];
	}

	int getVertBorderDim() const {
		return m_sizes[Border::TOP] + m_sizes[Border::BOTTOM];
	}
};

// =====================================================
//  PaddingStyle
// =====================================================

struct PaddingStyle {
	int	 m_sizes[Border::COUNT];

	PaddingStyle();

	void setUniform(int pad); // all
	void setVertical(int pad); // left & right
	void setHorizontal(int pad); // top & bottom
	void setValues(int top, int right, int bottom, int left);
};

// =====================================================
//  BackgroundStyle
// =====================================================

STRINGY_ENUM( BackgroundType, NONE, COLOUR, CUSTOM_COLOURS, TEXTURE );

struct BackgroundStyle {
	BackgroundType	m_type;
	int				m_colourIndices[Corner::COUNT];
	int				m_imageIndex;
	bool			m_insideBorders; // render within borders

	BackgroundStyle();
	BackgroundStyle(const BackgroundStyle &style);

	void setNone();
	void setColour(int colourIndex);
	void setCustom(int colourIndex1, int colourIndex2, int colourIndex3, int colourIndex4);
	void setTexture(int imageIndex);
};

// =====================================================
//  TextStyle
// =====================================================

struct TextStyle {
	int		 m_fontIndex;
	int      m_smallFontIndex;
	int      m_largeFontIndex;
	int      m_tinyFontIndex;
	int		 m_colourIndex;
	int		 m_shadowColourIndex;
	bool	 m_shadow;

	TextStyle();

	void setNormal(int fontIndex, int colourIndex);
	void setShadow(int fontIndex, int colourIndex, int shadowColourIndex);
};

// =====================================================
//  HighLightStyle
// =====================================================

STRINGY_ENUM( HighLightType,
	NONE, OSCILLATE, FIXED
);

struct HighLightStyle {
	HighLightType  m_type;
	int            m_colourIndex;
	//Vec2i          m_offset;
	//Vec2i          m_size;

	HighLightStyle(HighLightType  type, int colour) : m_type(type), m_colourIndex(colour) {}
	HighLightStyle() : m_type(HighLightType::NONE), m_colourIndex(-1) {}
};

struct OverlayStyle {
	int     m_tex;
	bool    m_insideBorders;
	//Vec2i   m_offset;
	//Vec2i   m_size;

	OverlayStyle(int tex, bool inBorders) : m_tex(tex), m_insideBorders(inBorders) {}
	OverlayStyle() : m_tex(-1), m_insideBorders(false) {}
};

// =====================================================
//  WidgetStyle
// =====================================================

class WidgetStyle {
protected:
	BorderStyle      m_borderStyle;
	BackgroundStyle  m_backgroundStyle;
	HighLightStyle   m_highlightStyle;
	TextStyle        m_textStyle;
	OverlayStyle     m_overlayStyle;

public:
	WidgetStyle() {}

	BorderStyle&      borderStyle() { return m_borderStyle; }
	BackgroundStyle&  backgroundStyle() { return m_backgroundStyle; }
	HighLightStyle&   highLightStyle() { return m_highlightStyle; }
	TextStyle&        textStyle() { return m_textStyle; }
	OverlayStyle&     overlayStyle() { return m_overlayStyle; }

	const BorderStyle&      borderStyle() const { return m_borderStyle; }
	const BackgroundStyle&  backgroundStyle() const { return m_backgroundStyle; }
	const HighLightStyle&   highLightStyle() const { return m_highlightStyle; }
	const TextStyle&        textStyle() const { return m_textStyle; }
	const OverlayStyle&     overlayStyle() const { return m_overlayStyle; }
};

}}

#endif

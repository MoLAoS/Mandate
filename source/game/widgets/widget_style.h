// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_WIDGET_STYLE_INCLUDED_
#define _GLEST_WIDGETS_WIDGET_STYLE_INCLUDED_

#include <string>
#include <vector>
#include <string>

#include "math_util.h"

using namespace Shared::Math;

namespace Glest { namespace Widgets {

WRAPPED_ENUM( Border, TOP, RIGHT, BOTTOM, LEFT );
WRAPPED_ENUM( Corner, TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT );

STRINGY_ENUM( BorderType, NONE, RAISE, EMBED, SOLID, CUSTOM_SIDES, CUSTOM_CORNERS, TEXTURE );

struct BorderStyle {
	BorderType	m_type;
	int			m_colourIndices[Corner::COUNT];
	int			m_sizes[Border::COUNT];
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

struct PaddingStyle {
	int	 m_sizes[Border::COUNT];

	PaddingStyle();

	void setUniform(int pad); // all
	void setVertical(int pad); // left & right
	void setHorizontal(int pad); // top & bottom
	void setValues(int top, int right, int bottom, int left);
};

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

struct TextStyle {
	int		m_fontIndex;
	int		m_colourIndex;
	int		m_shadowColourIndex;
	bool	m_shadow;

	TextStyle();

	void setNormal(int fontIndex, int colourIndex);
	void setShadow(int fontIndex, int colourIndex, int shadowColourIndex);
};


}}

#endif
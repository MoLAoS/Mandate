// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGET_CONFIG_INCLUDED_
#define _GLEST_WIDGET_CONFIG_INCLUDED_

#include <vector>
#include <map>

using std::vector;
using std::map;

#include "math_util.h"	// Colour (Vec4<unsigned char>)
#include "font.h"		// Font
#include "texture.h"	// Texture2D

#include "widget_style.h"
#include "lua_script.h"

using Shared::Platform::uint32;
using Shared::Math::Colour;
using Shared::Graphics::Font;
using Shared::Graphics::Texture2D;
using namespace Shared::Lua;

namespace Glest { namespace Widgets {

WRAPPED_ENUM( WidgetFont,
	MENU_NORMAL, MENU_SMALL, MENU_BIG, MENU_VERY_BIG, CONSOLE
);

WRAPPED_ENUM( WidgetColour,
	WHITE, BLACK, LIGHT_BORDER, DARK_BORDER, BACKGROUND, DARK_BACKGROUND
);

WRAPPED_ENUM( WidgetTexture,
	BUTTON_SMALL, BUTTON_BIG, TEXT_ENTRY, CHECKBOX_CHECKED, CHECKBOX_UNCHECKED, 
	VERTICAL_SCROLL_UP, VERTICAL_SCROLL_DOWN
);

WRAPPED_ENUM( WidgetType,
	STATIC_WIDGET, BUTTON, CHECK_BOX, TEXT_BOX, LIST_ITEM, LIST_BOX, DROP_LIST, 
	SCROLL_BAR, SLIDER, TITLE_BAR, MESSAGE_BOX
);

class WidgetConfig {
	typedef const Texture2D*		TexPtr;
	typedef const Font*				FontPtr;
	typedef map<string, uint32>		IndexByNameMap;

private:
	vector<Colour>	m_colours;
	vector<FontPtr>	m_fonts;
	vector<TexPtr>	m_textures;

	IndexByNameMap	m_namedColours;
	IndexByNameMap	m_namedFonts;
	IndexByNameMap	m_namedTextures;

	vector<BorderStyle>		m_customBorderStyles;
	vector<BackgroundStyle> m_customBgStyles;

	IndexByNameMap	m_namedBorderStyles;
	IndexByNameMap	m_namedBgStyles;

	BorderStyle		m_borderStyles[WidgetType::COUNT];
	BorderStyle		m_focusBorderStyles[WidgetType::COUNT];
	BackgroundStyle m_backgroundStyles[WidgetType::COUNT];

	LuaScript luaScript;

private:
	WidgetConfig();

	void addGlestColour(const string &name, Colour colour);
	void addGlestFont(const string &name, FontPtr font);
	void addGlestTexture(const string &name, TexPtr tex);

	// load helpers
	bool loadStyles(const char *tableName, WidgetType wType);
	void loadBorderStyle(WidgetType widgetType, const char *table, BorderStyle &style);
	void loadBackgroundStyle(WidgetType widgetType);

public:
	static WidgetConfig& getInstance();

public:
	// get index of a Colour/Font/Texture, adding to collection if needed
	uint32 getColourIndex(const Vec3f &c);
	uint32 getColourIndex(const Colour &c);
	uint32 getFontIndex(const Font *f);
	uint32 getTextureIndex(const Texture2D *t);

	// get index of a named 'Glest' Colour/Font/Texture
	uint32 getColourIndex(const string &name) const;
	uint32 getFontIndex(const string &name) const;
	uint32 getTextureIndex(const string &name) const;

	// get Colour/Font/Texture by index
	const Colour&		getColour(uint32 ndx) const		{ return m_colours[ndx]; }
	const Font*			getFont(uint32 ndx) const		{ return m_fonts[ndx]; }
	const Texture2D*	getTexture(uint32 ndx) const	{ return m_textures[ndx]; }

	const BorderStyle& getBorderStyle(WidgetType type) const {
		return m_borderStyles[type];
	}

	const BorderStyle& getFocusBorderStyle(WidgetType type) const {
		return m_focusBorderStyles[type];
	}

	const BackgroundStyle& getBackgroundStyle(WidgetType type) const {
		return m_backgroundStyles[type];
	}
};

#define g_widgetConfig (Widgets::WidgetConfig::getInstance())

}}

#endif
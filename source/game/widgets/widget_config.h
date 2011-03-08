// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_WIDGET_CONFIG_INCLUDED_
#define _GLEST_WIDGETS_WIDGET_CONFIG_INCLUDED_

#include <vector>
#include <map>

using std::vector;
using std::map;

#include "math_util.h"	// Colour (Vec4<unsigned char>)
#include "texture.h"	// Texture2D

#include "widget_style.h"
#include "lua_script.h"

using Shared::Platform::uint32;
using Shared::Math::Colour;
using Shared::Graphics::Texture2D;
using namespace Shared::Lua;

namespace Glest { namespace Widgets {

STRINGY_ENUM( WidgetState,
	NORMAL, HOVER, FOCUS, DISABLED, SELECTED
);

STRINGY_ENUM( WidgetType,
	STATIC_WIDGET, BUTTON, CHECK_BOX, CHECK_BOX_CHK, TEXT_BOX, LIST_ITEM, LIST_BOX, DROP_LIST, 
	SCROLL_BAR, SLIDER, TITLE_BAR, MESSAGE_BOX, TOOL_TIP,
	SCROLLBAR_BTN_UP, SCROLLBAR_BTN_DOWN, SCROLLBAR_BTN_LEFT, SCROLLBAR_BTN_RIGHT,
	SCROLLBAR_VERT_SHAFT, SCROLLBAR_VERT_THUMB, SCROLLBAR_HORIZ_SHAFT, SCROLLBAR_HORIZ_THUMB,
	SLIDER_VERT_THUMB, SLIDER_HORIZ_THUMB, SLIDER_VERT_SHAFT, SLIDER_HORIZ_SHAFT,
	TITLE_BAR_CLOSE, TITLE_BAR_ROLL_UP, TITLE_BAR_ROLL_DOWN, TITLE_BAR_EXPAND, TITLE_BAR_SHRINK,
	COLOUR_BUTTON
);

STRINGY_ENUM( FontUsage, MENU, GAME, LUA, FANCY );
STRINGY_ENUM( OverlayUsage, TICK, CROSS, QUESTION );

// =====================================================
// 	class WidgetConfig
// =====================================================
/** Static Widget 'Configuration' class, maintains textures, colours & fonts,
  * and background and broder styles for all widget 'types', loaded from lua
  * */
class WidgetConfig {
	typedef const Texture2D*		TexPtr;
	typedef const Font*				FontPtr;
	typedef map<string, uint32>		IndexByNameMap;
	//typedef vector<BorderStyle>     BorderStyles;
	//typedef vector<BackgroundStyle> BackgroundStyles;
	//typedef vector<FontStyle>       FontStyles;

private:
	vector<Colour>	 m_colours;
	vector<FontSet>  m_fonts;
	vector<TexPtr>	 m_textures;

	IndexByNameMap	 m_namedColours;
	IndexByNameMap	 m_namedFonts;
	IndexByNameMap	 m_namedTextures;

	//BorderStyles     m_customBorderStyles;
	//BackgroundStyles m_customBgStyles;
	//FontStyles       m_customFontStyle;

	IndexByNameMap	 m_namedBorderStyles;
	IndexByNameMap	 m_namedBgStyles;

	WidgetStyle      m_styles[WidgetType::COUNT][WidgetState::COUNT];
	//BorderStyle		 m_borderStyles[WidgetType::COUNT][WidgetState::COUNT];
	//BackgroundStyle  m_backgroundStyles[WidgetType::COUNT][WidgetState::COUNT];
	//HighLightStyle   m_highLightStyles[WidgetType::COUNT][WidgetState::COUNT];
	//TextStyle        m_fontStyles[WidgetType::COUNT][WidgetState::COUNT];
	//int              m_overlayTextures[WidgetType::COUNT][WidgetState::COUNT];

	int  m_defaultElementHeight;
	int  m_defaultFonts[FontUsage::COUNT];
	int  m_defaultOverlays[OverlayUsage::COUNT];
	int  m_mouseTexture;
	//FontSet          m_menuFont;
	//FontSet          m_gameFont;
	//FontSet          m_fancyFont;

	LuaScript luaScript;

private:
	WidgetConfig();

	// load helpers
	bool loadStyles(const char *tableName, WidgetType wType, bool glob = true);

	void loadBorderStyle(WidgetType widgetType, BorderStyle &style, BorderStyle *src = 0);
	void loadBackgroundStyle(WidgetType widgetType, BackgroundStyle &style, BackgroundStyle *src = 0);
	void loadHighLightStyle(WidgetType widgetType, HighLightStyle &style, HighLightStyle *src = 0);
	void loadTextStyle(WidgetType widgetType, TextStyle &style, TextStyle *src = 0);

	int computeFontSize(int size);

public:
	static WidgetConfig& getInstance();

	void load();

public:
	int loadTexture(const string &name, const string &path);
	void loadFont(const string &name, const string &path, int size);

	void setDefaultFont(const string &type, const string &name);
	void setMouseTexture(const string &name);
	void setOverlayTexture(const string &type, const string &name);

	void addGlestColour(const string &name, Colour colour);
	//void addGlestFont(const string &name, FontPtr font);
	void addGlestTexture(const string &name, TexPtr tex);

	// get index of a Colour/Font/Texture, adding to collection if needed
	int getColourIndex(const Vec3f &c);
	int getColourIndex(const Colour &c);
	//int getFontIndex(const Font *f);
	int getTextureIndex(const Texture2D *t);

	int getDefaultItemHeight() const;

	FontSet& getMenuFont() { return m_fonts[m_defaultFonts[FontUsage::MENU]]; }
	FontSet& getTitleFont() { return m_fonts[m_defaultFonts[FontUsage::FANCY]]; }
	FontSet& getGameFont() { return m_fonts[m_defaultFonts[FontUsage::GAME]]; }

	TexPtr getTickTexture() { return m_textures[m_defaultOverlays[OverlayUsage::TICK]]; }
	TexPtr getCrossTexture() { return m_textures[m_defaultOverlays[OverlayUsage::CROSS]]; }
	TexPtr getQuestionTexture() { return m_textures[m_defaultOverlays[OverlayUsage::QUESTION]]; }

	TexPtr getMouseTexture() { return m_textures[m_mouseTexture]; }

	// get index of a named 'Glest' Colour/Font/Texture
	int getColourIndex(const string &name) const;
	int getFontIndex(const string &name) const;
	int getTextureIndex(const string &name) const;

	// get Colour/Font/Texture by index
	const Colour&		getColour(uint32 ndx) const		{ return m_colours[ndx]; }
	FontSet&            getFontSet(uint32 ndx)          { return m_fonts[ndx]; }
	//const Font*			getFont(uint32 ndx) const		{ return m_fonts[ndx]; }
	const Texture2D*	getTexture(uint32 ndx) const	{ return m_textures[ndx]; }

	Font* getFont(uint32 ndx, FontSize size = FontSize::NORMAL) {
		ASSERT_RANGE(ndx, m_fonts.size());
		return m_fonts[ndx][size];
	}
	Font* getFont(const string &name, FontSize size = FontSize::NORMAL) {
		int ndx = getFontIndex(name);
		if (ndx != -1) {
			return getFont(ndx, size);
		} else {
			return 0;
		}
	}

	//Font* getMenuFont(FontSize size = FontSize::NORMAL) { return m_menuFont[size]; }
	//Font* getGameFont(FontSize size = FontSize::NORMAL) { return m_gameFont[size]; }
	//Font* getFancyFont(FontSize size = FontSize::NORMAL) { return m_fancyFont[size]; }

	const WidgetStyle& getWidgetStyle(WidgetType type, WidgetState state = WidgetState::NORMAL) const {
		return m_styles[type][state];
	}

	const BorderStyle& getBorderStyle(WidgetType type, WidgetState state = WidgetState::NORMAL) const {
		return m_styles[type][state].borderStyle();
	}

	const BackgroundStyle& getBackgroundStyle(WidgetType type, WidgetState state = WidgetState::NORMAL) const {
		return m_styles[type][state].backgroundStyle();
	}

	HighLightStyle getHighLightStyle(WidgetType type, WidgetState state = WidgetState::NORMAL) const {
		return m_styles[type][state].highLightStyle();
	}

	const TextStyle& getFontStyle(WidgetType type, WidgetState state = WidgetState::NORMAL) const {
		return m_styles[type][state].textStyle();
	}

	int getOverlay(WidgetType type, WidgetState state = WidgetState::NORMAL) const {
		return m_styles[type][state].overlay();
	}
};

namespace GuiScript {

// script callbacks
int setDefaultFont(LuaHandle *lh);
int setOverlayTexture(LuaHandle *lh);
int setMouseTexture(LuaHandle *lh);
int addColour(LuaHandle *lh);
int loadTexture(LuaHandle *lh);
int loadFont(LuaHandle *lh);

}

#define g_widgetConfig (Widgets::WidgetConfig::getInstance())

}} // Glest::Widgets

#endif
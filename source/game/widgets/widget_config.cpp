// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "widget_config.h"
#include "core_data.h"
#include "script_manager.h"

namespace Glest { namespace Widgets {
using namespace Shared::PhysFS;
using Script::ScriptManager;
using Global::CoreData;

WidgetConfig& WidgetConfig::getInstance() {
	static WidgetConfig config;
	return config;
}

void WidgetConfig::addGlestFont(const string &name, const Font *font) {
	m_fonts.push_back(font);
	m_namedFonts[name] = m_fonts.size() - 1;
}

void WidgetConfig::addGlestColour(const string &name, Colour colour) {
	m_colours.push_back(colour);
	m_namedColours[name] = m_colours.size() - 1;
}

void WidgetConfig::addGlestTexture(const string &name, TexPtr tex) {
	m_textures.push_back(tex);
	m_namedTextures[name] = m_textures.size() - 1;
}

void WidgetConfig::loadBorderStyle(WidgetType widgetType, const char *table, BorderStyle &style) {
	if (luaScript.getTable(table)) {
		string type = luaScript.getStringField("Type");
		BorderType bt = BorderTypeNames.match(type.c_str());
		if (bt == BorderType::INVALID) {
			///@todo warn / report error
			bt = BorderType::NONE;
		}
		style.m_type = bt;
		if (bt != BorderType::NONE) {
			try {
				Vec4i sizes = luaScript.getVec4iField("Sizes");
				style.setSizes(sizes);
			} catch (LuaError &e) {
				///@todo warning ?					
			}
			int numColours = 0;
			if (bt == BorderType::SOLID) {
				numColours = 1;
			} else if (bt == BorderType::RAISE || bt == BorderType::EMBED) {
				numColours = 2;
			} else if (bt ==  BorderType::CUSTOM_CORNERS || bt == BorderType::CUSTOM_SIDES) {
				numColours = 4;
			}
			if (numColours) {
				try { ///@todo error stuff
					StringSet strings = luaScript.getStringSet("Colours");
					for (int i=0; i < numColours; ++i) {
						if (!strings[i].empty()) {
							style.m_colourIndices[i] = getColourIndex(strings[i]);
						}
					}
				} catch (LuaError &e) {
					///@todo report
				}
			}
		}
		luaScript.popTable();
	}
}

void WidgetConfig::loadBackgroundStyle(WidgetType widgetType) {
	if (luaScript.getTable("Background")) {
		string type = luaScript.getStringField("Type");
		BackgroundType bt = BackgroundTypeNames.match(type.c_str());
		if (bt == BackgroundType::INVALID) {
			bt = BackgroundType::NONE;
		}
		m_backgroundStyles[widgetType].m_type = bt;
		if (bt == BackgroundType::COLOUR || bt == BackgroundType::CUSTOM_COLOURS) {
			try { ///@todo error stuff
				StringSet strings = luaScript.getStringSet("Colours");
				for (int i=0; i < 4; ++i) {
					if (!strings[i].empty()) {
						m_backgroundStyles[widgetType].m_colourIndices[i] = getColourIndex(strings[i]);
					}
				}
			} catch (LuaError &e) {
				///@todo report
			}
		}
		if (bt == BackgroundType::TEXTURE) {
			try {
				string tex = luaScript.getStringField("Texture");
				m_backgroundStyles[widgetType].m_imageIndex = getTextureIndex(tex);
			} catch (LuaError &e) {

			}
		}
	}
	luaScript.popTable();
}

bool WidgetConfig::loadStyles(const char *tableName, WidgetType widgetType) {
	if (luaScript.getGlobal(tableName)) {
		loadBorderStyle(widgetType, "Borders", m_borderStyles[widgetType]);
		loadBorderStyle(widgetType, "BordersFocused", m_focusBorderStyles[widgetType]);
		loadBackgroundStyle(widgetType);
		luaScript.popAll();
		return true;
	} else {
		return false;
	}
}

WidgetConfig::WidgetConfig() {
	addGlestFont("menu-font-normal", g_coreData.getFTMenuFontNormal());
	addGlestFont("menu-font-small", g_coreData.getFTMenuFontSmall());
	addGlestFont("menu-font-big", g_coreData.getFTMenuFontBig());
	addGlestFont("menu-font-very-big", g_coreData.getFTMenuFontVeryBig());

	addGlestColour("colour-white", Colour(255u));
	addGlestColour("colour-black", Colour(0u, 0u, 0u, 255u));
	addGlestColour("colour-border-light", Colour(255u, 255u, 255u, 102u));
	addGlestColour("colour-border-dark", Colour(0u, 0u, 0u, 102u));
	addGlestColour("colour-background", Colour(91u, 91u, 91u, 102u));
	addGlestColour("colour-dark-background", Colour(63u, 63u, 63u, 159u));

	addGlestTexture("texture-button-small", g_coreData.getButtonSmallTexture());
	addGlestTexture("texture-button-big", g_coreData.getButtonBigTexture());
	addGlestTexture("texture-text-entry", g_coreData.getTextEntryTexture());
	addGlestTexture("texture-checkbox-checked", g_coreData.getCheckBoxTickTexture());
	addGlestTexture("texture-checkbox-unchecked", g_coreData.getCheckBoxCrossTexture());
	addGlestTexture("texture-vertical-scroll-up", g_coreData.getVertScrollUpTexture());
	addGlestTexture("texture-vertical-scroll-down", g_coreData.getVertScrollDownTexture());

	luaScript.startUp();
	FileOps *f = g_fileFactory.getFileOps();
	f->openRead("data/core/widget.cfg");
	int size = f->fileSize();
	char *someLua = new char[size + 1];
	f->read(someLua, size, 1);
	someLua[size] = '\0';
	delete f;
	luaScript.luaDoLine(someLua);
	delete [] someLua;
	if (!loadStyles("StaticWidget", WidgetType::STATIC_WIDGET)) { // load or set default
		m_borderStyles[WidgetType::STATIC_WIDGET].setNone();
		m_backgroundStyles[WidgetType::STATIC_WIDGET].setNone();
		//m_borderStyles[WidgetType::STATIC_WIDGET].setSolid(getColourIndex(Colour(255u, 0u, 0u, 255u)));
		//m_borderStyles[WidgetType::STATIC_WIDGET].setSizes(1);
	}
	if (!loadStyles("Button", WidgetType::BUTTON)) {
		m_borderStyles[WidgetType::BUTTON].setNone();
		m_backgroundStyles[WidgetType::BUTTON].setNone();
		//m_backgroundStyles[WidgetType::BUTTON].setTexture(WidgetTexture::BUTTON_BIG);
	}
	if (!loadStyles("CheckBox", WidgetType::CHECK_BOX)) {
		m_borderStyles[WidgetType::CHECK_BOX].setNone();
	}
	if (!loadStyles("TextBox", WidgetType::TEXT_BOX)) {
		m_borderStyles[WidgetType::TEXT_BOX].setEmbed(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
		m_borderStyles[WidgetType::TEXT_BOX].setSizes(2);
		m_focusBorderStyles[WidgetType::TEXT_BOX].setSolid(WidgetColour::LIGHT_BORDER);
		m_focusBorderStyles[WidgetType::TEXT_BOX].setSizes(2);
		m_backgroundStyles[WidgetType::TEXT_BOX].setColour(WidgetColour::DARK_BACKGROUND);
	}
	// FIXME
	if (true || !loadStyles("ListItem", WidgetType::LIST_ITEM)) {
		///@todo fix. ListBoxItem doesn't work like other widgets and needs to be refactored before
		// loading a custom style from widget.cfg

		m_borderStyles[WidgetType::LIST_ITEM].setSolid(WidgetColour::DARK_BORDER);
		m_borderStyles[WidgetType::LIST_ITEM].setSizes(2);
		
		m_borderStyles[WidgetType::LIST_ITEM].m_colourIndices[0] = WidgetColour::DARK_BORDER;
		m_borderStyles[WidgetType::LIST_ITEM].m_colourIndices[1] = WidgetColour::DARK_BORDER;
		m_borderStyles[WidgetType::LIST_ITEM].m_colourIndices[2] = WidgetColour::LIGHT_BORDER;

		m_focusBorderStyles[WidgetType::LIST_ITEM].setSolid(WidgetColour::LIGHT_BORDER);
		m_focusBorderStyles[WidgetType::LIST_ITEM].setSizes(2);

		m_focusBorderStyles[WidgetType::LIST_ITEM].m_colourIndices[0] = WidgetColour::DARK_BORDER;
		m_focusBorderStyles[WidgetType::LIST_ITEM].m_colourIndices[1] = WidgetColour::DARK_BORDER;
		m_focusBorderStyles[WidgetType::LIST_ITEM].m_colourIndices[2] = WidgetColour::LIGHT_BORDER;

		m_backgroundStyles[WidgetType::LIST_ITEM].setColour(WidgetColour::DARK_BACKGROUND);

	}
	if (!loadStyles("ListBox", WidgetType::LIST_BOX)) {
		m_borderStyles[WidgetType::LIST_BOX].setEmbed(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
		m_borderStyles[WidgetType::LIST_BOX].setSizes(2);
	}
	if (!loadStyles("DropList", WidgetType::DROP_LIST)) {
		m_borderStyles[WidgetType::DROP_LIST].setNone();
		//m_borderStyles[WidgetType::DROP_LIST].setSolid(WidgetColour::DARK_BORDER);
		//m_borderStyles[WidgetType::DROP_LIST].setSizes(2);
	}
	if (!loadStyles("ScrollBar", WidgetType::SCROLL_BAR)) {
		m_borderStyles[WidgetType::SCROLL_BAR].setNone();
		m_backgroundStyles[WidgetType::SCROLL_BAR].setColour(WidgetColour::BACKGROUND);
	}
	if (!loadStyles("Slider", WidgetType::SLIDER)) {
		m_borderStyles[WidgetType::SLIDER].setNone();
	}
	if (!loadStyles("TitleBar", WidgetType::TITLE_BAR)) {
		m_borderStyles[WidgetType::TITLE_BAR].setRaise(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
		m_borderStyles[WidgetType::TITLE_BAR].setSizes(2);
		m_backgroundStyles[WidgetType::TITLE_BAR].setColour(WidgetColour::BACKGROUND);
	}
	if (!loadStyles("MessageBox", WidgetType::MESSAGE_BOX)) {
		m_borderStyles[WidgetType::MESSAGE_BOX].setSolid(WidgetColour::BLACK);
		m_borderStyles[WidgetType::MESSAGE_BOX].setSizes(2);
		m_backgroundStyles[WidgetType::MESSAGE_BOX].setColour(WidgetColour::DARK_BACKGROUND);
	}
	luaScript.close();
}

uint32 WidgetConfig::getColourIndex(const Vec3f &c) {
	return getColourIndex(Colour(uint8(c.r * 255), uint8(c.g * 255), uint8(c.b * 255), 255u));
}

uint32 WidgetConfig::getColourIndex(const Colour &c) {
	const int &n = m_colours.size();
	for (int i = 0; i < n; ++i) {
		if (m_colours[i] == c) {
			return i;
		}
	}
	m_colours.push_back(c);
	return n;
}

uint32 WidgetConfig::getFontIndex(const Font *f) {
	const int &n = m_fonts.size();
	for (int i = 0; i < n; ++i) {
		if (m_fonts[i] == f) {
			return i;
		}
	}
	m_fonts.push_back(f);
	return n;
}

uint32 WidgetConfig::getTextureIndex(const Texture2D *t) {
	const int &n = m_textures.size();
	for (int i = 0; i < n; ++i) {
		if (m_textures[i] == t) {
			return i;
		}
	}
	m_textures.push_back(t);
	return n;
}

uint32 WidgetConfig::getColourIndex(const string &name) const {
	IndexByNameMap::const_iterator it = m_namedColours.find(name);
	if (it != m_namedColours.end()) {
		return it->second;
	}
	return 0;
}

uint32 WidgetConfig::getTextureIndex(const string &name) const {
	IndexByNameMap::const_iterator it = m_namedTextures.find(name);
	if (it != m_namedTextures.end()) {
		return it->second;
	}
	return 0;
}

uint32 WidgetConfig::getFontIndex(const string &name) const {
	IndexByNameMap::const_iterator it = m_namedFonts.find(name);
	if (it != m_namedFonts.end()) {
		return it->second;
	}
	return 0;
}

}}


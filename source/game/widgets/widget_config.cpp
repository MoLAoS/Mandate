// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "widget_config.h"
#include "core_data.h"
#include "script_manager.h"
#include "renderer.h"

namespace Glest { namespace Widgets {
using namespace Shared::PhysFS;
using Script::ScriptManager;
using Global::CoreData;
using namespace Graphics;

#undef WIDGET_LOG
#define WIDGET_LOG(x) {stringstream _ss; _ss << x; g_logger.logProgramEvent(_ss.str()); }

// =====================================================
// 	scripting helpers & callbacks
// =====================================================

namespace GuiScript {

///@todo de-duplicate
bool isHex(string s) {
	foreach (string, c, s) {
		if (!(isdigit(*c) || ((*c >= 'a' && *c <= 'f') || (*c >= 'A' && *c <= 'F')))) {
			return false;
		}
	}
	return true;
}

Colour extractColour(string s) {
	Colour colour;
	if (s.size() > 8) { // trim pre/post-fixes
		if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
			s = s.substr(2);
		} else if (s[0] == '#') {
			s = s.substr(1);
		} else if (s[s.size()-1] == 'h' || s[s.size()-1] == 'H') {
			s = s.substr(0, s.size() - 1);
		}
	}
	if ((s.size() == 6 || s.size() == 8) && isHex(s)) {
		string tmp = s.substr(0, 2);
		colour.r = Conversion::strToInt(tmp, 16);
		tmp = s.substr(2, 2);
		colour.g = Conversion::strToInt(tmp, 16);
		tmp = s.substr(4, 2);
		colour.b = Conversion::strToInt(tmp, 16);
		if (s.size() == 8) {
			tmp = s.substr(6, 2);
			colour.a = Conversion::strToInt(tmp, 16);
		} else {
			colour.a = 0xFFu;
		}
	} else {
		throw runtime_error("bad colour string " + s);
	}
	return colour;
}

int setOverlayTexture(LuaHandle *lh) {
	LuaArguments args(lh);
	string type, name;
	if (Script::ScriptManager::extractArgs(args, "setOverlayTexture", "str,str", &type, &name)) {
		WidgetConfig::getInstance().setOverlayTexture(type, name);
	}
	return args.getReturnCount();
}

int setMouseTexture(LuaHandle *lh) {
	LuaArguments args(lh);
	string name;
	if (Script::ScriptManager::extractArgs(args, "setMouseTexture", "str", &name)) {
		WidgetConfig::getInstance().setMouseTexture(name);
	}
	return args.getReturnCount();
}

int setDefaultFont(LuaHandle *lh) {
	LuaArguments args(lh);
	string type, name;
	if (Script::ScriptManager::extractArgs(args, "setDefaultFont", "str,str", &type, &name)) {
		WidgetConfig::getInstance().setDefaultFont(type, name);
	}
	return args.getReturnCount();
}

int addColour(LuaHandle *lh) {
	LuaArguments args(lh);
	string name, colour;
	if (Script::ScriptManager::extractArgs(args, "addColour", "str,str", &name, &colour)) {
		Colour c = extractColour(colour);
		WidgetConfig::getInstance().addGlestColour(name, c);
	}
	return args.getReturnCount();
}

int loadTexture(LuaHandle *lh) {
	LuaArguments args(lh);
	string name, path;
	bool mipmap;
	if (Script::ScriptManager::extractArgs(args, 0, "str,str,bln", &name, &path, &mipmap)) {
		WidgetConfig::getInstance().loadTexture(name, path, mipmap);
	} else if (Script::ScriptManager::extractArgs(args, "loadTexture", "str,str", &name, &path)) {
		WidgetConfig::getInstance().loadTexture(name, path);
	} else {
	}
	return args.getReturnCount();
}

int loadFont(LuaHandle *lh) {
	LuaArguments args(lh);
	string name, path;
	int size;
	if (Script::ScriptManager::extractArgs(args, "loadFont", "str,str,int", &name, &path, &size)) {
		WidgetConfig::getInstance().loadFont(name, path, size);
	}
	return args.getReturnCount();
}

} // GuiScript

// =====================================================
// 	class WidgetConfig
// =====================================================

WidgetConfig& WidgetConfig::getInstance() {
	static WidgetConfig config;
	return config;
}

void WidgetConfig::setDefaultFont(const string &type, const string &name) {
	FontUsage fu = FontUsageNames.match(type);
	if (fu == FontUsage::INVALID) {
		std::stringstream ss;
		ss << "Error loading widget.cfg, setDefaultFont(): font type '" << type << "'" 
			<< " is not valid (must be 'menu', 'game' or 'fancy').";
		WIDGET_LOG( ss.str() );
		g_logger.logError(ss.str());
		return;
	}
	int ndx = getFontIndex(name);
	if (ndx == -1) {
		std::stringstream ss;
		ss << "Error loading widget.cfg, setDefaultFont(): font named '" << name << "'" 
			<< " has not been loaded, or load failed.";
		WIDGET_LOG( ss.str() );
		g_logger.logError(ss.str());
		return;
	}
	WIDGET_LOG( "Setting '" << type << "' font to font named '" << name << "' @ ndx " << ndx );
	m_defaultFonts[fu] = ndx;
}

void WidgetConfig::setMouseTexture(const string &name) {
	int ndx = getTextureIndex(name);
	if (ndx == -1) {
		std::stringstream ss;
		ss << "Error loading widget.cfg, setMouseTexture(): texture named '" << name << "'" 
			<< " has not been loaded, or load failed.";
		WIDGET_LOG( ss.str() );
		g_logger.logError(ss.str());
		return;
	}
	WIDGET_LOG( "Setting mouse texture, name: '" << name << "' @ ndx " << ndx );
	m_mouseTexture = ndx;
}

void WidgetConfig::setOverlayTexture(const string &type, const string &name) {
	OverlayUsage ou = OverlayUsageNames.match(type);
	if (ou == OverlayUsage::INVALID) {
		std::stringstream ss;
		ss << "Error loading widget.cfg, setDefaultTexture(): texture type '" << type << "'" 
			<< " is not valid (must be 'tick', 'cross' or 'question').";
		WIDGET_LOG( ss.str() );
		g_logger.logError(ss.str());
		return;

	}
	int ndx = getTextureIndex(name);
	if (ndx == -1) {
		std::stringstream ss;
		ss << "Error loading widget.cfg, setDefaultTexture(): texture named '" << name << "'" 
			<< " has not been loaded, or load failed.";
		WIDGET_LOG( ss.str() );
		g_logger.logError(ss.str());
		return;
	}
	WIDGET_LOG( "Setting '" << type << "' overlay to texture named '" << name << "' @ ndx " << ndx );
	m_defaultOverlays[ou] = ndx;
}

void WidgetConfig::loadFont(const string &name, const string &path, int size) {
	Font *font = g_renderer.newFreeTypeFont(ResourceScope::GLOBAL);
	font->setType(path);
	font->setSize(computeFontSize(size));
	m_requestedFontSizes[font] = size;
	m_fonts.push_back(font);
	m_namedFonts[name] = m_fonts.size() - 1;
	WIDGET_LOG( "adding font named '" << name << "' from path '" << path << "' @ size: " << size );
}

void WidgetConfig::reloadFonts() {
	foreach (FontSizeMap, it, m_requestedFontSizes) {
		it->first->setSize(computeFontSize(it->second));
		it->first->reInit();
	}
}

int WidgetConfig::loadTexture(const string &name, const string &path, bool mipmap) {
	int n = getTextureIndex(path);
	if (n != -1) {
		return n;
	}
	Texture2D *tex = g_renderer.newTexture2D(ResourceScope::GLOBAL);
	try {
		tex->setMipmap(mipmap);
		tex->load(path);
		tex->init();
		addGlestTexture(name, tex);
		WIDGET_LOG( "loaded texture named '" << name << "' from path '" << path << "' @ ndx " 
			<< (m_textures.size() - 1));
		return m_textures.size() - 1;
	} catch (const runtime_error &e) {
		g_logger.logError("While running widget.cfg,\n\t Error loading '" + path + "'");
		WIDGET_LOG( "Error loading texture named '" << name << "' from path '" << path << "'");
		return -1;
	}
}

//void WidgetConfig::addGlestFont(const string &name, const Font *font) {
//	m_fonts.push_back(font);
//	m_namedFonts[name] = m_fonts.size() - 1;
//}

inline string colourString(Colour &c) {
	static char hexBuffer[32];
	sprintf(hexBuffer, "0x%.2X%.2X%.2X%.2X", int(c.r), int(c.g), int(c.b), int(c.a));
	return string(hexBuffer);
}

void WidgetConfig::addGlestColour(const string &name, Colour colour) {
	for (unsigned i = 0; i < m_colours.size(); ++i) {
		if (m_colours[i] == colour) {
			m_namedColours[name] = i;
			return;
		}
	}
	m_colours.push_back(colour);
	m_namedColours[name] = m_colours.size() - 1;
	WIDGET_LOG( "added colour named '" << name << "' (" << colourString(colour) 
		<< ") @ ndx " << m_namedColours[name]);
}

void WidgetConfig::addGlestTexture(const string &name, TexPtr tex) {
	m_textures.push_back(tex);
	m_namedTextures[name] = m_textures.size() - 1;
}

int WidgetConfig::computeFontSize(int size) {
	float sz = float(size);
	float scale = g_config.getDisplayHeight() / 800.f;
	sz *= scale;
	if (sz < 10.f) {
		sz = 10.f;
	}
	return int(roundf(sz));
}

void WidgetConfig::loadBorderStyle(WidgetType widgetType, BorderStyle &style, BorderStyle *src) {
	if (luaScript.getTable("Borders")) {
		WIDGET_LOG("\tLoading BorderStyle.");
		string errorPreamble("\t\tWhile loading border style for widget type '" 
			+ string(WidgetTypeNames[widgetType]) + "'\n\t\t\t");
		string type;
		BorderType bt = BorderType::INVALID;
		if (luaScript.getStringField("Type", type)) {
			type = luaScript.getStringField("Type");
		} else {
			if (!src) {
				string msg = errorPreamble + " error reading 'Type'";
				WIDGET_LOG(msg);
				g_logger.logError(msg);
				return;
			} else {
				bt = src->m_type;
				WIDGET_LOG("\t\tType not specified, copying from default");
			}
		}
		if (bt == BorderType::INVALID) {
			bt = BorderTypeNames.match(type.c_str());
		}
		if (bt == BorderType::INVALID) {
			string msg = errorPreamble + " border type is invalid '" + type + "'";
			WIDGET_LOG(msg);
			g_logger.logError(msg);
			bt = BorderType::NONE;
		} else {
			WIDGET_LOG("\t\tType: " << BorderTypeNames[bt]);
		}
		style.m_type = bt;
		if (bt != BorderType::NONE) {
			if (bt != BorderType::TEXTURE) {
				Vec4i sizes;
				if (luaScript.getVec4iField("Sizes", sizes)) {
					WIDGET_LOG("\t\tSizes: left-top-right-bottom: " << sizes.x << "-" << sizes.y
						<< "-" << sizes.z << "-" << sizes.w);
					style.setSizes(sizes);
				} else {
					if (!src) {
						string msg = errorPreamble + " loading 'Sizes'";
						WIDGET_LOG(msg);
						g_logger.logError(msg);
					} else {
						for (int i=0; i < Border::COUNT; ++i) {
							style.m_sizes[i] = src->m_sizes[i];
						}
						WIDGET_LOG("\t\tSizes not specified, copying from default");
					}
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
					StringSet strings;
					if (luaScript.getStringSet("Colours", strings)) {
						string logBits;
						for (int i=0; i < numColours; ++i) {
							if (!strings[i].empty()) {
								int ndx = getColourIndex(strings[i]);
								if (ndx != -1) {
									style.m_colourIndices[i] = ndx;
									logBits += "'" + strings[i] + "'(ndx: " + intToStr(ndx) + ") ";
								} else {
									string msg = errorPreamble + " loading 'Colours' : colour named '" +
										strings[i] + "' was not found. do you have a matching addColour() ?";
									WIDGET_LOG(msg);
									g_logger.logError(msg);
								}
							} else {
								string msg = errorPreamble + " loading 'Colours' : insufficient named colours.";
								WIDGET_LOG(msg);
								g_logger.logError(msg);
							}
						}
						WIDGET_LOG("\t\tColours: " << logBits);
					} else {
						if (!src) {
							string msg = errorPreamble + " loading 'Colours'";
							WIDGET_LOG(msg);
							g_logger.logError(msg);
						} else {
							for (int i=0; i < 4; ++i) {
								style.m_colourIndices[i] = src->m_colourIndices[i];
							}
							WIDGET_LOG("No Colours supplied, using indices from default");
						}
					}
				}
			} else { // bt == BorderType::TEXTURE
				string tex;
				Vec2i sizes;
				int ndx = -1;
				if (luaScript.getStringField("Texture", tex)) {
					ndx = getTextureIndex(tex);
					if (ndx != -1) {
						WIDGET_LOG("\t\tTexture: '" << tex << "' @ ndx " << ndx);
					} else {
						WIDGET_LOG(errorPreamble << " texture named '" << tex << "' not found.");
						g_logger.logError(errorPreamble + " texture named '" + tex + "' not found.");
					}
				} else {
					style.setNone();
					WIDGET_LOG(errorPreamble << " loading 'Texture'");
					g_logger.logError(errorPreamble + " loading 'Texture'");
				}
				if (luaScript.getVec2iField("Sizes", sizes)) {
					WIDGET_LOG("\t\tSizes: side-corner: " << sizes.x << "-" << sizes.y);
				} else {
					style.setNone();
					WIDGET_LOG(errorPreamble << " loading 'Sizes'");
					g_logger.logError(errorPreamble + " loading 'Sizes'");
				}
				if (ndx != -1) {
					style.setImage(ndx, sizes.x, sizes.y);
				} else {
					style.setNone();
					g_logger.logError(errorPreamble + " texture '" + tex + "' is not known.");
				}	
			}
		}
		luaScript.popTable();
	} else {
		if (src) {
			style = *src;
			WIDGET_LOG( "\tNo BorderStyle specified, using Default." );
		} else {
			style.setNone();
			WIDGET_LOG( "\tNo BorderStyle specified, setting Default (None)." );
		}
	}
}

void WidgetConfig::loadBackgroundStyle(WidgetType widgetType, BackgroundStyle &style, BackgroundStyle *src) {
	if (luaScript.getTable("Background")) {
		WIDGET_LOG("\tLoading BackgroundStyle.");
		string errorPreamble("\t\tWhile loading background style for widget type '" 
			+ string(WidgetTypeNames[widgetType]) + "'\n\t\t\t");
		string bType;
		BackgroundType bt(BackgroundType::INVALID);
		if (luaScript.getStringField("Type", bType)) {
			bt = BackgroundTypeNames.match(bType.c_str());
			if (bt == BackgroundType::INVALID) {
				WIDGET_LOG(errorPreamble << " type '" << bType << "' is not valid!");
				g_logger.logError(errorPreamble + " type is not valid!");
				bt = BackgroundType::NONE;
			} else {
				WIDGET_LOG("\t\tType: " << BackgroundTypeNames[bt]);
				style.m_type = bt;
			}
		} else {
			if (src) {
				WIDGET_LOG("\t\tType not present, using Default");
				style.m_type = src->m_type;
			} else {
				WIDGET_LOG(errorPreamble << " error reading 'Type'");
				g_logger.logError(errorPreamble + " error reading 'Type'");
				style.m_type = BackgroundType::NONE;
			}
		}
		bool ib;
		if (luaScript.getBoolField("InsideBorders", ib)) {
			WIDGET_LOG( "\t\tInsideBorders set to " << (ib ? "true" : "false"));
			style.m_insideBorders = ib;
		} else {
			if (src) {
				WIDGET_LOG( "\t\tInsideBorders not specified, using Default." );
				style.m_insideBorders = src->m_insideBorders;
			} else {
				WIDGET_LOG( "\t\tInsideBorders not specified, setting Default false." );
				style.m_insideBorders = false;
			}
		}

		if (bt == BackgroundType::COLOUR || bt == BackgroundType::CUSTOM_COLOURS) {
			StringSet strings;
			if (luaScript.getStringSet("Colours", strings)) {
				string logBits;
				for (int i=0; i < 4; ++i) {
					if (!strings[i].empty()) {
						int ndx = getColourIndex(strings[i]);
						if (ndx != -1) {
							logBits += "'" + strings[i] + "'(ndx: " + intToStr(ndx) + ") ";
							style.m_colourIndices[i] = getColourIndex(strings[i]);
						} else {
							WIDGET_LOG( "\t\tError: Colour named '" << strings[i] << "' was not found." );
						}
					} else if ((bt == BackgroundType::COLOUR && !i) || bt == BackgroundType::CUSTOM_COLOURS) {
						WIDGET_LOG( "\t\tError: Insufficient colours." );
					}
				}
				WIDGET_LOG( "\t\tColours: " << logBits);
			} else {
				if (!src) {
					WIDGET_LOG(errorPreamble << " loading 'Colours'");
					g_logger.logError(errorPreamble + " loading 'Colours'");
				} else {
					WIDGET_LOG( "\t\tColours not present, using Default" );
					for (int i=0; i < 4; ++i) {
						style.m_colourIndices[i] = src->m_colourIndices[i];
					}
				}
			}
		}
		if (bt == BackgroundType::TEXTURE) {
			string tex;
			if (luaScript.getStringField("Texture", tex)) {
				int ndx = getTextureIndex(tex);
				if (ndx != -1) {
					style.m_imageIndex = ndx;
					WIDGET_LOG( "\t\tTexture: '" << tex << "' @ ndx " << ndx );
				} else {
					WIDGET_LOG( "\t\tError: Texture '" << tex << "' not found.");
					g_logger.logError(errorPreamble + " texture '" + tex + "' is not known.");
				}
			} else {
				if (!src) {
					WIDGET_LOG(errorPreamble << " loading 'Texture'");
					g_logger.logError(errorPreamble + " loading 'Texture'");
				} else {
					WIDGET_LOG( "\t\tTexture not present, using Default" );
					style.m_imageIndex = src->m_imageIndex;
				}
			}
		}
		luaScript.popTable();
	} else {
		if (src) {
			style = *src;
			WIDGET_LOG( "\tNo BackgroundStyle specified, using Default." );
		} else {
			style.setNone();
			WIDGET_LOG( "\tNo BackgroundStyle specified, setting Default (None)." );
		}
	}
}

void WidgetConfig::loadHighLightStyle(WidgetType widgetType, HighLightStyle &style, HighLightStyle *src) {
	if (luaScript.getTable("HighLight")) {
		WIDGET_LOG("\tLoading HighLightStyle.");
		string tmp;
		if (luaScript.getStringField("Type", tmp)) {
			tmp = luaScript.getStringField("Type");
		} else {
			if (src) {
				WIDGET_LOG( "\t\tType not specified, using Default." );
				style = *src;
			} else {
				WIDGET_LOG( "\t\tType not specified, setting Default (none)." );
				style = HighLightStyle();
			}
			tmp = "none";
		}
		HighLightType t = HighLightTypeNames.match(tmp.c_str());
		if (t == HighLightType::INVALID) {
			if (src) {
				WIDGET_LOG( "\t\tError: Type '" << tmp << "' is not valid, using Default." );
				style.m_type = src->m_type;
			} else {
				WIDGET_LOG( "\t\tError: Type '" << tmp << "' is not valid, setting Default (none)." );
				style = HighLightStyle();
			}
		} else {
			WIDGET_LOG( "\t\tType: " << HighLightTypeNames[t] );
			style.m_type = t;
		}
		if (style.m_type != HighLightType::NONE) {
			string name;
			if (luaScript.getStringField("Colour", name)) {
				int ndx = getColourIndex(name);
				if (ndx != -1) {
					WIDGET_LOG( "\t\tHighlight Colour: '" << name << "' @ ndx " << ndx );
					style.m_colourIndex = ndx;
				} else {
					WIDGET_LOG("\t\tError loading HighLightStyle, colour named '" << name << "' not found");
				}
			} else {
				WIDGET_LOG("\t\tNo Colour specified, using white.");
				style.m_colourIndex = getColourIndex(Colour(255u, 255u, 255u, 255u));
			}
		}
		luaScript.popTable();
	} else {
		if (src) {
			WIDGET_LOG("\tNo HighLight table found, using Default");
			style = *src;
		} else {
			WIDGET_LOG("\tNo HighLight table found, setting Default (none)");
			style = HighLightStyle();
		}
	}
}

void WidgetConfig::loadTextStyle(WidgetType widgetType, TextStyle &style, TextStyle *src) {
	if (luaScript.getTable("Text")) {
		WIDGET_LOG( "\tLoading TextStyle.");
		string name, size, colour, shadowColour;
		StringSet fontSet;

		// font(s)
		if (luaScript.getStringSet("Font", fontSet)) { 
			style.m_smallFontIndex = getFontIndex(fontSet[0]);
			if (style.m_smallFontIndex == -1) {
				WIDGET_LOG( "\t\tError: font named '" << fontSet[0] << "' not found" );
			} else {
				WIDGET_LOG( "\t\tSmall Font: '" << fontSet[0] << "' @ ndx " << style.m_smallFontIndex );
			}
			style.m_fontIndex = getFontIndex(fontSet[1]);
			if (style.m_fontIndex == -1) {
				WIDGET_LOG( "\t\tError: font named '" << fontSet[1] << "' not found" );
			} else {
				WIDGET_LOG( "\t\tMedium Font: '" << fontSet[1] << "' @ ndx " << style.m_fontIndex );
			}
			style.m_largeFontIndex = getFontIndex(fontSet[2]);
			if (style.m_largeFontIndex == -1) {
				WIDGET_LOG( "\t\tError: font named '" << fontSet[2] << "' not found" );
			} else {
				WIDGET_LOG( "\t\tLarge Font: '" << fontSet[2] << "' @ ndx " << style.m_largeFontIndex );
			}
		} else if (luaScript.getStringField("Font", name)) {
			style.m_fontIndex = style.m_smallFontIndex = style.m_largeFontIndex = getFontIndex(name);
			if (style.m_fontIndex == -1) {
				WIDGET_LOG( "\t\tError: font named '" << name << "' not found" );
			} else {
				WIDGET_LOG( "\t\tFont: '" << name << "' @ ndx " << style.m_fontIndex );
			}
		} else {
			if (src) {
				WIDGET_LOG( "\t\tFont not specified, using Default." );
				style.m_fontIndex = src->m_fontIndex;
				style.m_smallFontIndex = src->m_smallFontIndex;
				style.m_largeFontIndex = src->m_largeFontIndex;
			} else {
				WIDGET_LOG( "\t\tError: font named '" << name << "' not found" );
				style.m_fontIndex = style.m_smallFontIndex = style.m_largeFontIndex = -1;
			}
		}
		if (luaScript.getStringField("Colour", colour)) {
			style.m_colourIndex = getColourIndex(colour);
			if (style.m_colourIndex != -1) {
				WIDGET_LOG( "\t\tColour: '" << colour << "' @ ndx " << style.m_colourIndex );
			} else {
				WIDGET_LOG( "\t\tError: Colour '" << colour << "' not found");
			}
		} else {
			if (src) {
				style.m_colourIndex = src->m_colourIndex;
			}
		}
		bool shadow;
		if (luaScript.getBoolField("Shadow", shadow)) {
			WIDGET_LOG( "\t\tShadow: " << (shadow ? "true" : "false") );
			style.m_shadow = shadow;
		} else {
			if (src) {
				style.m_shadow = src->m_shadow;
				WIDGET_LOG( "\t\tShadow not specified, using Default." );
			} else {
				style.m_shadow = false;
				WIDGET_LOG( "\t\tShadow not specified, setting Default to false." );
			}
		}
		if (style.m_shadow) {
			if (luaScript.getStringField("ShadowColour", shadowColour)) {
				style.m_shadowColourIndex = getColourIndex(shadowColour);
				if (style.m_shadowColourIndex != -1) {
					WIDGET_LOG( "\t\tShadowColour: '" << shadowColour << " @ ndx " << style.m_shadowColourIndex );
				} else {
					WIDGET_LOG( "\t\tError: ShadowColour: '" << shadowColour << " not found." );
				}
			} else {
				if (src) {
					WIDGET_LOG( "\t\tShadowColour not specified using Default." );
					style.m_shadowColourIndex = src->m_shadowColourIndex;
				} else {
					WIDGET_LOG( "\t\tError: ShadowColour not specified." );
					style.m_shadowColourIndex = -1;
				}
			}
		}
		luaScript.popTable();
	} else {
		if (src) {
			WIDGET_LOG( "\tNo Text table found, using Default." );
			style = *src;
		} else {
			WIDGET_LOG( "\tNo Text table found, setting Default (none)." );
			style = TextStyle();
		}
	}
}

void WidgetConfig::loadOverlayStyle(WidgetType widgetType, OverlayStyle &style, OverlayStyle *src) {
	if (luaScript.getTable("Overlay")) {
		WIDGET_LOG( "\tLoading Overlay" );
		string name;
		if (luaScript.getStringField("Texture", name)) {
			int ndx = getTextureIndex(name);
			if (ndx != -1) {
				style.m_tex = ndx;
				WIDGET_LOG("\t\tOverlay texture: '" << name << "' @ ndx " << ndx);
			} else {
				style.m_tex = -1;
				// don't treat "" as error, can be used to turn overlay off in non-default stats
				if (!name.empty()) {
					WIDGET_LOG("\t\tError: the texture named '" << name << "' was not found.");
					g_logger.logError("Error: the texture named '" + name + "' was not found.");
				} else {
					WIDGET_LOG("\t\tOverlay texture set to none.");
				}
			}
		} else {
			WIDGET_LOG("\t\tError, overlay does not specify texture");
			style.m_tex = -1;
		}
		bool ib;
		if (luaScript.getBoolField("InsideBorders", ib)) {
			WIDGET_LOG( "\t\tInsideBorders set to " << (ib ? "true" : "false"));
			style.m_insideBorders = ib;
		} else {
			if (src) {
				WIDGET_LOG( "\t\tInsideBorders not specified, using Default." );
				style.m_insideBorders = src->m_insideBorders;
			} else {
				WIDGET_LOG( "\t\tInsideBorders not specified, setting Default false." );
				style.m_insideBorders = false;
			}
		}
		luaScript.popTable();
	} else {
		if (src) {
			WIDGET_LOG( "\tOverlay not specified, using Default." );
			style = *src;
		} else {
			WIDGET_LOG( "\tOverlay not specified, setting Default (none)." );
			style = OverlayStyle();
		}
	}
}

string getErrMsg(const string &type) {
	return "Error loading widget.cfg : styles for " + type + " could not be loaded.";
}

bool WidgetConfig::loadStyles(const char *tableName, WidgetType type, bool glob) {
	string name = formatString(WidgetTypeNames[type]);
	WIDGET_LOG( "====================================================" );
	WIDGET_LOG( "Loading styles for widget type '" << name << "'" );
	WIDGET_LOG( "====================================================" );
	if ((glob && luaScript.getGlobal(tableName)) || (!glob && luaScript.getTable(tableName))) {
		WIDGET_LOG( "Default state." );
		if (luaScript.getTable("Default")) { // load default style (must be present)
			loadBorderStyle(type, m_styles[type][WidgetState::NORMAL].borderStyle());
			loadBackgroundStyle(type, m_styles[type][WidgetState::NORMAL].backgroundStyle());
			loadHighLightStyle(type, m_styles[type][WidgetState::NORMAL].highLightStyle());
			loadTextStyle(type, m_styles[type][WidgetState::NORMAL].textStyle());
			loadOverlayStyle(type, m_styles[type][WidgetState::NORMAL].overlayStyle());
			assert(luaScript.checkType(LuaType::TABLE));
			luaScript.popTable();
		} else {
			WIDGET_LOG( "\tError loading styles for widget type '" << tableName << "' no Default style." );
		}

		// anything missing from States get the default styles
		BorderStyle *normBorderStyle = &m_styles[type][WidgetState::NORMAL].borderStyle();
		BackgroundStyle *normBackgroundStyle = &m_styles[type][WidgetState::NORMAL].backgroundStyle();
		HighLightStyle *normHighLightStyle = &m_styles[type][WidgetState::NORMAL].highLightStyle();
		TextStyle *normTextStyle = &m_styles[type][WidgetState::NORMAL].textStyle();
		OverlayStyle *normOverlayStyle = &m_styles[type][WidgetState::NORMAL].overlayStyle();

		if (luaScript.getTable("States")) { // load States if present
			for (WidgetState state(1); state < WidgetState::COUNT; ++state) {
				string name = formatString(WidgetStateNames[state]);
				WIDGET_LOG( "State: " << name );
				if (luaScript.getTable(name.c_str())) { // load state style, with 'fallbacks'
					WIDGET_LOG( "\ttable found for widget type '" << tableName << "' in state '"
						<< name << "'." );
					loadBorderStyle(type, m_styles[type][state].borderStyle(), normBorderStyle);
					loadBackgroundStyle(type, m_styles[type][state].backgroundStyle(), normBackgroundStyle);
					loadHighLightStyle(type, m_styles[type][state].highLightStyle(), normHighLightStyle);
					loadTextStyle(type, m_styles[type][state].textStyle(), normTextStyle);
					loadOverlayStyle(type, m_styles[type][state].overlayStyle(), normOverlayStyle);
					luaScript.popTable();
				} else { // no style for this state, just copy default
					WIDGET_LOG( "\tno table found for widget type '" << tableName << "' in state '"
						<< name << "'. Using 'Default'" );
					m_styles[type][state].borderStyle() = *normBorderStyle;
					m_styles[type][state].backgroundStyle() = *normBackgroundStyle;
					m_styles[type][state].highLightStyle() = *normHighLightStyle;
					m_styles[type][state].textStyle() = *normTextStyle;
					m_styles[type][state].overlayStyle() = *normOverlayStyle;
				}
			}
			luaScript.popTable();
		} else { // no States specified, just copy default to all others			
			WIDGET_LOG( "No 'States' table found for widget type '" << tableName << "'."
				<< " Using 'Default' for all states." );
			for (WidgetState state(1); state < WidgetState::COUNT; ++state) {
				m_styles[type][state].borderStyle() = *normBorderStyle;
				m_styles[type][state].backgroundStyle() = *normBackgroundStyle;
				m_styles[type][state].highLightStyle() = *normHighLightStyle;
				m_styles[type][state].textStyle() = *normTextStyle;
				m_styles[type][state].overlayStyle() = *normOverlayStyle;
			}
		}
		luaScript.popTable();
		return true;
	} else {
		WIDGET_LOG( "Error loading styles for widget type '" << tableName << "' table not found." );
		g_logger.logError(getErrMsg(tableName));
		return false;
	}
}

WidgetConfig::WidgetConfig() {
}

void WidgetConfig::load() {
	if (!fileExists("data/core/widget.cfg")) {
		string msg = "Oh noes! widget.cfg was not found, that's a fatal error :-(";
		cout << "\n" << msg << "\n\n";
		g_logger.logError(msg);
		throw runtime_error(msg);
	}

	luaScript.startUp();
	luaScript.atPanic(ScriptManager::panicFunc);
	
#	define GUI_FUNC(x) luaScript.registerFunction(GuiScript::x, #x)
	GUI_FUNC(setDefaultFont);
	GUI_FUNC(setOverlayTexture);
	GUI_FUNC(setMouseTexture);
	GUI_FUNC(addColour);
	GUI_FUNC(loadTexture);
	GUI_FUNC(loadFont);

	FileOps *f = g_fileFactory.getFileOps();
	f->openRead("data/core/widget.cfg");
	int size = f->fileSize();
	char *someLua = new char[size + 1];
	f->read(someLua, size, 1);
	someLua[size] = '\0';
	delete f;
	bool ok = false;
	if (luaScript.luaDoLine(someLua)) {
		ok = true;
	} else {
		string errMsg = luaScript.getLastError();
		cout << errMsg;
		ok = false;
		g_logger.logError(errMsg);
	}
	//f = g_fileFactory.getFileOps();
	//f->openWrite("widget_cfg.txt");
	//f->write(someLua, size, 1);
	//f->close();
	//delete f;
	delete [] someLua;

	if (!ok) {
		string msg = "Oh noes! Error loading widget.cfg, that's a fatal error :-(";
		cout << "\n" << msg << "\n\n";
		g_logger.logError(msg);
		throw runtime_error(msg);
	}

	loadStyles("StaticWidget", WidgetType::STATIC_WIDGET);
	loadStyles("Button", WidgetType::BUTTON);

	if (luaScript.getGlobal("CheckBox")) {
		loadStyles("UnChecked", WidgetType::CHECK_BOX, false);
		loadStyles("Checked", WidgetType::CHECK_BOX_CHK, false);
		luaScript.popTable();
	}
	loadStyles("TextBox", WidgetType::TEXT_BOX);
	loadStyles("ListItem", WidgetType::LIST_ITEM);
	loadStyles("ListBox", WidgetType::LIST_BOX);
	loadStyles("DropList", WidgetType::DROP_LIST);
	loadStyles("ScrollBar", WidgetType::SCROLL_BAR);
	loadStyles("Slider", WidgetType::SLIDER);
	loadStyles("TitleBar", WidgetType::TITLE_BAR);
	loadStyles("GameWidgetFrame", WidgetType::GAME_WIDGET_FRAME);
	loadStyles("MessageBox", WidgetType::MESSAGE_BOX);
	//loadStyles("ToolTip", WidgetType::TOOL_TIP);
	loadStyles("ToolTip", WidgetType::TOOLTIP);
	if (luaScript.getGlobal("ToolTip")) {
		loadStyles("Header", WidgetType::TOOLTIP_HEADER, false);
		loadStyles("Main", WidgetType::TOOLTIP_MAIN, false);
		loadStyles("Item", WidgetType::TOOLTIP_ITEM, false);
		loadStyles("RequirementMet", WidgetType::TOOLTIP_REQ_OK, false);
		loadStyles("RequirementNotMet", WidgetType::TOOLTIP_REQ_NOK, false);
		luaScript.popTable();
	}
	loadStyles("ScrollBarButtonUp", WidgetType::SCROLLBAR_BTN_UP);
	loadStyles("ScrollBarButtonDown", WidgetType::SCROLLBAR_BTN_DOWN);
	loadStyles("ScrollBarButtonLeft", WidgetType::SCROLLBAR_BTN_LEFT);
	loadStyles("ScrollBarButtonRight", WidgetType::SCROLLBAR_BTN_RIGHT);
	loadStyles("ScrollBarVerticalShaft", WidgetType::SCROLLBAR_VERT_SHAFT);
	loadStyles("ScrollBarVerticalThumb", WidgetType::SCROLLBAR_VERT_THUMB);
	loadStyles("ScrollBarHorizontalShaft",WidgetType::SCROLLBAR_HORIZ_SHAFT);
	loadStyles("ScrollBarHorizontalThumb", WidgetType::SCROLLBAR_HORIZ_THUMB);
	loadStyles("SliderVerticalThumb", WidgetType::SLIDER_VERT_THUMB);
	loadStyles("SliderVerticalShaft", WidgetType::SLIDER_VERT_SHAFT);
	loadStyles("SliderHorizontalThumb", WidgetType::SLIDER_HORIZ_THUMB);
	loadStyles("SliderHorizontalShaft", WidgetType::SLIDER_HORIZ_SHAFT);
	loadStyles("TitleBarCloseButton", WidgetType::TITLE_BAR_CLOSE);
	loadStyles("TitleBarRollUpButton", WidgetType::TITLE_BAR_ROLL_UP);
	loadStyles("TitleBarRollDownButton", WidgetType::TITLE_BAR_ROLL_DOWN);
	loadStyles("TitleBarExpandButton", WidgetType::TITLE_BAR_EXPAND);
	loadStyles("TitleBarShrinkButton", WidgetType::TITLE_BAR_SHRINK);
	loadStyles("ColourPicker", WidgetType::COLOUR_PICKER);
	loadStyles("ColourButton", WidgetType::COLOUR_BUTTON);
	loadStyles("TickerTape", WidgetType::TICKER_TAPE);
	loadStyles("InfoWidget", WidgetType::INFO_WIDGET);
	loadStyles("Logger", WidgetType::LOGGER_WIDGET);
	if (luaScript.getGlobal("Logger")) {
		loadStyles("Header", WidgetType::LOG_HEADER, false);
		loadStyles("LogLine", WidgetType::LOG_LINE, false);
		luaScript.popTable();
	}
	loadStyles("CodeView", WidgetType::CODE_VIEW);
	loadStyles("CodeEdit", WidgetType::CODE_EDIT);
	loadStyles("ResourceBar", WidgetType::RESOURCE_BAR);
	loadStyles("MiniMap", WidgetType::MINIMAP);
	loadStyles("Display", WidgetType::DISPLAY);
	loadStyles("Console", WidgetType::CONSOLE);
	loadStyles("GameStats", WidgetType::GAME_STATS);
	loadStyles("OptionsPanel", WidgetType::OPTIONS_PANEL);

	loadStyles("TestWidget", WidgetType::TEST_WIDGET);
	if (luaScript.getGlobal("TestWidget")) {
		loadStyles("Header", WidgetType::TEST_WIDGET_HEADER, false);
		loadStyles("MainBit", WidgetType::TEST_WIDGET_MAINBIT, false);
		loadStyles("CodeBit", WidgetType::TEST_WIDGET_CODEBIT, false);
		luaScript.popTable();
	}
	WIDGET_LOG( "====================================================" );
	WIDGET_LOG( "==       Finished Loading widget styles           ==" );
	WIDGET_LOG( "====================================================" );

	luaScript.close();
}

int WidgetConfig::getDefaultItemHeight() const {
	return int(m_fonts[m_defaultFonts[FontUsage::MENU]]->getMetrics()->getHeight() * 1.3f);
}

int WidgetConfig::getColourIndex(const Colour &c) {
	const int &n = m_colours.size();
	for (int i = 0; i < n; ++i) {
		if (m_colours[i] == c) {
			return i;
		}
	}
	m_colours.push_back(c);
	return n;
}

int WidgetConfig::getColourIndex(const Vec3f &c) {
	return getColourIndex(Colour(uint8(c.r * 255), uint8(c.g * 255), uint8(c.b * 255), 255u));
}

int WidgetConfig::getColourIndex(const Vec4f &c) {
	return getColourIndex(Colour(uint8(c.r * 255), uint8(c.g * 255), uint8(c.b * 255), uint8(c.a * 255)));
}
//
//int WidgetConfig::getFontIndex(const Font *f) {
//	const int &n = m_fonts.size();
//	for (int i = 0; i < n; ++i) {
//		if (m_fonts[i] == f) {
//			return i;
//		}
//	}
//	m_fonts.push_back(f);
//	return n;
//}

int WidgetConfig::getTextureIndex(const Texture2D *t) {
	const int &n = m_textures.size();
	for (int i = 0; i < n; ++i) {
		if (m_textures[i] == t) {
			return i;
		}
	}
	m_textures.push_back(t);
	return n;
}

int WidgetConfig::getColourIndex(const string &name) const {
	IndexByNameMap::const_iterator it = m_namedColours.find(name);
	if (it != m_namedColours.end()) {
		return it->second;
	}
	return -1;
}

int WidgetConfig::getTextureIndex(const string &name) const {
	IndexByNameMap::const_iterator it = m_namedTextures.find(name);
	if (it != m_namedTextures.end()) {
		return it->second;
	}
	return -1;
}

int WidgetConfig::getFontIndex(const string &name) const {
	IndexByNameMap::const_iterator it = m_namedFonts.find(name);
	if (it != m_namedFonts.end()) {
		return it->second;
	}
	return -1;
}

}}


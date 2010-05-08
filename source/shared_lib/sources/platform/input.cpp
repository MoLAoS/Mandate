// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include <stdexcept>
#include <sstream>

#include "input.h"

#include "leak_dumper.h"

using std::stringstream;
using std::range_error;

namespace Shared { namespace Platform {

// =======================================
// class Input
// =======================================

#ifdef USE_SDL
#pragma pack(push, 1)
const unsigned char Input::native2mb[Input::NATIVE_MOUSE_BUTTON_LAST + 1] = {
	MouseButton::UNKNOWN,	// 0
	MouseButton::LEFT,		// 1 SDL_BUTTON_LEFT
	MouseButton::MIDDLE,	// 2 SDL_BUTTON_MIDDLE
	MouseButton::RIGHT,		// 3 SDL_BUTTON_RIGHT
	MouseButton::WHEEL_UP,		// 4 SDL_BUTTON_WHEELUP
	MouseButton::WHEEL_DOWN,	// 5 SDL_BUTTON_WHEELDOWN
	MouseButton::BUTTON_X1,	// 6 SDL_BUTTON_X1
	MouseButton::BUTTON_X2	// 7 SDL_BUTTON_X2
};

const unsigned char Input::mb2native[MouseButton::COUNT] = {
	0,						// MouseButton::UNKNOWN
	SDL_BUTTON_LEFT,		// MouseButton::LEFT
	SDL_BUTTON_MIDDLE,		// MouseButton::MIDDLE
	SDL_BUTTON_RIGHT,		// MouseButton::RIGHT
#ifdef SDL_BUTTON_WHEELUP
	SDL_BUTTON_WHEELUP,		// MouseButton::WHEEL_UP
	SDL_BUTTON_WHEELDOWN,	// MouseButton::WHEEL_DOWN
#else
	0,						// MouseButton::WHEEL_UP
	0,						// MouseButton::WHEEL_DOWN
#endif
#ifdef SDL_BUTTON_X1
	SDL_BUTTON_X1,			// MouseButton::BUTTON_X1
	SDL_BUTTON_X2			// MouseButton::BUTTON_X2
#else
	0,						// MouseButton::BUTTON_X1
	0						// MouseButton::BUTTON_X2
#endif
};

const short Input::native2kc[Input::NATIVE_KEY_CODE_LAST + 1] = {
	KeyCode::UNKNOWN,			// 0 SDLK_UNKNOWN
	KeyCode::UNKNOWN,			// 1
	KeyCode::UNKNOWN,			// 2
	KeyCode::UNKNOWN,			// 3
	KeyCode::UNKNOWN,			// 4
	KeyCode::UNKNOWN,			// 5
	KeyCode::UNKNOWN,			// 6
	KeyCode::UNKNOWN,			// 7
	KeyCode::BACK_SPACE,		// 8 SDLK_BACKSPACE
	KeyCode::TAB,				// 9 SDLK_TAB
	KeyCode::UNKNOWN,			// 10
	KeyCode::UNKNOWN,			// 11
	KeyCode::CLEAR,				// 12 SDLK_CLEAR
	KeyCode::RETURN,			// 13 SDLK_RETURN
	KeyCode::UNKNOWN,			// 14
	KeyCode::UNKNOWN,			// 15
	KeyCode::UNKNOWN,			// 16
	KeyCode::UNKNOWN,			// 17
	KeyCode::UNKNOWN,			// 18
	KeyCode::PAUSE,				// 19 SDLK_PAUSE
	KeyCode::UNKNOWN,			// 20
	KeyCode::UNKNOWN,			// 21
	KeyCode::UNKNOWN,			// 22
	KeyCode::UNKNOWN,			// 23
	KeyCode::UNKNOWN,			// 24
	KeyCode::UNKNOWN,			// 25
	KeyCode::UNKNOWN,			// 26
	KeyCode::ESCAPE,			// 27 SDLK_ESCAPE
	KeyCode::UNKNOWN,			// 28
	KeyCode::UNKNOWN,			// 29
	KeyCode::UNKNOWN,			// 30
	KeyCode::UNKNOWN,			// 31
	KeyCode::SPACE,				// 32 SDLK_SPACE
	KeyCode::EXCLAIM,			// 33 SDLK_EXCLAIM
	KeyCode::DOUBLE_QUOTE,		// 34 SDLK_QUOTEDBL
	KeyCode::HASH,				// 35 SDLK_HASH
	KeyCode::DOLLAR,			// 36 SDLK_DOLLAR
	KeyCode::UNKNOWN,			// 37
	KeyCode::AMPERSAND,			// 38 SDLK_AMPERSAND
	KeyCode::QUOTE,				// 39 SDLK_QUOTE
	KeyCode::LEFT_PAREN,		// 40 SDLK_LEFTPAREN
	KeyCode::RIGHT_PAREN,		// 41 SDLK_RIGHTPAREN
	KeyCode::ASTERISK,			// 42 SDLK_ASTERISK
	KeyCode::PLUS,				// 43 SDLK_PLUS
	KeyCode::COMMA,				// 44 SDLK_COMMA
	KeyCode::MINUS,				// 45 SDLK_MINUS
	KeyCode::PERIOD,			// 46 SDLK_PERIOD
	KeyCode::SLASH,				// 47 SDLK_SLASH
	KeyCode::ZERO,				// 48 SDLK_0
	KeyCode::ONE,				// 49 SDLK_1
	KeyCode::TWO,				// 50 SDLK_2
	KeyCode::THREE,				// 51 SDLK_3
	KeyCode::FOUR,				// 52 SDLK_4
	KeyCode::FIVE,				// 53 SDLK_5
	KeyCode::SIX,				// 54 SDLK_6
	KeyCode::SEVEN,				// 55 SDLK_7
	KeyCode::EIGHT,				// 56 SDLK_8
	KeyCode::NINE,				// 57 SDLK_9
	KeyCode::COLON,				// 58 SDLK_COLON
	KeyCode::SEMI_COLON,		// 59 SDLK_SEMICOLON
	KeyCode::LESS,				// 60 SDLK_LESS
	KeyCode::EQUAL,				// 61 SDLK_EQUALS
	KeyCode::GREATER,			// 62 SDLK_GREATER
	KeyCode::QUESTION_MARK,		// 63 SDLK_QUESTION
	KeyCode::AT,				// 64 SDLK_AT
	KeyCode::A,					// 65 'A'
	KeyCode::B,					// 66 'B'
	KeyCode::C,					// 67 'C'
	KeyCode::D,					// 68 'D'
	KeyCode::E,					// 69 'E'
	KeyCode::F,					// 70 'F'
	KeyCode::G,					// 71 'G'
	KeyCode::H,					// 72 'H'
	KeyCode::I,					// 73 'I'
	KeyCode::J,					// 74 'J'
	KeyCode::K,					// 75 'K'
	KeyCode::L,					// 76 'L'
	KeyCode::M,					// 77 'M'
	KeyCode::N,					// 78 'N'
	KeyCode::O,					// 79 'O'
	KeyCode::P,					// 80 'P'
	KeyCode::Q,					// 81 'Q'
	KeyCode::R,					// 82 'R'
	KeyCode::S,					// 83 'S'
	KeyCode::T,					// 84 'T'
	KeyCode::U,					// 85 'U'
	KeyCode::V,					// 86 'V'
	KeyCode::W,					// 87 'W'
	KeyCode::X,					// 88 'X'
	KeyCode::Y,					// 89 'Y'
	KeyCode::Z,					// 90 'Z'
	KeyCode::LEFT_BRACKET,		// 91 SDLK_LEFTBRACKET
	KeyCode::BACK_SLASH,		// 92 SDLK_BACKSLASH
	KeyCode::RIGHT_BRACKET,		// 93 SDLK_RIGHTBRACKET
	KeyCode::CARET,				// 94 SDLK_CARET
	KeyCode::UNDER_SCORE,		// 95 SDLK_UNDERSCORE
	KeyCode::BACK_QUOTE,		// 96 SDLK_BACKQUOTE
	KeyCode::A,					// 97 SDLK_a
	KeyCode::B,					// 98 SDLK_b
	KeyCode::C,					// 99 SDLK_c
	KeyCode::D,					// 100 SDLK_d
	KeyCode::E,					// 101 SDLK_e
	KeyCode::F,					// 102 SDLK_f
	KeyCode::G,					// 103 SDLK_g
	KeyCode::H,					// 104 SDLK_h
	KeyCode::I,					// 105 SDLK_i
	KeyCode::J,					// 106 SDLK_j
	KeyCode::K,					// 107 SDLK_k
	KeyCode::L,					// 108 SDLK_l
	KeyCode::M,					// 109 SDLK_m
	KeyCode::N,					// 110 SDLK_n
	KeyCode::O,					// 111 SDLK_o
	KeyCode::P,					// 112 SDLK_p
	KeyCode::Q,					// 113 SDLK_q
	KeyCode::R,					// 114 SDLK_r
	KeyCode::S,					// 115 SDLK_s
	KeyCode::T,					// 116 SDLK_t
	KeyCode::U,					// 117 SDLK_u
	KeyCode::V,					// 118 SDLK_v
	KeyCode::W,					// 119 SDLK_w
	KeyCode::X,					// 120 SDLK_x
	KeyCode::Y,					// 121 SDLK_y
	KeyCode::Z,					// 122 SDLK_z
	KeyCode::UNKNOWN,			// 123
	KeyCode::UNKNOWN,			// 124
	KeyCode::UNKNOWN,			// 125
	KeyCode::UNKNOWN,			// 126
	KeyCode::DELETE_,			// 127 SDLK_DELETE
	KeyCode::UNKNOWN,			// 128
	KeyCode::UNKNOWN,			// 129
	KeyCode::UNKNOWN,			// 130
	KeyCode::UNKNOWN,			// 131
	KeyCode::UNKNOWN,			// 132
	KeyCode::UNKNOWN,			// 133
	KeyCode::UNKNOWN,			// 134
	KeyCode::UNKNOWN,			// 135
	KeyCode::UNKNOWN,			// 136
	KeyCode::UNKNOWN,			// 137
	KeyCode::UNKNOWN,			// 138
	KeyCode::UNKNOWN,			// 139
	KeyCode::UNKNOWN,			// 140
	KeyCode::UNKNOWN,			// 141
	KeyCode::UNKNOWN,			// 142
	KeyCode::UNKNOWN,			// 143
	KeyCode::UNKNOWN,			// 144
	KeyCode::UNKNOWN,			// 145
	KeyCode::UNKNOWN,			// 146
	KeyCode::UNKNOWN,			// 147
	KeyCode::UNKNOWN,			// 148
	KeyCode::UNKNOWN,			// 149
	KeyCode::UNKNOWN,			// 150
	KeyCode::UNKNOWN,			// 151
	KeyCode::UNKNOWN,			// 152
	KeyCode::UNKNOWN,			// 153
	KeyCode::UNKNOWN,			// 154
	KeyCode::UNKNOWN,			// 155
	KeyCode::UNKNOWN,			// 156
	KeyCode::UNKNOWN,			// 157
	KeyCode::UNKNOWN,			// 158
	KeyCode::UNKNOWN,			// 159
	KeyCode::WORLD_0,			// 160 SDLK_WORLD_0
	KeyCode::WORLD_1,			// 161 SDLK_WORLD_1
	KeyCode::WORLD_2,			// 162 SDLK_WORLD_2
	KeyCode::WORLD_3,			// 163 SDLK_WORLD_3
	KeyCode::WORLD_4,			// 164 SDLK_WORLD_4
	KeyCode::WORLD_5,			// 165 SDLK_WORLD_5
	KeyCode::WORLD_6,			// 166 SDLK_WORLD_6
	KeyCode::WORLD_7,			// 167 SDLK_WORLD_7
	KeyCode::WORLD_8,			// 168 SDLK_WORLD_8
	KeyCode::WORLD_9,			// 169 SDLK_WORLD_9
	KeyCode::WORLD_10,			// 170 SDLK_WORLD_10
	KeyCode::WORLD_11,			// 171 SDLK_WORLD_11
	KeyCode::WORLD_12,			// 172 SDLK_WORLD_12
	KeyCode::WORLD_13,			// 173 SDLK_WORLD_13
	KeyCode::WORLD_14,			// 174 SDLK_WORLD_14
	KeyCode::WORLD_15,			// 175 SDLK_WORLD_15
	KeyCode::WORLD_16,			// 176 SDLK_WORLD_16
	KeyCode::WORLD_17,			// 177 SDLK_WORLD_17
	KeyCode::WORLD_18,			// 178 SDLK_WORLD_18
	KeyCode::WORLD_19,			// 179 SDLK_WORLD_19
	KeyCode::WORLD_20,			// 180 SDLK_WORLD_20
	KeyCode::WORLD_21,			// 181 SDLK_WORLD_21
	KeyCode::WORLD_22,			// 182 SDLK_WORLD_22
	KeyCode::WORLD_23,			// 183 SDLK_WORLD_23
	KeyCode::WORLD_24,			// 184 SDLK_WORLD_24
	KeyCode::WORLD_25,			// 185 SDLK_WORLD_25
	KeyCode::WORLD_26,			// 186 SDLK_WORLD_26
	KeyCode::WORLD_27,			// 187 SDLK_WORLD_27
	KeyCode::WORLD_28,			// 188 SDLK_WORLD_28
	KeyCode::WORLD_29,			// 189 SDLK_WORLD_29
	KeyCode::WORLD_30,			// 190 SDLK_WORLD_30
	KeyCode::WORLD_31,			// 191 SDLK_WORLD_31
	KeyCode::WORLD_32,			// 192 SDLK_WORLD_32
	KeyCode::WORLD_33,			// 193 SDLK_WORLD_33
	KeyCode::WORLD_34,			// 194 SDLK_WORLD_34
	KeyCode::WORLD_35,			// 195 SDLK_WORLD_35
	KeyCode::WORLD_36,			// 196 SDLK_WORLD_36
	KeyCode::WORLD_37,			// 197 SDLK_WORLD_37
	KeyCode::WORLD_38,			// 198 SDLK_WORLD_38
	KeyCode::WORLD_39,			// 199 SDLK_WORLD_39
	KeyCode::WORLD_40,			// 200 SDLK_WORLD_40
	KeyCode::WORLD_41,			// 201 SDLK_WORLD_41
	KeyCode::WORLD_42,			// 202 SDLK_WORLD_42
	KeyCode::WORLD_43,			// 203 SDLK_WORLD_43
	KeyCode::WORLD_44,			// 204 SDLK_WORLD_44
	KeyCode::WORLD_45,			// 205 SDLK_WORLD_45
	KeyCode::WORLD_46,			// 206 SDLK_WORLD_46
	KeyCode::WORLD_47,			// 207 SDLK_WORLD_47
	KeyCode::WORLD_48,			// 208 SDLK_WORLD_48
	KeyCode::WORLD_49,			// 209 SDLK_WORLD_49
	KeyCode::WORLD_50,			// 210 SDLK_WORLD_50
	KeyCode::WORLD_51,			// 211 SDLK_WORLD_51
	KeyCode::WORLD_52,			// 212 SDLK_WORLD_52
	KeyCode::WORLD_53,			// 213 SDLK_WORLD_53
	KeyCode::WORLD_54,			// 214 SDLK_WORLD_54
	KeyCode::WORLD_55,			// 215 SDLK_WORLD_55
	KeyCode::WORLD_56,			// 216 SDLK_WORLD_56
	KeyCode::WORLD_57,			// 217 SDLK_WORLD_57
	KeyCode::WORLD_58,			// 218 SDLK_WORLD_58
	KeyCode::WORLD_59,			// 219 SDLK_WORLD_59
	KeyCode::WORLD_60,			// 220 SDLK_WORLD_60
	KeyCode::WORLD_61,			// 221 SDLK_WORLD_61
	KeyCode::WORLD_62,			// 222 SDLK_WORLD_62
	KeyCode::WORLD_63,			// 223 SDLK_WORLD_63
	KeyCode::WORLD_64,			// 224 SDLK_WORLD_64
	KeyCode::WORLD_65,			// 225 SDLK_WORLD_65
	KeyCode::WORLD_66,			// 226 SDLK_WORLD_66
	KeyCode::WORLD_67,			// 227 SDLK_WORLD_67
	KeyCode::WORLD_68,			// 228 SDLK_WORLD_68
	KeyCode::WORLD_69,			// 229 SDLK_WORLD_69
	KeyCode::WORLD_70,			// 230 SDLK_WORLD_70
	KeyCode::WORLD_71,			// 231 SDLK_WORLD_71
	KeyCode::WORLD_72,			// 232 SDLK_WORLD_72
	KeyCode::WORLD_73,			// 233 SDLK_WORLD_73
	KeyCode::WORLD_74,			// 234 SDLK_WORLD_74
	KeyCode::WORLD_75,			// 235 SDLK_WORLD_75
	KeyCode::WORLD_76,			// 236 SDLK_WORLD_76
	KeyCode::WORLD_77,			// 237 SDLK_WORLD_77
	KeyCode::WORLD_78,			// 238 SDLK_WORLD_78
	KeyCode::WORLD_79,			// 239 SDLK_WORLD_79
	KeyCode::WORLD_80,			// 240 SDLK_WORLD_80
	KeyCode::WORLD_81,			// 241 SDLK_WORLD_81
	KeyCode::WORLD_82,			// 242 SDLK_WORLD_82
	KeyCode::WORLD_83,			// 243 SDLK_WORLD_83
	KeyCode::WORLD_84,			// 244 SDLK_WORLD_84
	KeyCode::WORLD_85,			// 245 SDLK_WORLD_85
	KeyCode::WORLD_86,			// 246 SDLK_WORLD_86
	KeyCode::WORLD_87,			// 247 SDLK_WORLD_87
	KeyCode::WORLD_88,			// 248 SDLK_WORLD_88
	KeyCode::WORLD_89,			// 249 SDLK_WORLD_89
	KeyCode::WORLD_90,			// 250 SDLK_WORLD_90
	KeyCode::WORLD_91,			// 251 SDLK_WORLD_91
	KeyCode::WORLD_92,			// 252 SDLK_WORLD_92
	KeyCode::WORLD_93,			// 253 SDLK_WORLD_93
	KeyCode::WORLD_94,			// 254 SDLK_WORLD_94
	KeyCode::WORLD_95,			// 255 SDLK_WORLD_95
	KeyCode::KEYPAD_0,			// 256 SDLK_KP0
	KeyCode::KEYPAD_1,			// 257 SDLK_KP1
	KeyCode::KEYPAD_2,			// 258 SDLK_KP2
	KeyCode::KEYPAD_3,			// 259 SDLK_KP3
	KeyCode::KEYPAD_4,			// 260 SDLK_KP4
	KeyCode::KEYPAD_5,			// 261 SDLK_KP5
	KeyCode::KEYPAD_6,			// 262 SDLK_KP6
	KeyCode::KEYPAD_7,			// 263 SDLK_KP7
	KeyCode::KEYPAD_8,			// 264 SDLK_KP8
	KeyCode::KEYPAD_9,			// 265 SDLK_KP9
	KeyCode::KEYPAD_PERIOD,		// 266 SDLK_KP_PERIOD
	KeyCode::KEYPAD_DIVIDE,		// 267 SDLK_KP_DIVIDE
	KeyCode::KEYPAD_MULTIPLY,	// 268 SDLK_KP_MULTIPLY
	KeyCode::KEYPAD_MINUS,		// 269 SDLK_KP_MINUS
	KeyCode::KEYPAD_PLUS,		// 270 SDLK_KP_PLUS
	KeyCode::KEYPAD_ENTER,		// 271 SDLK_KP_ENTER
	KeyCode::KEYPAD_EQUAL,		// 272 SDLK_KP_EQUALS
	KeyCode::ARROW_UP,			// 273 SDLK_UP
	KeyCode::ARROW_DOWN,		// 274 SDLK_DOWN
	KeyCode::ARROW_RIGHT,		// 275 SDLK_RIGHT
	KeyCode::ARROW_LEFT,		// 276 SDLK_LEFT
	KeyCode::INSERT,			// 277 SDLK_INSERT
	KeyCode::HOME,				// 278 SDLK_HOME
	KeyCode::END,				// 279 SDLK_END
	KeyCode::PAGE_UP,			// 280 SDLK_PAGEUP
	KeyCode::PAGE_DOWN,			// 281 SDLK_PAGEDOWN
	KeyCode::FUNCTION_1,		// 282 SDLK_F1
	KeyCode::FUNCTION_2,		// 283 SDLK_F2
	KeyCode::FUNCTION_3,		// 284 SDLK_F3
	KeyCode::FUNCTION_4,		// 285 SDLK_F4
	KeyCode::FUNCTION_5,		// 286 SDLK_F5
	KeyCode::FUNCTION_6,		// 287 SDLK_F6
	KeyCode::FUNCTION_7,		// 288 SDLK_F7
	KeyCode::FUNCTION_8,		// 289 SDLK_F8
	KeyCode::FUNCTION_9,		// 290 SDLK_F9
	KeyCode::FUNCTION_10,		// 291 SDLK_F10
	KeyCode::FUNCTION_11,		// 292 SDLK_F11
	KeyCode::FUNCTION_12,		// 293 SDLK_F12
	KeyCode::FUNCTION_13,		// 294 SDLK_F13
	KeyCode::FUNCTION_14,		// 295 SDLK_F14
	KeyCode::FUNCTION_15,		// 296 SDLK_F15
	KeyCode::UNKNOWN,			// 297
	KeyCode::UNKNOWN,			// 298
	KeyCode::UNKNOWN,			// 299
	KeyCode::NUM_LOCK,			// 300 SDLK_NUMLOCK
	KeyCode::CAPS_LOCK,			// 301 SDLK_CAPSLOCK
	KeyCode::SCROLL_LOCK,		// 302 SDLK_SCROLLOCK
	KeyCode::RIGHT_SHIFT,		// 303 SDLK_RSHIFT
	KeyCode::LEFT_SHIFT,		// 304 SDLK_LSHIFT
	KeyCode::RIGHT_CTRL,		// 305 SDLK_RCTRL
	KeyCode::LEFT_CTRL,			// 306 SDLK_LCTRL
	KeyCode::RIGHT_ALT,			// 307 SDLK_RALT
	KeyCode::LEFT_ALT,			// 308 SDLK_LALT
	KeyCode::RIGHT_META,		// 309 SDLK_RMETA
	KeyCode::LEFT_META,			// 310 SDLK_LMETA
	KeyCode::LEFT_SUPER,		// 311 SDLK_LSUPER
	KeyCode::RIGHT_SUPER,		// 312 SDLK_RSUPER
	KeyCode::MODE,				// 313 SDLK_MODE
	KeyCode::COMPOSE,			// 314 SDLK_COMPOSE
	KeyCode::HELP,				// 315 SDLK_HELP
	KeyCode::PRINT,				// 316 SDLK_PRINT
	KeyCode::SYS_REQ,			// 317 SDLK_SYSREQ
	KeyCode::BREAK,				// 318 SDLK_BREAK
	KeyCode::MENU,				// 319 SDLK_MENU
	KeyCode::POWER,				// 320 SDLK_POWER
	KeyCode::EURO,				// 321 SDLK_EURO
	KeyCode::UNDO				// 322 SDLK_UNDO
};

const NativeKeyCodeCompact Input::kc2native[KeyCode::COUNT] = {
	SDLK_UNKNOWN,		// KeyCode::NONE
	SDLK_UNKNOWN,		// KeyCode::UNKNOWN
	SDLK_UNKNOWN,		// KeyCode::L_BUTTON
	SDLK_UNKNOWN,		// KeyCode::R_BUTTON
	SDLK_UNKNOWN,		// KeyCode::M_BUTTON
	SDLK_UNKNOWN,		// KeyCode::X_BUTTON1
	SDLK_UNKNOWN,		// KeyCode::X_BUTTON2
	SDLK_BACKSPACE,		// KeyCode::BACK_SPACE
	SDLK_TAB,			// KeyCode::TAB
	SDLK_CLEAR,			// KeyCode::CLEAR
	SDLK_RETURN,		// KeyCode::RETURN
	SDLK_PAUSE,			// KeyCode::PAUSE
	SDLK_UNKNOWN,		// KeyCode::IME_KANA
	SDLK_UNKNOWN,		// KeyCode::IME_HANGUL
	SDLK_UNKNOWN,		// KeyCode::IME_JUNJA
	SDLK_UNKNOWN,		// KeyCode::IME_FINAL
	SDLK_UNKNOWN,		// KeyCode::IME_HANJA
	SDLK_UNKNOWN,		// KeyCode::IME_KANJI
	SDLK_ESCAPE,		// KeyCode::ESCAPE
	SDLK_UNKNOWN,		// KeyCode::IME_CONVERT
	SDLK_UNKNOWN,		// KeyCode::IME_NON_CONVERT
	SDLK_UNKNOWN,		// KeyCode::IME_ACCEPT
	SDLK_UNKNOWN,		// KeyCode::IME_MODE_CHANGE
	SDLK_UNKNOWN,		// KeyCode::IME_PROCESS
	SDLK_SPACE,			// KeyCode::SPACE
	SDLK_EXCLAIM,		// KeyCode::EXCLAIM
	SDLK_QUOTEDBL,		// KeyCode::DOUBLE_QUOTE
	SDLK_HASH,			// KeyCode::HASH
	SDLK_DOLLAR,		// KeyCode::DOLLAR
	SDLK_UNKNOWN,		// KeyCode::PERCENT
	SDLK_AMPERSAND,		// KeyCode::AMPERSAND
	SDLK_QUOTE,			// KeyCode::QUOTE
	SDLK_LEFTPAREN,		// KeyCode::LEFT_PAREN
	SDLK_RIGHTPAREN,	// KeyCode::RIGHT_PAREN
	SDLK_ASTERISK,		// KeyCode::ASTERISK
	SDLK_PLUS,			// KeyCode::PLUS
	SDLK_COMMA,			// KeyCode::COMMA
	SDLK_MINUS,			// KeyCode::MINUS
	SDLK_PERIOD,		// KeyCode::PERIOD
	SDLK_SLASH,			// KeyCode::SLASH
	SDLK_0,				// KeyCode::ZERO
	SDLK_1,				// KeyCode::ONE
	SDLK_2,				// KeyCode::TWO
	SDLK_3,				// KeyCode::THREE
	SDLK_4,				// KeyCode::FOUR
	SDLK_5,				// KeyCode::FIVE
	SDLK_6,				// KeyCode::SIX
	SDLK_7,				// KeyCode::SEVEN
	SDLK_8,				// KeyCode::EIGHT
	SDLK_9,				// KeyCode::NINE
	SDLK_COLON,			// KeyCode::COLON
	SDLK_SEMICOLON,		// KeyCode::SEMI_COLON
	SDLK_LESS,			// KeyCode::LESS
	SDLK_EQUALS,		// KeyCode::EQUAL
	SDLK_GREATER,		// KeyCode::GREATER
	SDLK_QUESTION,		// KeyCode::QUESTION
	SDLK_AT,			// KeyCode::AT
	'A',				// KeyCode::A
	'B',				// KeyCode::B
	'C',				// KeyCode::C
	'D',				// KeyCode::D
	'E',				// KeyCode::E
	'F',				// KeyCode::F
	'G',				// KeyCode::G
	'H',				// KeyCode::H
	'I',				// KeyCode::I
	'J',				// KeyCode::J
	'K',				// KeyCode::K
	'L',				// KeyCode::L
	'M',				// KeyCode::M
	'N',				// KeyCode::N
	'O',				// KeyCode::O
	'P',				// KeyCode::P
	'Q',				// KeyCode::Q
	'R',				// KeyCode::R
	'S',				// KeyCode::S
	'T',				// KeyCode::T
	'U',				// KeyCode::U
	'V',				// KeyCode::V
	'W',				// KeyCode::W
	'X',				// KeyCode::X
	'Y',				// KeyCode::Y
	'Z',				// KeyCode::Z
	SDLK_LEFTBRACKET,	// KeyCode::LEFT_BRACKET
	SDLK_BACKSLASH,		// KeyCode::BACK_SLASH
	SDLK_RIGHTBRACKET,	// KeyCode::RIGHT_BRACKET
	SDLK_CARET,			// KeyCode::CARET
	SDLK_UNDERSCORE,	// KeyCode::UNDER_SCORE
	SDLK_BACKQUOTE,		// KeyCode::BACK_QUOTE
	SDLK_DELETE,		// KeyCode::DELETE_
	SDLK_WORLD_0,		// KeyCode::WORLD_0
	SDLK_WORLD_1,		// KeyCode::WORLD_1
	SDLK_WORLD_2,		// KeyCode::WORLD_2
	SDLK_WORLD_3,		// KeyCode::WORLD_3
	SDLK_WORLD_4,		// KeyCode::WORLD_4
	SDLK_WORLD_5,		// KeyCode::WORLD_5
	SDLK_WORLD_6,		// KeyCode::WORLD_6
	SDLK_WORLD_7,		// KeyCode::WORLD_7
	SDLK_WORLD_8,		// KeyCode::WORLD_8
	SDLK_WORLD_9,		// KeyCode::WORLD_9
	SDLK_WORLD_10,		// KeyCode::WORLD_10
	SDLK_WORLD_11,		// KeyCode::WORLD_11
	SDLK_WORLD_12,		// KeyCode::WORLD_12
	SDLK_WORLD_13,		// KeyCode::WORLD_13
	SDLK_WORLD_14,		// KeyCode::WORLD_14
	SDLK_WORLD_15,		// KeyCode::WORLD_15
	SDLK_WORLD_16,		// KeyCode::WORLD_16
	SDLK_WORLD_17,		// KeyCode::WORLD_17
	SDLK_WORLD_18,		// KeyCode::WORLD_18
	SDLK_WORLD_19,		// KeyCode::WORLD_19
	SDLK_WORLD_20,		// KeyCode::WORLD_20
	SDLK_WORLD_21,		// KeyCode::WORLD_21
	SDLK_WORLD_22,		// KeyCode::WORLD_22
	SDLK_WORLD_23,		// KeyCode::WORLD_23
	SDLK_WORLD_24,		// KeyCode::WORLD_24
	SDLK_WORLD_25,		// KeyCode::WORLD_25
	SDLK_WORLD_26,		// KeyCode::WORLD_26
	SDLK_WORLD_27,		// KeyCode::WORLD_27
	SDLK_WORLD_28,		// KeyCode::WORLD_28
	SDLK_WORLD_29,		// KeyCode::WORLD_29
	SDLK_WORLD_30,		// KeyCode::WORLD_30
	SDLK_WORLD_31,		// KeyCode::WORLD_31
	SDLK_WORLD_32,		// KeyCode::WORLD_32
	SDLK_WORLD_33,		// KeyCode::WORLD_33
	SDLK_WORLD_34,		// KeyCode::WORLD_34
	SDLK_WORLD_35,		// KeyCode::WORLD_35
	SDLK_WORLD_36,		// KeyCode::WORLD_36
	SDLK_WORLD_37,		// KeyCode::WORLD_37
	SDLK_WORLD_38,		// KeyCode::WORLD_38
	SDLK_WORLD_39,		// KeyCode::WORLD_39
	SDLK_WORLD_40,		// KeyCode::WORLD_40
	SDLK_WORLD_41,		// KeyCode::WORLD_41
	SDLK_WORLD_42,		// KeyCode::WORLD_42
	SDLK_WORLD_43,		// KeyCode::WORLD_43
	SDLK_WORLD_44,		// KeyCode::WORLD_44
	SDLK_WORLD_45,		// KeyCode::WORLD_45
	SDLK_WORLD_46,		// KeyCode::WORLD_46
	SDLK_WORLD_47,		// KeyCode::WORLD_47
	SDLK_WORLD_48,		// KeyCode::WORLD_48
	SDLK_WORLD_49,		// KeyCode::WORLD_49
	SDLK_WORLD_50,		// KeyCode::WORLD_50
	SDLK_WORLD_51,		// KeyCode::WORLD_51
	SDLK_WORLD_52,		// KeyCode::WORLD_52
	SDLK_WORLD_53,		// KeyCode::WORLD_53
	SDLK_WORLD_54,		// KeyCode::WORLD_54
	SDLK_WORLD_55,		// KeyCode::WORLD_55
	SDLK_WORLD_56,		// KeyCode::WORLD_56
	SDLK_WORLD_57,		// KeyCode::WORLD_57
	SDLK_WORLD_58,		// KeyCode::WORLD_58
	SDLK_WORLD_59,		// KeyCode::WORLD_59
	SDLK_WORLD_60,		// KeyCode::WORLD_60
	SDLK_WORLD_61,		// KeyCode::WORLD_61
	SDLK_WORLD_62,		// KeyCode::WORLD_62
	SDLK_WORLD_63,		// KeyCode::WORLD_63
	SDLK_WORLD_64,		// KeyCode::WORLD_64
	SDLK_WORLD_65,		// KeyCode::WORLD_65
	SDLK_WORLD_66,		// KeyCode::WORLD_66
	SDLK_WORLD_67,		// KeyCode::WORLD_67
	SDLK_WORLD_68,		// KeyCode::WORLD_68
	SDLK_WORLD_69,		// KeyCode::WORLD_69
	SDLK_WORLD_70,		// KeyCode::WORLD_70
	SDLK_WORLD_71,		// KeyCode::WORLD_71
	SDLK_WORLD_72,		// KeyCode::WORLD_72
	SDLK_WORLD_73,		// KeyCode::WORLD_73
	SDLK_WORLD_74,		// KeyCode::WORLD_74
	SDLK_WORLD_75,		// KeyCode::WORLD_75
	SDLK_WORLD_76,		// KeyCode::WORLD_76
	SDLK_WORLD_77,		// KeyCode::WORLD_77
	SDLK_WORLD_78,		// KeyCode::WORLD_78
	SDLK_WORLD_79,		// KeyCode::WORLD_79
	SDLK_WORLD_80,		// KeyCode::WORLD_80
	SDLK_WORLD_81,		// KeyCode::WORLD_81
	SDLK_WORLD_82,		// KeyCode::WORLD_82
	SDLK_WORLD_83,		// KeyCode::WORLD_83
	SDLK_WORLD_84,		// KeyCode::WORLD_84
	SDLK_WORLD_85,		// KeyCode::WORLD_85
	SDLK_WORLD_86,		// KeyCode::WORLD_86
	SDLK_WORLD_87,		// KeyCode::WORLD_87
	SDLK_WORLD_88,		// KeyCode::WORLD_88
	SDLK_WORLD_89,		// KeyCode::WORLD_89
	SDLK_WORLD_90,		// KeyCode::WORLD_90
	SDLK_WORLD_91,		// KeyCode::WORLD_91
	SDLK_WORLD_92,		// KeyCode::WORLD_92
	SDLK_WORLD_93,		// KeyCode::WORLD_93
	SDLK_WORLD_94,		// KeyCode::WORLD_94
	SDLK_WORLD_95,		// KeyCode::WORLD_95
	SDLK_KP0,			// KeyCode::KEYPAD_0
	SDLK_KP1,			// KeyCode::KEYPAD_1
	SDLK_KP2,			// KeyCode::KEYPAD_2
	SDLK_KP3,			// KeyCode::KEYPAD_3
	SDLK_KP4,			// KeyCode::KEYPAD_4
	SDLK_KP5,			// KeyCode::KEYPAD_5
	SDLK_KP6,			// KeyCode::KEYPAD_6
	SDLK_KP7,			// KeyCode::KEYPAD_7
	SDLK_KP8,			// KeyCode::KEYPAD_8
	SDLK_KP9,			// KeyCode::KEYPAD_9
	SDLK_KP_PERIOD,		// KeyCode::KEYPAD_PERIOD
	SDLK_KP_DIVIDE,		// KeyCode::KEYPAD_DIVIDE
	SDLK_KP_MULTIPLY,	// KeyCode::KEYPAD_MULTIPLY
	SDLK_KP_MINUS,		// KeyCode::KEYPAD_MINUS
	SDLK_KP_PLUS,		// KeyCode::KEYPAD_PLUS
	SDLK_KP_ENTER,		// KeyCode::KEYPAD_ENTER
	SDLK_KP_EQUALS,		// KeyCode::KEYPAD_EQUAL
	SDLK_UP,			// KeyCode::ARROW_UP
	SDLK_DOWN,			// KeyCode::ARROW_DOWN
	SDLK_RIGHT,			// KeyCode::ARROW_RIGHT
	SDLK_LEFT,			// KeyCode::ARROW_LEFT
	SDLK_INSERT,		// KeyCode::INSERT
	SDLK_HOME,			// KeyCode::HOME
	SDLK_END,			// KeyCode::END
	SDLK_PAGEUP,		// KeyCode::PAGE_UP
	SDLK_PAGEDOWN,		// KeyCode::PAGE_DOWN
	SDLK_F1,			// KeyCode::FUNCTION_1
	SDLK_F2,			// KeyCode::FUNCTION_2
	SDLK_F3,			// KeyCode::FUNCTION_3
	SDLK_F4,			// KeyCode::FUNCTION_4
	SDLK_F5,			// KeyCode::FUNCTION_5
	SDLK_F6,			// KeyCode::FUNCTION_6
	SDLK_F7,			// KeyCode::FUNCTION_7
	SDLK_F8,			// KeyCode::FUNCTION_8
	SDLK_F9,			// KeyCode::FUNCTION_9
	SDLK_F10,			// KeyCode::FUNCTION_10
	SDLK_F11,			// KeyCode::FUNCTION_11
	SDLK_F12,			// KeyCode::FUNCTION_12
	SDLK_F13,			// KeyCode::FUNCTION_13
	SDLK_F14,			// KeyCode::FUNCTION_14
	SDLK_F15,			// KeyCode::FUNCTION_15
	SDLK_UNKNOWN,		// KeyCode::FUNCTION_16
	SDLK_UNKNOWN,		// KeyCode::FUNCTION_17
	SDLK_UNKNOWN,		// KeyCode::FUNCTION_18
	SDLK_UNKNOWN,		// KeyCode::FUNCTION_19
	SDLK_UNKNOWN,		// KeyCode::FUNCTION_20
	SDLK_UNKNOWN,		// KeyCode::FUNCTION_21
	SDLK_UNKNOWN,		// KeyCode::FUNCTION_22
	SDLK_UNKNOWN,		// KeyCode::FUNCTION_23
	SDLK_UNKNOWN,		// KeyCode::FUNCTION_24
	SDLK_NUMLOCK,		// KeyCode::NUM_LOCK
	SDLK_CAPSLOCK,		// KeyCode::CAPS_LOCK
	SDLK_SCROLLOCK,		// KeyCode::SCROLL_LOCK
	SDLK_LSHIFT,		// KeyCode::LEFT_SHIFT
	SDLK_RSHIFT,		// KeyCode::RIGHT_SHIFT
	SDLK_LCTRL,			// KeyCode::LEFT_CTRL
	SDLK_RCTRL,			// KeyCode::RIGHT_CTRL
	SDLK_LALT,			// KeyCode::LEFT_ALT
	SDLK_RALT,			// KeyCode::RIGHT_ALT
	SDLK_LMETA,			// KeyCode::LEFT_META
	SDLK_RMETA,			// KeyCode::RIGHT_META
	SDLK_RSUPER,		// KeyCode::RIGHT_SUPER
	SDLK_LSUPER,		// KeyCode::LEFT_SUPER
	SDLK_MODE,			// KeyCode::MODE
	SDLK_COMPOSE,		// KeyCode::COMPOSE
	SDLK_HELP,			// KeyCode::HELP
	SDLK_PRINT,			// KeyCode::PRINT
	SDLK_SYSREQ,		// KeyCode::SYS_REQ
	SDLK_BREAK,			// KeyCode::BREAK
	SDLK_MENU,			// KeyCode::MENU
	SDLK_POWER,			// KeyCode::POWER
	SDLK_EURO,			// KeyCode::EURO
	SDLK_UNDO,			// KeyCode::UNDO
	SDLK_UNKNOWN,		// KeyCode::BROWSER_BACK
	SDLK_UNKNOWN,		// KeyCode::BROWSER_FORWARD
	SDLK_UNKNOWN,		// KeyCode::BROWSER_REFRESH
	SDLK_UNKNOWN,		// KeyCode::BROWSER_STOP
	SDLK_UNKNOWN,		// KeyCode::BROWSER_SEARCH
	SDLK_UNKNOWN,		// KeyCode::BROWSER_FAVORITES
	SDLK_UNKNOWN,		// KeyCode::BROWSER_HOME
	SDLK_UNKNOWN,		// KeyCode::VOLUME_MUTE
	SDLK_UNKNOWN,		// KeyCode::VOLUME_DOWN
	SDLK_UNKNOWN,		// KeyCode::VOLUME_UP
	SDLK_UNKNOWN,		// KeyCode::MEDIA_NEXT
	SDLK_UNKNOWN,		// KeyCode::MEDIA_PREV
	SDLK_UNKNOWN,		// KeyCode::MEDIA_STOP
	SDLK_UNKNOWN,		// KeyCode::MEDIA_PLAY_PAUSE
	SDLK_UNKNOWN,		// KeyCode::LAUNCH_MAIL
	SDLK_UNKNOWN,		// KeyCode::LAUNCH_MEDIA
	SDLK_UNKNOWN,		// KeyCode::LAUNCH_APP1
	SDLK_UNKNOWN,		// KeyCode::LAUNCH_APP2
	SDLK_UNKNOWN,		// KeyCode::PLAY
	SDLK_UNKNOWN		// KeyCode::ZOOM
};

#pragma pack(pop)

void Input::verifyTranslationTables() {
	// verify integrity of SDL to KeyCode table
	assert(native2kc[SDLK_UNKNOWN] == KeyCode::UNKNOWN);
	assert(native2kc[SDLK_BACKSPACE] == KeyCode::BACK_SPACE);
	assert(native2kc[SDLK_TAB] == KeyCode::TAB);
	assert(native2kc[SDLK_CLEAR] == KeyCode::CLEAR);
	assert(native2kc[SDLK_RETURN] == KeyCode::RETURN);
	assert(native2kc[SDLK_PAUSE] == KeyCode::PAUSE);
	assert(native2kc[SDLK_ESCAPE] == KeyCode::ESCAPE);
	assert(native2kc[SDLK_SPACE] == KeyCode::SPACE);
	assert(native2kc[SDLK_EXCLAIM] == KeyCode::EXCLAIM);
	assert(native2kc[SDLK_QUOTEDBL] == KeyCode::DOUBLE_QUOTE);
	assert(native2kc[SDLK_HASH] == KeyCode::HASH);
	assert(native2kc[SDLK_DOLLAR] == KeyCode::DOLLAR);
	assert(native2kc[SDLK_AMPERSAND] == KeyCode::AMPERSAND);
	assert(native2kc[SDLK_QUOTE] == KeyCode::QUOTE);
	assert(native2kc[SDLK_LEFTPAREN] == KeyCode::LEFT_PAREN);
	assert(native2kc[SDLK_RIGHTPAREN] == KeyCode::RIGHT_PAREN);
	assert(native2kc[SDLK_ASTERISK] == KeyCode::ASTERISK);
	assert(native2kc[SDLK_PLUS] == KeyCode::PLUS);
	assert(native2kc[SDLK_COMMA] == KeyCode::COMMA);
	assert(native2kc[SDLK_MINUS] == KeyCode::MINUS);
	assert(native2kc[SDLK_PERIOD] == KeyCode::PERIOD);
	assert(native2kc[SDLK_SLASH] == KeyCode::SLASH);
	assert(native2kc[SDLK_0] == KeyCode::ZERO);
	assert(native2kc[SDLK_1] == KeyCode::ONE);
	assert(native2kc[SDLK_2] == KeyCode::TWO);
	assert(native2kc[SDLK_3] == KeyCode::THREE);
	assert(native2kc[SDLK_4] == KeyCode::FOUR);
	assert(native2kc[SDLK_5] == KeyCode::FIVE);
	assert(native2kc[SDLK_6] == KeyCode::SIX);
	assert(native2kc[SDLK_7] == KeyCode::SEVEN);
	assert(native2kc[SDLK_8] == KeyCode::EIGHT);
	assert(native2kc[SDLK_9] == KeyCode::NINE);
	assert(native2kc[SDLK_COLON] == KeyCode::COLON);
	assert(native2kc[SDLK_SEMICOLON] == KeyCode::SEMI_COLON);
	assert(native2kc[SDLK_LESS] == KeyCode::LESS);
	assert(native2kc[SDLK_EQUALS] == KeyCode::EQUAL);
	assert(native2kc[SDLK_GREATER] == KeyCode::GREATER);
	assert(native2kc[SDLK_QUESTION] == KeyCode::QUESTION);
	assert(native2kc[SDLK_AT] == KeyCode::AT);
	assert(native2kc['A'] == KeyCode::A);
	assert(native2kc['B'] == KeyCode::B);
	assert(native2kc['C'] == KeyCode::C);
	assert(native2kc['D'] == KeyCode::D);
	assert(native2kc['E'] == KeyCode::E);
	assert(native2kc['F'] == KeyCode::F);
	assert(native2kc['G'] == KeyCode::G);
	assert(native2kc['H'] == KeyCode::H);
	assert(native2kc['I'] == KeyCode::I);
	assert(native2kc['J'] == KeyCode::J);
	assert(native2kc['K'] == KeyCode::K);
	assert(native2kc['L'] == KeyCode::L);
	assert(native2kc['M'] == KeyCode::M);
	assert(native2kc['N'] == KeyCode::N);
	assert(native2kc['O'] == KeyCode::O);
	assert(native2kc['P'] == KeyCode::P);
	assert(native2kc['Q'] == KeyCode::Q);
	assert(native2kc['R'] == KeyCode::R);
	assert(native2kc['S'] == KeyCode::S);
	assert(native2kc['T'] == KeyCode::T);
	assert(native2kc['U'] == KeyCode::U);
	assert(native2kc['V'] == KeyCode::V);
	assert(native2kc['W'] == KeyCode::W);
	assert(native2kc['X'] == KeyCode::X);
	assert(native2kc['Y'] == KeyCode::Y);
	assert(native2kc['Z'] == KeyCode::Z);
	assert(native2kc[SDLK_LEFTBRACKET] == KeyCode::LEFT_BRACKET);
	assert(native2kc[SDLK_BACKSLASH] == KeyCode::BACK_SLASH);
	assert(native2kc[SDLK_RIGHTBRACKET] == KeyCode::RIGHT_BRACKET);
	assert(native2kc[SDLK_CARET] == KeyCode::CARET);
	assert(native2kc[SDLK_UNDERSCORE] == KeyCode::UNDER_SCORE);
	assert(native2kc[SDLK_BACKQUOTE] == KeyCode::BACK_QUOTE);
	assert(native2kc[SDLK_a] == KeyCode::A);
	assert(native2kc[SDLK_b] == KeyCode::B);
	assert(native2kc[SDLK_c] == KeyCode::C);
	assert(native2kc[SDLK_d] == KeyCode::D);
	assert(native2kc[SDLK_e] == KeyCode::E);
	assert(native2kc[SDLK_f] == KeyCode::F);
	assert(native2kc[SDLK_g] == KeyCode::G);
	assert(native2kc[SDLK_h] == KeyCode::H);
	assert(native2kc[SDLK_i] == KeyCode::I);
	assert(native2kc[SDLK_j] == KeyCode::J);
	assert(native2kc[SDLK_k] == KeyCode::K);
	assert(native2kc[SDLK_l] == KeyCode::L);
	assert(native2kc[SDLK_m] == KeyCode::M);
	assert(native2kc[SDLK_n] == KeyCode::N);
	assert(native2kc[SDLK_o] == KeyCode::O);
	assert(native2kc[SDLK_p] == KeyCode::P);
	assert(native2kc[SDLK_q] == KeyCode::Q);
	assert(native2kc[SDLK_r] == KeyCode::R);
	assert(native2kc[SDLK_s] == KeyCode::S);
	assert(native2kc[SDLK_t] == KeyCode::T);
	assert(native2kc[SDLK_u] == KeyCode::U);
	assert(native2kc[SDLK_v] == KeyCode::V);
	assert(native2kc[SDLK_w] == KeyCode::W);
	assert(native2kc[SDLK_x] == KeyCode::X);
	assert(native2kc[SDLK_y] == KeyCode::Y);
	assert(native2kc[SDLK_z] == KeyCode::Z);
	assert(native2kc[SDLK_DELETE] == KeyCode::DELETE_);
	assert(native2kc[SDLK_DELETE] == KeyCode::DELETE_);
	assert(native2kc[SDLK_WORLD_0] == KeyCode::WORLD_0);
	assert(native2kc[SDLK_WORLD_1] == KeyCode::WORLD_1);
	assert(native2kc[SDLK_WORLD_2] == KeyCode::WORLD_2);
	assert(native2kc[SDLK_WORLD_3] == KeyCode::WORLD_3);
	assert(native2kc[SDLK_WORLD_4] == KeyCode::WORLD_4);
	assert(native2kc[SDLK_WORLD_5] == KeyCode::WORLD_5);
	assert(native2kc[SDLK_WORLD_6] == KeyCode::WORLD_6);
	assert(native2kc[SDLK_WORLD_7] == KeyCode::WORLD_7);
	assert(native2kc[SDLK_WORLD_8] == KeyCode::WORLD_8);
	assert(native2kc[SDLK_WORLD_9] == KeyCode::WORLD_9);
	assert(native2kc[SDLK_WORLD_10] == KeyCode::WORLD_10);
	assert(native2kc[SDLK_WORLD_11] == KeyCode::WORLD_11);
	assert(native2kc[SDLK_WORLD_12] == KeyCode::WORLD_12);
	assert(native2kc[SDLK_WORLD_13] == KeyCode::WORLD_13);
	assert(native2kc[SDLK_WORLD_14] == KeyCode::WORLD_14);
	assert(native2kc[SDLK_WORLD_15] == KeyCode::WORLD_15);
	assert(native2kc[SDLK_WORLD_16] == KeyCode::WORLD_16);
	assert(native2kc[SDLK_WORLD_17] == KeyCode::WORLD_17);
	assert(native2kc[SDLK_WORLD_18] == KeyCode::WORLD_18);
	assert(native2kc[SDLK_WORLD_19] == KeyCode::WORLD_19);
	assert(native2kc[SDLK_WORLD_20] == KeyCode::WORLD_20);
	assert(native2kc[SDLK_WORLD_21] == KeyCode::WORLD_21);
	assert(native2kc[SDLK_WORLD_22] == KeyCode::WORLD_22);
	assert(native2kc[SDLK_WORLD_23] == KeyCode::WORLD_23);
	assert(native2kc[SDLK_WORLD_24] == KeyCode::WORLD_24);
	assert(native2kc[SDLK_WORLD_25] == KeyCode::WORLD_25);
	assert(native2kc[SDLK_WORLD_26] == KeyCode::WORLD_26);
	assert(native2kc[SDLK_WORLD_27] == KeyCode::WORLD_27);
	assert(native2kc[SDLK_WORLD_28] == KeyCode::WORLD_28);
	assert(native2kc[SDLK_WORLD_29] == KeyCode::WORLD_29);
	assert(native2kc[SDLK_WORLD_30] == KeyCode::WORLD_30);
	assert(native2kc[SDLK_WORLD_31] == KeyCode::WORLD_31);
	assert(native2kc[SDLK_WORLD_32] == KeyCode::WORLD_32);
	assert(native2kc[SDLK_WORLD_33] == KeyCode::WORLD_33);
	assert(native2kc[SDLK_WORLD_34] == KeyCode::WORLD_34);
	assert(native2kc[SDLK_WORLD_35] == KeyCode::WORLD_35);
	assert(native2kc[SDLK_WORLD_36] == KeyCode::WORLD_36);
	assert(native2kc[SDLK_WORLD_37] == KeyCode::WORLD_37);
	assert(native2kc[SDLK_WORLD_38] == KeyCode::WORLD_38);
	assert(native2kc[SDLK_WORLD_39] == KeyCode::WORLD_39);
	assert(native2kc[SDLK_WORLD_40] == KeyCode::WORLD_40);
	assert(native2kc[SDLK_WORLD_41] == KeyCode::WORLD_41);
	assert(native2kc[SDLK_WORLD_42] == KeyCode::WORLD_42);
	assert(native2kc[SDLK_WORLD_43] == KeyCode::WORLD_43);
	assert(native2kc[SDLK_WORLD_44] == KeyCode::WORLD_44);
	assert(native2kc[SDLK_WORLD_45] == KeyCode::WORLD_45);
	assert(native2kc[SDLK_WORLD_46] == KeyCode::WORLD_46);
	assert(native2kc[SDLK_WORLD_47] == KeyCode::WORLD_47);
	assert(native2kc[SDLK_WORLD_48] == KeyCode::WORLD_48);
	assert(native2kc[SDLK_WORLD_49] == KeyCode::WORLD_49);
	assert(native2kc[SDLK_WORLD_50] == KeyCode::WORLD_50);
	assert(native2kc[SDLK_WORLD_51] == KeyCode::WORLD_51);
	assert(native2kc[SDLK_WORLD_52] == KeyCode::WORLD_52);
	assert(native2kc[SDLK_WORLD_53] == KeyCode::WORLD_53);
	assert(native2kc[SDLK_WORLD_54] == KeyCode::WORLD_54);
	assert(native2kc[SDLK_WORLD_55] == KeyCode::WORLD_55);
	assert(native2kc[SDLK_WORLD_56] == KeyCode::WORLD_56);
	assert(native2kc[SDLK_WORLD_57] == KeyCode::WORLD_57);
	assert(native2kc[SDLK_WORLD_58] == KeyCode::WORLD_58);
	assert(native2kc[SDLK_WORLD_59] == KeyCode::WORLD_59);
	assert(native2kc[SDLK_WORLD_60] == KeyCode::WORLD_60);
	assert(native2kc[SDLK_WORLD_61] == KeyCode::WORLD_61);
	assert(native2kc[SDLK_WORLD_62] == KeyCode::WORLD_62);
	assert(native2kc[SDLK_WORLD_63] == KeyCode::WORLD_63);
	assert(native2kc[SDLK_WORLD_64] == KeyCode::WORLD_64);
	assert(native2kc[SDLK_WORLD_65] == KeyCode::WORLD_65);
	assert(native2kc[SDLK_WORLD_66] == KeyCode::WORLD_66);
	assert(native2kc[SDLK_WORLD_67] == KeyCode::WORLD_67);
	assert(native2kc[SDLK_WORLD_68] == KeyCode::WORLD_68);
	assert(native2kc[SDLK_WORLD_69] == KeyCode::WORLD_69);
	assert(native2kc[SDLK_WORLD_70] == KeyCode::WORLD_70);
	assert(native2kc[SDLK_WORLD_71] == KeyCode::WORLD_71);
	assert(native2kc[SDLK_WORLD_72] == KeyCode::WORLD_72);
	assert(native2kc[SDLK_WORLD_73] == KeyCode::WORLD_73);
	assert(native2kc[SDLK_WORLD_74] == KeyCode::WORLD_74);
	assert(native2kc[SDLK_WORLD_75] == KeyCode::WORLD_75);
	assert(native2kc[SDLK_WORLD_76] == KeyCode::WORLD_76);
	assert(native2kc[SDLK_WORLD_77] == KeyCode::WORLD_77);
	assert(native2kc[SDLK_WORLD_78] == KeyCode::WORLD_78);
	assert(native2kc[SDLK_WORLD_79] == KeyCode::WORLD_79);
	assert(native2kc[SDLK_WORLD_80] == KeyCode::WORLD_80);
	assert(native2kc[SDLK_WORLD_81] == KeyCode::WORLD_81);
	assert(native2kc[SDLK_WORLD_82] == KeyCode::WORLD_82);
	assert(native2kc[SDLK_WORLD_83] == KeyCode::WORLD_83);
	assert(native2kc[SDLK_WORLD_84] == KeyCode::WORLD_84);
	assert(native2kc[SDLK_WORLD_85] == KeyCode::WORLD_85);
	assert(native2kc[SDLK_WORLD_86] == KeyCode::WORLD_86);
	assert(native2kc[SDLK_WORLD_87] == KeyCode::WORLD_87);
	assert(native2kc[SDLK_WORLD_88] == KeyCode::WORLD_88);
	assert(native2kc[SDLK_WORLD_89] == KeyCode::WORLD_89);
	assert(native2kc[SDLK_WORLD_90] == KeyCode::WORLD_90);
	assert(native2kc[SDLK_WORLD_91] == KeyCode::WORLD_91);
	assert(native2kc[SDLK_WORLD_92] == KeyCode::WORLD_92);
	assert(native2kc[SDLK_WORLD_93] == KeyCode::WORLD_93);
	assert(native2kc[SDLK_WORLD_94] == KeyCode::WORLD_94);
	assert(native2kc[SDLK_WORLD_95] == KeyCode::WORLD_95);
	assert(native2kc[SDLK_KP0] == KeyCode::KEYPAD_0);
	assert(native2kc[SDLK_KP1] == KeyCode::KEYPAD_1);
	assert(native2kc[SDLK_KP2] == KeyCode::KEYPAD_2);
	assert(native2kc[SDLK_KP3] == KeyCode::KEYPAD_3);
	assert(native2kc[SDLK_KP4] == KeyCode::KEYPAD_4);
	assert(native2kc[SDLK_KP5] == KeyCode::KEYPAD_5);
	assert(native2kc[SDLK_KP6] == KeyCode::KEYPAD_6);
	assert(native2kc[SDLK_KP7] == KeyCode::KEYPAD_7);
	assert(native2kc[SDLK_KP8] == KeyCode::KEYPAD_8);
	assert(native2kc[SDLK_KP9] == KeyCode::KEYPAD_9);
	assert(native2kc[SDLK_KP_PERIOD] == KeyCode::KEYPAD_PERIOD);
	assert(native2kc[SDLK_KP_DIVIDE] == KeyCode::KEYPAD_DIVIDE);
	assert(native2kc[SDLK_KP_MULTIPLY] == KeyCode::KEYPAD_MULTIPLY);
	assert(native2kc[SDLK_KP_MINUS] == KeyCode::KEYPAD_MINUS);
	assert(native2kc[SDLK_KP_PLUS] == KeyCode::KEYPAD_PLUS);
	assert(native2kc[SDLK_KP_ENTER] == KeyCode::KEYPAD_ENTER);
	assert(native2kc[SDLK_KP_EQUALS] == KeyCode::KEYPAD_EQUAL);
	assert(native2kc[SDLK_UP] == KeyCode::ARROW_UP);
	assert(native2kc[SDLK_DOWN] == KeyCode::ARROW_DOWN);
	assert(native2kc[SDLK_RIGHT] == KeyCode::ARROW_RIGHT);
	assert(native2kc[SDLK_LEFT] == KeyCode::ARROW_LEFT);
	assert(native2kc[SDLK_INSERT] == KeyCode::INSERT);
	assert(native2kc[SDLK_HOME] == KeyCode::HOME);
	assert(native2kc[SDLK_END] == KeyCode::END);
	assert(native2kc[SDLK_PAGEUP] == KeyCode::PAGE_UP);
	assert(native2kc[SDLK_PAGEDOWN] == KeyCode::PAGE_DOWN);
	assert(native2kc[SDLK_F1] == KeyCode::FUNCTION_1);
	assert(native2kc[SDLK_F2] == KeyCode::FUNCTION_2);
	assert(native2kc[SDLK_F3] == KeyCode::FUNCTION_3);
	assert(native2kc[SDLK_F4] == KeyCode::FUNCTION_4);
	assert(native2kc[SDLK_F5] == KeyCode::FUNCTION_5);
	assert(native2kc[SDLK_F6] == KeyCode::FUNCTION_6);
	assert(native2kc[SDLK_F7] == KeyCode::FUNCTION_7);
	assert(native2kc[SDLK_F8] == KeyCode::FUNCTION_8);
	assert(native2kc[SDLK_F9] == KeyCode::FUNCTION_9);
	assert(native2kc[SDLK_F10] == KeyCode::FUNCTION_10);
	assert(native2kc[SDLK_F11] == KeyCode::FUNCTION_11);
	assert(native2kc[SDLK_F12] == KeyCode::FUNCTION_12);
	assert(native2kc[SDLK_F13] == KeyCode::FUNCTION_13);
	assert(native2kc[SDLK_F14] == KeyCode::FUNCTION_14);
	assert(native2kc[SDLK_F15] == KeyCode::FUNCTION_15);
	assert(native2kc[SDLK_NUMLOCK] == KeyCode::NUM_LOCK);
	assert(native2kc[SDLK_CAPSLOCK] == KeyCode::CAPS_LOCK);
	assert(native2kc[SDLK_SCROLLOCK] == KeyCode::SCROLL_LOCK);
	assert(native2kc[SDLK_RSHIFT] == KeyCode::RIGHT_SHIFT);
	assert(native2kc[SDLK_LSHIFT] == KeyCode::LEFT_SHIFT);
	assert(native2kc[SDLK_RCTRL] == KeyCode::RIGHT_CTRL);
	assert(native2kc[SDLK_LCTRL] == KeyCode::LEFT_CTRL);
	assert(native2kc[SDLK_RALT] == KeyCode::RIGHT_ALT);
	assert(native2kc[SDLK_LALT] == KeyCode::LEFT_ALT);
	assert(native2kc[SDLK_RMETA] == KeyCode::RIGHT_META);
	assert(native2kc[SDLK_LMETA] == KeyCode::LEFT_META);
	assert(native2kc[SDLK_LSUPER] == KeyCode::LEFT_SUPER);
	assert(native2kc[SDLK_RSUPER] == KeyCode::RIGHT_SUPER);
	assert(native2kc[SDLK_MODE] == KeyCode::MODE);
	assert(native2kc[SDLK_COMPOSE] == KeyCode::COMPOSE);
	assert(native2kc[SDLK_HELP] == KeyCode::HELP);
	assert(native2kc[SDLK_PRINT] == KeyCode::PRINT);
	assert(native2kc[SDLK_SYSREQ] == KeyCode::SYS_REQ);
	assert(native2kc[SDLK_BREAK] == KeyCode::BREAK);
	assert(native2kc[SDLK_MENU] == KeyCode::MENU);
	assert(native2kc[SDLK_POWER] == KeyCode::POWER);
	assert(native2kc[SDLK_EURO] == KeyCode::EURO);
	assert(native2kc[SDLK_UNDO] == KeyCode::UNDO);
	assert(kc2native[KeyCode::UNKNOWN] == SDLK_UNKNOWN);
	assert(kc2native[KeyCode::BACK_SPACE] == SDLK_BACKSPACE);
	assert(kc2native[KeyCode::TAB] == SDLK_TAB);
	assert(kc2native[KeyCode::CLEAR] == SDLK_CLEAR);
	assert(kc2native[KeyCode::RETURN] == SDLK_RETURN);
	assert(kc2native[KeyCode::PAUSE] == SDLK_PAUSE);
	assert(kc2native[KeyCode::ESCAPE] == SDLK_ESCAPE);
	assert(kc2native[KeyCode::SPACE] == SDLK_SPACE);
	assert(kc2native[KeyCode::EXCLAIM] == SDLK_EXCLAIM);
	assert(kc2native[KeyCode::DOUBLE_QUOTE] == SDLK_QUOTEDBL);
	assert(kc2native[KeyCode::HASH] == SDLK_HASH);
	assert(kc2native[KeyCode::DOLLAR] == SDLK_DOLLAR);
	assert(kc2native[KeyCode::AMPERSAND] == SDLK_AMPERSAND);
	assert(kc2native[KeyCode::QUOTE] == SDLK_QUOTE);
	assert(kc2native[KeyCode::LEFT_PAREN] == SDLK_LEFTPAREN);
	assert(kc2native[KeyCode::RIGHT_PAREN] == SDLK_RIGHTPAREN);
	assert(kc2native[KeyCode::ASTERISK] == SDLK_ASTERISK);
	assert(kc2native[KeyCode::PLUS] == SDLK_PLUS);
	assert(kc2native[KeyCode::COMMA] == SDLK_COMMA);
	assert(kc2native[KeyCode::MINUS] == SDLK_MINUS);
	assert(kc2native[KeyCode::PERIOD] == SDLK_PERIOD);
	assert(kc2native[KeyCode::SLASH] == SDLK_SLASH);
	assert(kc2native[KeyCode::ZERO] == SDLK_0);
	assert(kc2native[KeyCode::ONE] == SDLK_1);
	assert(kc2native[KeyCode::TWO] == SDLK_2);
	assert(kc2native[KeyCode::THREE] == SDLK_3);
	assert(kc2native[KeyCode::FOUR] == SDLK_4);
	assert(kc2native[KeyCode::FIVE] == SDLK_5);
	assert(kc2native[KeyCode::SIX] == SDLK_6);
	assert(kc2native[KeyCode::SEVEN] == SDLK_7);
	assert(kc2native[KeyCode::EIGHT] == SDLK_8);
	assert(kc2native[KeyCode::NINE] == SDLK_9);
	assert(kc2native[KeyCode::COLON] == SDLK_COLON);
	assert(kc2native[KeyCode::SEMI_COLON] == SDLK_SEMICOLON);
	assert(kc2native[KeyCode::LESS] == SDLK_LESS);
	assert(kc2native[KeyCode::EQUAL] == SDLK_EQUALS);
	assert(kc2native[KeyCode::GREATER] == SDLK_GREATER);
	assert(kc2native[KeyCode::QUESTION] == SDLK_QUESTION);
	assert(kc2native[KeyCode::AT] == SDLK_AT);
	assert(kc2native[KeyCode::A] == 'A');
	assert(kc2native[KeyCode::B] == 'B');
	assert(kc2native[KeyCode::C] == 'C');
	assert(kc2native[KeyCode::D] == 'D');
	assert(kc2native[KeyCode::E] == 'E');
	assert(kc2native[KeyCode::F] == 'F');
	assert(kc2native[KeyCode::G] == 'G');
	assert(kc2native[KeyCode::H] == 'H');
	assert(kc2native[KeyCode::I] == 'I');
	assert(kc2native[KeyCode::J] == 'J');
	assert(kc2native[KeyCode::K] == 'K');
	assert(kc2native[KeyCode::L] == 'L');
	assert(kc2native[KeyCode::M] == 'M');
	assert(kc2native[KeyCode::N] == 'N');
	assert(kc2native[KeyCode::O] == 'O');
	assert(kc2native[KeyCode::P] == 'P');
	assert(kc2native[KeyCode::Q] == 'Q');
	assert(kc2native[KeyCode::R] == 'R');
	assert(kc2native[KeyCode::S] == 'S');
	assert(kc2native[KeyCode::T] == 'T');
	assert(kc2native[KeyCode::U] == 'U');
	assert(kc2native[KeyCode::V] == 'V');
	assert(kc2native[KeyCode::W] == 'W');
	assert(kc2native[KeyCode::X] == 'X');
	assert(kc2native[KeyCode::Y] == 'Y');
	assert(kc2native[KeyCode::Z] == 'Z');
	assert(kc2native[KeyCode::LEFT_BRACKET] == SDLK_LEFTBRACKET);
	assert(kc2native[KeyCode::BACK_SLASH] == SDLK_BACKSLASH);
	assert(kc2native[KeyCode::RIGHT_BRACKET] == SDLK_RIGHTBRACKET);
	assert(kc2native[KeyCode::CARET] == SDLK_CARET);
	assert(kc2native[KeyCode::UNDER_SCORE] == SDLK_UNDERSCORE);
	assert(kc2native[KeyCode::BACK_QUOTE] == SDLK_BACKQUOTE);
	assert(kc2native[KeyCode::DELETE_] == SDLK_DELETE);
	assert(kc2native[KeyCode::WORLD_0] == SDLK_WORLD_0);
	assert(kc2native[KeyCode::WORLD_1] == SDLK_WORLD_1);
	assert(kc2native[KeyCode::WORLD_2] == SDLK_WORLD_2);
	assert(kc2native[KeyCode::WORLD_3] == SDLK_WORLD_3);
	assert(kc2native[KeyCode::WORLD_4] == SDLK_WORLD_4);
	assert(kc2native[KeyCode::WORLD_5] == SDLK_WORLD_5);
	assert(kc2native[KeyCode::WORLD_6] == SDLK_WORLD_6);
	assert(kc2native[KeyCode::WORLD_7] == SDLK_WORLD_7);
	assert(kc2native[KeyCode::WORLD_8] == SDLK_WORLD_8);
	assert(kc2native[KeyCode::WORLD_9] == SDLK_WORLD_9);
	assert(kc2native[KeyCode::WORLD_10] == SDLK_WORLD_10);
	assert(kc2native[KeyCode::WORLD_11] == SDLK_WORLD_11);
	assert(kc2native[KeyCode::WORLD_12] == SDLK_WORLD_12);
	assert(kc2native[KeyCode::WORLD_13] == SDLK_WORLD_13);
	assert(kc2native[KeyCode::WORLD_14] == SDLK_WORLD_14);
	assert(kc2native[KeyCode::WORLD_15] == SDLK_WORLD_15);
	assert(kc2native[KeyCode::WORLD_16] == SDLK_WORLD_16);
	assert(kc2native[KeyCode::WORLD_17] == SDLK_WORLD_17);
	assert(kc2native[KeyCode::WORLD_18] == SDLK_WORLD_18);
	assert(kc2native[KeyCode::WORLD_19] == SDLK_WORLD_19);
	assert(kc2native[KeyCode::WORLD_20] == SDLK_WORLD_20);
	assert(kc2native[KeyCode::WORLD_21] == SDLK_WORLD_21);
	assert(kc2native[KeyCode::WORLD_22] == SDLK_WORLD_22);
	assert(kc2native[KeyCode::WORLD_23] == SDLK_WORLD_23);
	assert(kc2native[KeyCode::WORLD_24] == SDLK_WORLD_24);
	assert(kc2native[KeyCode::WORLD_25] == SDLK_WORLD_25);
	assert(kc2native[KeyCode::WORLD_26] == SDLK_WORLD_26);
	assert(kc2native[KeyCode::WORLD_27] == SDLK_WORLD_27);
	assert(kc2native[KeyCode::WORLD_28] == SDLK_WORLD_28);
	assert(kc2native[KeyCode::WORLD_29] == SDLK_WORLD_29);
	assert(kc2native[KeyCode::WORLD_30] == SDLK_WORLD_30);
	assert(kc2native[KeyCode::WORLD_31] == SDLK_WORLD_31);
	assert(kc2native[KeyCode::WORLD_32] == SDLK_WORLD_32);
	assert(kc2native[KeyCode::WORLD_33] == SDLK_WORLD_33);
	assert(kc2native[KeyCode::WORLD_34] == SDLK_WORLD_34);
	assert(kc2native[KeyCode::WORLD_35] == SDLK_WORLD_35);
	assert(kc2native[KeyCode::WORLD_36] == SDLK_WORLD_36);
	assert(kc2native[KeyCode::WORLD_37] == SDLK_WORLD_37);
	assert(kc2native[KeyCode::WORLD_38] == SDLK_WORLD_38);
	assert(kc2native[KeyCode::WORLD_39] == SDLK_WORLD_39);
	assert(kc2native[KeyCode::WORLD_40] == SDLK_WORLD_40);
	assert(kc2native[KeyCode::WORLD_41] == SDLK_WORLD_41);
	assert(kc2native[KeyCode::WORLD_42] == SDLK_WORLD_42);
	assert(kc2native[KeyCode::WORLD_43] == SDLK_WORLD_43);
	assert(kc2native[KeyCode::WORLD_44] == SDLK_WORLD_44);
	assert(kc2native[KeyCode::WORLD_45] == SDLK_WORLD_45);
	assert(kc2native[KeyCode::WORLD_46] == SDLK_WORLD_46);
	assert(kc2native[KeyCode::WORLD_47] == SDLK_WORLD_47);
	assert(kc2native[KeyCode::WORLD_48] == SDLK_WORLD_48);
	assert(kc2native[KeyCode::WORLD_49] == SDLK_WORLD_49);
	assert(kc2native[KeyCode::WORLD_50] == SDLK_WORLD_50);
	assert(kc2native[KeyCode::WORLD_51] == SDLK_WORLD_51);
	assert(kc2native[KeyCode::WORLD_52] == SDLK_WORLD_52);
	assert(kc2native[KeyCode::WORLD_53] == SDLK_WORLD_53);
	assert(kc2native[KeyCode::WORLD_54] == SDLK_WORLD_54);
	assert(kc2native[KeyCode::WORLD_55] == SDLK_WORLD_55);
	assert(kc2native[KeyCode::WORLD_56] == SDLK_WORLD_56);
	assert(kc2native[KeyCode::WORLD_57] == SDLK_WORLD_57);
	assert(kc2native[KeyCode::WORLD_58] == SDLK_WORLD_58);
	assert(kc2native[KeyCode::WORLD_59] == SDLK_WORLD_59);
	assert(kc2native[KeyCode::WORLD_60] == SDLK_WORLD_60);
	assert(kc2native[KeyCode::WORLD_61] == SDLK_WORLD_61);
	assert(kc2native[KeyCode::WORLD_62] == SDLK_WORLD_62);
	assert(kc2native[KeyCode::WORLD_63] == SDLK_WORLD_63);
	assert(kc2native[KeyCode::WORLD_64] == SDLK_WORLD_64);
	assert(kc2native[KeyCode::WORLD_65] == SDLK_WORLD_65);
	assert(kc2native[KeyCode::WORLD_66] == SDLK_WORLD_66);
	assert(kc2native[KeyCode::WORLD_67] == SDLK_WORLD_67);
	assert(kc2native[KeyCode::WORLD_68] == SDLK_WORLD_68);
	assert(kc2native[KeyCode::WORLD_69] == SDLK_WORLD_69);
	assert(kc2native[KeyCode::WORLD_70] == SDLK_WORLD_70);
	assert(kc2native[KeyCode::WORLD_71] == SDLK_WORLD_71);
	assert(kc2native[KeyCode::WORLD_72] == SDLK_WORLD_72);
	assert(kc2native[KeyCode::WORLD_73] == SDLK_WORLD_73);
	assert(kc2native[KeyCode::WORLD_74] == SDLK_WORLD_74);
	assert(kc2native[KeyCode::WORLD_75] == SDLK_WORLD_75);
	assert(kc2native[KeyCode::WORLD_76] == SDLK_WORLD_76);
	assert(kc2native[KeyCode::WORLD_77] == SDLK_WORLD_77);
	assert(kc2native[KeyCode::WORLD_78] == SDLK_WORLD_78);
	assert(kc2native[KeyCode::WORLD_79] == SDLK_WORLD_79);
	assert(kc2native[KeyCode::WORLD_80] == SDLK_WORLD_80);
	assert(kc2native[KeyCode::WORLD_81] == SDLK_WORLD_81);
	assert(kc2native[KeyCode::WORLD_82] == SDLK_WORLD_82);
	assert(kc2native[KeyCode::WORLD_83] == SDLK_WORLD_83);
	assert(kc2native[KeyCode::WORLD_84] == SDLK_WORLD_84);
	assert(kc2native[KeyCode::WORLD_85] == SDLK_WORLD_85);
	assert(kc2native[KeyCode::WORLD_86] == SDLK_WORLD_86);
	assert(kc2native[KeyCode::WORLD_87] == SDLK_WORLD_87);
	assert(kc2native[KeyCode::WORLD_88] == SDLK_WORLD_88);
	assert(kc2native[KeyCode::WORLD_89] == SDLK_WORLD_89);
	assert(kc2native[KeyCode::WORLD_90] == SDLK_WORLD_90);
	assert(kc2native[KeyCode::WORLD_91] == SDLK_WORLD_91);
	assert(kc2native[KeyCode::WORLD_92] == SDLK_WORLD_92);
	assert(kc2native[KeyCode::WORLD_93] == SDLK_WORLD_93);
	assert(kc2native[KeyCode::WORLD_94] == SDLK_WORLD_94);
	assert(kc2native[KeyCode::WORLD_95] == SDLK_WORLD_95);
	assert(kc2native[KeyCode::KEYPAD_0] == SDLK_KP0);
	assert(kc2native[KeyCode::KEYPAD_1] == SDLK_KP1);
	assert(kc2native[KeyCode::KEYPAD_2] == SDLK_KP2);
	assert(kc2native[KeyCode::KEYPAD_3] == SDLK_KP3);
	assert(kc2native[KeyCode::KEYPAD_4] == SDLK_KP4);
	assert(kc2native[KeyCode::KEYPAD_5] == SDLK_KP5);
	assert(kc2native[KeyCode::KEYPAD_6] == SDLK_KP6);
	assert(kc2native[KeyCode::KEYPAD_7] == SDLK_KP7);
	assert(kc2native[KeyCode::KEYPAD_8] == SDLK_KP8);
	assert(kc2native[KeyCode::KEYPAD_9] == SDLK_KP9);
	assert(kc2native[KeyCode::KEYPAD_PERIOD] == SDLK_KP_PERIOD);
	assert(kc2native[KeyCode::KEYPAD_DIVIDE] == SDLK_KP_DIVIDE);
	assert(kc2native[KeyCode::KEYPAD_MULTIPLY] == SDLK_KP_MULTIPLY);
	assert(kc2native[KeyCode::KEYPAD_MINUS] == SDLK_KP_MINUS);
	assert(kc2native[KeyCode::KEYPAD_PLUS] == SDLK_KP_PLUS);
	assert(kc2native[KeyCode::KEYPAD_ENTER] == SDLK_KP_ENTER);
	assert(kc2native[KeyCode::KEYPAD_EQUAL] == SDLK_KP_EQUALS);
	assert(kc2native[KeyCode::ARROW_UP] == SDLK_UP);
	assert(kc2native[KeyCode::ARROW_DOWN] == SDLK_DOWN);
	assert(kc2native[KeyCode::ARROW_RIGHT] == SDLK_RIGHT);
	assert(kc2native[KeyCode::ARROW_LEFT] == SDLK_LEFT);
	assert(kc2native[KeyCode::INSERT] == SDLK_INSERT);
	assert(kc2native[KeyCode::HOME] == SDLK_HOME);
	assert(kc2native[KeyCode::END] == SDLK_END);
	assert(kc2native[KeyCode::PAGE_UP] == SDLK_PAGEUP);
	assert(kc2native[KeyCode::PAGE_DOWN] == SDLK_PAGEDOWN);
	assert(kc2native[KeyCode::FUNCTION_1] == SDLK_F1);
	assert(kc2native[KeyCode::FUNCTION_2] == SDLK_F2);
	assert(kc2native[KeyCode::FUNCTION_3] == SDLK_F3);
	assert(kc2native[KeyCode::FUNCTION_4] == SDLK_F4);
	assert(kc2native[KeyCode::FUNCTION_5] == SDLK_F5);
	assert(kc2native[KeyCode::FUNCTION_6] == SDLK_F6);
	assert(kc2native[KeyCode::FUNCTION_7] == SDLK_F7);
	assert(kc2native[KeyCode::FUNCTION_8] == SDLK_F8);
	assert(kc2native[KeyCode::FUNCTION_9] == SDLK_F9);
	assert(kc2native[KeyCode::FUNCTION_10] == SDLK_F10);
	assert(kc2native[KeyCode::FUNCTION_11] == SDLK_F11);
	assert(kc2native[KeyCode::FUNCTION_12] == SDLK_F12);
	assert(kc2native[KeyCode::FUNCTION_13] == SDLK_F13);
	assert(kc2native[KeyCode::FUNCTION_14] == SDLK_F14);
	assert(kc2native[KeyCode::FUNCTION_15] == SDLK_F15);
	assert(kc2native[KeyCode::NUM_LOCK] == SDLK_NUMLOCK);
	assert(kc2native[KeyCode::CAPS_LOCK] == SDLK_CAPSLOCK);
	assert(kc2native[KeyCode::SCROLL_LOCK] == SDLK_SCROLLOCK);
	assert(kc2native[KeyCode::LEFT_SHIFT] == SDLK_LSHIFT);
	assert(kc2native[KeyCode::RIGHT_SHIFT] == SDLK_RSHIFT);
	assert(kc2native[KeyCode::LEFT_CTRL] == SDLK_LCTRL);
	assert(kc2native[KeyCode::RIGHT_CTRL] == SDLK_RCTRL);
	assert(kc2native[KeyCode::LEFT_ALT] == SDLK_LALT);
	assert(kc2native[KeyCode::RIGHT_ALT] == SDLK_RALT);
	assert(kc2native[KeyCode::LEFT_META] == SDLK_LMETA);
	assert(kc2native[KeyCode::RIGHT_META] == SDLK_RMETA);
	assert(kc2native[KeyCode::RIGHT_SUPER] == SDLK_RSUPER);
	assert(kc2native[KeyCode::LEFT_SUPER] == SDLK_LSUPER);
	assert(kc2native[KeyCode::MODE] == SDLK_MODE);
	assert(kc2native[KeyCode::COMPOSE] == SDLK_COMPOSE);
	assert(kc2native[KeyCode::HELP] == SDLK_HELP);
	assert(kc2native[KeyCode::PRINT] == SDLK_PRINT);
	assert(kc2native[KeyCode::SYS_REQ] == SDLK_SYSREQ);
	assert(kc2native[KeyCode::BREAK] == SDLK_BREAK);
	assert(kc2native[KeyCode::MENU] == SDLK_MENU);
	assert(kc2native[KeyCode::POWER] == SDLK_POWER);
	assert(kc2native[KeyCode::EURO] == SDLK_EURO);
	assert(kc2native[KeyCode::UNDO] == SDLK_UNDO);

}

#elif defined(WIN32)  || defined(WIN64)
#pragma pack(push, 1)
const unsigned char Input::native2mb[Input::NATIVE_MOUSE_BUTTON_LAST + 1] = {
	MouseButton::UNKNOWN,		// 0
	MouseButton::LEFT,			// 1 VK_LBUTTON
	MouseButton::RIGHT,			// 2 VK_RBUTTON
	MouseButton::UNKNOWN,		// 3
	MouseButton::MIDDLE,		// 4 VK_MBUTTON
	MouseButton::BUTTON_X1,		// 5 VK_XBUTTON1
	MouseButton::BUTTON_X2		// 6 VK_XBUTTON2
};

const unsigned char Input::mb2native[MouseButton::COUNT] = {
	0 ,						// MouseButton::UNKNOWN
	VK_LBUTTON,				// MouseButton::LEFT
	VK_MBUTTON,				// MouseButton::MIDDLE
	VK_RBUTTON,				// MouseButton::RIGHT
	0,						// MouseButton::WHEEL_UP
	0,						// MouseButton::WHEEL_DOWN
	VK_XBUTTON1,			// MouseButton::BUTTON_X1
	VK_XBUTTON2,			// MouseButton::BUTTON_X2
};

const short Input::native2kc[Input::NATIVE_KEY_CODE_LAST + 1] = {
	KeyCode::NONE,				// 0
	KeyCode::L_BUTTON,			// 1 VK_LBUTTON
	KeyCode::R_BUTTON,			// 2 VK_RBUTTON
	KeyCode::BREAK,				// 3 VK_CANCEL
	KeyCode::M_BUTTON,			// 4 VK_MBUTTON
	KeyCode::X_BUTTON1,			// 5 VK_XBUTTON1
	KeyCode::X_BUTTON2,			// 6 VK_XBUTTON2
	KeyCode::NONE,				// 7
	KeyCode::BACK_SPACE,		// 8 VK_BACK
	KeyCode::TAB,				// 9 VK_TAB
	KeyCode::NONE,				// 10
	KeyCode::NONE,				// 11
	KeyCode::CLEAR,				// 12 VK_CLEAR
	KeyCode::RETURN,			// 13 VK_RETURN
	KeyCode::NONE,				// 14
	KeyCode::NONE,				// 15
	KeyCode::LEFT_SHIFT,		// 16 VK_SHIFT
	KeyCode::LEFT_CTRL,			// 17 VK_CONTROL
	KeyCode::LEFT_ALT,			// 18 VK_MENU
	KeyCode::PAUSE,				// 19 VK_PAUSE
	KeyCode::CAPS_LOCK,			// 20 VK_CAPITAL
	KeyCode::IME_KANA,			// 21 VK_KANA
	KeyCode::NONE,				// 22
	KeyCode::IME_JUNJA,			// 23 VK_JUNJA
	KeyCode::IME_FINAL,			// 24 VK_FINAL
	KeyCode::IME_HANJA,			// 25 VK_HANJA
	KeyCode::NONE,				// 26
	KeyCode::ESCAPE,			// 27 VK_ESCAPE
	KeyCode::IME_CONVERT,		// 28 VK_CONVERT
	KeyCode::IME_NON_CONVERT,	// 29 VK_NONCONVERT
	KeyCode::IME_ACCEPT,		// 30 VK_ACCEPT
	KeyCode::IME_MODE_CHANGE,	// 31 VK_MODECHANGE
	KeyCode::SPACE,				// 32 VK_SPACE
	KeyCode::PAGE_UP,			// 33 VK_PRIOR
	KeyCode::PAGE_DOWN,			// 34 VK_NEXT
	KeyCode::END,				// 35 VK_END
	KeyCode::HOME,				// 36 VK_HOME
	KeyCode::ARROW_LEFT,		// 37 VK_LEFT
	KeyCode::ARROW_UP,			// 38 VK_UP
	KeyCode::ARROW_RIGHT,		// 39 VK_RIGHT
	KeyCode::ARROW_DOWN,		// 40 VK_DOWN
	KeyCode::NONE,				// 41 VK_SELECT
	KeyCode::PRINT,				// 42 VK_PRINT
	KeyCode::NONE,				// 43 VK_EXECUTE
	KeyCode::PRINT,				// 44 VK_SNAPSHOT
	KeyCode::INSERT,			// 45 VK_INSERT
	KeyCode::DELETE_,			// 46 VK_DELETE
	KeyCode::HELP,				// 47 VK_HELP
	KeyCode::ZERO,				// 48 '0'
	KeyCode::ONE,				// 49 '1'
	KeyCode::TWO,				// 50 '2'
	KeyCode::THREE,				// 51 '3'
	KeyCode::FOUR,				// 52 '4'
	KeyCode::FIVE,				// 53 '5'
	KeyCode::SIX,				// 54 '6'
	KeyCode::SEVEN,				// 55 '7'
	KeyCode::EIGHT,				// 56 '8'
	KeyCode::NINE,				// 57 '9'
	KeyCode::NONE,				// 58
	KeyCode::NONE,				// 59
	KeyCode::NONE,				// 60
	KeyCode::NONE,				// 61
	KeyCode::NONE,				// 62
	KeyCode::NONE,				// 63
	KeyCode::NONE,				// 64
	KeyCode::A,					// 65 'A'
	KeyCode::B,					// 66 'B'
	KeyCode::C,					// 67 'C'
	KeyCode::D,					// 68 'D'
	KeyCode::E,					// 69 'E'
	KeyCode::F,					// 70 'F'
	KeyCode::G,					// 71 'G'
	KeyCode::H,					// 72 'H'
	KeyCode::I,					// 73 'I'
	KeyCode::J,					// 74 'J'
	KeyCode::K,					// 75 'K'
	KeyCode::L,					// 76 'L'
	KeyCode::M,					// 77 'M'
	KeyCode::N,					// 78 'N'
	KeyCode::O,					// 79 'O'
	KeyCode::P,					// 80 'P'
	KeyCode::Q,					// 81 'Q'
	KeyCode::R,					// 82 'R'
	KeyCode::S,					// 83 'S'
	KeyCode::T,					// 84 'T'
	KeyCode::U,					// 85 'U'
	KeyCode::V,					// 86 'V'
	KeyCode::W,					// 87 'W'
	KeyCode::X,					// 88 'X'
	KeyCode::Y,					// 89 'Y'
	KeyCode::Z,					// 90 'Z'
	KeyCode::LEFT_SUPER,		// 91 VK_LWIN
	KeyCode::RIGHT_SUPER,		// 92 VK_RWIN
	KeyCode::MENU,				// 93 VK_APPS
	KeyCode::NONE,				// 94
	KeyCode::NONE,				// 95 VK_SLEEP
	KeyCode::KEYPAD_0,			// 96 VK_NUMPAD0
	KeyCode::KEYPAD_1,			// 97 VK_NUMPAD1
	KeyCode::KEYPAD_2,			// 98 VK_NUMPAD2
	KeyCode::KEYPAD_3,			// 99 VK_NUMPAD3
	KeyCode::KEYPAD_4,			// 100 VK_NUMPAD4
	KeyCode::KEYPAD_5,			// 101 VK_NUMPAD5
	KeyCode::KEYPAD_6,			// 102 VK_NUMPAD6
	KeyCode::KEYPAD_7,			// 103 VK_NUMPAD7
	KeyCode::KEYPAD_8,			// 104 VK_NUMPAD8
	KeyCode::KEYPAD_9,			// 105 VK_NUMPAD9
	KeyCode::KEYPAD_MULTIPLY,	// 106 VK_MULTIPLY
	KeyCode::KEYPAD_PLUS,		// 107 VK_ADD
	KeyCode::KEYPAD_ENTER,		// 108 VK_SEPARATOR
	KeyCode::KEYPAD_MINUS,		// 109 VK_SUBTRACT
	KeyCode::KEYPAD_PERIOD,		// 110 VK_DECIMAL
	KeyCode::KEYPAD_DIVIDE,		// 111 VK_DIVIDE
	KeyCode::FUNCTION_1,		// 112 VK_F1
	KeyCode::FUNCTION_2,		// 113 VK_F2
	KeyCode::FUNCTION_3,		// 114 VK_F3
	KeyCode::FUNCTION_4,		// 115 VK_F4
	KeyCode::FUNCTION_5,		// 116 VK_F5
	KeyCode::FUNCTION_6,		// 117 VK_F6
	KeyCode::FUNCTION_7,		// 118 VK_F7
	KeyCode::FUNCTION_8,		// 119 VK_F8
	KeyCode::FUNCTION_9,		// 120 VK_F9
	KeyCode::FUNCTION_10,		// 121 VK_F10
	KeyCode::FUNCTION_11,		// 122 VK_F11
	KeyCode::FUNCTION_12,		// 123 VK_F12
	KeyCode::FUNCTION_13,		// 124 VK_F13
	KeyCode::FUNCTION_14,		// 125 VK_F14
	KeyCode::FUNCTION_15,		// 126 VK_F15
	KeyCode::FUNCTION_16,		// 127 VK_F16
	KeyCode::FUNCTION_17,		// 128 VK_F17
	KeyCode::FUNCTION_18,		// 129 VK_F18
	KeyCode::FUNCTION_19,		// 130 VK_F19
	KeyCode::FUNCTION_20,		// 131 VK_F20
	KeyCode::FUNCTION_21,		// 132 VK_F21
	KeyCode::FUNCTION_22,		// 133 VK_F22
	KeyCode::FUNCTION_23,		// 134 VK_F23
	KeyCode::FUNCTION_24,		// 135 VK_F24
	KeyCode::NONE,				// 136
	KeyCode::NONE,				// 137
	KeyCode::NONE,				// 138
	KeyCode::NONE,				// 139
	KeyCode::NONE,				// 140
	KeyCode::NONE,				// 141
	KeyCode::NONE,				// 142
	KeyCode::NONE,				// 143
	KeyCode::NUM_LOCK,			// 144 VK_NUMLOCK
	KeyCode::SCROLL_LOCK,		// 145 VK_SCROLL
	KeyCode::NONE,				// 146
	KeyCode::NONE,				// 147
	KeyCode::NONE,				// 148
	KeyCode::NONE,				// 149
	KeyCode::NONE,				// 150
	KeyCode::NONE,				// 151
	KeyCode::NONE,				// 152
	KeyCode::NONE,				// 153
	KeyCode::NONE,				// 154
	KeyCode::NONE,				// 155
	KeyCode::NONE,				// 156
	KeyCode::NONE,				// 157
	KeyCode::NONE,				// 158
	KeyCode::NONE,				// 159
	KeyCode::LEFT_SHIFT,		// 160 VK_LSHIFT
	KeyCode::RIGHT_SHIFT,		// 161 VK_RSHIFT
	KeyCode::LEFT_CTRL,			// 162 VK_LCONTROL
	KeyCode::RIGHT_CTRL,		// 163 VK_RCONTROL
	KeyCode::LEFT_ALT,			// 164 VK_LMENU
	KeyCode::RIGHT_ALT,			// 165 VK_RMENU
	KeyCode::BROWSER_BACK,		// 166 VK_BROWSER_BACK
	KeyCode::BROWSER_FORWARD,	// 167 VK_BROWSER_FORWARD
	KeyCode::BROWSER_REFRESH,	// 168 VK_BROWSER_REFRESH
	KeyCode::BROWSER_STOP,		// 169 VK_BROWSER_STOP
	KeyCode::BROWSER_SEARCH,	// 170 VK_BROWSER_SEARCH
	KeyCode::BROWSER_FAVORITES,	// 171 VK_BROWSER_FAVORITES
	KeyCode::BROWSER_HOME,		// 172 VK_BROWSER_HOME
	KeyCode::VOLUME_MUTE,		// 173 VK_VOLUME_MUTE
	KeyCode::VOLUME_DOWN,		// 174 VK_VOLUME_DOWN
	KeyCode::VOLUME_UP,			// 175 VK_VOLUME_UP
	KeyCode::MEDIA_NEXT,		// 176 VK_MEDIA_NEXT_TRACK
	KeyCode::MEDIA_PREV,		// 177 VK_MEDIA_PREV_TRACK
	KeyCode::MEDIA_STOP,		// 178 VK_MEDIA_STOP
	KeyCode::MEDIA_PLAY_PAUSE,	// 179 VK_MEDIA_PLAY_PAUSE
	KeyCode::LAUNCH_MAIL,		// 180 VK_LAUNCH_MAIL
	KeyCode::LAUNCH_MEDIA,		// 181 VK_LAUNCH_MEDIA_SELECT
	KeyCode::LAUNCH_APP1,		// 182 VK_LAUNCH_APP1
	KeyCode::LAUNCH_APP2,		// 183 VK_LAUNCH_APP2
	KeyCode::NONE,				// 184
	KeyCode::NONE,				// 185
	KeyCode::SEMI_COLON,		// 186 VK_OEM_1
	KeyCode::EQUAL,				// 187 VK_OEM_PLUS
	KeyCode::COMMA,				// 188 VK_OEM_COMMA
	KeyCode::MINUS,				// 189 VK_OEM_MINUS
	KeyCode::PERIOD,			// 190 VK_OEM_PERIOD
	KeyCode::SLASH,				// 191 VK_OEM_2
	KeyCode::BACK_QUOTE,		// 192 VK_OEM_3
	KeyCode::NONE,				// 193
	KeyCode::NONE,				// 194
	KeyCode::NONE,				// 195
	KeyCode::NONE,				// 196
	KeyCode::NONE,				// 197
	KeyCode::NONE,				// 198
	KeyCode::NONE,				// 199
	KeyCode::NONE,				// 200
	KeyCode::NONE,				// 201
	KeyCode::NONE,				// 202
	KeyCode::NONE,				// 203
	KeyCode::NONE,				// 204
	KeyCode::NONE,				// 205
	KeyCode::NONE,				// 206
	KeyCode::NONE,				// 207
	KeyCode::NONE,				// 208
	KeyCode::NONE,				// 209
	KeyCode::NONE,				// 210
	KeyCode::NONE,				// 211
	KeyCode::NONE,				// 212
	KeyCode::NONE,				// 213
	KeyCode::NONE,				// 214
	KeyCode::NONE,				// 215
	KeyCode::NONE,				// 216
	KeyCode::NONE,				// 217
	KeyCode::NONE,				// 218
	KeyCode::LEFT_BRACKET,		// 219 VK_OEM_4
	KeyCode::BACK_SLASH,		// 220 VK_OEM_5
	KeyCode::RIGHT_BRACKET,		// 221 VK_OEM_6
	KeyCode::QUOTE,				// 222 VK_OEM_7
	KeyCode::BACK_QUOTE,		// 223 VK_OEM_8
	KeyCode::NONE,				// 224
	KeyCode::NONE,				// 225
	KeyCode::NONE,				// 226 VK_OEM_102
	KeyCode::NONE,				// 227
	KeyCode::NONE,				// 228
	KeyCode::IME_PROCESS,		// 229 VK_PROCESSKEY
	KeyCode::NONE,				// 230
	KeyCode::NONE,				// 231 VK_PACKET
	KeyCode::NONE,				// 232
	KeyCode::NONE,				// 233
	KeyCode::NONE,				// 234
	KeyCode::NONE,				// 235
	KeyCode::NONE,				// 236
	KeyCode::NONE,				// 237
	KeyCode::NONE,				// 238
	KeyCode::NONE,				// 239
	KeyCode::NONE,				// 240
	KeyCode::NONE,				// 241
	KeyCode::NONE,				// 242
	KeyCode::NONE,				// 243
	KeyCode::NONE,				// 244
	KeyCode::NONE,				// 245
	KeyCode::NONE,				// 246 VK_ATTN
	KeyCode::NONE,				// 247 VK_CRSEL
	KeyCode::NONE,				// 248 VK_EXSEL
	KeyCode::NONE,				// 249 VK_EREOF
	KeyCode::PLAY,				// 250 VK_PLAY
	KeyCode::ZOOM,				// 251 VK_ZOOM
	KeyCode::NONE,				// 252 VK_NONAME
	KeyCode::NONE,				// 253 VK_PA1
	KeyCode::NONE,				// 254 VK_OEM_CLEAR
	KeyCode::NONE				// 255
};

const NativeKeyCodeCompact Input::kc2native[KeyCode::COUNT] = {
	0,						// KeyCode::NONE
	0,						// KeyCode::UNKNOWN
	VK_LBUTTON,				// KeyCode::L_BUTTON
	VK_RBUTTON,				// KeyCode::R_BUTTON
	VK_MBUTTON,				// KeyCode::M_BUTTON
	VK_XBUTTON1,			// KeyCode::X_BUTTON1
	VK_XBUTTON2,			// KeyCode::X_BUTTON2
	VK_BACK,				// KeyCode::BACK_SPACE
	VK_TAB,					// KeyCode::TAB
	VK_CLEAR,				// KeyCode::CLEAR
	VK_RETURN,				// KeyCode::RETURN
	VK_PAUSE,				// KeyCode::PAUSE
	VK_KANA,				// KeyCode::IME_KANA
	0,						// KeyCode::IME_HANGUL
	VK_JUNJA,				// KeyCode::IME_JUNJA
	VK_FINAL,				// KeyCode::IME_FINAL
	VK_HANJA,				// KeyCode::IME_HANJA
	0,						// KeyCode::IME_KANJI
	VK_ESCAPE,				// KeyCode::ESCAPE
	VK_CONVERT,				// KeyCode::IME_CONVERT
	VK_NONCONVERT,			// KeyCode::IME_NON_CONVERT
	VK_ACCEPT,				// KeyCode::IME_ACCEPT
	VK_MODECHANGE,			// KeyCode::IME_MODE_CHANGE
	VK_PROCESSKEY,			// KeyCode::IME_PROCESS
	VK_SPACE,				// KeyCode::SPACE
	0,						// KeyCode::EXCLAIM
	0,						// KeyCode::DOUBLE_QUOTE
	0,						// KeyCode::HASH
	0,						// KeyCode::DOLLAR
	0,						// KeyCode::PERCENT
	0,						// KeyCode::AMPERSAND
	VK_OEM_7,				// KeyCode::QUOTE
	0,						// KeyCode::LEFT_PAREN
	0,						// KeyCode::RIGHT_PAREN
	0,						// KeyCode::ASTERISK
	VK_OEM_PLUS,			// KeyCode::PLUS
	VK_OEM_COMMA,			// KeyCode::COMMA
	VK_OEM_MINUS,			// KeyCode::MINUS
	VK_OEM_PERIOD,			// KeyCode::PERIOD
	VK_OEM_2,				// KeyCode::SLASH
	'0',					// KeyCode::ZERO
	'1',					// KeyCode::ONE
	'2',					// KeyCode::TWO
	'3',					// KeyCode::THREE
	'4',					// KeyCode::FOUR
	'5',					// KeyCode::FIVE
	'6',					// KeyCode::SIX
	'7',					// KeyCode::SEVEN
	'8',					// KeyCode::EIGHT
	'9',					// KeyCode::NINE
	0,						// KeyCode::COLON
	VK_OEM_1,				// KeyCode::SEMI_COLON
	0,						// KeyCode::LESS
	VK_OEM_PLUS,			// KeyCode::EQUAL
	0,						// KeyCode::GREATER
	0,						// KeyCode::QUESTION
	0,						// KeyCode::AT
	'A',					// KeyCode::A
	'B',					// KeyCode::B
	'C',					// KeyCode::C
	'D',					// KeyCode::D
	'E',					// KeyCode::E
	'F',					// KeyCode::F
	'G',					// KeyCode::G
	'H',					// KeyCode::H
	'I',					// KeyCode::I
	'J',					// KeyCode::J
	'K',					// KeyCode::K
	'L',					// KeyCode::L
	'M',					// KeyCode::M
	'N',					// KeyCode::N
	'O',					// KeyCode::O
	'P',					// KeyCode::P
	'Q',					// KeyCode::Q
	'R',					// KeyCode::R
	'S',					// KeyCode::S
	'T',					// KeyCode::T
	'U',					// KeyCode::U
	'V',					// KeyCode::V
	'W',					// KeyCode::W
	'X',					// KeyCode::X
	'Y',					// KeyCode::Y
	'Z',					// KeyCode::Z
	VK_OEM_4,				// KeyCode::LEFT_BRACKET
	VK_OEM_5,				// KeyCode::BACK_SLASH
	VK_OEM_6,				// KeyCode::RIGHT_BRACKET
	0,						// KeyCode::CARET
	0,						// KeyCode::UNDER_SCORE
	VK_OEM_3,				// KeyCode::BACK_QUOTE
	VK_DELETE,				// KeyCode::DELETE_
	0,						// KeyCode::WORLD_0
	0,						// KeyCode::WORLD_1
	0,						// KeyCode::WORLD_2
	0,						// KeyCode::WORLD_3
	0,						// KeyCode::WORLD_4
	0,						// KeyCode::WORLD_5
	0,						// KeyCode::WORLD_6
	0,						// KeyCode::WORLD_7
	0,						// KeyCode::WORLD_8
	0,						// KeyCode::WORLD_9
	0,						// KeyCode::WORLD_10
	0,						// KeyCode::WORLD_11
	0,						// KeyCode::WORLD_12
	0,						// KeyCode::WORLD_13
	0,						// KeyCode::WORLD_14
	0,						// KeyCode::WORLD_15
	0,						// KeyCode::WORLD_16
	0,						// KeyCode::WORLD_17
	0,						// KeyCode::WORLD_18
	0,						// KeyCode::WORLD_19
	0,						// KeyCode::WORLD_20
	0,						// KeyCode::WORLD_21
	0,						// KeyCode::WORLD_22
	0,						// KeyCode::WORLD_23
	0,						// KeyCode::WORLD_24
	0,						// KeyCode::WORLD_25
	0,						// KeyCode::WORLD_26
	0,						// KeyCode::WORLD_27
	0,						// KeyCode::WORLD_28
	0,						// KeyCode::WORLD_29
	0,						// KeyCode::WORLD_30
	0,						// KeyCode::WORLD_31
	0,						// KeyCode::WORLD_32
	0,						// KeyCode::WORLD_33
	0,						// KeyCode::WORLD_34
	0,						// KeyCode::WORLD_35
	0,						// KeyCode::WORLD_36
	0,						// KeyCode::WORLD_37
	0,						// KeyCode::WORLD_38
	0,						// KeyCode::WORLD_39
	0,						// KeyCode::WORLD_40
	0,						// KeyCode::WORLD_41
	0,						// KeyCode::WORLD_42
	0,						// KeyCode::WORLD_43
	0,						// KeyCode::WORLD_44
	0,						// KeyCode::WORLD_45
	0,						// KeyCode::WORLD_46
	0,						// KeyCode::WORLD_47
	0,						// KeyCode::WORLD_48
	0,						// KeyCode::WORLD_49
	0,						// KeyCode::WORLD_50
	0,						// KeyCode::WORLD_51
	0,						// KeyCode::WORLD_52
	0,						// KeyCode::WORLD_53
	0,						// KeyCode::WORLD_54
	0,						// KeyCode::WORLD_55
	0,						// KeyCode::WORLD_56
	0,						// KeyCode::WORLD_57
	0,						// KeyCode::WORLD_58
	0,						// KeyCode::WORLD_59
	0,						// KeyCode::WORLD_60
	0,						// KeyCode::WORLD_61
	0,						// KeyCode::WORLD_62
	0,						// KeyCode::WORLD_63
	0,						// KeyCode::WORLD_64
	0,						// KeyCode::WORLD_65
	0,						// KeyCode::WORLD_66
	0,						// KeyCode::WORLD_67
	0,						// KeyCode::WORLD_68
	0,						// KeyCode::WORLD_69
	0,						// KeyCode::WORLD_70
	0,						// KeyCode::WORLD_71
	0,						// KeyCode::WORLD_72
	0,						// KeyCode::WORLD_73
	0,						// KeyCode::WORLD_74
	0,						// KeyCode::WORLD_75
	0,						// KeyCode::WORLD_76
	0,						// KeyCode::WORLD_77
	0,						// KeyCode::WORLD_78
	0,						// KeyCode::WORLD_79
	0,						// KeyCode::WORLD_80
	0,						// KeyCode::WORLD_81
	0,						// KeyCode::WORLD_82
	0,						// KeyCode::WORLD_83
	0,						// KeyCode::WORLD_84
	0,						// KeyCode::WORLD_85
	0,						// KeyCode::WORLD_86
	0,						// KeyCode::WORLD_87
	0,						// KeyCode::WORLD_88
	0,						// KeyCode::WORLD_89
	0,						// KeyCode::WORLD_90
	0,						// KeyCode::WORLD_91
	0,						// KeyCode::WORLD_92
	0,						// KeyCode::WORLD_93
	0,						// KeyCode::WORLD_94
	0,						// KeyCode::WORLD_95
	VK_NUMPAD0,				// KeyCode::KEYPAD_0
	VK_NUMPAD1,				// KeyCode::KEYPAD_1
	VK_NUMPAD2,				// KeyCode::KEYPAD_2
	VK_NUMPAD3,				// KeyCode::KEYPAD_3
	VK_NUMPAD4,				// KeyCode::KEYPAD_4
	VK_NUMPAD5,				// KeyCode::KEYPAD_5
	VK_NUMPAD6,				// KeyCode::KEYPAD_6
	VK_NUMPAD7,				// KeyCode::KEYPAD_7
	VK_NUMPAD8,				// KeyCode::KEYPAD_8
	VK_NUMPAD9,				// KeyCode::KEYPAD_9
	VK_DECIMAL,				// KeyCode::KEYPAD_PERIOD
	VK_DIVIDE,				// KeyCode::KEYPAD_DIVIDE
	VK_MULTIPLY,			// KeyCode::KEYPAD_MULTIPLY
	VK_SUBTRACT,			// KeyCode::KEYPAD_MINUS
	VK_ADD,					// KeyCode::KEYPAD_PLUS
	VK_SEPARATOR,			// KeyCode::KEYPAD_ENTER
	0,						// KeyCode::KEYPAD_EQUAL
	VK_UP,					// KeyCode::ARROW_UP
	VK_DOWN,				// KeyCode::ARROW_DOWN
	VK_RIGHT,				// KeyCode::ARROW_RIGHT
	VK_LEFT,				// KeyCode::ARROW_LEFT
	VK_INSERT,				// KeyCode::INSERT
	VK_HOME,				// KeyCode::HOME
	VK_END,					// KeyCode::END
	VK_PRIOR,				// KeyCode::PAGE_UP
	VK_NEXT,				// KeyCode::PAGE_DOWN
	VK_F1,					// KeyCode::FUNCTION_1
	VK_F2,					// KeyCode::FUNCTION_2
	VK_F3,					// KeyCode::FUNCTION_3
	VK_F4,					// KeyCode::FUNCTION_4
	VK_F5,					// KeyCode::FUNCTION_5
	VK_F6,					// KeyCode::FUNCTION_6
	VK_F7,					// KeyCode::FUNCTION_7
	VK_F8,					// KeyCode::FUNCTION_8
	VK_F9,					// KeyCode::FUNCTION_9
	VK_F10,					// KeyCode::FUNCTION_10
	VK_F11,					// KeyCode::FUNCTION_11
	VK_F12,					// KeyCode::FUNCTION_12
	VK_F13,					// KeyCode::FUNCTION_13
	VK_F14,					// KeyCode::FUNCTION_14
	VK_F15,					// KeyCode::FUNCTION_15
	VK_F16,					// KeyCode::FUNCTION_16
	VK_F17,					// KeyCode::FUNCTION_17
	VK_F18,					// KeyCode::FUNCTION_18
	VK_F19,					// KeyCode::FUNCTION_19
	VK_F20,					// KeyCode::FUNCTION_20
	VK_F21,					// KeyCode::FUNCTION_21
	VK_F22,					// KeyCode::FUNCTION_22
	VK_F23,					// KeyCode::FUNCTION_23
	VK_F24,					// KeyCode::FUNCTION_24
	VK_NUMLOCK,				// KeyCode::NUM_LOCK
	VK_CAPITAL,				// KeyCode::CAPS_LOCK
	VK_SCROLL,				// KeyCode::SCROLL_LOCK
	VK_SHIFT,				// KeyCode::LEFT_SHIFT
	VK_RSHIFT,				// KeyCode::RIGHT_SHIFT
	VK_CONTROL,				// KeyCode::LEFT_CTRL
	VK_RCONTROL,			// KeyCode::RIGHT_CTRL
	VK_MENU,				// KeyCode::LEFT_ALT
	VK_RMENU,				// KeyCode::RIGHT_ALT
	0,						// KeyCode::LEFT_META
	0,						// KeyCode::RIGHT_META
	VK_RWIN,				// KeyCode::RIGHT_SUPER
	VK_LWIN,				// KeyCode::LEFT_SUPER
	0,						// KeyCode::MODE
	0,						// KeyCode::COMPOSE
	VK_HELP,				// KeyCode::HELP
	VK_PRINT,				// KeyCode::PRINT
	0,						// KeyCode::SYS_REQ
	VK_CANCEL,				// KeyCode::BREAK
	VK_APPS,				// KeyCode::MENU
	0,						// KeyCode::POWER
	0,						// KeyCode::EURO
	0,						// KeyCode::UNDO
	VK_BROWSER_BACK,		// KeyCode::BROWSER_BACK
	VK_BROWSER_FORWARD,		// KeyCode::BROWSER_FORWARD
	VK_BROWSER_REFRESH,		// KeyCode::BROWSER_REFRESH
	VK_BROWSER_STOP,		// KeyCode::BROWSER_STOP
	VK_BROWSER_SEARCH,		// KeyCode::BROWSER_SEARCH
	VK_BROWSER_FAVORITES,	// KeyCode::BROWSER_FAVORITES
	VK_BROWSER_HOME,		// KeyCode::BROWSER_HOME
	VK_VOLUME_MUTE,			// KeyCode::VOLUME_MUTE
	VK_VOLUME_DOWN,			// KeyCode::VOLUME_DOWN
	VK_VOLUME_UP,			// KeyCode::VOLUME_UP
	VK_MEDIA_NEXT_TRACK,	// KeyCode::MEDIA_NEXT
	VK_MEDIA_PREV_TRACK,	// KeyCode::MEDIA_PREV
	VK_MEDIA_STOP,			// KeyCode::MEDIA_STOP
	VK_MEDIA_PLAY_PAUSE,	// KeyCode::MEDIA_PLAY_PAUSE
	VK_LAUNCH_MAIL,			// KeyCode::LAUNCH_MAIL
	VK_LAUNCH_MEDIA_SELECT,	// KeyCode::LAUNCH_MEDIA
	VK_LAUNCH_APP1,			// KeyCode::LAUNCH_APP1
	VK_LAUNCH_APP2,			// KeyCode::LAUNCH_APP2
	VK_PLAY,				// KeyCode::PLAY
	VK_ZOOM					// KeyCode::ZOOM
};

#pragma pack(pop)

void Input::verifyTranslationTables() {
	// verify integrity of Windows to KeyCode table
	assert(native2kc[VK_LBUTTON] == KeyCode::L_BUTTON);
	assert(native2kc[VK_RBUTTON] == KeyCode::R_BUTTON);
	assert(native2kc[VK_CANCEL] == KeyCode::BREAK);
	assert(native2kc[VK_MBUTTON] == KeyCode::M_BUTTON);
	assert(native2kc[VK_XBUTTON1] == KeyCode::X_BUTTON1);
	assert(native2kc[VK_XBUTTON2] == KeyCode::X_BUTTON2);
	assert(native2kc[VK_BACK] == KeyCode::BACK_SPACE);
	assert(native2kc[VK_TAB] == KeyCode::TAB);
	assert(native2kc[VK_CLEAR] == KeyCode::CLEAR);
	assert(native2kc[VK_RETURN] == KeyCode::RETURN);
	assert(native2kc[VK_SHIFT] == KeyCode::LEFT_SHIFT);
	assert(native2kc[VK_CONTROL] == KeyCode::LEFT_CTRL);
	assert(native2kc[VK_MENU] == KeyCode::LEFT_ALT);
	assert(native2kc[VK_PAUSE] == KeyCode::PAUSE);
	assert(native2kc[VK_CAPITAL] == KeyCode::CAPS_LOCK);
	assert(native2kc[VK_KANA] == KeyCode::IME_KANA);
	assert(native2kc[VK_JUNJA] == KeyCode::IME_JUNJA);
	assert(native2kc[VK_FINAL] == KeyCode::IME_FINAL);
	assert(native2kc[VK_HANJA] == KeyCode::IME_HANJA);
	assert(native2kc[VK_ESCAPE] == KeyCode::ESCAPE);
	assert(native2kc[VK_CONVERT] == KeyCode::IME_CONVERT);
	assert(native2kc[VK_NONCONVERT] == KeyCode::IME_NON_CONVERT);
	assert(native2kc[VK_ACCEPT] == KeyCode::IME_ACCEPT);
	assert(native2kc[VK_MODECHANGE] == KeyCode::IME_MODE_CHANGE);
	assert(native2kc[VK_SPACE] == KeyCode::SPACE);
	assert(native2kc[VK_PRIOR] == KeyCode::PAGE_UP);
	assert(native2kc[VK_NEXT] == KeyCode::PAGE_DOWN);
	assert(native2kc[VK_END] == KeyCode::END);
	assert(native2kc[VK_HOME] == KeyCode::HOME);
	assert(native2kc[VK_LEFT] == KeyCode::ARROW_LEFT);
	assert(native2kc[VK_UP] == KeyCode::ARROW_UP);
	assert(native2kc[VK_RIGHT] == KeyCode::ARROW_RIGHT);
	assert(native2kc[VK_DOWN] == KeyCode::ARROW_DOWN);
	assert(native2kc[VK_PRINT] == KeyCode::PRINT);
	assert(native2kc[VK_SNAPSHOT] == KeyCode::PRINT);
	assert(native2kc[VK_INSERT] == KeyCode::INSERT);
	assert(native2kc[VK_DELETE] == KeyCode::DELETE_);
	assert(native2kc[VK_HELP] == KeyCode::HELP);
	assert(native2kc['0'] == KeyCode::ZERO);
	assert(native2kc['1'] == KeyCode::ONE);
	assert(native2kc['2'] == KeyCode::TWO);
	assert(native2kc['3'] == KeyCode::THREE);
	assert(native2kc['4'] == KeyCode::FOUR);
	assert(native2kc['5'] == KeyCode::FIVE);
	assert(native2kc['6'] == KeyCode::SIX);
	assert(native2kc['7'] == KeyCode::SEVEN);
	assert(native2kc['8'] == KeyCode::EIGHT);
	assert(native2kc['9'] == KeyCode::NINE);
	assert(native2kc['A'] == KeyCode::A);
	assert(native2kc['B'] == KeyCode::B);
	assert(native2kc['C'] == KeyCode::C);
	assert(native2kc['D'] == KeyCode::D);
	assert(native2kc['E'] == KeyCode::E);
	assert(native2kc['F'] == KeyCode::F);
	assert(native2kc['G'] == KeyCode::G);
	assert(native2kc['H'] == KeyCode::H);
	assert(native2kc['I'] == KeyCode::I);
	assert(native2kc['J'] == KeyCode::J);
	assert(native2kc['K'] == KeyCode::K);
	assert(native2kc['L'] == KeyCode::L);
	assert(native2kc['M'] == KeyCode::M);
	assert(native2kc['N'] == KeyCode::N);
	assert(native2kc['O'] == KeyCode::O);
	assert(native2kc['P'] == KeyCode::P);
	assert(native2kc['Q'] == KeyCode::Q);
	assert(native2kc['R'] == KeyCode::R);
	assert(native2kc['S'] == KeyCode::S);
	assert(native2kc['T'] == KeyCode::T);
	assert(native2kc['U'] == KeyCode::U);
	assert(native2kc['V'] == KeyCode::V);
	assert(native2kc['W'] == KeyCode::W);
	assert(native2kc['X'] == KeyCode::X);
	assert(native2kc['Y'] == KeyCode::Y);
	assert(native2kc['Z'] == KeyCode::Z);
	assert(native2kc[VK_LWIN] == KeyCode::LEFT_SUPER);
	assert(native2kc[VK_RWIN] == KeyCode::RIGHT_SUPER);
	assert(native2kc[VK_APPS] == KeyCode::MENU);
	assert(native2kc[VK_NUMPAD0] == KeyCode::KEYPAD_0);
	assert(native2kc[VK_NUMPAD1] == KeyCode::KEYPAD_1);
	assert(native2kc[VK_NUMPAD2] == KeyCode::KEYPAD_2);
	assert(native2kc[VK_NUMPAD3] == KeyCode::KEYPAD_3);
	assert(native2kc[VK_NUMPAD4] == KeyCode::KEYPAD_4);
	assert(native2kc[VK_NUMPAD5] == KeyCode::KEYPAD_5);
	assert(native2kc[VK_NUMPAD6] == KeyCode::KEYPAD_6);
	assert(native2kc[VK_NUMPAD7] == KeyCode::KEYPAD_7);
	assert(native2kc[VK_NUMPAD8] == KeyCode::KEYPAD_8);
	assert(native2kc[VK_NUMPAD9] == KeyCode::KEYPAD_9);
	assert(native2kc[VK_MULTIPLY] == KeyCode::KEYPAD_MULTIPLY);
	assert(native2kc[VK_ADD] == KeyCode::KEYPAD_PLUS);
	assert(native2kc[VK_SEPARATOR] == KeyCode::KEYPAD_ENTER);
	assert(native2kc[VK_SUBTRACT] == KeyCode::KEYPAD_MINUS);
	assert(native2kc[VK_DECIMAL] == KeyCode::KEYPAD_PERIOD);
	assert(native2kc[VK_DIVIDE] == KeyCode::KEYPAD_DIVIDE);
	assert(native2kc[VK_F1] == KeyCode::FUNCTION_1);
	assert(native2kc[VK_F2] == KeyCode::FUNCTION_2);
	assert(native2kc[VK_F3] == KeyCode::FUNCTION_3);
	assert(native2kc[VK_F4] == KeyCode::FUNCTION_4);
	assert(native2kc[VK_F5] == KeyCode::FUNCTION_5);
	assert(native2kc[VK_F6] == KeyCode::FUNCTION_6);
	assert(native2kc[VK_F7] == KeyCode::FUNCTION_7);
	assert(native2kc[VK_F8] == KeyCode::FUNCTION_8);
	assert(native2kc[VK_F9] == KeyCode::FUNCTION_9);
	assert(native2kc[VK_F10] == KeyCode::FUNCTION_10);
	assert(native2kc[VK_F11] == KeyCode::FUNCTION_11);
	assert(native2kc[VK_F12] == KeyCode::FUNCTION_12);
	assert(native2kc[VK_F13] == KeyCode::FUNCTION_13);
	assert(native2kc[VK_F14] == KeyCode::FUNCTION_14);
	assert(native2kc[VK_F15] == KeyCode::FUNCTION_15);
	assert(native2kc[VK_F16] == KeyCode::FUNCTION_16);
	assert(native2kc[VK_F17] == KeyCode::FUNCTION_17);
	assert(native2kc[VK_F18] == KeyCode::FUNCTION_18);
	assert(native2kc[VK_F19] == KeyCode::FUNCTION_19);
	assert(native2kc[VK_F20] == KeyCode::FUNCTION_20);
	assert(native2kc[VK_F21] == KeyCode::FUNCTION_21);
	assert(native2kc[VK_F22] == KeyCode::FUNCTION_22);
	assert(native2kc[VK_F23] == KeyCode::FUNCTION_23);
	assert(native2kc[VK_F24] == KeyCode::FUNCTION_24);
	assert(native2kc[VK_NUMLOCK] == KeyCode::NUM_LOCK);
	assert(native2kc[VK_SCROLL] == KeyCode::SCROLL_LOCK);
	assert(native2kc[VK_LSHIFT] == KeyCode::LEFT_SHIFT);
	assert(native2kc[VK_RSHIFT] == KeyCode::RIGHT_SHIFT);
	assert(native2kc[VK_LCONTROL] == KeyCode::LEFT_CTRL);
	assert(native2kc[VK_RCONTROL] == KeyCode::RIGHT_CTRL);
	assert(native2kc[VK_LMENU] == KeyCode::LEFT_ALT);
	assert(native2kc[VK_RMENU] == KeyCode::RIGHT_ALT);
	assert(native2kc[VK_BROWSER_BACK] == KeyCode::BROWSER_BACK);
	assert(native2kc[VK_BROWSER_FORWARD] == KeyCode::BROWSER_FORWARD);
	assert(native2kc[VK_BROWSER_REFRESH] == KeyCode::BROWSER_REFRESH);
	assert(native2kc[VK_BROWSER_STOP] == KeyCode::BROWSER_STOP);
	assert(native2kc[VK_BROWSER_SEARCH] == KeyCode::BROWSER_SEARCH);
	assert(native2kc[VK_BROWSER_FAVORITES] == KeyCode::BROWSER_FAVORITES);
	assert(native2kc[VK_BROWSER_HOME] == KeyCode::BROWSER_HOME);
	assert(native2kc[VK_VOLUME_MUTE] == KeyCode::VOLUME_MUTE);
	assert(native2kc[VK_VOLUME_DOWN] == KeyCode::VOLUME_DOWN);
	assert(native2kc[VK_VOLUME_UP] == KeyCode::VOLUME_UP);
	assert(native2kc[VK_MEDIA_NEXT_TRACK] == KeyCode::MEDIA_NEXT);
	assert(native2kc[VK_MEDIA_PREV_TRACK] == KeyCode::MEDIA_PREV);
	assert(native2kc[VK_MEDIA_STOP] == KeyCode::MEDIA_STOP);
	assert(native2kc[VK_MEDIA_PLAY_PAUSE] == KeyCode::MEDIA_PLAY_PAUSE);
	assert(native2kc[VK_LAUNCH_MAIL] == KeyCode::LAUNCH_MAIL);
	assert(native2kc[VK_LAUNCH_MEDIA_SELECT] == KeyCode::LAUNCH_MEDIA);
	assert(native2kc[VK_LAUNCH_APP1] == KeyCode::LAUNCH_APP1);
	assert(native2kc[VK_LAUNCH_APP2] == KeyCode::LAUNCH_APP2);
	assert(native2kc[VK_OEM_1] == KeyCode::SEMI_COLON);
//	assert(native2kc[VK_OEM_PLUS] == KeyCode::PLUS);
//	assert(native2kc[VK_OEM_PLUS] == KeyCode::EQUAL);
	assert(native2kc[VK_OEM_COMMA] == KeyCode::COMMA);
	assert(native2kc[VK_OEM_MINUS] == KeyCode::MINUS);
	assert(native2kc[VK_OEM_PERIOD] == KeyCode::PERIOD);
	assert(native2kc[VK_OEM_2] == KeyCode::SLASH);
	assert(native2kc[VK_OEM_3] == KeyCode::BACK_QUOTE);
	assert(native2kc[VK_OEM_4] == KeyCode::LEFT_BRACKET);
	assert(native2kc[VK_OEM_5] == KeyCode::BACK_SLASH);
	assert(native2kc[VK_OEM_6] == KeyCode::RIGHT_BRACKET);
	assert(native2kc[VK_OEM_7] == KeyCode::QUOTE);
	assert(native2kc[VK_OEM_8] == KeyCode::BACK_QUOTE);
	assert(native2kc[VK_PROCESSKEY] == KeyCode::IME_PROCESS);
	assert(native2kc[VK_PLAY] == KeyCode::PLAY);
	assert(native2kc[VK_ZOOM] == KeyCode::ZOOM);
	assert(kc2native[KeyCode::L_BUTTON] == VK_LBUTTON);
	assert(kc2native[KeyCode::R_BUTTON] == VK_RBUTTON);
	assert(kc2native[KeyCode::M_BUTTON] == VK_MBUTTON);
	assert(kc2native[KeyCode::X_BUTTON1] == VK_XBUTTON1);
	assert(kc2native[KeyCode::X_BUTTON2] == VK_XBUTTON2);
	assert(kc2native[KeyCode::BACK_SPACE] == VK_BACK);
	assert(kc2native[KeyCode::TAB] == VK_TAB);
	assert(kc2native[KeyCode::CLEAR] == VK_CLEAR);
	assert(kc2native[KeyCode::RETURN] == VK_RETURN);
	assert(kc2native[KeyCode::PAUSE] == VK_PAUSE);
	assert(kc2native[KeyCode::IME_KANA] == VK_KANA);
	assert(kc2native[KeyCode::IME_JUNJA] == VK_JUNJA);
	assert(kc2native[KeyCode::IME_FINAL] == VK_FINAL);
	assert(kc2native[KeyCode::IME_HANJA] == VK_HANJA);
	assert(kc2native[KeyCode::ESCAPE] == VK_ESCAPE);
	assert(kc2native[KeyCode::IME_CONVERT] == VK_CONVERT);
	assert(kc2native[KeyCode::IME_NON_CONVERT] == VK_NONCONVERT);
	assert(kc2native[KeyCode::IME_ACCEPT] == VK_ACCEPT);
	assert(kc2native[KeyCode::IME_MODE_CHANGE] == VK_MODECHANGE);
	assert(kc2native[KeyCode::IME_PROCESS] == VK_PROCESSKEY);
	assert(kc2native[KeyCode::SPACE] == VK_SPACE);
	assert(kc2native[KeyCode::QUOTE] == VK_OEM_7);
//	assert(kc2native[KeyCode::PLUS] == VK_OEM_PLUS);
	assert(kc2native[KeyCode::EQUAL] == VK_OEM_PLUS);
	assert(kc2native[KeyCode::COMMA] == VK_OEM_COMMA);
	assert(kc2native[KeyCode::MINUS] == VK_OEM_MINUS);
	assert(kc2native[KeyCode::PERIOD] == VK_OEM_PERIOD);
	assert(kc2native[KeyCode::SLASH] == VK_OEM_2);
	assert(kc2native[KeyCode::ZERO] == '0');
	assert(kc2native[KeyCode::ONE] == '1');
	assert(kc2native[KeyCode::TWO] == '2');
	assert(kc2native[KeyCode::THREE] == '3');
	assert(kc2native[KeyCode::FOUR] == '4');
	assert(kc2native[KeyCode::FIVE] == '5');
	assert(kc2native[KeyCode::SIX] == '6');
	assert(kc2native[KeyCode::SEVEN] == '7');
	assert(kc2native[KeyCode::EIGHT] == '8');
	assert(kc2native[KeyCode::NINE] == '9');
	assert(kc2native[KeyCode::SEMI_COLON] == VK_OEM_1);
	assert(kc2native[KeyCode::A] == 'A');
	assert(kc2native[KeyCode::B] == 'B');
	assert(kc2native[KeyCode::C] == 'C');
	assert(kc2native[KeyCode::D] == 'D');
	assert(kc2native[KeyCode::E] == 'E');
	assert(kc2native[KeyCode::F] == 'F');
	assert(kc2native[KeyCode::G] == 'G');
	assert(kc2native[KeyCode::H] == 'H');
	assert(kc2native[KeyCode::I] == 'I');
	assert(kc2native[KeyCode::J] == 'J');
	assert(kc2native[KeyCode::K] == 'K');
	assert(kc2native[KeyCode::L] == 'L');
	assert(kc2native[KeyCode::M] == 'M');
	assert(kc2native[KeyCode::N] == 'N');
	assert(kc2native[KeyCode::O] == 'O');
	assert(kc2native[KeyCode::P] == 'P');
	assert(kc2native[KeyCode::Q] == 'Q');
	assert(kc2native[KeyCode::R] == 'R');
	assert(kc2native[KeyCode::S] == 'S');
	assert(kc2native[KeyCode::T] == 'T');
	assert(kc2native[KeyCode::U] == 'U');
	assert(kc2native[KeyCode::V] == 'V');
	assert(kc2native[KeyCode::W] == 'W');
	assert(kc2native[KeyCode::X] == 'X');
	assert(kc2native[KeyCode::Y] == 'Y');
	assert(kc2native[KeyCode::Z] == 'Z');
	assert(kc2native[KeyCode::LEFT_BRACKET] == VK_OEM_4);
	assert(kc2native[KeyCode::BACK_SLASH] == VK_OEM_5);
	assert(kc2native[KeyCode::RIGHT_BRACKET] == VK_OEM_6);
	assert(kc2native[KeyCode::BACK_QUOTE] == VK_OEM_3);
	assert(kc2native[KeyCode::DELETE_] == VK_DELETE);
	assert(kc2native[KeyCode::KEYPAD_0] == VK_NUMPAD0);
	assert(kc2native[KeyCode::KEYPAD_1] == VK_NUMPAD1);
	assert(kc2native[KeyCode::KEYPAD_2] == VK_NUMPAD2);
	assert(kc2native[KeyCode::KEYPAD_3] == VK_NUMPAD3);
	assert(kc2native[KeyCode::KEYPAD_4] == VK_NUMPAD4);
	assert(kc2native[KeyCode::KEYPAD_5] == VK_NUMPAD5);
	assert(kc2native[KeyCode::KEYPAD_6] == VK_NUMPAD6);
	assert(kc2native[KeyCode::KEYPAD_7] == VK_NUMPAD7);
	assert(kc2native[KeyCode::KEYPAD_8] == VK_NUMPAD8);
	assert(kc2native[KeyCode::KEYPAD_9] == VK_NUMPAD9);
	assert(kc2native[KeyCode::KEYPAD_PERIOD] == VK_DECIMAL);
	assert(kc2native[KeyCode::KEYPAD_DIVIDE] == VK_DIVIDE);
	assert(kc2native[KeyCode::KEYPAD_MULTIPLY] == VK_MULTIPLY);
	assert(kc2native[KeyCode::KEYPAD_MINUS] == VK_SUBTRACT);
	assert(kc2native[KeyCode::KEYPAD_PLUS] == VK_ADD);
	assert(kc2native[KeyCode::KEYPAD_ENTER] == VK_SEPARATOR);
	assert(kc2native[KeyCode::ARROW_UP] == VK_UP);
	assert(kc2native[KeyCode::ARROW_DOWN] == VK_DOWN);
	assert(kc2native[KeyCode::ARROW_RIGHT] == VK_RIGHT);
	assert(kc2native[KeyCode::ARROW_LEFT] == VK_LEFT);
	assert(kc2native[KeyCode::INSERT] == VK_INSERT);
	assert(kc2native[KeyCode::HOME] == VK_HOME);
	assert(kc2native[KeyCode::END] == VK_END);
	assert(kc2native[KeyCode::PAGE_UP] == VK_PRIOR);
	assert(kc2native[KeyCode::PAGE_DOWN] == VK_NEXT);
	assert(kc2native[KeyCode::FUNCTION_1] == VK_F1);
	assert(kc2native[KeyCode::FUNCTION_2] == VK_F2);
	assert(kc2native[KeyCode::FUNCTION_3] == VK_F3);
	assert(kc2native[KeyCode::FUNCTION_4] == VK_F4);
	assert(kc2native[KeyCode::FUNCTION_5] == VK_F5);
	assert(kc2native[KeyCode::FUNCTION_6] == VK_F6);
	assert(kc2native[KeyCode::FUNCTION_7] == VK_F7);
	assert(kc2native[KeyCode::FUNCTION_8] == VK_F8);
	assert(kc2native[KeyCode::FUNCTION_9] == VK_F9);
	assert(kc2native[KeyCode::FUNCTION_10] == VK_F10);
	assert(kc2native[KeyCode::FUNCTION_11] == VK_F11);
	assert(kc2native[KeyCode::FUNCTION_12] == VK_F12);
	assert(kc2native[KeyCode::FUNCTION_13] == VK_F13);
	assert(kc2native[KeyCode::FUNCTION_14] == VK_F14);
	assert(kc2native[KeyCode::FUNCTION_15] == VK_F15);
	assert(kc2native[KeyCode::FUNCTION_16] == VK_F16);
	assert(kc2native[KeyCode::FUNCTION_17] == VK_F17);
	assert(kc2native[KeyCode::FUNCTION_18] == VK_F18);
	assert(kc2native[KeyCode::FUNCTION_19] == VK_F19);
	assert(kc2native[KeyCode::FUNCTION_20] == VK_F20);
	assert(kc2native[KeyCode::FUNCTION_21] == VK_F21);
	assert(kc2native[KeyCode::FUNCTION_22] == VK_F22);
	assert(kc2native[KeyCode::FUNCTION_23] == VK_F23);
	assert(kc2native[KeyCode::FUNCTION_24] == VK_F24);
	assert(kc2native[KeyCode::NUM_LOCK] == VK_NUMLOCK);
	assert(kc2native[KeyCode::CAPS_LOCK] == VK_CAPITAL);
	assert(kc2native[KeyCode::SCROLL_LOCK] == VK_SCROLL);
	assert(kc2native[KeyCode::LEFT_SHIFT] == VK_SHIFT);
	assert(kc2native[KeyCode::RIGHT_SHIFT] == VK_RSHIFT);
	assert(kc2native[KeyCode::LEFT_CTRL] == VK_CONTROL);
	assert(kc2native[KeyCode::RIGHT_CTRL] == VK_RCONTROL);
	assert(kc2native[KeyCode::LEFT_ALT] == VK_MENU);
	assert(kc2native[KeyCode::RIGHT_ALT] == VK_RMENU);
	assert(kc2native[KeyCode::RIGHT_SUPER] == VK_RWIN);
	assert(kc2native[KeyCode::LEFT_SUPER] == VK_LWIN);
	assert(kc2native[KeyCode::HELP] == VK_HELP);
	assert(kc2native[KeyCode::PRINT] == VK_PRINT);
	assert(kc2native[KeyCode::BREAK] == VK_CANCEL);
	assert(kc2native[KeyCode::MENU] == VK_APPS);
	assert(kc2native[KeyCode::BROWSER_BACK] == VK_BROWSER_BACK);
	assert(kc2native[KeyCode::BROWSER_FORWARD] == VK_BROWSER_FORWARD);
	assert(kc2native[KeyCode::BROWSER_REFRESH] == VK_BROWSER_REFRESH);
	assert(kc2native[KeyCode::BROWSER_STOP] == VK_BROWSER_STOP);
	assert(kc2native[KeyCode::BROWSER_SEARCH] == VK_BROWSER_SEARCH);
	assert(kc2native[KeyCode::BROWSER_FAVORITES] == VK_BROWSER_FAVORITES);
	assert(kc2native[KeyCode::BROWSER_HOME] == VK_BROWSER_HOME);
	assert(kc2native[KeyCode::VOLUME_MUTE] == VK_VOLUME_MUTE);
	assert(kc2native[KeyCode::VOLUME_DOWN] == VK_VOLUME_DOWN);
	assert(kc2native[KeyCode::VOLUME_UP] == VK_VOLUME_UP);
	assert(kc2native[KeyCode::MEDIA_NEXT] == VK_MEDIA_NEXT_TRACK);
	assert(kc2native[KeyCode::MEDIA_PREV] == VK_MEDIA_PREV_TRACK);
	assert(kc2native[KeyCode::MEDIA_STOP] == VK_MEDIA_STOP);
	assert(kc2native[KeyCode::MEDIA_PLAY_PAUSE] == VK_MEDIA_PLAY_PAUSE);
	assert(kc2native[KeyCode::LAUNCH_MAIL] == VK_LAUNCH_MAIL);
	assert(kc2native[KeyCode::LAUNCH_MEDIA] == VK_LAUNCH_MEDIA_SELECT);
	assert(kc2native[KeyCode::LAUNCH_APP1] == VK_LAUNCH_APP1);
	assert(kc2native[KeyCode::LAUNCH_APP2] == VK_LAUNCH_APP2);
	assert(kc2native[KeyCode::PLAY] == VK_PLAY);
	assert(kc2native[KeyCode::ZOOM] == VK_ZOOM);
}
#endif

// =======================================
// class Key
// =======================================

/**
 * Find a KeyCode by name.  Note that this function was never meant to be fast because we aren't
 * indexing, using std::map or anything.
 */
KeyCode Key::findByName(const char *name) {
	KeyCode code = KeyCodeNames.match(name);
	if (code == KeyCode::INVALID) {
		throw range_error(string("Invalid key name: ") + name);
	}
	return code;
}

}}//end namespace

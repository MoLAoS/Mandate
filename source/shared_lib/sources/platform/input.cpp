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
	mbUnknown,		// 0 
	mbLeft,			// 1 SDL_BUTTON_LEFT
	mbCenter,		// 2 SDL_BUTTON_MIDDLE
	mbRight,		// 3 SDL_BUTTON_RIGHT
	mbWheelUp,		// 4 SDL_BUTTON_WHEELUP
	mbWheelDown,	// 5 SDL_BUTTON_WHEELDOWN
	mbButtonX1,		// 6 SDL_BUTTON_X1
	mbButtonX2		// 7 SDL_BUTTON_X2
};

const unsigned char Input::mb2native[mbCount] = {
	0,						// mbUnknown
	SDL_BUTTON_LEFT,		// mbLeft
	SDL_BUTTON_MIDDLE,		// mbCenter
	SDL_BUTTON_RIGHT,		// mbRight
#ifdef SDL_BUTTON_WHEELUP
	SDL_BUTTON_WHEELUP,		// mbWheelUp
	SDL_BUTTON_WHEELDOWN,	// mbWheelDown
#else
	0,						// mbWheelUp
	0,						// mbWheelDown
#endif
#ifdef SDL_BUTTON_X1
	SDL_BUTTON_X1,			// mbButtonX1
	SDL_BUTTON_X2			// mbButtonX2
#else
	0,						// mbButtonX1
	0						// mbButtonX2
#endif
};

const short Input::native2kc[Input::NATIVE_KEY_CODE_LAST + 1] = {
	keyUnknown,		// 0 SDLK_UNKNOWN
	keyUnknown,		// 1
	keyUnknown,		// 2
	keyUnknown,		// 3
	keyUnknown,		// 4
	keyUnknown,		// 5
	keyUnknown,		// 6
	keyUnknown,		// 7
	keyBackspace,	// 8 SDLK_BACKSPACE
	keyTab,			// 9 SDLK_TAB
	keyUnknown,		// 10
	keyUnknown,		// 11
	keyClear,		// 12 SDLK_CLEAR
	keyReturn,		// 13 SDLK_RETURN
	keyUnknown,		// 14
	keyUnknown,		// 15
	keyUnknown,		// 16
	keyUnknown,		// 17
	keyUnknown,		// 18
	keyPause,		// 19 SDLK_PAUSE
	keyUnknown,		// 20
	keyUnknown,		// 21
	keyUnknown,		// 22
	keyUnknown,		// 23
	keyUnknown,		// 24
	keyUnknown,		// 25
	keyUnknown,		// 26
	keyEscape,		// 27 SDLK_ESCAPE
	keyUnknown,		// 28
	keyUnknown,		// 29
	keyUnknown,		// 30
	keyUnknown,		// 31
	keySpace,		// 32 SDLK_SPACE
	keyExclaim,		// 33 SDLK_EXCLAIM
	keyQuoteDbl,	// 34 SDLK_QUOTEDBL
	keyHash,		// 35 SDLK_HASH
	keyDollar,		// 36 SDLK_DOLLAR
	keyUnknown,		// 37
	keyAmpersand,	// 38 SDLK_AMPERSAND
	keyQuote,		// 39 SDLK_QUOTE
	keyLeftParen,	// 40 SDLK_LEFTPAREN
	keyRightParen,	// 41 SDLK_RIGHTPAREN
	keyAsterisk,	// 42 SDLK_ASTERISK
	keyPlus,		// 43 SDLK_PLUS
	keyComma,		// 44 SDLK_COMMA
	keyMinus,		// 45 SDLK_MINUS
	keyPeriod,		// 46 SDLK_PERIOD
	keySlash,		// 47 SDLK_SLASH
	key0,			// 48 SDLK_0
	key1,			// 49 SDLK_1
	key2,			// 50 SDLK_2
	key3,			// 51 SDLK_3
	key4,			// 52 SDLK_4
	key5,			// 53 SDLK_5
	key6,			// 54 SDLK_6
	key7,			// 55 SDLK_7
	key8,			// 56 SDLK_8
	key9,			// 57 SDLK_9
	keyColon,		// 58 SDLK_COLON
	keySemicolon,	// 59 SDLK_SEMICOLON
	keyLess,		// 60 SDLK_LESS
	keyEquals,		// 61 SDLK_EQUALS
	keyGreater,		// 62 SDLK_GREATER
	keyQuestion,	// 63 SDLK_QUESTION
	keyAt,			// 64 SDLK_AT
	keyA,			// 65 'A'
	keyB,			// 66 'B'
	keyC,			// 67 'C'
	keyD,			// 68 'D'
	keyE,			// 69 'E'
	keyF,			// 70 'F'
	keyG,			// 71 'G'
	keyH,			// 72 'H'
	keyI,			// 73 'I'
	keyJ,			// 74 'J'
	keyK,			// 75 'K'
	keyL,			// 76 'L'
	keyM,			// 77 'M'
	keyN,			// 78 'N'
	keyO,			// 79 'O'
	keyP,			// 80 'P'
	keyQ,			// 81 'Q'
	keyR,			// 82 'R'
	keyS,			// 83 'S'
	keyT,			// 84 'T'
	keyU,			// 85 'U'
	keyV,			// 86 'V'
	keyW,			// 87 'W'
	keyX,			// 88 'X'
	keyY,			// 89 'Y'
	keyZ,			// 90 'Z'
	keyLeftBracket,	// 91 SDLK_LEFTBRACKET
	keyBackslash,	// 92 SDLK_BACKSLASH
	keyRightBracket,	// 93 SDLK_RIGHTBRACKET
	keyCaret,		// 94 SDLK_CARET
	keyUnderscore,	// 95 SDLK_UNDERSCORE
	keyBackquote,	// 96 SDLK_BACKQUOTE
	keyA,			// 97 SDLK_a
	keyB,			// 98 SDLK_b
	keyC,			// 99 SDLK_c
	keyD,			// 100 SDLK_d
	keyE,			// 101 SDLK_e
	keyF,			// 102 SDLK_f
	keyG,			// 103 SDLK_g
	keyH,			// 104 SDLK_h
	keyI,			// 105 SDLK_i
	keyJ,			// 106 SDLK_j
	keyK,			// 107 SDLK_k
	keyL,			// 108 SDLK_l
	keyM,			// 109 SDLK_m
	keyN,			// 110 SDLK_n
	keyO,			// 111 SDLK_o
	keyP,			// 112 SDLK_p
	keyQ,			// 113 SDLK_q
	keyR,			// 114 SDLK_r
	keyS,			// 115 SDLK_s
	keyT,			// 116 SDLK_t
	keyU,			// 117 SDLK_u
	keyV,			// 118 SDLK_v
	keyW,			// 119 SDLK_w
	keyX,			// 120 SDLK_x
	keyY,			// 121 SDLK_y
	keyZ,			// 122 SDLK_z
	keyUnknown,		// 123
	keyUnknown,		// 124
	keyUnknown,		// 125
	keyUnknown,		// 126
	keyDelete,		// 127 SDLK_DELETE
	keyUnknown,		// 128
	keyUnknown,		// 129
	keyUnknown,		// 130
	keyUnknown,		// 131
	keyUnknown,		// 132
	keyUnknown,		// 133
	keyUnknown,		// 134
	keyUnknown,		// 135
	keyUnknown,		// 136
	keyUnknown,		// 137
	keyUnknown,		// 138
	keyUnknown,		// 139
	keyUnknown,		// 140
	keyUnknown,		// 141
	keyUnknown,		// 142
	keyUnknown,		// 143
	keyUnknown,		// 144
	keyUnknown,		// 145
	keyUnknown,		// 146
	keyUnknown,		// 147
	keyUnknown,		// 148
	keyUnknown,		// 149
	keyUnknown,		// 150
	keyUnknown,		// 151
	keyUnknown,		// 152
	keyUnknown,		// 153
	keyUnknown,		// 154
	keyUnknown,		// 155
	keyUnknown,		// 156
	keyUnknown,		// 157
	keyUnknown,		// 158
	keyUnknown,		// 159
	keyWorld0,		// 160 SDLK_WORLD_0
	keyWorld1,		// 161 SDLK_WORLD_1
	keyWorld2,		// 162 SDLK_WORLD_2
	keyWorld3,		// 163 SDLK_WORLD_3
	keyWorld4,		// 164 SDLK_WORLD_4
	keyWorld5,		// 165 SDLK_WORLD_5
	keyWorld6,		// 166 SDLK_WORLD_6
	keyWorld7,		// 167 SDLK_WORLD_7
	keyWorld8,		// 168 SDLK_WORLD_8
	keyWorld9,		// 169 SDLK_WORLD_9
	keyWorld10,		// 170 SDLK_WORLD_10
	keyWorld11,		// 171 SDLK_WORLD_11
	keyWorld12,		// 172 SDLK_WORLD_12
	keyWorld13,		// 173 SDLK_WORLD_13
	keyWorld14,		// 174 SDLK_WORLD_14
	keyWorld15,		// 175 SDLK_WORLD_15
	keyWorld16,		// 176 SDLK_WORLD_16
	keyWorld17,		// 177 SDLK_WORLD_17
	keyWorld18,		// 178 SDLK_WORLD_18
	keyWorld19,		// 179 SDLK_WORLD_19
	keyWorld20,		// 180 SDLK_WORLD_20
	keyWorld21,		// 181 SDLK_WORLD_21
	keyWorld22,		// 182 SDLK_WORLD_22
	keyWorld23,		// 183 SDLK_WORLD_23
	keyWorld24,		// 184 SDLK_WORLD_24
	keyWorld25,		// 185 SDLK_WORLD_25
	keyWorld26,		// 186 SDLK_WORLD_26
	keyWorld27,		// 187 SDLK_WORLD_27
	keyWorld28,		// 188 SDLK_WORLD_28
	keyWorld29,		// 189 SDLK_WORLD_29
	keyWorld30,		// 190 SDLK_WORLD_30
	keyWorld31,		// 191 SDLK_WORLD_31
	keyWorld32,		// 192 SDLK_WORLD_32
	keyWorld33,		// 193 SDLK_WORLD_33
	keyWorld34,		// 194 SDLK_WORLD_34
	keyWorld35,		// 195 SDLK_WORLD_35
	keyWorld36,		// 196 SDLK_WORLD_36
	keyWorld37,		// 197 SDLK_WORLD_37
	keyWorld38,		// 198 SDLK_WORLD_38
	keyWorld39,		// 199 SDLK_WORLD_39
	keyWorld40,		// 200 SDLK_WORLD_40
	keyWorld41,		// 201 SDLK_WORLD_41
	keyWorld42,		// 202 SDLK_WORLD_42
	keyWorld43,		// 203 SDLK_WORLD_43
	keyWorld44,		// 204 SDLK_WORLD_44
	keyWorld45,		// 205 SDLK_WORLD_45
	keyWorld46,		// 206 SDLK_WORLD_46
	keyWorld47,		// 207 SDLK_WORLD_47
	keyWorld48,		// 208 SDLK_WORLD_48
	keyWorld49,		// 209 SDLK_WORLD_49
	keyWorld50,		// 210 SDLK_WORLD_50
	keyWorld51,		// 211 SDLK_WORLD_51
	keyWorld52,		// 212 SDLK_WORLD_52
	keyWorld53,		// 213 SDLK_WORLD_53
	keyWorld54,		// 214 SDLK_WORLD_54
	keyWorld55,		// 215 SDLK_WORLD_55
	keyWorld56,		// 216 SDLK_WORLD_56
	keyWorld57,		// 217 SDLK_WORLD_57
	keyWorld58,		// 218 SDLK_WORLD_58
	keyWorld59,		// 219 SDLK_WORLD_59
	keyWorld60,		// 220 SDLK_WORLD_60
	keyWorld61,		// 221 SDLK_WORLD_61
	keyWorld62,		// 222 SDLK_WORLD_62
	keyWorld63,		// 223 SDLK_WORLD_63
	keyWorld64,		// 224 SDLK_WORLD_64
	keyWorld65,		// 225 SDLK_WORLD_65
	keyWorld66,		// 226 SDLK_WORLD_66
	keyWorld67,		// 227 SDLK_WORLD_67
	keyWorld68,		// 228 SDLK_WORLD_68
	keyWorld69,		// 229 SDLK_WORLD_69
	keyWorld70,		// 230 SDLK_WORLD_70
	keyWorld71,		// 231 SDLK_WORLD_71
	keyWorld72,		// 232 SDLK_WORLD_72
	keyWorld73,		// 233 SDLK_WORLD_73
	keyWorld74,		// 234 SDLK_WORLD_74
	keyWorld75,		// 235 SDLK_WORLD_75
	keyWorld76,		// 236 SDLK_WORLD_76
	keyWorld77,		// 237 SDLK_WORLD_77
	keyWorld78,		// 238 SDLK_WORLD_78
	keyWorld79,		// 239 SDLK_WORLD_79
	keyWorld80,		// 240 SDLK_WORLD_80
	keyWorld81,		// 241 SDLK_WORLD_81
	keyWorld82,		// 242 SDLK_WORLD_82
	keyWorld83,		// 243 SDLK_WORLD_83
	keyWorld84,		// 244 SDLK_WORLD_84
	keyWorld85,		// 245 SDLK_WORLD_85
	keyWorld86,		// 246 SDLK_WORLD_86
	keyWorld87,		// 247 SDLK_WORLD_87
	keyWorld88,		// 248 SDLK_WORLD_88
	keyWorld89,		// 249 SDLK_WORLD_89
	keyWorld90,		// 250 SDLK_WORLD_90
	keyWorld91,		// 251 SDLK_WORLD_91
	keyWorld92,		// 252 SDLK_WORLD_92
	keyWorld93,		// 253 SDLK_WORLD_93
	keyWorld94,		// 254 SDLK_WORLD_94
	keyWorld95,		// 255 SDLK_WORLD_95
	keyKP0,			// 256 SDLK_KP0
	keyKP1,			// 257 SDLK_KP1
	keyKP2,			// 258 SDLK_KP2
	keyKP3,			// 259 SDLK_KP3
	keyKP4,			// 260 SDLK_KP4
	keyKP5,			// 261 SDLK_KP5
	keyKP6,			// 262 SDLK_KP6
	keyKP7,			// 263 SDLK_KP7
	keyKP8,			// 264 SDLK_KP8
	keyKP9,			// 265 SDLK_KP9
	keyKPPeriod,	// 266 SDLK_KP_PERIOD
	keyKPDivide,	// 267 SDLK_KP_DIVIDE
	keyKPMultiply,	// 268 SDLK_KP_MULTIPLY
	keyKPMinus,		// 269 SDLK_KP_MINUS
	keyKPPlus,		// 270 SDLK_KP_PLUS
	keyKPEnter,		// 271 SDLK_KP_ENTER
	keyKPEquals,	// 272 SDLK_KP_EQUALS
	keyUp,			// 273 SDLK_UP
	keyDown,		// 274 SDLK_DOWN
	keyRight,		// 275 SDLK_RIGHT
	keyLeft,		// 276 SDLK_LEFT
	keyInsert,		// 277 SDLK_INSERT
	keyHome,		// 278 SDLK_HOME
	keyEnd,			// 279 SDLK_END
	keyPageUp,		// 280 SDLK_PAGEUP
	keyPageDown,	// 281 SDLK_PAGEDOWN
	keyF1,			// 282 SDLK_F1
	keyF2,			// 283 SDLK_F2
	keyF3,			// 284 SDLK_F3
	keyF4,			// 285 SDLK_F4
	keyF5,			// 286 SDLK_F5
	keyF6,			// 287 SDLK_F6
	keyF7,			// 288 SDLK_F7
	keyF8,			// 289 SDLK_F8
	keyF9,			// 290 SDLK_F9
	keyF10,			// 291 SDLK_F10
	keyF11,			// 292 SDLK_F11
	keyF12,			// 293 SDLK_F12
	keyF13,			// 294 SDLK_F13
	keyF14,			// 295 SDLK_F14
	keyF15,			// 296 SDLK_F15
	keyUnknown,		// 297
	keyUnknown,		// 298
	keyUnknown,		// 299
	keyNumLock,		// 300 SDLK_NUMLOCK
	keyCapsLock,	// 301 SDLK_CAPSLOCK
	keyScrollLock,	// 302 SDLK_SCROLLOCK
	keyRShift,		// 303 SDLK_RSHIFT
	keyLShift,		// 304 SDLK_LSHIFT
	keyRCtrl,		// 305 SDLK_RCTRL
	keyLCtrl,		// 306 SDLK_LCTRL
	keyRAlt,		// 307 SDLK_RALT
	keyLAlt,		// 308 SDLK_LALT
	keyRMeta,		// 309 SDLK_RMETA
	keyLMeta,		// 310 SDLK_LMETA
	keyLSuper,		// 311 SDLK_LSUPER
	keyRSuper,		// 312 SDLK_RSUPER
	keyMode,		// 313 SDLK_MODE
	keyCompose,		// 314 SDLK_COMPOSE
	keyHelp,		// 315 SDLK_HELP
	keyPrint,		// 316 SDLK_PRINT
	keySysreq,		// 317 SDLK_SYSREQ
	keyBreak,		// 318 SDLK_BREAK
	keyMenu,		// 319 SDLK_MENU
	keyPower,		// 320 SDLK_POWER
	keyEuro,		// 321 SDLK_EURO
	keyUndo			// 322 SDLK_UNDO
};

const NativeKeyCodeCompact Input::kc2native[keyCount] = {
	SDLK_UNKNOWN,		// keyNone
	SDLK_UNKNOWN,		// keyUnknown
	SDLK_UNKNOWN,		// keyLButton
	SDLK_UNKNOWN,		// keyRButton
	SDLK_UNKNOWN,		// keyMButton
	SDLK_UNKNOWN,		// keyXButton1
	SDLK_UNKNOWN,		// keyXButton2
	SDLK_BACKSPACE,		// keyBackspace
	SDLK_TAB,			// keyTab
	SDLK_CLEAR,			// keyClear
	SDLK_RETURN,		// keyReturn
	SDLK_PAUSE,			// keyPause
	SDLK_UNKNOWN,		// keyIMEKana
	SDLK_UNKNOWN,		// keyIMEHangul
	SDLK_UNKNOWN,		// keyIMEJunja
	SDLK_UNKNOWN,		// keyIMEFinal
	SDLK_UNKNOWN,		// keyIMEHanja
	SDLK_UNKNOWN,		// keyIMEKanji
	SDLK_ESCAPE,		// keyEscape
	SDLK_UNKNOWN,		// keyIMEConvert
	SDLK_UNKNOWN,		// keyIMENonConvert
	SDLK_UNKNOWN,		// keyIMEAccept
	SDLK_UNKNOWN,		// keyIMEModeChange
	SDLK_UNKNOWN,		// keyIMEProcess
	SDLK_SPACE,			// keySpace
	SDLK_EXCLAIM,		// keyExclaim
	SDLK_QUOTEDBL,		// keyQuoteDbl
	SDLK_HASH,			// keyHash
	SDLK_DOLLAR,		// keyDollar
	SDLK_UNKNOWN,		// keyPercent
	SDLK_AMPERSAND,		// keyAmpersand
	SDLK_QUOTE,			// keyQuote
	SDLK_LEFTPAREN,		// keyLeftParen
	SDLK_RIGHTPAREN,	// keyRightParen
	SDLK_ASTERISK,		// keyAsterisk
	SDLK_PLUS,			// keyPlus
	SDLK_COMMA,			// keyComma
	SDLK_MINUS,			// keyMinus
	SDLK_PERIOD,		// keyPeriod
	SDLK_SLASH,			// keySlash
	SDLK_0,				// key0
	SDLK_1,				// key1
	SDLK_2,				// key2
	SDLK_3,				// key3
	SDLK_4,				// key4
	SDLK_5,				// key5
	SDLK_6,				// key6
	SDLK_7,				// key7
	SDLK_8,				// key8
	SDLK_9,				// key9
	SDLK_COLON,			// keyColon
	SDLK_SEMICOLON,		// keySemicolon
	SDLK_LESS,			// keyLess
	SDLK_EQUALS,		// keyEquals
	SDLK_GREATER,		// keyGreater
	SDLK_QUESTION,		// keyQuestion
	SDLK_AT,			// keyAt
	'A',				// keyA
	'B',				// keyB
	'C',				// keyC
	'D',				// keyD
	'E',				// keyE
	'F',				// keyF
	'G',				// keyG
	'H',				// keyH
	'I',				// keyI
	'J',				// keyJ
	'K',				// keyK
	'L',				// keyL
	'M',				// keyM
	'N',				// keyN
	'O',				// keyO
	'P',				// keyP
	'Q',				// keyQ
	'R',				// keyR
	'S',				// keyS
	'T',				// keyT
	'U',				// keyU
	'V',				// keyV
	'W',				// keyW
	'X',				// keyX
	'Y',				// keyY
	'Z',				// keyZ
	SDLK_LEFTBRACKET,	// keyLeftBracket
	SDLK_BACKSLASH,		// keyBackslash
	SDLK_RIGHTBRACKET,	// keyRightBracket
	SDLK_CARET,			// keyCaret
	SDLK_UNDERSCORE,	// keyUnderscore
	SDLK_BACKQUOTE,		// keyBackquote
	SDLK_DELETE,		// keyDelete
	SDLK_WORLD_0,		// keyWorld0
	SDLK_WORLD_1,		// keyWorld1
	SDLK_WORLD_2,		// keyWorld2
	SDLK_WORLD_3,		// keyWorld3
	SDLK_WORLD_4,		// keyWorld4
	SDLK_WORLD_5,		// keyWorld5
	SDLK_WORLD_6,		// keyWorld6
	SDLK_WORLD_7,		// keyWorld7
	SDLK_WORLD_8,		// keyWorld8
	SDLK_WORLD_9,		// keyWorld9
	SDLK_WORLD_10,		// keyWorld10
	SDLK_WORLD_11,		// keyWorld11
	SDLK_WORLD_12,		// keyWorld12
	SDLK_WORLD_13,		// keyWorld13
	SDLK_WORLD_14,		// keyWorld14
	SDLK_WORLD_15,		// keyWorld15
	SDLK_WORLD_16,		// keyWorld16
	SDLK_WORLD_17,		// keyWorld17
	SDLK_WORLD_18,		// keyWorld18
	SDLK_WORLD_19,		// keyWorld19
	SDLK_WORLD_20,		// keyWorld20
	SDLK_WORLD_21,		// keyWorld21
	SDLK_WORLD_22,		// keyWorld22
	SDLK_WORLD_23,		// keyWorld23
	SDLK_WORLD_24,		// keyWorld24
	SDLK_WORLD_25,		// keyWorld25
	SDLK_WORLD_26,		// keyWorld26
	SDLK_WORLD_27,		// keyWorld27
	SDLK_WORLD_28,		// keyWorld28
	SDLK_WORLD_29,		// keyWorld29
	SDLK_WORLD_30,		// keyWorld30
	SDLK_WORLD_31,		// keyWorld31
	SDLK_WORLD_32,		// keyWorld32
	SDLK_WORLD_33,		// keyWorld33
	SDLK_WORLD_34,		// keyWorld34
	SDLK_WORLD_35,		// keyWorld35
	SDLK_WORLD_36,		// keyWorld36
	SDLK_WORLD_37,		// keyWorld37
	SDLK_WORLD_38,		// keyWorld38
	SDLK_WORLD_39,		// keyWorld39
	SDLK_WORLD_40,		// keyWorld40
	SDLK_WORLD_41,		// keyWorld41
	SDLK_WORLD_42,		// keyWorld42
	SDLK_WORLD_43,		// keyWorld43
	SDLK_WORLD_44,		// keyWorld44
	SDLK_WORLD_45,		// keyWorld45
	SDLK_WORLD_46,		// keyWorld46
	SDLK_WORLD_47,		// keyWorld47
	SDLK_WORLD_48,		// keyWorld48
	SDLK_WORLD_49,		// keyWorld49
	SDLK_WORLD_50,		// keyWorld50
	SDLK_WORLD_51,		// keyWorld51
	SDLK_WORLD_52,		// keyWorld52
	SDLK_WORLD_53,		// keyWorld53
	SDLK_WORLD_54,		// keyWorld54
	SDLK_WORLD_55,		// keyWorld55
	SDLK_WORLD_56,		// keyWorld56
	SDLK_WORLD_57,		// keyWorld57
	SDLK_WORLD_58,		// keyWorld58
	SDLK_WORLD_59,		// keyWorld59
	SDLK_WORLD_60,		// keyWorld60
	SDLK_WORLD_61,		// keyWorld61
	SDLK_WORLD_62,		// keyWorld62
	SDLK_WORLD_63,		// keyWorld63
	SDLK_WORLD_64,		// keyWorld64
	SDLK_WORLD_65,		// keyWorld65
	SDLK_WORLD_66,		// keyWorld66
	SDLK_WORLD_67,		// keyWorld67
	SDLK_WORLD_68,		// keyWorld68
	SDLK_WORLD_69,		// keyWorld69
	SDLK_WORLD_70,		// keyWorld70
	SDLK_WORLD_71,		// keyWorld71
	SDLK_WORLD_72,		// keyWorld72
	SDLK_WORLD_73,		// keyWorld73
	SDLK_WORLD_74,		// keyWorld74
	SDLK_WORLD_75,		// keyWorld75
	SDLK_WORLD_76,		// keyWorld76
	SDLK_WORLD_77,		// keyWorld77
	SDLK_WORLD_78,		// keyWorld78
	SDLK_WORLD_79,		// keyWorld79
	SDLK_WORLD_80,		// keyWorld80
	SDLK_WORLD_81,		// keyWorld81
	SDLK_WORLD_82,		// keyWorld82
	SDLK_WORLD_83,		// keyWorld83
	SDLK_WORLD_84,		// keyWorld84
	SDLK_WORLD_85,		// keyWorld85
	SDLK_WORLD_86,		// keyWorld86
	SDLK_WORLD_87,		// keyWorld87
	SDLK_WORLD_88,		// keyWorld88
	SDLK_WORLD_89,		// keyWorld89
	SDLK_WORLD_90,		// keyWorld90
	SDLK_WORLD_91,		// keyWorld91
	SDLK_WORLD_92,		// keyWorld92
	SDLK_WORLD_93,		// keyWorld93
	SDLK_WORLD_94,		// keyWorld94
	SDLK_WORLD_95,		// keyWorld95
	SDLK_KP0,			// keyKP0
	SDLK_KP1,			// keyKP1
	SDLK_KP2,			// keyKP2
	SDLK_KP3,			// keyKP3
	SDLK_KP4,			// keyKP4
	SDLK_KP5,			// keyKP5
	SDLK_KP6,			// keyKP6
	SDLK_KP7,			// keyKP7
	SDLK_KP8,			// keyKP8
	SDLK_KP9,			// keyKP9
	SDLK_KP_PERIOD,		// keyKPPeriod
	SDLK_KP_DIVIDE,		// keyKPDivide
	SDLK_KP_MULTIPLY,	// keyKPMultiply
	SDLK_KP_MINUS,		// keyKPMinus
	SDLK_KP_PLUS,		// keyKPPlus
	SDLK_KP_ENTER,		// keyKPEnter
	SDLK_KP_EQUALS,		// keyKPEquals
	SDLK_UP,			// keyUp
	SDLK_DOWN,			// keyDown
	SDLK_RIGHT,			// keyRight
	SDLK_LEFT,			// keyLeft
	SDLK_INSERT,		// keyInsert
	SDLK_HOME,			// keyHome
	SDLK_END,			// keyEnd
	SDLK_PAGEUP,		// keyPageUp
	SDLK_PAGEDOWN,		// keyPageDown
	SDLK_F1,			// keyF1
	SDLK_F2,			// keyF2
	SDLK_F3,			// keyF3
	SDLK_F4,			// keyF4
	SDLK_F5,			// keyF5
	SDLK_F6,			// keyF6
	SDLK_F7,			// keyF7
	SDLK_F8,			// keyF8
	SDLK_F9,			// keyF9
	SDLK_F10,			// keyF10
	SDLK_F11,			// keyF11
	SDLK_F12,			// keyF12
	SDLK_F13,			// keyF13
	SDLK_F14,			// keyF14
	SDLK_F15,			// keyF15
	SDLK_UNKNOWN,		// keyF16
	SDLK_UNKNOWN,		// keyF17
	SDLK_UNKNOWN,		// keyF18
	SDLK_UNKNOWN,		// keyF19
	SDLK_UNKNOWN,		// keyF20
	SDLK_UNKNOWN,		// keyF21
	SDLK_UNKNOWN,		// keyF22
	SDLK_UNKNOWN,		// keyF23
	SDLK_UNKNOWN,		// keyF24
	SDLK_NUMLOCK,		// keyNumLock
	SDLK_CAPSLOCK,		// keyCapsLock
	SDLK_SCROLLOCK,		// keyScrollLock
	SDLK_LSHIFT,		// keyLShift
	SDLK_RSHIFT,		// keyRShift
	SDLK_LCTRL,			// keyLCtrl
	SDLK_RCTRL,			// keyRCtrl
	SDLK_LALT,			// keyLAlt
	SDLK_RALT,			// keyRAlt
	SDLK_LMETA,			// keyLMeta
	SDLK_RMETA,			// keyRMeta
	SDLK_RSUPER,		// keyRSuper
	SDLK_LSUPER,		// keyLSuper
	SDLK_MODE,			// keyMode
	SDLK_COMPOSE,		// keyCompose
	SDLK_HELP,			// keyHelp
	SDLK_PRINT,			// keyPrint
	SDLK_SYSREQ,		// keySysreq
	SDLK_BREAK,			// keyBreak
	SDLK_MENU,			// keyMenu
	SDLK_POWER,			// keyPower
	SDLK_EURO,			// keyEuro
	SDLK_UNDO,			// keyUndo
	SDLK_UNKNOWN,		// keyBrowserBack
	SDLK_UNKNOWN,		// keyBrowserForward
	SDLK_UNKNOWN,		// keyBrowserRefresh
	SDLK_UNKNOWN,		// keyBrowserStop
	SDLK_UNKNOWN,		// keyBrowserSearch
	SDLK_UNKNOWN,		// keyBrowserFavorites
	SDLK_UNKNOWN,		// keyBrowserHome
	SDLK_UNKNOWN,		// keyVolumeMute
	SDLK_UNKNOWN,		// keyVolumeDown
	SDLK_UNKNOWN,		// keyVolumeUp
	SDLK_UNKNOWN,		// keyMediaNext
	SDLK_UNKNOWN,		// keyMediaPrev
	SDLK_UNKNOWN,		// keyMediaStop
	SDLK_UNKNOWN,		// keyMediaPlayPause
	SDLK_UNKNOWN,		// keyLaunchMail
	SDLK_UNKNOWN,		// keyLaunchMediaSelect
	SDLK_UNKNOWN,		// keyLaunchApp1
	SDLK_UNKNOWN,		// keyLaunchApp2
	SDLK_UNKNOWN,		// keyPlay
	SDLK_UNKNOWN		// keyZoom
};

#pragma pack(pop)

void Input::verifyTranslationTables() {
	// verify integrity of SDL to KeyCode table
	assert(native2kc[SDLK_UNKNOWN] == keyUnknown);
	assert(native2kc[SDLK_BACKSPACE] == keyBackspace);
	assert(native2kc[SDLK_TAB] == keyTab);
	assert(native2kc[SDLK_CLEAR] == keyClear);
	assert(native2kc[SDLK_RETURN] == keyReturn);
	assert(native2kc[SDLK_PAUSE] == keyPause);
	assert(native2kc[SDLK_ESCAPE] == keyEscape);
	assert(native2kc[SDLK_SPACE] == keySpace);
	assert(native2kc[SDLK_EXCLAIM] == keyExclaim);
	assert(native2kc[SDLK_QUOTEDBL] == keyQuoteDbl);
	assert(native2kc[SDLK_HASH] == keyHash);
	assert(native2kc[SDLK_DOLLAR] == keyDollar);
	assert(native2kc[SDLK_AMPERSAND] == keyAmpersand);
	assert(native2kc[SDLK_QUOTE] == keyQuote);
	assert(native2kc[SDLK_LEFTPAREN] == keyLeftParen);
	assert(native2kc[SDLK_RIGHTPAREN] == keyRightParen);
	assert(native2kc[SDLK_ASTERISK] == keyAsterisk);
	assert(native2kc[SDLK_PLUS] == keyPlus);
	assert(native2kc[SDLK_COMMA] == keyComma);
	assert(native2kc[SDLK_MINUS] == keyMinus);
	assert(native2kc[SDLK_PERIOD] == keyPeriod);
	assert(native2kc[SDLK_SLASH] == keySlash);
	assert(native2kc[SDLK_0] == key0);
	assert(native2kc[SDLK_1] == key1);
	assert(native2kc[SDLK_2] == key2);
	assert(native2kc[SDLK_3] == key3);
	assert(native2kc[SDLK_4] == key4);
	assert(native2kc[SDLK_5] == key5);
	assert(native2kc[SDLK_6] == key6);
	assert(native2kc[SDLK_7] == key7);
	assert(native2kc[SDLK_8] == key8);
	assert(native2kc[SDLK_9] == key9);
	assert(native2kc[SDLK_COLON] == keyColon);
	assert(native2kc[SDLK_SEMICOLON] == keySemicolon);
	assert(native2kc[SDLK_LESS] == keyLess);
	assert(native2kc[SDLK_EQUALS] == keyEquals);
	assert(native2kc[SDLK_GREATER] == keyGreater);
	assert(native2kc[SDLK_QUESTION] == keyQuestion);
	assert(native2kc[SDLK_AT] == keyAt);
	assert(native2kc['A'] == keyA);
	assert(native2kc['B'] == keyB);
	assert(native2kc['C'] == keyC);
	assert(native2kc['D'] == keyD);
	assert(native2kc['E'] == keyE);
	assert(native2kc['F'] == keyF);
	assert(native2kc['G'] == keyG);
	assert(native2kc['H'] == keyH);
	assert(native2kc['I'] == keyI);
	assert(native2kc['J'] == keyJ);
	assert(native2kc['K'] == keyK);
	assert(native2kc['L'] == keyL);
	assert(native2kc['M'] == keyM);
	assert(native2kc['N'] == keyN);
	assert(native2kc['O'] == keyO);
	assert(native2kc['P'] == keyP);
	assert(native2kc['Q'] == keyQ);
	assert(native2kc['R'] == keyR);
	assert(native2kc['S'] == keyS);
	assert(native2kc['T'] == keyT);
	assert(native2kc['U'] == keyU);
	assert(native2kc['V'] == keyV);
	assert(native2kc['W'] == keyW);
	assert(native2kc['X'] == keyX);
	assert(native2kc['Y'] == keyY);
	assert(native2kc['Z'] == keyZ);
	assert(native2kc[SDLK_LEFTBRACKET] == keyLeftBracket);
	assert(native2kc[SDLK_BACKSLASH] == keyBackslash);
	assert(native2kc[SDLK_RIGHTBRACKET] == keyRightBracket);
	assert(native2kc[SDLK_CARET] == keyCaret);
	assert(native2kc[SDLK_UNDERSCORE] == keyUnderscore);
	assert(native2kc[SDLK_BACKQUOTE] == keyBackquote);
	assert(native2kc[SDLK_a] == keyA);
	assert(native2kc[SDLK_b] == keyB);
	assert(native2kc[SDLK_c] == keyC);
	assert(native2kc[SDLK_d] == keyD);
	assert(native2kc[SDLK_e] == keyE);
	assert(native2kc[SDLK_f] == keyF);
	assert(native2kc[SDLK_g] == keyG);
	assert(native2kc[SDLK_h] == keyH);
	assert(native2kc[SDLK_i] == keyI);
	assert(native2kc[SDLK_j] == keyJ);
	assert(native2kc[SDLK_k] == keyK);
	assert(native2kc[SDLK_l] == keyL);
	assert(native2kc[SDLK_m] == keyM);
	assert(native2kc[SDLK_n] == keyN);
	assert(native2kc[SDLK_o] == keyO);
	assert(native2kc[SDLK_p] == keyP);
	assert(native2kc[SDLK_q] == keyQ);
	assert(native2kc[SDLK_r] == keyR);
	assert(native2kc[SDLK_s] == keyS);
	assert(native2kc[SDLK_t] == keyT);
	assert(native2kc[SDLK_u] == keyU);
	assert(native2kc[SDLK_v] == keyV);
	assert(native2kc[SDLK_w] == keyW);
	assert(native2kc[SDLK_x] == keyX);
	assert(native2kc[SDLK_y] == keyY);
	assert(native2kc[SDLK_z] == keyZ);
	assert(native2kc[SDLK_DELETE] == keyDelete);
	assert(native2kc[SDLK_DELETE] == keyDelete);
	assert(native2kc[SDLK_WORLD_0] == keyWorld0);
	assert(native2kc[SDLK_WORLD_1] == keyWorld1);
	assert(native2kc[SDLK_WORLD_2] == keyWorld2);
	assert(native2kc[SDLK_WORLD_3] == keyWorld3);
	assert(native2kc[SDLK_WORLD_4] == keyWorld4);
	assert(native2kc[SDLK_WORLD_5] == keyWorld5);
	assert(native2kc[SDLK_WORLD_6] == keyWorld6);
	assert(native2kc[SDLK_WORLD_7] == keyWorld7);
	assert(native2kc[SDLK_WORLD_8] == keyWorld8);
	assert(native2kc[SDLK_WORLD_9] == keyWorld9);
	assert(native2kc[SDLK_WORLD_10] == keyWorld10);
	assert(native2kc[SDLK_WORLD_11] == keyWorld11);
	assert(native2kc[SDLK_WORLD_12] == keyWorld12);
	assert(native2kc[SDLK_WORLD_13] == keyWorld13);
	assert(native2kc[SDLK_WORLD_14] == keyWorld14);
	assert(native2kc[SDLK_WORLD_15] == keyWorld15);
	assert(native2kc[SDLK_WORLD_16] == keyWorld16);
	assert(native2kc[SDLK_WORLD_17] == keyWorld17);
	assert(native2kc[SDLK_WORLD_18] == keyWorld18);
	assert(native2kc[SDLK_WORLD_19] == keyWorld19);
	assert(native2kc[SDLK_WORLD_20] == keyWorld20);
	assert(native2kc[SDLK_WORLD_21] == keyWorld21);
	assert(native2kc[SDLK_WORLD_22] == keyWorld22);
	assert(native2kc[SDLK_WORLD_23] == keyWorld23);
	assert(native2kc[SDLK_WORLD_24] == keyWorld24);
	assert(native2kc[SDLK_WORLD_25] == keyWorld25);
	assert(native2kc[SDLK_WORLD_26] == keyWorld26);
	assert(native2kc[SDLK_WORLD_27] == keyWorld27);
	assert(native2kc[SDLK_WORLD_28] == keyWorld28);
	assert(native2kc[SDLK_WORLD_29] == keyWorld29);
	assert(native2kc[SDLK_WORLD_30] == keyWorld30);
	assert(native2kc[SDLK_WORLD_31] == keyWorld31);
	assert(native2kc[SDLK_WORLD_32] == keyWorld32);
	assert(native2kc[SDLK_WORLD_33] == keyWorld33);
	assert(native2kc[SDLK_WORLD_34] == keyWorld34);
	assert(native2kc[SDLK_WORLD_35] == keyWorld35);
	assert(native2kc[SDLK_WORLD_36] == keyWorld36);
	assert(native2kc[SDLK_WORLD_37] == keyWorld37);
	assert(native2kc[SDLK_WORLD_38] == keyWorld38);
	assert(native2kc[SDLK_WORLD_39] == keyWorld39);
	assert(native2kc[SDLK_WORLD_40] == keyWorld40);
	assert(native2kc[SDLK_WORLD_41] == keyWorld41);
	assert(native2kc[SDLK_WORLD_42] == keyWorld42);
	assert(native2kc[SDLK_WORLD_43] == keyWorld43);
	assert(native2kc[SDLK_WORLD_44] == keyWorld44);
	assert(native2kc[SDLK_WORLD_45] == keyWorld45);
	assert(native2kc[SDLK_WORLD_46] == keyWorld46);
	assert(native2kc[SDLK_WORLD_47] == keyWorld47);
	assert(native2kc[SDLK_WORLD_48] == keyWorld48);
	assert(native2kc[SDLK_WORLD_49] == keyWorld49);
	assert(native2kc[SDLK_WORLD_50] == keyWorld50);
	assert(native2kc[SDLK_WORLD_51] == keyWorld51);
	assert(native2kc[SDLK_WORLD_52] == keyWorld52);
	assert(native2kc[SDLK_WORLD_53] == keyWorld53);
	assert(native2kc[SDLK_WORLD_54] == keyWorld54);
	assert(native2kc[SDLK_WORLD_55] == keyWorld55);
	assert(native2kc[SDLK_WORLD_56] == keyWorld56);
	assert(native2kc[SDLK_WORLD_57] == keyWorld57);
	assert(native2kc[SDLK_WORLD_58] == keyWorld58);
	assert(native2kc[SDLK_WORLD_59] == keyWorld59);
	assert(native2kc[SDLK_WORLD_60] == keyWorld60);
	assert(native2kc[SDLK_WORLD_61] == keyWorld61);
	assert(native2kc[SDLK_WORLD_62] == keyWorld62);
	assert(native2kc[SDLK_WORLD_63] == keyWorld63);
	assert(native2kc[SDLK_WORLD_64] == keyWorld64);
	assert(native2kc[SDLK_WORLD_65] == keyWorld65);
	assert(native2kc[SDLK_WORLD_66] == keyWorld66);
	assert(native2kc[SDLK_WORLD_67] == keyWorld67);
	assert(native2kc[SDLK_WORLD_68] == keyWorld68);
	assert(native2kc[SDLK_WORLD_69] == keyWorld69);
	assert(native2kc[SDLK_WORLD_70] == keyWorld70);
	assert(native2kc[SDLK_WORLD_71] == keyWorld71);
	assert(native2kc[SDLK_WORLD_72] == keyWorld72);
	assert(native2kc[SDLK_WORLD_73] == keyWorld73);
	assert(native2kc[SDLK_WORLD_74] == keyWorld74);
	assert(native2kc[SDLK_WORLD_75] == keyWorld75);
	assert(native2kc[SDLK_WORLD_76] == keyWorld76);
	assert(native2kc[SDLK_WORLD_77] == keyWorld77);
	assert(native2kc[SDLK_WORLD_78] == keyWorld78);
	assert(native2kc[SDLK_WORLD_79] == keyWorld79);
	assert(native2kc[SDLK_WORLD_80] == keyWorld80);
	assert(native2kc[SDLK_WORLD_81] == keyWorld81);
	assert(native2kc[SDLK_WORLD_82] == keyWorld82);
	assert(native2kc[SDLK_WORLD_83] == keyWorld83);
	assert(native2kc[SDLK_WORLD_84] == keyWorld84);
	assert(native2kc[SDLK_WORLD_85] == keyWorld85);
	assert(native2kc[SDLK_WORLD_86] == keyWorld86);
	assert(native2kc[SDLK_WORLD_87] == keyWorld87);
	assert(native2kc[SDLK_WORLD_88] == keyWorld88);
	assert(native2kc[SDLK_WORLD_89] == keyWorld89);
	assert(native2kc[SDLK_WORLD_90] == keyWorld90);
	assert(native2kc[SDLK_WORLD_91] == keyWorld91);
	assert(native2kc[SDLK_WORLD_92] == keyWorld92);
	assert(native2kc[SDLK_WORLD_93] == keyWorld93);
	assert(native2kc[SDLK_WORLD_94] == keyWorld94);
	assert(native2kc[SDLK_WORLD_95] == keyWorld95);
	assert(native2kc[SDLK_KP0] == keyKP0);
	assert(native2kc[SDLK_KP1] == keyKP1);
	assert(native2kc[SDLK_KP2] == keyKP2);
	assert(native2kc[SDLK_KP3] == keyKP3);
	assert(native2kc[SDLK_KP4] == keyKP4);
	assert(native2kc[SDLK_KP5] == keyKP5);
	assert(native2kc[SDLK_KP6] == keyKP6);
	assert(native2kc[SDLK_KP7] == keyKP7);
	assert(native2kc[SDLK_KP8] == keyKP8);
	assert(native2kc[SDLK_KP9] == keyKP9);
	assert(native2kc[SDLK_KP_PERIOD] == keyKPPeriod);
	assert(native2kc[SDLK_KP_DIVIDE] == keyKPDivide);
	assert(native2kc[SDLK_KP_MULTIPLY] == keyKPMultiply);
	assert(native2kc[SDLK_KP_MINUS] == keyKPMinus);
	assert(native2kc[SDLK_KP_PLUS] == keyKPPlus);
	assert(native2kc[SDLK_KP_ENTER] == keyKPEnter);
	assert(native2kc[SDLK_KP_EQUALS] == keyKPEquals);
	assert(native2kc[SDLK_UP] == keyUp);
	assert(native2kc[SDLK_DOWN] == keyDown);
	assert(native2kc[SDLK_RIGHT] == keyRight);
	assert(native2kc[SDLK_LEFT] == keyLeft);
	assert(native2kc[SDLK_INSERT] == keyInsert);
	assert(native2kc[SDLK_HOME] == keyHome);
	assert(native2kc[SDLK_END] == keyEnd);
	assert(native2kc[SDLK_PAGEUP] == keyPageUp);
	assert(native2kc[SDLK_PAGEDOWN] == keyPageDown);
	assert(native2kc[SDLK_F1] == keyF1);
	assert(native2kc[SDLK_F2] == keyF2);
	assert(native2kc[SDLK_F3] == keyF3);
	assert(native2kc[SDLK_F4] == keyF4);
	assert(native2kc[SDLK_F5] == keyF5);
	assert(native2kc[SDLK_F6] == keyF6);
	assert(native2kc[SDLK_F7] == keyF7);
	assert(native2kc[SDLK_F8] == keyF8);
	assert(native2kc[SDLK_F9] == keyF9);
	assert(native2kc[SDLK_F10] == keyF10);
	assert(native2kc[SDLK_F11] == keyF11);
	assert(native2kc[SDLK_F12] == keyF12);
	assert(native2kc[SDLK_F13] == keyF13);
	assert(native2kc[SDLK_F14] == keyF14);
	assert(native2kc[SDLK_F15] == keyF15);
	assert(native2kc[SDLK_NUMLOCK] == keyNumLock);
	assert(native2kc[SDLK_CAPSLOCK] == keyCapsLock);
	assert(native2kc[SDLK_SCROLLOCK] == keyScrollLock);
	assert(native2kc[SDLK_RSHIFT] == keyRShift);
	assert(native2kc[SDLK_LSHIFT] == keyLShift);
	assert(native2kc[SDLK_RCTRL] == keyRCtrl);
	assert(native2kc[SDLK_LCTRL] == keyLCtrl);
	assert(native2kc[SDLK_RALT] == keyRAlt);
	assert(native2kc[SDLK_LALT] == keyLAlt);
	assert(native2kc[SDLK_RMETA] == keyRMeta);
	assert(native2kc[SDLK_LMETA] == keyLMeta);
	assert(native2kc[SDLK_LSUPER] == keyLSuper);
	assert(native2kc[SDLK_RSUPER] == keyRSuper);
	assert(native2kc[SDLK_MODE] == keyMode);
	assert(native2kc[SDLK_COMPOSE] == keyCompose);
	assert(native2kc[SDLK_HELP] == keyHelp);
	assert(native2kc[SDLK_PRINT] == keyPrint);
	assert(native2kc[SDLK_SYSREQ] == keySysreq);
	assert(native2kc[SDLK_BREAK] == keyBreak);
	assert(native2kc[SDLK_MENU] == keyMenu);
	assert(native2kc[SDLK_POWER] == keyPower);
	assert(native2kc[SDLK_EURO] == keyEuro);
	assert(native2kc[SDLK_UNDO] == keyUndo);
	assert(kc2native[keyUnknown] == SDLK_UNKNOWN);
	assert(kc2native[keyBackspace] == SDLK_BACKSPACE);
	assert(kc2native[keyTab] == SDLK_TAB);
	assert(kc2native[keyClear] == SDLK_CLEAR);
	assert(kc2native[keyReturn] == SDLK_RETURN);
	assert(kc2native[keyPause] == SDLK_PAUSE);
	assert(kc2native[keyEscape] == SDLK_ESCAPE);
	assert(kc2native[keySpace] == SDLK_SPACE);
	assert(kc2native[keyExclaim] == SDLK_EXCLAIM);
	assert(kc2native[keyQuoteDbl] == SDLK_QUOTEDBL);
	assert(kc2native[keyHash] == SDLK_HASH);
	assert(kc2native[keyDollar] == SDLK_DOLLAR);
	assert(kc2native[keyAmpersand] == SDLK_AMPERSAND);
	assert(kc2native[keyQuote] == SDLK_QUOTE);
	assert(kc2native[keyLeftParen] == SDLK_LEFTPAREN);
	assert(kc2native[keyRightParen] == SDLK_RIGHTPAREN);
	assert(kc2native[keyAsterisk] == SDLK_ASTERISK);
	assert(kc2native[keyPlus] == SDLK_PLUS);
	assert(kc2native[keyComma] == SDLK_COMMA);
	assert(kc2native[keyMinus] == SDLK_MINUS);
	assert(kc2native[keyPeriod] == SDLK_PERIOD);
	assert(kc2native[keySlash] == SDLK_SLASH);
	assert(kc2native[key0] == SDLK_0);
	assert(kc2native[key1] == SDLK_1);
	assert(kc2native[key2] == SDLK_2);
	assert(kc2native[key3] == SDLK_3);
	assert(kc2native[key4] == SDLK_4);
	assert(kc2native[key5] == SDLK_5);
	assert(kc2native[key6] == SDLK_6);
	assert(kc2native[key7] == SDLK_7);
	assert(kc2native[key8] == SDLK_8);
	assert(kc2native[key9] == SDLK_9);
	assert(kc2native[keyColon] == SDLK_COLON);
	assert(kc2native[keySemicolon] == SDLK_SEMICOLON);
	assert(kc2native[keyLess] == SDLK_LESS);
	assert(kc2native[keyEquals] == SDLK_EQUALS);
	assert(kc2native[keyGreater] == SDLK_GREATER);
	assert(kc2native[keyQuestion] == SDLK_QUESTION);
	assert(kc2native[keyAt] == SDLK_AT);
	assert(kc2native[keyA] == 'A');
	assert(kc2native[keyB] == 'B');
	assert(kc2native[keyC] == 'C');
	assert(kc2native[keyD] == 'D');
	assert(kc2native[keyE] == 'E');
	assert(kc2native[keyF] == 'F');
	assert(kc2native[keyG] == 'G');
	assert(kc2native[keyH] == 'H');
	assert(kc2native[keyI] == 'I');
	assert(kc2native[keyJ] == 'J');
	assert(kc2native[keyK] == 'K');
	assert(kc2native[keyL] == 'L');
	assert(kc2native[keyM] == 'M');
	assert(kc2native[keyN] == 'N');
	assert(kc2native[keyO] == 'O');
	assert(kc2native[keyP] == 'P');
	assert(kc2native[keyQ] == 'Q');
	assert(kc2native[keyR] == 'R');
	assert(kc2native[keyS] == 'S');
	assert(kc2native[keyT] == 'T');
	assert(kc2native[keyU] == 'U');
	assert(kc2native[keyV] == 'V');
	assert(kc2native[keyW] == 'W');
	assert(kc2native[keyX] == 'X');
	assert(kc2native[keyY] == 'Y');
	assert(kc2native[keyZ] == 'Z');
	assert(kc2native[keyLeftBracket] == SDLK_LEFTBRACKET);
	assert(kc2native[keyBackslash] == SDLK_BACKSLASH);
	assert(kc2native[keyRightBracket] == SDLK_RIGHTBRACKET);
	assert(kc2native[keyCaret] == SDLK_CARET);
	assert(kc2native[keyUnderscore] == SDLK_UNDERSCORE);
	assert(kc2native[keyBackquote] == SDLK_BACKQUOTE);
	assert(kc2native[keyDelete] == SDLK_DELETE);
	assert(kc2native[keyWorld0] == SDLK_WORLD_0);
	assert(kc2native[keyWorld1] == SDLK_WORLD_1);
	assert(kc2native[keyWorld2] == SDLK_WORLD_2);
	assert(kc2native[keyWorld3] == SDLK_WORLD_3);
	assert(kc2native[keyWorld4] == SDLK_WORLD_4);
	assert(kc2native[keyWorld5] == SDLK_WORLD_5);
	assert(kc2native[keyWorld6] == SDLK_WORLD_6);
	assert(kc2native[keyWorld7] == SDLK_WORLD_7);
	assert(kc2native[keyWorld8] == SDLK_WORLD_8);
	assert(kc2native[keyWorld9] == SDLK_WORLD_9);
	assert(kc2native[keyWorld10] == SDLK_WORLD_10);
	assert(kc2native[keyWorld11] == SDLK_WORLD_11);
	assert(kc2native[keyWorld12] == SDLK_WORLD_12);
	assert(kc2native[keyWorld13] == SDLK_WORLD_13);
	assert(kc2native[keyWorld14] == SDLK_WORLD_14);
	assert(kc2native[keyWorld15] == SDLK_WORLD_15);
	assert(kc2native[keyWorld16] == SDLK_WORLD_16);
	assert(kc2native[keyWorld17] == SDLK_WORLD_17);
	assert(kc2native[keyWorld18] == SDLK_WORLD_18);
	assert(kc2native[keyWorld19] == SDLK_WORLD_19);
	assert(kc2native[keyWorld20] == SDLK_WORLD_20);
	assert(kc2native[keyWorld21] == SDLK_WORLD_21);
	assert(kc2native[keyWorld22] == SDLK_WORLD_22);
	assert(kc2native[keyWorld23] == SDLK_WORLD_23);
	assert(kc2native[keyWorld24] == SDLK_WORLD_24);
	assert(kc2native[keyWorld25] == SDLK_WORLD_25);
	assert(kc2native[keyWorld26] == SDLK_WORLD_26);
	assert(kc2native[keyWorld27] == SDLK_WORLD_27);
	assert(kc2native[keyWorld28] == SDLK_WORLD_28);
	assert(kc2native[keyWorld29] == SDLK_WORLD_29);
	assert(kc2native[keyWorld30] == SDLK_WORLD_30);
	assert(kc2native[keyWorld31] == SDLK_WORLD_31);
	assert(kc2native[keyWorld32] == SDLK_WORLD_32);
	assert(kc2native[keyWorld33] == SDLK_WORLD_33);
	assert(kc2native[keyWorld34] == SDLK_WORLD_34);
	assert(kc2native[keyWorld35] == SDLK_WORLD_35);
	assert(kc2native[keyWorld36] == SDLK_WORLD_36);
	assert(kc2native[keyWorld37] == SDLK_WORLD_37);
	assert(kc2native[keyWorld38] == SDLK_WORLD_38);
	assert(kc2native[keyWorld39] == SDLK_WORLD_39);
	assert(kc2native[keyWorld40] == SDLK_WORLD_40);
	assert(kc2native[keyWorld41] == SDLK_WORLD_41);
	assert(kc2native[keyWorld42] == SDLK_WORLD_42);
	assert(kc2native[keyWorld43] == SDLK_WORLD_43);
	assert(kc2native[keyWorld44] == SDLK_WORLD_44);
	assert(kc2native[keyWorld45] == SDLK_WORLD_45);
	assert(kc2native[keyWorld46] == SDLK_WORLD_46);
	assert(kc2native[keyWorld47] == SDLK_WORLD_47);
	assert(kc2native[keyWorld48] == SDLK_WORLD_48);
	assert(kc2native[keyWorld49] == SDLK_WORLD_49);
	assert(kc2native[keyWorld50] == SDLK_WORLD_50);
	assert(kc2native[keyWorld51] == SDLK_WORLD_51);
	assert(kc2native[keyWorld52] == SDLK_WORLD_52);
	assert(kc2native[keyWorld53] == SDLK_WORLD_53);
	assert(kc2native[keyWorld54] == SDLK_WORLD_54);
	assert(kc2native[keyWorld55] == SDLK_WORLD_55);
	assert(kc2native[keyWorld56] == SDLK_WORLD_56);
	assert(kc2native[keyWorld57] == SDLK_WORLD_57);
	assert(kc2native[keyWorld58] == SDLK_WORLD_58);
	assert(kc2native[keyWorld59] == SDLK_WORLD_59);
	assert(kc2native[keyWorld60] == SDLK_WORLD_60);
	assert(kc2native[keyWorld61] == SDLK_WORLD_61);
	assert(kc2native[keyWorld62] == SDLK_WORLD_62);
	assert(kc2native[keyWorld63] == SDLK_WORLD_63);
	assert(kc2native[keyWorld64] == SDLK_WORLD_64);
	assert(kc2native[keyWorld65] == SDLK_WORLD_65);
	assert(kc2native[keyWorld66] == SDLK_WORLD_66);
	assert(kc2native[keyWorld67] == SDLK_WORLD_67);
	assert(kc2native[keyWorld68] == SDLK_WORLD_68);
	assert(kc2native[keyWorld69] == SDLK_WORLD_69);
	assert(kc2native[keyWorld70] == SDLK_WORLD_70);
	assert(kc2native[keyWorld71] == SDLK_WORLD_71);
	assert(kc2native[keyWorld72] == SDLK_WORLD_72);
	assert(kc2native[keyWorld73] == SDLK_WORLD_73);
	assert(kc2native[keyWorld74] == SDLK_WORLD_74);
	assert(kc2native[keyWorld75] == SDLK_WORLD_75);
	assert(kc2native[keyWorld76] == SDLK_WORLD_76);
	assert(kc2native[keyWorld77] == SDLK_WORLD_77);
	assert(kc2native[keyWorld78] == SDLK_WORLD_78);
	assert(kc2native[keyWorld79] == SDLK_WORLD_79);
	assert(kc2native[keyWorld80] == SDLK_WORLD_80);
	assert(kc2native[keyWorld81] == SDLK_WORLD_81);
	assert(kc2native[keyWorld82] == SDLK_WORLD_82);
	assert(kc2native[keyWorld83] == SDLK_WORLD_83);
	assert(kc2native[keyWorld84] == SDLK_WORLD_84);
	assert(kc2native[keyWorld85] == SDLK_WORLD_85);
	assert(kc2native[keyWorld86] == SDLK_WORLD_86);
	assert(kc2native[keyWorld87] == SDLK_WORLD_87);
	assert(kc2native[keyWorld88] == SDLK_WORLD_88);
	assert(kc2native[keyWorld89] == SDLK_WORLD_89);
	assert(kc2native[keyWorld90] == SDLK_WORLD_90);
	assert(kc2native[keyWorld91] == SDLK_WORLD_91);
	assert(kc2native[keyWorld92] == SDLK_WORLD_92);
	assert(kc2native[keyWorld93] == SDLK_WORLD_93);
	assert(kc2native[keyWorld94] == SDLK_WORLD_94);
	assert(kc2native[keyWorld95] == SDLK_WORLD_95);
	assert(kc2native[keyKP0] == SDLK_KP0);
	assert(kc2native[keyKP1] == SDLK_KP1);
	assert(kc2native[keyKP2] == SDLK_KP2);
	assert(kc2native[keyKP3] == SDLK_KP3);
	assert(kc2native[keyKP4] == SDLK_KP4);
	assert(kc2native[keyKP5] == SDLK_KP5);
	assert(kc2native[keyKP6] == SDLK_KP6);
	assert(kc2native[keyKP7] == SDLK_KP7);
	assert(kc2native[keyKP8] == SDLK_KP8);
	assert(kc2native[keyKP9] == SDLK_KP9);
	assert(kc2native[keyKPPeriod] == SDLK_KP_PERIOD);
	assert(kc2native[keyKPDivide] == SDLK_KP_DIVIDE);
	assert(kc2native[keyKPMultiply] == SDLK_KP_MULTIPLY);
	assert(kc2native[keyKPMinus] == SDLK_KP_MINUS);
	assert(kc2native[keyKPPlus] == SDLK_KP_PLUS);
	assert(kc2native[keyKPEnter] == SDLK_KP_ENTER);
	assert(kc2native[keyKPEquals] == SDLK_KP_EQUALS);
	assert(kc2native[keyUp] == SDLK_UP);
	assert(kc2native[keyDown] == SDLK_DOWN);
	assert(kc2native[keyRight] == SDLK_RIGHT);
	assert(kc2native[keyLeft] == SDLK_LEFT);
	assert(kc2native[keyInsert] == SDLK_INSERT);
	assert(kc2native[keyHome] == SDLK_HOME);
	assert(kc2native[keyEnd] == SDLK_END);
	assert(kc2native[keyPageUp] == SDLK_PAGEUP);
	assert(kc2native[keyPageDown] == SDLK_PAGEDOWN);
	assert(kc2native[keyF1] == SDLK_F1);
	assert(kc2native[keyF2] == SDLK_F2);
	assert(kc2native[keyF3] == SDLK_F3);
	assert(kc2native[keyF4] == SDLK_F4);
	assert(kc2native[keyF5] == SDLK_F5);
	assert(kc2native[keyF6] == SDLK_F6);
	assert(kc2native[keyF7] == SDLK_F7);
	assert(kc2native[keyF8] == SDLK_F8);
	assert(kc2native[keyF9] == SDLK_F9);
	assert(kc2native[keyF10] == SDLK_F10);
	assert(kc2native[keyF11] == SDLK_F11);
	assert(kc2native[keyF12] == SDLK_F12);
	assert(kc2native[keyF13] == SDLK_F13);
	assert(kc2native[keyF14] == SDLK_F14);
	assert(kc2native[keyF15] == SDLK_F15);
	assert(kc2native[keyNumLock] == SDLK_NUMLOCK);
	assert(kc2native[keyCapsLock] == SDLK_CAPSLOCK);
	assert(kc2native[keyScrollLock] == SDLK_SCROLLOCK);
	assert(kc2native[keyLShift] == SDLK_LSHIFT);
	assert(kc2native[keyRShift] == SDLK_RSHIFT);
	assert(kc2native[keyLCtrl] == SDLK_LCTRL);
	assert(kc2native[keyRCtrl] == SDLK_RCTRL);
	assert(kc2native[keyLAlt] == SDLK_LALT);
	assert(kc2native[keyRAlt] == SDLK_RALT);
	assert(kc2native[keyLMeta] == SDLK_LMETA);
	assert(kc2native[keyRMeta] == SDLK_RMETA);
	assert(kc2native[keyRSuper] == SDLK_RSUPER);
	assert(kc2native[keyLSuper] == SDLK_LSUPER);
	assert(kc2native[keyMode] == SDLK_MODE);
	assert(kc2native[keyCompose] == SDLK_COMPOSE);
	assert(kc2native[keyHelp] == SDLK_HELP);
	assert(kc2native[keyPrint] == SDLK_PRINT);
	assert(kc2native[keySysreq] == SDLK_SYSREQ);
	assert(kc2native[keyBreak] == SDLK_BREAK);
	assert(kc2native[keyMenu] == SDLK_MENU);
	assert(kc2native[keyPower] == SDLK_POWER);
	assert(kc2native[keyEuro] == SDLK_EURO);
	assert(kc2native[keyUndo] == SDLK_UNDO);

}

#elif defined(WIN32)  || defined(WIN64)
#pragma pack(push, 1)
const unsigned char Input::native2mb[Input::NATIVE_MOUSE_BUTTON_LAST + 1] = {
	mbUnknown,		// 0 
	mbLeft,			// 1 VK_LBUTTON
	mbRight,		// 2 VK_RBUTTON
	mbUnknown,		// 3
	mbCenter,		// 4 VK_MBUTTON
	mbButtonX1,		// 5 VK_XBUTTON1
	mbButtonX2		// 6 VK_XBUTTON2
};

const unsigned char Input::mb2native[mbCount] = {
	0 ,						// mbUnknown
	VK_LBUTTON,				// mbLeft
	VK_MBUTTON,				// mbCenter
	VK_RBUTTON,				// mbRight
	0,						// mbWheelUp
	0,						// mbWheelDown
	VK_XBUTTON1,			// mbButtonX1
	VK_XBUTTON2,			// mbButtonX2
};

const short Input::native2kc[Input::NATIVE_KEY_CODE_LAST + 1] = {
	keyNone,				// 0
	keyLButton,				// 1 VK_LBUTTON
	keyRButton,				// 2 VK_RBUTTON
	keyBreak,				// 3 VK_CANCEL
	keyMButton,				// 4 VK_MBUTTON
	keyXButton1,			// 5 VK_XBUTTON1
	keyXButton2,			// 6 VK_XBUTTON2
	keyNone,				// 7
	keyBackspace,			// 8 VK_BACK
	keyTab,					// 9 VK_TAB
	keyNone,				// 10
	keyNone,				// 11
	keyClear,				// 12 VK_CLEAR
	keyReturn,				// 13 VK_RETURN
	keyNone,				// 14
	keyNone,				// 15
	keyLShift,				// 16 VK_SHIFT
	keyLCtrl,				// 17 VK_CONTROL
	keyLAlt,				// 18 VK_MENU
	keyPause,				// 19 VK_PAUSE
	keyCapsLock,			// 20 VK_CAPITAL
	keyIMEKana,				// 21 VK_KANA
	keyNone,				// 22
	keyIMEJunja,			// 23 VK_JUNJA
	keyIMEFinal,			// 24 VK_FINAL
	keyIMEHanja,			// 25 VK_HANJA
	keyNone,				// 26
	keyEscape,				// 27 VK_ESCAPE
	keyIMEConvert,			// 28 VK_CONVERT
	keyIMENonConvert,		// 29 VK_NONCONVERT
	keyIMEAccept,			// 30 VK_ACCEPT
	keyIMEModeChange,		// 31 VK_MODECHANGE
	keySpace,				// 32 VK_SPACE
	keyPageUp,				// 33 VK_PRIOR
	keyPageDown,			// 34 VK_NEXT
	keyEnd,					// 35 VK_END
	keyHome,				// 36 VK_HOME
	keyLeft,				// 37 VK_LEFT
	keyUp,					// 38 VK_UP
	keyRight,				// 39 VK_RIGHT
	keyDown,				// 40 VK_DOWN
	keyNone,				// 41 VK_SELECT
	keyPrint,				// 42 VK_PRINT
	keyNone,				// 43 VK_EXECUTE
	keyPrint,				// 44 VK_SNAPSHOT
	keyInsert,				// 45 VK_INSERT
	keyDelete,				// 46 VK_DELETE
	keyHelp,				// 47 VK_HELP
	key0,					// 48 '0'
	key1,					// 49 '1'
	key2,					// 50 '2'
	key3,					// 51 '3'
	key4,					// 52 '4'
	key5,					// 53 '5'
	key6,					// 54 '6'
	key7,					// 55 '7'
	key8,					// 56 '8'
	key9,					// 57 '9'
	keyNone,				// 58
	keyNone,				// 59
	keyNone,				// 60
	keyNone,				// 61
	keyNone,				// 62
	keyNone,				// 63
	keyNone,				// 64
	keyA,					// 65 'A'
	keyB,					// 66 'B'
	keyC,					// 67 'C'
	keyD,					// 68 'D'
	keyE,					// 69 'E'
	keyF,					// 70 'F'
	keyG,					// 71 'G'
	keyH,					// 72 'H'
	keyI,					// 73 'I'
	keyJ,					// 74 'J'
	keyK,					// 75 'K'
	keyL,					// 76 'L'
	keyM,					// 77 'M'
	keyN,					// 78 'N'
	keyO,					// 79 'O'
	keyP,					// 80 'P'
	keyQ,					// 81 'Q'
	keyR,					// 82 'R'
	keyS,					// 83 'S'
	keyT,					// 84 'T'
	keyU,					// 85 'U'
	keyV,					// 86 'V'
	keyW,					// 87 'W'
	keyX,					// 88 'X'
	keyY,					// 89 'Y'
	keyZ,					// 90 'Z'
	keyLSuper,				// 91 VK_LWIN
	keyRSuper,				// 92 VK_RWIN
	keyMenu,				// 93 VK_APPS
	keyNone,				// 94
	keyNone,				// 95 VK_SLEEP
	keyKP0,					// 96 VK_NUMPAD0
	keyKP1,					// 97 VK_NUMPAD1
	keyKP2,					// 98 VK_NUMPAD2
	keyKP3,					// 99 VK_NUMPAD3
	keyKP4,					// 100 VK_NUMPAD4
	keyKP5,					// 101 VK_NUMPAD5
	keyKP6,					// 102 VK_NUMPAD6
	keyKP7,					// 103 VK_NUMPAD7
	keyKP8,					// 104 VK_NUMPAD8
	keyKP9,					// 105 VK_NUMPAD9
	keyKPMultiply,			// 106 VK_MULTIPLY
	keyKPPlus,				// 107 VK_ADD
	keyKPEnter,				// 108 VK_SEPARATOR
	keyKPMinus,				// 109 VK_SUBTRACT
	keyKPPeriod,			// 110 VK_DECIMAL
	keyKPDivide,			// 111 VK_DIVIDE
	keyF1,					// 112 VK_F1
	keyF2,					// 113 VK_F2
	keyF3,					// 114 VK_F3
	keyF4,					// 115 VK_F4
	keyF5,					// 116 VK_F5
	keyF6,					// 117 VK_F6
	keyF7,					// 118 VK_F7
	keyF8,					// 119 VK_F8
	keyF9,					// 120 VK_F9
	keyF10,					// 121 VK_F10
	keyF11,					// 122 VK_F11
	keyF12,					// 123 VK_F12
	keyF13,					// 124 VK_F13
	keyF14,					// 125 VK_F14
	keyF15,					// 126 VK_F15
	keyF16,					// 127 VK_F16
	keyF17,					// 128 VK_F17
	keyF18,					// 129 VK_F18
	keyF19,					// 130 VK_F19
	keyF20,					// 131 VK_F20
	keyF21,					// 132 VK_F21
	keyF22,					// 133 VK_F22
	keyF23,					// 134 VK_F23
	keyF24,					// 135 VK_F24
	keyNone,				// 136
	keyNone,				// 137
	keyNone,				// 138
	keyNone,				// 139
	keyNone,				// 140
	keyNone,				// 141
	keyNone,				// 142
	keyNone,				// 143
	keyNumLock,				// 144 VK_NUMLOCK
	keyScrollLock,			// 145 VK_SCROLL
	keyNone,				// 146
	keyNone,				// 147
	keyNone,				// 148
	keyNone,				// 149
	keyNone,				// 150
	keyNone,				// 151
	keyNone,				// 152
	keyNone,				// 153
	keyNone,				// 154
	keyNone,				// 155
	keyNone,				// 156
	keyNone,				// 157
	keyNone,				// 158
	keyNone,				// 159
	keyLShift,				// 160 VK_LSHIFT
	keyRShift,				// 161 VK_RSHIFT
	keyLCtrl,				// 162 VK_LCONTROL
	keyRCtrl,				// 163 VK_RCONTROL
	keyLAlt,				// 164 VK_LMENU
	keyRAlt,				// 165 VK_RMENU
	keyBrowserBack,			// 166 VK_BROWSER_BACK
	keyBrowserForward,		// 167 VK_BROWSER_FORWARD
	keyBrowserRefresh,		// 168 VK_BROWSER_REFRESH
	keyBrowserStop,			// 169 VK_BROWSER_STOP
	keyBrowserSearch,		// 170 VK_BROWSER_SEARCH
	keyBrowserFavorites,	// 171 VK_BROWSER_FAVORITES
	keyBrowserHome,			// 172 VK_BROWSER_HOME
	keyVolumeMute,			// 173 VK_VOLUME_MUTE
	keyVolumeDown,			// 174 VK_VOLUME_DOWN
	keyVolumeUp,			// 175 VK_VOLUME_UP
	keyMediaNext,			// 176 VK_MEDIA_NEXT_TRACK
	keyMediaPrev,			// 177 VK_MEDIA_PREV_TRACK
	keyMediaStop,			// 178 VK_MEDIA_STOP
	keyMediaPlayPause,		// 179 VK_MEDIA_PLAY_PAUSE
	keyLaunchMail,			// 180 VK_LAUNCH_MAIL
	keyLaunchMediaSelect,	// 181 VK_LAUNCH_MEDIA_SELECT
	keyLaunchApp1,			// 182 VK_LAUNCH_APP1
	keyLaunchApp2,			// 183 VK_LAUNCH_APP2
	keyNone,				// 184
	keyNone,				// 185
	keySemicolon,			// 186 VK_OEM_1
//	keyPlus,				// 187 VK_OEM_PLUS
	keyEquals,				// 187 VK_OEM_PLUS
	keyComma,				// 188 VK_OEM_COMMA
	keyMinus,				// 189 VK_OEM_MINUS
	keyPeriod,				// 190 VK_OEM_PERIOD
	keySlash,				// 191 VK_OEM_2
	keyBackquote,			// 192 VK_OEM_3
	keyNone,				// 193
	keyNone,				// 194
	keyNone,				// 195
	keyNone,				// 196
	keyNone,				// 197
	keyNone,				// 198
	keyNone,				// 199
	keyNone,				// 200
	keyNone,				// 201
	keyNone,				// 202
	keyNone,				// 203
	keyNone,				// 204
	keyNone,				// 205
	keyNone,				// 206
	keyNone,				// 207
	keyNone,				// 208
	keyNone,				// 209
	keyNone,				// 210
	keyNone,				// 211
	keyNone,				// 212
	keyNone,				// 213
	keyNone,				// 214
	keyNone,				// 215
	keyNone,				// 216
	keyNone,				// 217
	keyNone,				// 218
	keyLeftBracket,			// 219 VK_OEM_4
	keyBackslash,			// 220 VK_OEM_5
	keyRightBracket,		// 221 VK_OEM_6
	keyQuote,				// 222 VK_OEM_7
	keyBackquote,			// 223 VK_OEM_8
	keyNone,				// 224
	keyNone,				// 225
	keyNone,				// 226 VK_OEM_102
	keyNone,				// 227
	keyNone,				// 228
	keyIMEProcess,			// 229 VK_PROCESSKEY
	keyNone,				// 230
	keyNone,				// 231 VK_PACKET
	keyNone,				// 232
	keyNone,				// 233
	keyNone,				// 234
	keyNone,				// 235
	keyNone,				// 236
	keyNone,				// 237
	keyNone,				// 238
	keyNone,				// 239
	keyNone,				// 240
	keyNone,				// 241
	keyNone,				// 242
	keyNone,				// 243
	keyNone,				// 244
	keyNone,				// 245
	keyNone,				// 246 VK_ATTN
	keyNone,				// 247 VK_CRSEL
	keyNone,				// 248 VK_EXSEL
	keyNone,				// 249 VK_EREOF
	keyPlay,				// 250 VK_PLAY
	keyZoom,				// 251 VK_ZOOM
	keyNone,				// 252 VK_NONAME
	keyNone,				// 253 VK_PA1
	keyNone,				// 254 VK_OEM_CLEAR
	keyNone					// 255
};

const NativeKeyCodeCompact Input::kc2native[keyCount] = {
	0,						// keyNone
	0,						// keyUnknown
	VK_LBUTTON,				// keyLButton
	VK_RBUTTON,				// keyRButton
	VK_MBUTTON,				// keyMButton
	VK_XBUTTON1,			// keyXButton1
	VK_XBUTTON2,			// keyXButton2
	VK_BACK,				// keyBackspace
	VK_TAB,					// keyTab
	VK_CLEAR,				// keyClear
	VK_RETURN,				// keyReturn
	VK_PAUSE,				// keyPause
	VK_KANA,				// keyIMEKana
	0,						// keyIMEHangul
	VK_JUNJA,				// keyIMEJunja
	VK_FINAL,				// keyIMEFinal
	VK_HANJA,				// keyIMEHanja
	0,						// keyIMEKanji
	VK_ESCAPE,				// keyEscape
	VK_CONVERT,				// keyIMEConvert
	VK_NONCONVERT,			// keyIMENonConvert
	VK_ACCEPT,				// keyIMEAccept
	VK_MODECHANGE,			// keyIMEModeChange
	VK_PROCESSKEY,			// keyIMEProcess
	VK_SPACE,				// keySpace
	0,						// keyExclaim
	0,						// keyQuoteDbl
	0,						// keyHash
	0,						// keyDollar
	0,						// keyPercent
	0,						// keyAmpersand
	VK_OEM_7,				// keyQuote
	0,						// keyLeftParen
	0,						// keyRightParen
	0,						// keyAsterisk
	VK_OEM_PLUS,			// keyPlus
	VK_OEM_COMMA,			// keyComma
	VK_OEM_MINUS,			// keyMinus
	VK_OEM_PERIOD,			// keyPeriod
	VK_OEM_2,				// keySlash
	'0',					// key0
	'1',					// key1
	'2',					// key2
	'3',					// key3
	'4',					// key4
	'5',					// key5
	'6',					// key6
	'7',					// key7
	'8',					// key8
	'9',					// key9
	0,						// keyColon
	VK_OEM_1,				// keySemicolon
	0,						// keyLess
	VK_OEM_PLUS,			// keyEquals
	0,						// keyGreater
	0,						// keyQuestion
	0,						// keyAt
	'A',					// keyA
	'B',					// keyB
	'C',					// keyC
	'D',					// keyD
	'E',					// keyE
	'F',					// keyF
	'G',					// keyG
	'H',					// keyH
	'I',					// keyI
	'J',					// keyJ
	'K',					// keyK
	'L',					// keyL
	'M',					// keyM
	'N',					// keyN
	'O',					// keyO
	'P',					// keyP
	'Q',					// keyQ
	'R',					// keyR
	'S',					// keyS
	'T',					// keyT
	'U',					// keyU
	'V',					// keyV
	'W',					// keyW
	'X',					// keyX
	'Y',					// keyY
	'Z',					// keyZ
	VK_OEM_4,				// keyLeftBracket
	VK_OEM_5,				// keyBackslash
	VK_OEM_6,				// keyRightBracket
	0,						// keyCaret
	0,						// keyUnderscore
	VK_OEM_3,				// keyBackquote
	VK_DELETE,				// keyDelete
	0,						// keyWorld0
	0,						// keyWorld1
	0,						// keyWorld2
	0,						// keyWorld3
	0,						// keyWorld4
	0,						// keyWorld5
	0,						// keyWorld6
	0,						// keyWorld7
	0,						// keyWorld8
	0,						// keyWorld9
	0,						// keyWorld10
	0,						// keyWorld11
	0,						// keyWorld12
	0,						// keyWorld13
	0,						// keyWorld14
	0,						// keyWorld15
	0,						// keyWorld16
	0,						// keyWorld17
	0,						// keyWorld18
	0,						// keyWorld19
	0,						// keyWorld20
	0,						// keyWorld21
	0,						// keyWorld22
	0,						// keyWorld23
	0,						// keyWorld24
	0,						// keyWorld25
	0,						// keyWorld26
	0,						// keyWorld27
	0,						// keyWorld28
	0,						// keyWorld29
	0,						// keyWorld30
	0,						// keyWorld31
	0,						// keyWorld32
	0,						// keyWorld33
	0,						// keyWorld34
	0,						// keyWorld35
	0,						// keyWorld36
	0,						// keyWorld37
	0,						// keyWorld38
	0,						// keyWorld39
	0,						// keyWorld40
	0,						// keyWorld41
	0,						// keyWorld42
	0,						// keyWorld43
	0,						// keyWorld44
	0,						// keyWorld45
	0,						// keyWorld46
	0,						// keyWorld47
	0,						// keyWorld48
	0,						// keyWorld49
	0,						// keyWorld50
	0,						// keyWorld51
	0,						// keyWorld52
	0,						// keyWorld53
	0,						// keyWorld54
	0,						// keyWorld55
	0,						// keyWorld56
	0,						// keyWorld57
	0,						// keyWorld58
	0,						// keyWorld59
	0,						// keyWorld60
	0,						// keyWorld61
	0,						// keyWorld62
	0,						// keyWorld63
	0,						// keyWorld64
	0,						// keyWorld65
	0,						// keyWorld66
	0,						// keyWorld67
	0,						// keyWorld68
	0,						// keyWorld69
	0,						// keyWorld70
	0,						// keyWorld71
	0,						// keyWorld72
	0,						// keyWorld73
	0,						// keyWorld74
	0,						// keyWorld75
	0,						// keyWorld76
	0,						// keyWorld77
	0,						// keyWorld78
	0,						// keyWorld79
	0,						// keyWorld80
	0,						// keyWorld81
	0,						// keyWorld82
	0,						// keyWorld83
	0,						// keyWorld84
	0,						// keyWorld85
	0,						// keyWorld86
	0,						// keyWorld87
	0,						// keyWorld88
	0,						// keyWorld89
	0,						// keyWorld90
	0,						// keyWorld91
	0,						// keyWorld92
	0,						// keyWorld93
	0,						// keyWorld94
	0,						// keyWorld95
	VK_NUMPAD0,				// keyKP0
	VK_NUMPAD1,				// keyKP1
	VK_NUMPAD2,				// keyKP2
	VK_NUMPAD3,				// keyKP3
	VK_NUMPAD4,				// keyKP4
	VK_NUMPAD5,				// keyKP5
	VK_NUMPAD6,				// keyKP6
	VK_NUMPAD7,				// keyKP7
	VK_NUMPAD8,				// keyKP8
	VK_NUMPAD9,				// keyKP9
	VK_DECIMAL,				// keyKPPeriod
	VK_DIVIDE,				// keyKPDivide
	VK_MULTIPLY,			// keyKPMultiply
	VK_SUBTRACT,			// keyKPMinus
	VK_ADD,					// keyKPPlus
	VK_SEPARATOR,			// keyKPEnter
	0,						// keyKPEquals
	VK_UP,					// keyUp
	VK_DOWN,				// keyDown
	VK_RIGHT,				// keyRight
	VK_LEFT,				// keyLeft
	VK_INSERT,				// keyInsert
	VK_HOME,				// keyHome
	VK_END,					// keyEnd
	VK_PRIOR,				// keyPageUp
	VK_NEXT,				// keyPageDown
	VK_F1,					// keyF1
	VK_F2,					// keyF2
	VK_F3,					// keyF3
	VK_F4,					// keyF4
	VK_F5,					// keyF5
	VK_F6,					// keyF6
	VK_F7,					// keyF7
	VK_F8,					// keyF8
	VK_F9,					// keyF9
	VK_F10,					// keyF10
	VK_F11,					// keyF11
	VK_F12,					// keyF12
	VK_F13,					// keyF13
	VK_F14,					// keyF14
	VK_F15,					// keyF15
	VK_F16,					// keyF16
	VK_F17,					// keyF17
	VK_F18,					// keyF18
	VK_F19,					// keyF19
	VK_F20,					// keyF20
	VK_F21,					// keyF21
	VK_F22,					// keyF22
	VK_F23,					// keyF23
	VK_F24,					// keyF24
	VK_NUMLOCK,				// keyNumLock
	VK_CAPITAL,				// keyCapsLock
	VK_SCROLL,				// keyScrollLock
	VK_SHIFT,				// keyLShift
	VK_RSHIFT,				// keyRShift
	VK_CONTROL,				// keyLCtrl
	VK_RCONTROL,			// keyRCtrl
	VK_MENU,				// keyLAlt
	VK_RMENU,				// keyRAlt
	0,						// keyLMeta
	0,						// keyRMeta
	VK_RWIN,				// keyRSuper
	VK_LWIN,				// keyLSuper
	0,						// keyMode
	0,						// keyCompose
	VK_HELP,				// keyHelp
	VK_PRINT,				// keyPrint
	0,						// keySysreq
	VK_CANCEL,				// keyBreak
	VK_APPS,				// keyMenu
	0,						// keyPower
	0,						// keyEuro
	0,						// keyUndo
	VK_BROWSER_BACK,		// keyBrowserBack
	VK_BROWSER_FORWARD,		// keyBrowserForward
	VK_BROWSER_REFRESH,		// keyBrowserRefresh
	VK_BROWSER_STOP,		// keyBrowserStop
	VK_BROWSER_SEARCH,		// keyBrowserSearch
	VK_BROWSER_FAVORITES,	// keyBrowserFavorites
	VK_BROWSER_HOME,		// keyBrowserHome
	VK_VOLUME_MUTE,			// keyVolumeMute
	VK_VOLUME_DOWN,			// keyVolumeDown
	VK_VOLUME_UP,			// keyVolumeUp
	VK_MEDIA_NEXT_TRACK,	// keyMediaNext
	VK_MEDIA_PREV_TRACK,	// keyMediaPrev
	VK_MEDIA_STOP,			// keyMediaStop
	VK_MEDIA_PLAY_PAUSE,	// keyMediaPlayPause
	VK_LAUNCH_MAIL,			// keyLaunchMail
	VK_LAUNCH_MEDIA_SELECT,	// keyLaunchMediaSelect
	VK_LAUNCH_APP1,			// keyLaunchApp1
	VK_LAUNCH_APP2,			// keyLaunchApp2
	VK_PLAY,				// keyPlay
	VK_ZOOM					// keyZoom
};

#pragma pack(pop)

void Input::verifyTranslationTables() {
	// verify integrity of Windows to KeyCode table
	assert(native2kc[VK_LBUTTON] == keyLButton);
	assert(native2kc[VK_RBUTTON] == keyRButton);
	assert(native2kc[VK_CANCEL] == keyBreak);
	assert(native2kc[VK_MBUTTON] == keyMButton);
	assert(native2kc[VK_XBUTTON1] == keyXButton1);
	assert(native2kc[VK_XBUTTON2] == keyXButton2);
	assert(native2kc[VK_BACK] == keyBackspace);
	assert(native2kc[VK_TAB] == keyTab);
	assert(native2kc[VK_CLEAR] == keyClear);
	assert(native2kc[VK_RETURN] == keyReturn);
	assert(native2kc[VK_SHIFT] == keyLShift);
	assert(native2kc[VK_CONTROL] == keyLCtrl);
	assert(native2kc[VK_MENU] == keyLAlt);
	assert(native2kc[VK_PAUSE] == keyPause);
	assert(native2kc[VK_CAPITAL] == keyCapsLock);
	assert(native2kc[VK_KANA] == keyIMEKana);
	assert(native2kc[VK_JUNJA] == keyIMEJunja);
	assert(native2kc[VK_FINAL] == keyIMEFinal);
	assert(native2kc[VK_HANJA] == keyIMEHanja);
	assert(native2kc[VK_ESCAPE] == keyEscape);
	assert(native2kc[VK_CONVERT] == keyIMEConvert);
	assert(native2kc[VK_NONCONVERT] == keyIMENonConvert);
	assert(native2kc[VK_ACCEPT] == keyIMEAccept);
	assert(native2kc[VK_MODECHANGE] == keyIMEModeChange);
	assert(native2kc[VK_SPACE] == keySpace);
	assert(native2kc[VK_PRIOR] == keyPageUp);
	assert(native2kc[VK_NEXT] == keyPageDown);
	assert(native2kc[VK_END] == keyEnd);
	assert(native2kc[VK_HOME] == keyHome);
	assert(native2kc[VK_LEFT] == keyLeft);
	assert(native2kc[VK_UP] == keyUp);
	assert(native2kc[VK_RIGHT] == keyRight);
	assert(native2kc[VK_DOWN] == keyDown);
	assert(native2kc[VK_PRINT] == keyPrint);
	assert(native2kc[VK_SNAPSHOT] == keyPrint);
	assert(native2kc[VK_INSERT] == keyInsert);
	assert(native2kc[VK_DELETE] == keyDelete);
	assert(native2kc[VK_HELP] == keyHelp);
	assert(native2kc['0'] == key0);
	assert(native2kc['1'] == key1);
	assert(native2kc['2'] == key2);
	assert(native2kc['3'] == key3);
	assert(native2kc['4'] == key4);
	assert(native2kc['5'] == key5);
	assert(native2kc['6'] == key6);
	assert(native2kc['7'] == key7);
	assert(native2kc['8'] == key8);
	assert(native2kc['9'] == key9);
	assert(native2kc['A'] == keyA);
	assert(native2kc['B'] == keyB);
	assert(native2kc['C'] == keyC);
	assert(native2kc['D'] == keyD);
	assert(native2kc['E'] == keyE);
	assert(native2kc['F'] == keyF);
	assert(native2kc['G'] == keyG);
	assert(native2kc['H'] == keyH);
	assert(native2kc['I'] == keyI);
	assert(native2kc['J'] == keyJ);
	assert(native2kc['K'] == keyK);
	assert(native2kc['L'] == keyL);
	assert(native2kc['M'] == keyM);
	assert(native2kc['N'] == keyN);
	assert(native2kc['O'] == keyO);
	assert(native2kc['P'] == keyP);
	assert(native2kc['Q'] == keyQ);
	assert(native2kc['R'] == keyR);
	assert(native2kc['S'] == keyS);
	assert(native2kc['T'] == keyT);
	assert(native2kc['U'] == keyU);
	assert(native2kc['V'] == keyV);
	assert(native2kc['W'] == keyW);
	assert(native2kc['X'] == keyX);
	assert(native2kc['Y'] == keyY);
	assert(native2kc['Z'] == keyZ);
	assert(native2kc[VK_LWIN] == keyLSuper);
	assert(native2kc[VK_RWIN] == keyRSuper);
	assert(native2kc[VK_APPS] == keyMenu);
	assert(native2kc[VK_NUMPAD0] == keyKP0);
	assert(native2kc[VK_NUMPAD1] == keyKP1);
	assert(native2kc[VK_NUMPAD2] == keyKP2);
	assert(native2kc[VK_NUMPAD3] == keyKP3);
	assert(native2kc[VK_NUMPAD4] == keyKP4);
	assert(native2kc[VK_NUMPAD5] == keyKP5);
	assert(native2kc[VK_NUMPAD6] == keyKP6);
	assert(native2kc[VK_NUMPAD7] == keyKP7);
	assert(native2kc[VK_NUMPAD8] == keyKP8);
	assert(native2kc[VK_NUMPAD9] == keyKP9);
	assert(native2kc[VK_MULTIPLY] == keyKPMultiply);
	assert(native2kc[VK_ADD] == keyKPPlus);
	assert(native2kc[VK_SEPARATOR] == keyKPEnter);
	assert(native2kc[VK_SUBTRACT] == keyKPMinus);
	assert(native2kc[VK_DECIMAL] == keyKPPeriod);
	assert(native2kc[VK_DIVIDE] == keyKPDivide);
	assert(native2kc[VK_F1] == keyF1);
	assert(native2kc[VK_F2] == keyF2);
	assert(native2kc[VK_F3] == keyF3);
	assert(native2kc[VK_F4] == keyF4);
	assert(native2kc[VK_F5] == keyF5);
	assert(native2kc[VK_F6] == keyF6);
	assert(native2kc[VK_F7] == keyF7);
	assert(native2kc[VK_F8] == keyF8);
	assert(native2kc[VK_F9] == keyF9);
	assert(native2kc[VK_F10] == keyF10);
	assert(native2kc[VK_F11] == keyF11);
	assert(native2kc[VK_F12] == keyF12);
	assert(native2kc[VK_F13] == keyF13);
	assert(native2kc[VK_F14] == keyF14);
	assert(native2kc[VK_F15] == keyF15);
	assert(native2kc[VK_F16] == keyF16);
	assert(native2kc[VK_F17] == keyF17);
	assert(native2kc[VK_F18] == keyF18);
	assert(native2kc[VK_F19] == keyF19);
	assert(native2kc[VK_F20] == keyF20);
	assert(native2kc[VK_F21] == keyF21);
	assert(native2kc[VK_F22] == keyF22);
	assert(native2kc[VK_F23] == keyF23);
	assert(native2kc[VK_F24] == keyF24);
	assert(native2kc[VK_NUMLOCK] == keyNumLock);
	assert(native2kc[VK_SCROLL] == keyScrollLock);
	assert(native2kc[VK_LSHIFT] == keyLShift);
	assert(native2kc[VK_RSHIFT] == keyRShift);
	assert(native2kc[VK_LCONTROL] == keyLCtrl);
	assert(native2kc[VK_RCONTROL] == keyRCtrl);
	assert(native2kc[VK_LMENU] == keyLAlt);
	assert(native2kc[VK_RMENU] == keyRAlt);
	assert(native2kc[VK_BROWSER_BACK] == keyBrowserBack);
	assert(native2kc[VK_BROWSER_FORWARD] == keyBrowserForward);
	assert(native2kc[VK_BROWSER_REFRESH] == keyBrowserRefresh);
	assert(native2kc[VK_BROWSER_STOP] == keyBrowserStop);
	assert(native2kc[VK_BROWSER_SEARCH] == keyBrowserSearch);
	assert(native2kc[VK_BROWSER_FAVORITES] == keyBrowserFavorites);
	assert(native2kc[VK_BROWSER_HOME] == keyBrowserHome);
	assert(native2kc[VK_VOLUME_MUTE] == keyVolumeMute);
	assert(native2kc[VK_VOLUME_DOWN] == keyVolumeDown);
	assert(native2kc[VK_VOLUME_UP] == keyVolumeUp);
	assert(native2kc[VK_MEDIA_NEXT_TRACK] == keyMediaNext);
	assert(native2kc[VK_MEDIA_PREV_TRACK] == keyMediaPrev);
	assert(native2kc[VK_MEDIA_STOP] == keyMediaStop);
	assert(native2kc[VK_MEDIA_PLAY_PAUSE] == keyMediaPlayPause);
	assert(native2kc[VK_LAUNCH_MAIL] == keyLaunchMail);
	assert(native2kc[VK_LAUNCH_MEDIA_SELECT] == keyLaunchMediaSelect);
	assert(native2kc[VK_LAUNCH_APP1] == keyLaunchApp1);
	assert(native2kc[VK_LAUNCH_APP2] == keyLaunchApp2);
	assert(native2kc[VK_OEM_1] == keySemicolon);
//	assert(native2kc[VK_OEM_PLUS] == keyPlus);
//	assert(native2kc[VK_OEM_PLUS] == keyEquals);
	assert(native2kc[VK_OEM_COMMA] == keyComma);
	assert(native2kc[VK_OEM_MINUS] == keyMinus);
	assert(native2kc[VK_OEM_PERIOD] == keyPeriod);
	assert(native2kc[VK_OEM_2] == keySlash);
	assert(native2kc[VK_OEM_3] == keyBackquote);
	assert(native2kc[VK_OEM_4] == keyLeftBracket);
	assert(native2kc[VK_OEM_5] == keyBackslash);
	assert(native2kc[VK_OEM_6] == keyRightBracket);
	assert(native2kc[VK_OEM_7] == keyQuote);
	assert(native2kc[VK_OEM_8] == keyBackquote);
	assert(native2kc[VK_PROCESSKEY] == keyIMEProcess);
	assert(native2kc[VK_PLAY] == keyPlay);
	assert(native2kc[VK_ZOOM] == keyZoom);
	assert(kc2native[keyLButton] == VK_LBUTTON);
	assert(kc2native[keyRButton] == VK_RBUTTON);
	assert(kc2native[keyMButton] == VK_MBUTTON);
	assert(kc2native[keyXButton1] == VK_XBUTTON1);
	assert(kc2native[keyXButton2] == VK_XBUTTON2);
	assert(kc2native[keyBackspace] == VK_BACK);
	assert(kc2native[keyTab] == VK_TAB);
	assert(kc2native[keyClear] == VK_CLEAR);
	assert(kc2native[keyReturn] == VK_RETURN);
	assert(kc2native[keyPause] == VK_PAUSE);
	assert(kc2native[keyIMEKana] == VK_KANA);
	assert(kc2native[keyIMEJunja] == VK_JUNJA);
	assert(kc2native[keyIMEFinal] == VK_FINAL);
	assert(kc2native[keyIMEHanja] == VK_HANJA);
	assert(kc2native[keyEscape] == VK_ESCAPE);
	assert(kc2native[keyIMEConvert] == VK_CONVERT);
	assert(kc2native[keyIMENonConvert] == VK_NONCONVERT);
	assert(kc2native[keyIMEAccept] == VK_ACCEPT);
	assert(kc2native[keyIMEModeChange] == VK_MODECHANGE);
	assert(kc2native[keyIMEProcess] == VK_PROCESSKEY);
	assert(kc2native[keySpace] == VK_SPACE);
	assert(kc2native[keyQuote] == VK_OEM_7);
//	assert(kc2native[keyPlus] == VK_OEM_PLUS);
	assert(kc2native[keyEquals] == VK_OEM_PLUS);
	assert(kc2native[keyComma] == VK_OEM_COMMA);
	assert(kc2native[keyMinus] == VK_OEM_MINUS);
	assert(kc2native[keyPeriod] == VK_OEM_PERIOD);
	assert(kc2native[keySlash] == VK_OEM_2);
	assert(kc2native[key0] == '0');
	assert(kc2native[key1] == '1');
	assert(kc2native[key2] == '2');
	assert(kc2native[key3] == '3');
	assert(kc2native[key4] == '4');
	assert(kc2native[key5] == '5');
	assert(kc2native[key6] == '6');
	assert(kc2native[key7] == '7');
	assert(kc2native[key8] == '8');
	assert(kc2native[key9] == '9');
	assert(kc2native[keySemicolon] == VK_OEM_1);
	assert(kc2native[keyA] == 'A');
	assert(kc2native[keyB] == 'B');
	assert(kc2native[keyC] == 'C');
	assert(kc2native[keyD] == 'D');
	assert(kc2native[keyE] == 'E');
	assert(kc2native[keyF] == 'F');
	assert(kc2native[keyG] == 'G');
	assert(kc2native[keyH] == 'H');
	assert(kc2native[keyI] == 'I');
	assert(kc2native[keyJ] == 'J');
	assert(kc2native[keyK] == 'K');
	assert(kc2native[keyL] == 'L');
	assert(kc2native[keyM] == 'M');
	assert(kc2native[keyN] == 'N');
	assert(kc2native[keyO] == 'O');
	assert(kc2native[keyP] == 'P');
	assert(kc2native[keyQ] == 'Q');
	assert(kc2native[keyR] == 'R');
	assert(kc2native[keyS] == 'S');
	assert(kc2native[keyT] == 'T');
	assert(kc2native[keyU] == 'U');
	assert(kc2native[keyV] == 'V');
	assert(kc2native[keyW] == 'W');
	assert(kc2native[keyX] == 'X');
	assert(kc2native[keyY] == 'Y');
	assert(kc2native[keyZ] == 'Z');
	assert(kc2native[keyLeftBracket] == VK_OEM_4);
	assert(kc2native[keyBackslash] == VK_OEM_5);
	assert(kc2native[keyRightBracket] == VK_OEM_6);
	assert(kc2native[keyBackquote] == VK_OEM_3);
	assert(kc2native[keyDelete] == VK_DELETE);
	assert(kc2native[keyKP0] == VK_NUMPAD0);
	assert(kc2native[keyKP1] == VK_NUMPAD1);
	assert(kc2native[keyKP2] == VK_NUMPAD2);
	assert(kc2native[keyKP3] == VK_NUMPAD3);
	assert(kc2native[keyKP4] == VK_NUMPAD4);
	assert(kc2native[keyKP5] == VK_NUMPAD5);
	assert(kc2native[keyKP6] == VK_NUMPAD6);
	assert(kc2native[keyKP7] == VK_NUMPAD7);
	assert(kc2native[keyKP8] == VK_NUMPAD8);
	assert(kc2native[keyKP9] == VK_NUMPAD9);
	assert(kc2native[keyKPPeriod] == VK_DECIMAL);
	assert(kc2native[keyKPDivide] == VK_DIVIDE);
	assert(kc2native[keyKPMultiply] == VK_MULTIPLY);
	assert(kc2native[keyKPMinus] == VK_SUBTRACT);
	assert(kc2native[keyKPPlus] == VK_ADD);
	assert(kc2native[keyKPEnter] == VK_SEPARATOR);
	assert(kc2native[keyUp] == VK_UP);
	assert(kc2native[keyDown] == VK_DOWN);
	assert(kc2native[keyRight] == VK_RIGHT);
	assert(kc2native[keyLeft] == VK_LEFT);
	assert(kc2native[keyInsert] == VK_INSERT);
	assert(kc2native[keyHome] == VK_HOME);
	assert(kc2native[keyEnd] == VK_END);
	assert(kc2native[keyPageUp] == VK_PRIOR);
	assert(kc2native[keyPageDown] == VK_NEXT);
	assert(kc2native[keyF1] == VK_F1);
	assert(kc2native[keyF2] == VK_F2);
	assert(kc2native[keyF3] == VK_F3);
	assert(kc2native[keyF4] == VK_F4);
	assert(kc2native[keyF5] == VK_F5);
	assert(kc2native[keyF6] == VK_F6);
	assert(kc2native[keyF7] == VK_F7);
	assert(kc2native[keyF8] == VK_F8);
	assert(kc2native[keyF9] == VK_F9);
	assert(kc2native[keyF10] == VK_F10);
	assert(kc2native[keyF11] == VK_F11);
	assert(kc2native[keyF12] == VK_F12);
	assert(kc2native[keyF13] == VK_F13);
	assert(kc2native[keyF14] == VK_F14);
	assert(kc2native[keyF15] == VK_F15);
	assert(kc2native[keyF16] == VK_F16);
	assert(kc2native[keyF17] == VK_F17);
	assert(kc2native[keyF18] == VK_F18);
	assert(kc2native[keyF19] == VK_F19);
	assert(kc2native[keyF20] == VK_F20);
	assert(kc2native[keyF21] == VK_F21);
	assert(kc2native[keyF22] == VK_F22);
	assert(kc2native[keyF23] == VK_F23);
	assert(kc2native[keyF24] == VK_F24);
	assert(kc2native[keyNumLock] == VK_NUMLOCK);
	assert(kc2native[keyCapsLock] == VK_CAPITAL);
	assert(kc2native[keyScrollLock] == VK_SCROLL);
	assert(kc2native[keyLShift] == VK_SHIFT);
	assert(kc2native[keyRShift] == VK_RSHIFT);
	assert(kc2native[keyLCtrl] == VK_CONTROL);
	assert(kc2native[keyRCtrl] == VK_RCONTROL);
	assert(kc2native[keyLAlt] == VK_MENU);
	assert(kc2native[keyRAlt] == VK_RMENU);
	assert(kc2native[keyRSuper] == VK_RWIN);
	assert(kc2native[keyLSuper] == VK_LWIN);
	assert(kc2native[keyHelp] == VK_HELP);
	assert(kc2native[keyPrint] == VK_PRINT);
	assert(kc2native[keyBreak] == VK_CANCEL);
	assert(kc2native[keyMenu] == VK_APPS);
	assert(kc2native[keyBrowserBack] == VK_BROWSER_BACK);
	assert(kc2native[keyBrowserForward] == VK_BROWSER_FORWARD);
	assert(kc2native[keyBrowserRefresh] == VK_BROWSER_REFRESH);
	assert(kc2native[keyBrowserStop] == VK_BROWSER_STOP);
	assert(kc2native[keyBrowserSearch] == VK_BROWSER_SEARCH);
	assert(kc2native[keyBrowserFavorites] == VK_BROWSER_FAVORITES);
	assert(kc2native[keyBrowserHome] == VK_BROWSER_HOME);
	assert(kc2native[keyVolumeMute] == VK_VOLUME_MUTE);
	assert(kc2native[keyVolumeDown] == VK_VOLUME_DOWN);
	assert(kc2native[keyVolumeUp] == VK_VOLUME_UP);
	assert(kc2native[keyMediaNext] == VK_MEDIA_NEXT_TRACK);
	assert(kc2native[keyMediaPrev] == VK_MEDIA_PREV_TRACK);
	assert(kc2native[keyMediaStop] == VK_MEDIA_STOP);
	assert(kc2native[keyMediaPlayPause] == VK_MEDIA_PLAY_PAUSE);
	assert(kc2native[keyLaunchMail] == VK_LAUNCH_MAIL);
	assert(kc2native[keyLaunchMediaSelect] == VK_LAUNCH_MEDIA_SELECT);
	assert(kc2native[keyLaunchApp1] == VK_LAUNCH_APP1);
	assert(kc2native[keyLaunchApp2] == VK_LAUNCH_APP2);
	assert(kc2native[keyPlay] == VK_PLAY);
	assert(kc2native[keyZoom] == VK_ZOOM);
}
#endif

// =======================================
// class Key
// =======================================

const char* Key::names[keyCount] = {
	"None",
	"Unknown",
	"LButton",
	"RButton",
	"MButton",
	"XButton1",
	"XButton2",
	"Backspace",
	"Tab",
	"Clear",
	"Return",
	"Pause",
	"IMEKana",
	"IMEHangul",
	"IMEJunja",
	"IMEFinal",
	"IMEHanja",
	"IMEKanji",
	"Escape",
	"IMEConvert",
	"IMENonConvert",
	"IMEAccept",
	"IMEModeChange",
	"IMEProcess",
	"Space",
	"Exclaim",
	"QuoteDbl",
	"Hash",
	"Dollar",
	"Percent",
	"Ampersand",
	"Quote",
	"LeftParen",
	"RightParen",
	"Asterisk",
	"Plus",
	"Comma",
	"Minus",
	"Period",
	"Slash",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"Colon",
	"Semicolon",
	"Less",
	"Equals",
	"Greater",
	"Question",
	"At",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"LeftBracket",
	"Backslash",
	"RightBracket",
	"Caret",
	"Underscore",
	"Backquote",
	"Delete",
	"keyWorld0",
	"keyWorld1",
	"keyWorld2",
	"keyWorld3",
	"keyWorld4",
	"keyWorld5",
	"keyWorld6",
	"keyWorld7",
	"keyWorld8",
	"keyWorld9",
	"keyWorld10",
	"keyWorld11",
	"keyWorld12",
	"keyWorld13",
	"keyWorld14",
	"keyWorld15",
	"keyWorld16",
	"keyWorld17",
	"keyWorld18",
	"keyWorld19",
	"keyWorld20",
	"keyWorld21",
	"keyWorld22",
	"keyWorld23",
	"keyWorld24",
	"keyWorld25",
	"keyWorld26",
	"keyWorld27",
	"keyWorld28",
	"keyWorld29",
	"keyWorld30",
	"keyWorld31",
	"keyWorld32",
	"keyWorld33",
	"keyWorld34",
	"keyWorld35",
	"keyWorld36",
	"keyWorld37",
	"keyWorld38",
	"keyWorld39",
	"keyWorld40",
	"keyWorld41",
	"keyWorld42",
	"keyWorld43",
	"keyWorld44",
	"keyWorld45",
	"keyWorld46",
	"keyWorld47",
	"keyWorld48",
	"keyWorld49",
	"keyWorld50",
	"keyWorld51",
	"keyWorld52",
	"keyWorld53",
	"keyWorld54",
	"keyWorld55",
	"keyWorld56",
	"keyWorld57",
	"keyWorld58",
	"keyWorld59",
	"keyWorld60",
	"keyWorld61",
	"keyWorld62",
	"keyWorld63",
	"keyWorld64",
	"keyWorld65",
	"keyWorld66",
	"keyWorld67",
	"keyWorld68",
	"keyWorld69",
	"keyWorld70",
	"keyWorld71",
	"keyWorld72",
	"keyWorld73",
	"keyWorld74",
	"keyWorld75",
	"keyWorld76",
	"keyWorld77",
	"keyWorld78",
	"keyWorld79",
	"keyWorld80",
	"keyWorld81",
	"keyWorld82",
	"keyWorld83",
	"keyWorld84",
	"keyWorld85",
	"keyWorld86",
	"keyWorld87",
	"keyWorld88",
	"keyWorld89",
	"keyWorld90",
	"keyWorld91",
	"keyWorld92",
	"keyWorld93",
	"keyWorld94",
	"keyWorld95",
	"KP0",
	"KP1",
	"KP2",
	"KP3",
	"KP4",
	"KP5",
	"KP6",
	"KP7",
	"KP8",
	"KP9",
	"KPPeriod",
	"KPDivide",
	"KPMultiply",
	"KPMinus",
	"KPPlus",
	"KPEnter",
	"KPEquals",
	"Up",
	"Down",
	"Right",
	"Left",
	"Insert",
	"Home",
	"End",
	"PageUp",
	"PageDown",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"F13",
	"F14",
	"F15",
	"F16",
	"F17",
	"F18",
	"F19",
	"F20",
	"F21",
	"F22",
	"F23",
	"F24",
	"NumLock",
	"CapsLock",
	"ScrollLock",
	"LShift",
	"RShift",
	"LCtrl",
	"RCtrl",
	"LAlt",
	"RAlt",
	"LMeta",
	"RMeta",
	"RSuper",
	"LSuper",
	"Mode",
	"Compose",
	"Help",
	"Print",
	"Sysreq",
	"Break",
	"Menu",
	"Power",
	"Euro",
	"Undo",
	"BrowserBack",
	"BrowserForward",
	"BrowserRefresh",
	"BrowserStop",
	"BrowserSearch",
	"BrowserFavorites",
	"BrowserHome",
	"VolumeMute",
	"VolumeDown",
	"VolumeUp",
	"MediaNext",
	"MediaPrev",
	"MediaStop",
	"MediaPlayPause",
	"LaunchMail",
	"LaunchMediaSelect",
	"LaunchApp1",
	"LaunchApp2",
	"Play",
	"Zoom"
};

/**
 * Find a KeyCode by name.  Note that this function was never meant to be fast because we aren't
 * indexing, using std::map or anything.
 */
KeyCode Key::findByName(const char *name) {
	for(size_t i = 0; i < keyCount; ++i) {
		if(!strcasecmp(name, names[i])) {
			return (KeyCode)i;
		}
	}
	{
		stringstream str;
		str << "Invalid key name: " << name;
		throw range_error(str.str());
	}
}

}}//end namespace

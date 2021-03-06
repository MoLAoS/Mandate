// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "platform_util.h"

#include <io.h>
#include <cassert>

#include "util.h"
#include "conversion.h"

#include "leak_dumper.h"
#include "symbol_engine.h"

using namespace Shared::Util;

namespace Shared { namespace Platform {

// =====================================================
// class PlatformExceptionHandler
// =====================================================

PlatformExceptionHandler *PlatformExceptionHandler::singleton = NULL;

LONG WINAPI PlatformExceptionHandler::handler(LPEXCEPTION_POINTERS pointers) {

	DWORD exceptionCode = pointers->ExceptionRecord->ExceptionCode;
	ULONG_PTR *exceptionInfo = pointers->ExceptionRecord->ExceptionInformation;

	string description = codeToStr(exceptionCode);

	DebugSymbolEngine &symbolEngine = DebugSymbolEngine::instance();

	if (exceptionCode == EXCEPTION_ACCESS_VIOLATION) {
		description += " (";
		description += exceptionInfo[0] == 0 ? "Reading" : "Writing";
		description += " address: 0x" + intToHex(static_cast<int>(exceptionInfo[1])) + ")";
	}
	std::ostringstream ss;
	symbolEngine.StackTrace(pointers->ContextRecord, ss);
	description += "\n\nCall Stack:\n";
	description += ss.str();

	singleton->log(description.c_str(), pointers->ExceptionRecord->ExceptionAddress, NULL, 0, NULL);
	singleton->notifyUser(false);

	return EXCEPTION_EXECUTE_HANDLER;
}

void PlatformExceptionHandler::install() {
	assert(this);
	assert(singleton == this);
	SetUnhandledExceptionFilter(handler);
}

string PlatformExceptionHandler::codeToStr(DWORD code) {
	switch (code) {
	case EXCEPTION_ACCESS_VIOLATION:		return "Access violation";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:	return "Array bounds exceeded";
	case EXCEPTION_BREAKPOINT:				return "Breakpoint";
	case EXCEPTION_DATATYPE_MISALIGNMENT:	return "Datatype misalignment";
	case EXCEPTION_FLT_DENORMAL_OPERAND:	return "Float denormal operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:		return "Float divide by zero";
	case EXCEPTION_FLT_INEXACT_RESULT:		return "Float inexact result";
	case EXCEPTION_FLT_INVALID_OPERATION:	return "Float invalid operation";
	case EXCEPTION_FLT_OVERFLOW:			return "Float overflow";
	case EXCEPTION_FLT_STACK_CHECK:			return "Float stack check";
	case EXCEPTION_FLT_UNDERFLOW:			return "Float underflow";
	case EXCEPTION_ILLEGAL_INSTRUCTION:		return "Illegal instruction";
	case EXCEPTION_IN_PAGE_ERROR:			return "In page error";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:		return "Integer divide by zero";
	case EXCEPTION_INT_OVERFLOW:			return "Integer overflow";
	case EXCEPTION_INVALID_DISPOSITION:		return "Invalid disposition";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:return "Noncontinuable exception";
	case EXCEPTION_PRIV_INSTRUCTION:		return "Private instruction";
	case EXCEPTION_SINGLE_STEP:				return "Single step";
	case EXCEPTION_STACK_OVERFLOW:			return "Stack overflow";
	default:								return "Unknown exception code";
	}
}

void fail(const char *msg, HRESULT hr) {
	stringstream ss;
	ss << msg << "\n\tHRESULT == " << hr;
	throw runtime_error(ss.str());
}

// =====================================================
// class Misc
// =====================================================

void mkdir(const string &path, bool ignoreDirExists) {
	if (!CreateDirectory(path.c_str(), NULL)) {
		DWORD err = GetLastError();
		if (!(ignoreDirExists && err == ERROR_ALREADY_EXISTS)) {
			throw runtime_error("Failed to create directory " + path);
		}
	}
}

size_t getFileSize(const string &path) {
	HANDLE h = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
						  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	int err;
	size_t ret;
	if (!h) {
		throw runtime_error("Failed to open file " + path);
	}
	ret = GetFileSize(h, NULL);
	if (ret == INVALID_FILE_SIZE) {
		CloseHandle(h);
		throw runtime_error("Failed to get file size for file " + path);
	}
	CloseHandle(h);
	return ret;
}

string videoModeToString(const VideoMode in_mode) {
	return toStr(in_mode.w) + "x" + toStr(in_mode.h) + " " + toStr(in_mode.bpp) + "bpp @ " 
		+ toStr(in_mode.freq) + "Hz.";
}

bool changeVideoMode(const VideoMode in_mode) {
	DEVMODE devMode;

	for (int i = 0; EnumDisplaySettings(NULL, i, &devMode) ;i++) {
		if (devMode.dmPelsWidth == in_mode.w && devMode.dmPelsHeight == in_mode.h
		&& devMode.dmBitsPerPel == in_mode.bpp && devMode.dmDisplayFrequency == in_mode.freq) {
			LONG result = ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
			if (result == DISP_CHANGE_SUCCESSFUL) {
				return true;
			} else {
				return false;
			}
		}
	}

	return false;
}

void restoreVideoMode() {
	int dispChangeErr = ChangeDisplaySettings(NULL, 0);
	assert(dispChangeErr == DISP_CHANGE_SUCCESSFUL);
}

void getPossibleScreenModes(vector<VideoMode> &out_modes) {
	DEVMODE dm = {0};
	dm.dmSize = sizeof(dm);
	for (int iModeNum = 0; EnumDisplaySettings(NULL, iModeNum, &dm) != 0; ++iModeNum) {
		if (dm.dmBitsPerPel == 32/* || dm.dmBitsPerPel == 16*/) {
			VideoMode mode(dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel, dm.dmDisplayFrequency);
			out_modes.push_back(mode);
		}
	}
}

void getScreenMode(int &width, int &height) {
	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);
}

void message(string message) {
	MessageBox(NULL, message.c_str(), "Message", MB_OK);
}

bool ask(string message) {
	return MessageBox(NULL, message.c_str(), "Confirmation", MB_YESNO) == IDYES;
}

void exceptionMessage(const exception &excp) {
	string message, title;
	showCursor(true);

	message += "ERROR(S):\n\n";
	message += excp.what();

	title = "Error: Unhandled Exception";
	MessageBox(NULL, message.c_str(), title.c_str(), MB_ICONSTOP | MB_OK | MB_TASKMODAL);
}


}
}//end namespace

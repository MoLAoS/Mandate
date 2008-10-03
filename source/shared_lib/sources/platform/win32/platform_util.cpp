// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "platform_util.h"

#include <io.h>
#include <cassert>

#include "util.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

// =====================================================
//	class PerformanceTimer
// =====================================================

void PerformanceTimer::init(int fps, int maxTimes){
	int64 freq;

	if(QueryPerformanceFrequency((LARGE_INTEGER*) &freq)==0){
		throw runtime_error("Performance counters not supported");
	}

	times= 0;
	this->maxTimes= maxTimes;

	QueryPerformanceCounter((LARGE_INTEGER*) &lastTicks);

	updateTicks= freq/fps;
}

bool PerformanceTimer::isTime(){
	QueryPerformanceCounter((LARGE_INTEGER*) &thisTicks);

	if((thisTicks-lastTicks)>=updateTicks && times<maxTimes){
		lastTicks+= updateTicks;
		times++;
		return true;
	}
	times= 0;
	return false;
}

void PerformanceTimer::reset(){
	QueryPerformanceCounter((LARGE_INTEGER*) &thisTicks);
	lastTicks= thisTicks;
}

// =====================================================
//	class Chrono
// =====================================================

Chrono::Chrono(){
	if(!QueryPerformanceFrequency((LARGE_INTEGER*) &freq)){
		throw runtime_error("Performance counters not supported");
	}
	stopped= true;
	accumCount= 0;
}

void Chrono::start(){
	stopped= false;
	QueryPerformanceCounter((LARGE_INTEGER*) &startCount);
}

void Chrono::stop(){
	int64 endCount;
	QueryPerformanceCounter((LARGE_INTEGER*) &endCount);
	accumCount+= endCount-startCount;
	stopped= true;
}

int64 Chrono::getMicros() const{
	return queryCounter(1000000);
}

int64 Chrono::getMillis() const{
	return queryCounter(1000);
}

int64 Chrono::getSeconds() const{
	return queryCounter(1);
}

int64 Chrono::queryCounter(int multiplier) const{
	if(stopped){
		return multiplier*accumCount/freq;
	}
	else{
		int64 endCount;
		QueryPerformanceCounter((LARGE_INTEGER*) &endCount);
		return multiplier*(accumCount+endCount-startCount)/freq;
	}
}

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

PlatformExceptionHandler *PlatformExceptionHandler::thisPointer= NULL;

LONG WINAPI PlatformExceptionHandler::handler(LPEXCEPTION_POINTERS pointers){

	DWORD exceptionCode= pointers->ExceptionRecord->ExceptionCode;
	ULONG_PTR *exceptionInfo= pointers->ExceptionRecord->ExceptionInformation;

	string description= codeToStr(exceptionCode);

	if(exceptionCode==EXCEPTION_ACCESS_VIOLATION){
		description+= " (";
		description+= exceptionInfo[0]==0? "Reading": "Writing";
		description+= " address 0x" + intToHex(static_cast<int>(exceptionInfo[1]))+")";
	}

	thisPointer->handle(description, pointers->ExceptionRecord->ExceptionAddress);

	return EXCEPTION_EXECUTE_HANDLER;
}

void PlatformExceptionHandler::install(){
	thisPointer= this;
	SetUnhandledExceptionFilter(handler);
}

string PlatformExceptionHandler::codeToStr(DWORD code){
	switch(code){
	case EXCEPTION_ACCESS_VIOLATION:         return "Access violation";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "Array bounds exceeded";
	case EXCEPTION_BREAKPOINT:               return "Breakpoint";
	case EXCEPTION_DATATYPE_MISALIGNMENT:    return "Datatype misalignment";
	case EXCEPTION_FLT_DENORMAL_OPERAND:     return "Float denormal operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "Float divide by zero";
	case EXCEPTION_FLT_INEXACT_RESULT:       return "Float inexact result";
	case EXCEPTION_FLT_INVALID_OPERATION:    return "Float invalid operation";
	case EXCEPTION_FLT_OVERFLOW:             return "Float overflow";
	case EXCEPTION_FLT_STACK_CHECK:          return "Float stack check";
	case EXCEPTION_FLT_UNDERFLOW:            return "Float underflow";
	case EXCEPTION_ILLEGAL_INSTRUCTION:      return "Illegal instruction";
	case EXCEPTION_IN_PAGE_ERROR:            return "In page error";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "Integer divide by zero";
	case EXCEPTION_INT_OVERFLOW:             return "Integer overflow";
	case EXCEPTION_INVALID_DISPOSITION:      return "Invalid disposition";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable exception";
	case EXCEPTION_PRIV_INSTRUCTION:         return "Private instruction";
	case EXCEPTION_SINGLE_STEP:              return "Single step";
	case EXCEPTION_STACK_OVERFLOW:           return "Stack overflow";
	default: return "Unknown exception code";
	}
}

// =====================================================
//	class Misc
// =====================================================

//finds all filenames like path and stores them in resultys
void findAll(const string &path, vector<string> &results, bool cutExtension){

	int i= 0;
	struct _finddata_t fi;
	intptr_t handle;
	char *cstr;

	results.clear();

	cstr= new char[path.length()+1];
	strcpy(cstr, path.c_str());

	if((handle=_findfirst(cstr,&fi))!=-1){
		do{
			if(strcmp(".", fi.name) && strcmp("..", fi.name)
					&& strcmp(".svn", fi.name)){
				i++;
				results.push_back(fi.name);
			}
		}
		while(_findnext(handle, &fi)==0);
	}
	else{
		throw runtime_error("Error opening files: "+ path);
	}

	if(i==0){
		throw runtime_error("No files found: "+ path);
	}

	if(cutExtension){
		for (int i=0; i<results.size(); ++i){
			results.at(i)=cutLastExt(results.at(i));
		}
	}

	delete [] cstr;
}

void mkdir(const string &path, bool ignoreDirExists) {
	if(!CreateDirectory(path.c_str(), NULL)) {
		DWORD err = GetLastError();
		if(!(ignoreDirExists && err == ERROR_ALREADY_EXISTS)) {
			throw runtime_error("Failed to create directory " + path);
		}
	}
}

size_t getFileSize(const string &path) {
	HANDLE h = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	int err;
	size_t ret;
	if(!h) {
		throw runtime_error("Failed to open file " + path);
	}
	ret = GetFileSize(h, NULL);
	if(ret == INVALID_FILE_SIZE) {
		CloseHandle(h);
		throw runtime_error("Failed to get file size for file " + path);
	}
	CloseHandle(h);
	return ret;
}



bool changeVideoMode(int resW, int resH, int colorBits, int refreshFrequency){
	DEVMODE devMode;

	for (int i=0; EnumDisplaySettings(NULL, i, &devMode) ;i++){
		if (devMode.dmPelsWidth== resW &&
			devMode.dmPelsHeight== resH &&
			devMode.dmBitsPerPel== colorBits){

			devMode.dmDisplayFrequency=refreshFrequency;

			LONG result= ChangeDisplaySettings(&devMode, 0);
			if(result == DISP_CHANGE_SUCCESSFUL){
				return true;
			}
			else{
				return false;
			}
		}
	}

	return false;
}

void restoreVideoMode(){
	int dispChangeErr= ChangeDisplaySettings(NULL, 0);
	assert(dispChangeErr==DISP_CHANGE_SUCCESSFUL);
}

void message(string message){
	MessageBox(NULL, message.c_str(), "Message", MB_OK);
}

bool ask(string message){
	return MessageBox(NULL, message.c_str(), "Confirmation", MB_YESNO)==IDYES;
}

void exceptionMessage(const exception &excp){
	string message, title;
	showCursor(true);

	message+= "ERROR(S):\n\n";
	message+= excp.what();

	title= "Error: Unhandled Exception";
	MessageBox(NULL, message.c_str(), title.c_str(), MB_ICONSTOP | MB_OK | MB_TASKMODAL);
}

int getScreenW(){
	return GetSystemMetrics(SM_CXSCREEN);
}

int getScreenH(){
	return GetSystemMetrics(SM_CYSCREEN);
}

void sleep(int millis){
	Sleep(millis);
}

void showCursor(bool b){
	ShowCursor(b);
}

bool isKeyDown(int virtualKey){
	return (GetKeyState(virtualKey) & 0x8000) != 0;
}

}}//end namespace

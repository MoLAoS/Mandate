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

#include "pch.h"
#include "thread.h"

#include "leak_dumper.h"

namespace Shared { namespace Platform {

// =====================================================
// class Thread
// =====================================================

ThreadId nextThreadId = 1000;

void Thread::start() {
	thread = CreateThread(NULL, 0, beginExecution, this, 0, &nextThreadId);
	nextThreadId++;
}

void Thread::setPriority(Thread::Priority threadPriority) {
	SetThreadPriority(thread, threadPriority);
}

DWORD WINAPI Thread::beginExecution(void *param) {
	static_cast<Thread*>(param)->execute();
	return 0;
}

void Thread::suspend() {
	SuspendThread(thread);
}

void Thread::resume() {
	ResumeThread(thread);
}

bool Thread::join(int maxWaitMillis) {
	return WaitForSingleObject(thread, maxWaitMillis) == WAIT_OBJECT_0;
}

// =====================================================
// class Mutex
// =====================================================

Mutex::Mutex() {
	InitializeCriticalSection(&mutex);
}

Mutex::~Mutex() {
	DeleteCriticalSection(&mutex);
}

void Mutex::p() {
	EnterCriticalSection(&mutex);
}

void Mutex::v() {
	LeaveCriticalSection(&mutex);
}

}}//end namespace

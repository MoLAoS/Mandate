// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "thread.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "noimpl.h"

#include "leak_dumper.h"

namespace Shared { namespace Platform {

// =====================================
//          Threads
// =====================================

void Thread::start() {
	thread = SDL_CreateThread(beginExecution, this);
}

void Thread::setPriority(Thread::Priority threadPriority) {
	NOIMPL;
}

int Thread::beginExecution(void* data) {
	Thread* thread = static_cast<Thread*>(data);
	thread->execute();
	return 0;
}

void Thread::suspend() {
	NOIMPL;
}

void Thread::resume() {
	NOIMPL;
}

bool Thread::join(int maxWaitMillis) {
	SDL_WaitThread(thread, 0);
	return true;
}

// =====================================
//          Mutex
// =====================================

Mutex::Mutex() {
	mutex = SDL_CreateMutex();
	if (mutex == 0)
		throw std::runtime_error("Couldn't initialize mutex");
}

Mutex::~Mutex() {
	SDL_DestroyMutex(mutex);
}

void Mutex::p() {
	SDL_mutexP(mutex);
}

void Mutex::v() {
	SDL_mutexV(mutex);
}

}
}//end namespace

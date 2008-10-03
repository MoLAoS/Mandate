// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2005 Matthias Braun <matze@braunis.de>,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_THREAD_H_
#define _SHARED_PLATFORM_THREAD_H_

#ifdef USE_SDL
	#include <SDL_thread.h>
	#include <SDL_mutex.h>
#endif
#if defined(WIN32)  || defined(WIN64)
	#include <windows.h>
#endif

#include "types.h"

namespace Shared{ namespace Platform{

// =====================================================
//	class Thread
// =====================================================
#ifdef USE_SDL
	#define THREAD_PRIORITY_IDLE			0
	#define THREAD_PRIORITY_BELOW_NORMAL	1
	#define THREAD_PRIORITY_NORMAL			2
	#define THREAD_PRIORITY_ABOVE_NORMAL	3
	#define THREAD_PRIORITY_TIME_CRITICAL	4
	#define INFINITE -1
#endif
#if defined(WIN32) || defined(WIN64)
	typedef LPTHREAD_START_ROUTINE ThreadFunction;
	typedef DWORD ThreadId;
#endif

class Thread{
public:
	enum Priority {
		pIdle= THREAD_PRIORITY_IDLE,
		pLow= THREAD_PRIORITY_BELOW_NORMAL,
		pNormal= THREAD_PRIORITY_NORMAL,
		pHigh= THREAD_PRIORITY_ABOVE_NORMAL,
		pRealTime= THREAD_PRIORITY_TIME_CRITICAL
	};

private:
	ThreadType thread;

public:
	virtual ~Thread() {}
	virtual void execute() = 0;
	void start();
	void setPriority(Thread::Priority threadPriority);
	void suspend();
	void resume();

	/**
	 * Waits a max of maxWaitMillis milliseconds for thread to die.
	 * @return true if the thread terminated or false if maxWaitMillis lapsed.
	 */
	bool join(int maxWaitMillis = INFINITE);

private:
#ifdef USE_SDL
	static int beginExecution(void *param);
#endif
#if defined(WIN32)  || defined(WIN64)
	static DWORD WINAPI beginExecution(void *param);
#endif
};

// =====================================================
//	class Mutex
// =====================================================

class Mutex{
private:
	MutexType mutex;

public:
	Mutex();
	~Mutex();
	void p();
	void v();
};

/** MutexLock is a convenicnce and safety class to manage locking and unlocking
 * a mutex and is intended to be created on the stack.  The advantage of using
 * MutexLock over explicitly calling Mutex.p() and .v() is that you can never
 * forget to unlock the mutex.  Even if an exception is thrown, the stack unwind
 * will cause the mutex to be unlocked.
 */
class MutexLock {
	Mutex &mutex;

public:
	MutexLock(Mutex &mutex) : mutex(mutex) {
		mutex.p();
	}

	~MutexLock() {
		mutex.v();
	}
};

}}//end namespace

#endif

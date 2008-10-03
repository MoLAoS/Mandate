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

#ifndef _SHARED_PLATFORM_THREAD_H_
#define _SHARED_PLATFORM_THREAD_H_

#include <windows.h>

// =====================================================
//	class Thread
// =====================================================

namespace Shared{ namespace Platform{

typedef LPTHREAD_START_ROUTINE ThreadFunction;
typedef DWORD ThreadId;

class Thread{
public:
	enum Priority{
		pIdle= THREAD_PRIORITY_IDLE,
		pLow= THREAD_PRIORITY_BELOW_NORMAL,
		pNormal= THREAD_PRIORITY_NORMAL,
		pHigh= THREAD_PRIORITY_ABOVE_NORMAL,
		pRealTime= THREAD_PRIORITY_TIME_CRITICAL
	};

private:
	HANDLE threadHandle;
	static const ThreadId threadIdBase= 1000;
	static ThreadId nextThreadId;

public:
	void start();
	virtual void execute()=0;
	void setPriority(Thread::Priority threadPriority);
	void suspend();
	void resume();

	/**
	 * Waits a max of maxWaitMillis milliseconds for thread to die.
	 * @return true if the thread terminated or false if maxWaitMillis lapsed.
	 */
	bool join(int maxWaitMillis = INFINITE);

private:
	static DWORD WINAPI beginExecution(void *param);
};

// =====================================================
//	class Mutex
// =====================================================

class Mutex{
private:
	CRITICAL_SECTION mutex;

public:
	Mutex();
	~Mutex();
	void p();
	void v();
};

//FIXME: move this to a platform-common location
/** MutexLock is a convenicnce and safety object to manage locking and unlocking
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

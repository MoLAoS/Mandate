// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_THREAD_H_
#define _SHARED_PLATFORM_THREAD_H_

#include <SDL_thread.h>
#include <SDL_mutex.h>

// =====================================================
//	class Thread
// =====================================================

namespace Shared{ namespace Platform{

class Thread{
public:
	enum Priority {
		pIdle = 0,
		pLow = 1,
		pNormal = 2,
		pHigh = 3,
		pRealTime = 4
	};

private:
	SDL_Thread* thread;

public:
	virtual ~Thread() {}

	void start();
	virtual void execute()=0;
	void setPriority(Thread::Priority threadPriority);
	void suspend();
	void resume();

	/**
	 * Waits a max of maxWaitMillis milliseconds for thread to die.
	 * @return true if the thread terminated or false if maxWaitMillis lapsed.
	 */
	bool join(int maxWaitMillis = -1);


private:
	static int beginExecution(void *param);
};

// =====================================================
//	class Mutex
// =====================================================

class Mutex{
private:
	SDL_mutex* mutex;

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

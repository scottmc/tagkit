/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "NetworkTimeout.h"

#include <OS.h>


namespace tagkit {

namespace {

volatile int32 sTimeoutSeconds = kDefaultNetworkTimeoutSeconds;


// Shared between the waiting caller and the helper thread; whichever lets
// go last frees it (and the semaphore), so an abandoned helper can finish
// safely after the caller has moved on.
struct TimedWork {
	sem_id					done;
	int32					references;
	std::function<void()>	work;
};


void
ReleaseTimedWork(TimedWork* timed)
{
	if (atomic_add(&timed->references, -1) == 1) {
		delete_sem(timed->done);
		delete timed;
	}
}


status_t
TimedWorkEntry(void* data)
{
	TimedWork* timed = static_cast<TimedWork*>(data);
	try {
		timed->work();
	} catch (...) {
	}
	release_sem(timed->done);
	ReleaseTimedWork(timed);
	return B_OK;
}

} // namespace


void
set_network_timeout(int32 seconds)
{
	if (seconds < 1)
		seconds = 1;
	if (seconds > 600)
		seconds = 600;
	sTimeoutSeconds = seconds;
}


int32
network_timeout()
{
	return sTimeoutSeconds;
}


bool
run_with_timeout(int32 seconds, const std::function<void()>& work)
{
	TimedWork* timed = new TimedWork;
	timed->done = create_sem(0, "tagkit timed work");
	timed->references = 2;
	timed->work = work;

	thread_id thread = timed->done >= 0
		? spawn_thread(TimedWorkEntry, "tagkit network call",
			B_NORMAL_PRIORITY, timed)
		: B_ERROR;
	if (thread < 0) {
		// No thread to time: just do the work, unlimited.
		if (timed->done >= 0)
			delete_sem(timed->done);
		delete timed;
		try {
			work();
		} catch (...) {
		}
		return true;
	}
	resume_thread(thread);

	status_t result;
	do {
		result = acquire_sem_etc(timed->done, 1, B_RELATIVE_TIMEOUT,
			(bigtime_t)seconds * 1000000LL);
	} while (result == B_INTERRUPTED);

	ReleaseTimedWork(timed);
	return result == B_OK;
}

} // namespace tagkit

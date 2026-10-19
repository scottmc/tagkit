/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// NetworkTimeout.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// The time limit every TagKit network request works under, and a helper
// for putting a limit on library calls (libmusicbrainz5, libcoverart)
// that have no time limit of their own.

#ifndef TAGKIT_NETWORK_TIMEOUT_H
#define TAGKIT_NETWORK_TIMEOUT_H

#include <functional>

#include <SupportDefs.h>


namespace tagkit {

// The limit, in seconds, used when nothing else is asked for.
const int32 kDefaultNetworkTimeoutSeconds = 30;

// Sets how long one network request may take before TagKit gives up on it,
// for the whole program (values are kept between 1 and 600 seconds).
void set_network_timeout(int32 seconds);

// The current limit in seconds.
int32 network_timeout();

// Runs work() on a helper thread and waits up to seconds for it to finish.
// Returns true if it finished in time (also if it couldn't start a thread
// and ran work() right here instead), false if the time ran out. On a
// timeout the helper thread is not killed (killing a thread in the middle
// of network code can leave locks held); it is left to finish by itself and
// its result is thrown away, so work() must only touch data it owns (copies
// or shared_ptr-held results), never the caller's locals. Exceptions
// escaping work() are caught and ignored.
bool run_with_timeout(int32 seconds, const std::function<void()>& work);

} // namespace tagkit

#endif // TAGKIT_NETWORK_TIMEOUT_H

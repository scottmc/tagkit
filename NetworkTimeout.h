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
// The time limit TagKit's downloads (HttpFetch) work under. The one network
// call that can't be limited this way is the MusicBrainz search, because
// libmusicbrainz5 has no time limit setting.

#ifndef TAGKIT_NETWORK_TIMEOUT_H
#define TAGKIT_NETWORK_TIMEOUT_H

#include <SupportDefs.h>


namespace tagkit {

// The limit, in seconds, used when nothing else is asked for.
const int32 kDefaultNetworkTimeoutSeconds = 30;

// Sets how long one network request may take before TagKit gives up on it,
// for the whole program (values are kept between 1 and 600 seconds).
void set_network_timeout(int32 seconds);

// The current limit in seconds.
int32 network_timeout();

} // namespace tagkit

#endif // TAGKIT_NETWORK_TIMEOUT_H

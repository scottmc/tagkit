/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "NetworkTimeout.h"



namespace tagkit {

namespace {

volatile int32 sTimeoutSeconds = kDefaultNetworkTimeoutSeconds;

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

} // namespace tagkit

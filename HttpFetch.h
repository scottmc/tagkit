/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// HttpFetch.h
//
// Part of "tagkit" -- shared tagging code, first written for TagView.
// One blocking HTTP(S) GET. Haiku's public network kit has no HTTP client
// (the BHttpRequest headers are private), so this runs the curl that ships
// with Haiku, which follows redirects and uses the system's trusted
// certificates. Kept as a single small function so the backend can be
// swapped without touching the callers.

#ifndef TAGKIT_HTTP_FETCH_H
#define TAGKIT_HTTP_FETCH_H

#include <vector>

#include <String.h>
#include <SupportDefs.h>


namespace tagkit {

// Downloads url into data (replacing its contents). Blocks until done, so
// call it from a worker thread, never the UI thread. Returns B_OK only for
// a successful (2xx) response; otherwise an error, with a short reason in
// errorMessage if one is given. B_TIMED_OUT means the server couldn't be
// reached, stalled, or took longer than timeoutSeconds in all (0, the
// default, uses network_timeout() from NetworkTimeout.h). B_BUSY means the
// server answered "too many requests" (HTTP 429 or 503): wait a moment and
// try again. If httpStatus is given it receives the final HTTP status, or 0
// when there was no answer at all.
status_t http_get(const BString& url, std::vector<unsigned char>& data,
	BString* errorMessage = NULL, int32 timeoutSeconds = 0,
	int32* httpStatus = NULL);

// Percent-encodes text so it can be used as a value in a URL's query (the
// "term=..." part): everything but letters, digits and - _ . ~ becomes
// %XX (UTF-8 bytes included, a space becomes %20).
BString url_encode(const BString& text);

} // namespace tagkit

#endif // TAGKIT_HTTP_FETCH_H

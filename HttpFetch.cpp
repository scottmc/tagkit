/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "HttpFetch.h"
#include "ClientInfo.h"
#include "NetworkTimeout.h"

#include <stdio.h>
#include <string.h>
#include <sys/wait.h>



// Downloads bigger than this are treated as an error (cover art is well
// under it).
static const size_t kMaxDownloadBytes = 32 * 1024 * 1024;


namespace tagkit {

namespace {

// The URL ends up inside a single-quoted shell argument, so only let
// through the characters a normal, already percent-encoded URL contains.
// (This also rules out quotes and anything else the shell could act on.)
bool
IsSafeUrl(const BString& url)
{
	if (url.FindFirst("http://") != 0 && url.FindFirst("https://") != 0)
		return false;

	for (int32 i = 0; i < url.Length(); i++) {
		unsigned char c = (unsigned char)url.ByteAt(i);
		bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9') || strchr("-._~:/?&=%+,;@!*()", c) != NULL;
		if (!ok)
			return false;
	}
	return true;
}

} // namespace


BString
url_encode(const BString& text)
{
	static const char* hex = "0123456789ABCDEF";

	BString encoded;
	for (int32 i = 0; i < text.Length(); i++) {
		unsigned char c = (unsigned char)text.ByteAt(i);
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
				|| (c >= '0' && c <= '9') || c == '-' || c == '_'
				|| c == '.' || c == '~') {
			encoded << (char)c;
		} else {
			char buffer[4] = { '%', hex[c >> 4], hex[c & 15], '\0' };
			encoded << buffer;
		}
	}
	return encoded;
}


status_t
http_get(const BString& url, std::vector<unsigned char>& data,
	BString* errorMessage, int32 timeoutSeconds)
{
	data.clear();

	if (timeoutSeconds <= 0)
		timeoutSeconds = network_timeout();

	if (!IsSafeUrl(url)) {
		if (errorMessage != NULL)
			*errorMessage = "unsupported characters in URL";
		return B_BAD_VALUE;
	}

	// Haiku's own curl: follows redirects (-L), fails on HTTP errors (-f),
	// stays quiet but still reports errors (-sS, which go to stderr and
	// are dropped). Gives up if no connection is made within the connect
	// limit (--connect-timeout), if the transfer stalls (under 1 byte a
	// second for --speed-time seconds), or after timeoutSeconds in all
	// (-m).
	int32 connectSeconds = timeoutSeconds < 10 ? timeoutSeconds : 10;
	int32 stallSeconds = timeoutSeconds < 15 ? timeoutSeconds : 15;
	BString command("curl -sSfL --connect-timeout ");
	command << connectSeconds << " --speed-limit 1 --speed-time "
		<< stallSeconds << " -m " << timeoutSeconds << " -A '";
	command << tagkit::user_agent() << "' -- '" << url << "' 2>/dev/null";

	FILE* pipe = popen(command.String(), "r");
	if (pipe == NULL) {
		if (errorMessage != NULL)
			*errorMessage = "couldn't run curl";
		return B_ERROR;
	}

	unsigned char buffer[16384];
	size_t count;
	bool tooBig = false;
	while ((count = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
		if (data.size() + count > kMaxDownloadBytes) {
			tooBig = true;
			break;
		}
		data.insert(data.end(), buffer, buffer + count);
	}

	int status = pclose(pipe);

	if (tooBig) {
		data.clear();
		if (errorMessage != NULL)
			*errorMessage = "download too large";
		return B_ERROR;
	}

	if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		data.clear();

		// curl's exit codes: 28 is any of its time limits, 6 and 7 mean
		// the host couldn't be found or reached.
		int32 exitCode = (status != -1 && WIFEXITED(status))
			? (int32)WEXITSTATUS(status) : -1;
		if (exitCode == 28) {
			if (errorMessage != NULL)
				*errorMessage = "timed out";
			return B_TIMED_OUT;
		}
		if (errorMessage != NULL) {
			*errorMessage << "curl failed";
			if (exitCode >= 0)
				*errorMessage << " (exit status " << exitCode << ")";
		}
		return B_ERROR;
	}

	return B_OK;
}


} // namespace tagkit

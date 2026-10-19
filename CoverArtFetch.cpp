/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CoverArtFetch.h"
#include "ClientInfo.h"
#include "NetworkTimeout.h"

#include <exception>
#include <memory>
#include <stdio.h>
#include <string>

#include <Debug.h>

#include <coverart/CoverArt.h>



namespace tagkit {

namespace {

// What the helper thread of one request leaves behind.
struct FetchResult {
	std::vector<unsigned char>	data;
	std::string					error;
};

} // namespace


int32
fetch_cover_art(const std::vector<ReleaseRef>& releases, int32 maxImages,
	const CoverArtFoundFunction& found, bool* timedOut)
{
	int32 count = 0;
	if (timedOut != NULL)
		*timedOut = false;

	for (size_t i = 0; i < releases.size() && count < maxImages; i++) {
		const ReleaseRef& release = releases[i];
		if (release.id.IsEmpty())
			continue;

		// libcoverart has no time limit of its own, so the request runs on
		// a helper thread that is abandoned if it takes too long.
		std::shared_ptr<FetchResult> result(new FetchResult);
		BString releaseId(release.id);
		bool finished = run_with_timeout(network_timeout(), [=]() {
			try {
				CoverArtArchive::CCoverArt coverArt(tagkit::user_agent());
				result->data = coverArt.FetchFront(releaseId.String());
			} catch (std::exception& ex) {
				result->error = ex.what();
				if (result->error.empty())
					result->error = "failed";
			}
		});
		if (!finished) {
			fprintf(stderr, "TagView: cover art request for release %s (%s) "
				"timed out; giving up on the rest\n", release.id.String(),
				release.title.String());
			if (timedOut != NULL)
				*timedOut = true;
			break;
		}
		if (!result->error.empty()) {
			// Most often just "no cover art for this release" (a 404).
			fprintf(stderr, "TagView: no cover art for release %s (%s): %s\n",
				release.id.String(), release.title.String(),
				result->error.c_str());
			continue;
		}
		std::vector<unsigned char> data;
		data.swap(result->data);

		if (data.empty()) {
			fprintf(stderr, "TagView: empty cover art for release %s (%s)\n",
				release.id.String(), release.title.String());
			continue;
		}

		CoverArtImage image;
		image.releaseId = release.id;
		image.releaseTitle = release.title;
		image.year = release.year;
		image.mimeType = detect_image_mime_type(&data[0], data.size());
		image.data.swap(data);

		// Anything that isn't a format we can both show and embed isn't
		// worth offering.
		if (image.mimeType.IsEmpty())
			continue;

		fprintf(stderr, "TagView: cover art for release %s (%s): %s, %d bytes\n",
			release.id.String(), release.title.String(),
			image.mimeType.String(), (int)image.data.size());
		count++;
		if (found)
			found(image);
	}

	return count;
}


std::vector<CoverArtImage>
fetch_cover_art(const std::vector<ReleaseRef>& releases, int32 maxImages,
	bool* timedOut)
{
	std::vector<CoverArtImage> images;
	fetch_cover_art(releases, maxImages,
		[&images](const CoverArtImage& image) { images.push_back(image); },
		timedOut);
	return images;
}


} // namespace tagkit

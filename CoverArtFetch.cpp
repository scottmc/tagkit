/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "CoverArtFetch.h"
#include "HttpFetch.h"

#include <stdio.h>

#include <Debug.h>



namespace tagkit {


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

		// The Cover Art Archive redirects this to the release's front
		// cover image (and answers 404 when it has none).
		BString url("https://coverartarchive.org/release/");
		url << release.id << "/front";

		std::vector<unsigned char> data;
		BString error;
		status_t status = http_get(url, data, &error);
		if (status == B_TIMED_OUT) {
			fprintf(stderr, "TagKit: cover art request for release %s (%s) "
				"timed out; giving up on the rest\n", release.id.String(),
				release.title.String());
			if (timedOut != NULL)
				*timedOut = true;
			break;
		}
		if (status != B_OK) {
			// Most often just "no cover art for this release" (a 404).
			fprintf(stderr, "TagKit: no cover art for release %s (%s): %s\n",
				release.id.String(), release.title.String(), error.String());
			continue;
		}

		if (data.empty()) {
			fprintf(stderr, "TagKit: empty cover art for release %s (%s)\n",
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

		fprintf(stderr, "TagKit: cover art for release %s (%s): %s, %d bytes\n",
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

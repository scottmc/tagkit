/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// RecordingMatch.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// One MusicBrainz recording (track) search result. Deliberately has no
// dependency on any JSON or web code, so callers (and other apps)
// don't need to include/link against it just to hold or display a
// search result.

#ifndef TAGKIT_RECORDING_MATCH_H
#define TAGKIT_RECORDING_MATCH_H

#include <vector>

#include <String.h>
#include <SupportDefs.h>


namespace tagkit {

// One release (album/single/...) a recording appears on. Cover art is
// looked up per release, so a recording that appears on several releases
// gives the user several candidate covers to choose between.
struct ReleaseRef {
	ReleaseRef()
		:
		year(0)
	{
	}

	BString	id;			// MusicBrainz release MBID
	BString	title;
	int32	year;			// 0 == unknown
};


struct RecordingMatch {
	RecordingMatch()
		:
		durationSeconds(-1),
		track(0),
		year(0),
		score(-1)
	{
	}

	BString	id;			// MusicBrainz recording MBID
	BString	title;
	BString	artist;
	BString	disambiguation;	// MusicBrainz's note that tells recordings with
							// the same title apart ("live", "remastered"...),
							// empty if it has none
	BString	album;			// best effort: the first release this
							// recording appears on, if any
	int32	durationSeconds;	// -1 == unknown
	int32	track;			// position on the first release's disc, 0 ==
							// unknown
	int32	year;			// first release's year, 0 == unknown
	std::vector<ReleaseRef>	releases;	// every release this recording
								// appears on (as many as MusicBrainz
								// listed, capped), in MusicBrainz's order
	int32	score;			// 0-100 relevance MusicBrainz assigned to
							// this result, or -1 if unknown. Matches come
							// back in MusicBrainz's own relevance order
							// anyway.
};

} // namespace tagkit

#endif // TAGKIT_RECORDING_MATCH_H

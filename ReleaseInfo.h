/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// ReleaseInfo.h
//
// Part of "tagkit" -- shared tagging code, first written for TagView.
// One MusicBrainz release as a lookup returns it: the album's details and the
// track list of the disc (medium) that was asked about. Like RecordingMatch
// it has no dependency on any JSON or web code, so callers can hold or show
// one without including anything else.

#ifndef TAGKIT_RELEASE_INFO_H
#define TAGKIT_RELEASE_INFO_H

#include <vector>

#include <String.h>
#include <SupportDefs.h>

#include "RecordingMatch.h"


namespace tagkit {

// One track on a disc.
struct TrackInfo {
	TrackInfo()
		:
		number(0),
		disc(0),
		durationSeconds(-1)
	{
	}

	int32	number;				// position on its disc, from 1
	int32	disc;				// which disc of the release (1 for the first),
								// 0 == unknown
	BString	title;
	BString	artist;				// the track's artist credit, or the album's
								// when the track has none of its own
	BString	artistId;			// MBID of the first credited artist, empty if
								// unknown
	BString	recordingId;		// MusicBrainz recording MBID
	int32	durationSeconds;	// -1 == unknown
};


struct ReleaseInfo {
	ReleaseInfo()
		:
		year(0),
		mediumCount(0),
		discNumber(0)
	{
	}

	BString	id;					// MusicBrainz release MBID
	BString	title;				// the album's title
	BString	artist;				// the album's artist credit ("A feat. B")
	BString	artistId;			// MBID of the first credited artist
	BString	releaseGroupId;		// MBID of the release group, empty if unknown
	BString	date;				// "1999", "1999-03" or "1999-03-14", or empty
	int32	year;				// 0 == unknown
	BString	discId;				// the disc ID that was looked up, if any

	int32	mediumCount;		// how many discs the release has
	int32	discNumber;			// which disc the tracks are from, 0 when they
								// are from several (no disc ID said which)

	std::vector<TrackInfo>	tracks;	// the tracks of that disc, in order
};


// The answer to a disc ID or release lookup.
struct ReleaseLookupResult {
	ReleaseLookupResult()
		:
		status(B_OK)
	{
	}

	// B_OK, B_ENTRY_NOT_FOUND (MusicBrainz doesn't know that ID: not a
	// failure, nobody has submitted that disc), B_TIMED_OUT, B_BUSY (too
	// many requests, even after waiting and trying again), B_BAD_VALUE (the
	// ID isn't well formed) or B_ERROR.
	status_t					status;

	// The matching releases in MusicBrainz's order: several when different
	// pressings share a table of contents.
	std::vector<ReleaseInfo>	releases;
};


// The answer to MusicBrainzSearch::LookupDisc(): the best release for a CD,
// found by its disc ID or, failing that, by artist and album.
struct DiscLookupResult {
	DiscLookupResult()
		:
		status(B_OK),
		foundByDiscId(false),
		hasTracks(false)
	{
	}

	// B_OK when a release was found; B_ENTRY_NOT_FOUND when nothing
	// matched; B_TIMED_OUT, B_BUSY, B_BAD_VALUE (nothing to look up by) or
	// B_ERROR otherwise.
	status_t					status;

	bool						foundByDiscId;	// by the disc ID itself,
								// rather than the artist and album search

	// True when release.tracks is the track list of the disc in the drive:
	// always for a disc ID match, and for a title search match only when
	// the release has a disc with as many tracks as the CD. When false
	// only the album-level details of release can be trusted.
	bool						hasTracks;

	ReleaseInfo					release;		// the best release

	// Releases whose cover art is worth offering, best first (release is
	// the first). From the disc ID match, or from the title search.
	std::vector<ReleaseRef>		candidates;
};

} // namespace tagkit

#endif // TAGKIT_RELEASE_INFO_H

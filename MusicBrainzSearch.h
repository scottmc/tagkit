/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// MusicBrainzSearch.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// A synchronous MusicBrainz recording (track) search -- by any mix of
// artist, title, album, year, track number and comment, a page at a time --
// and a release search by artist + album. Both ask MusicBrainz's web service
// for JSON (through http_get(), so they have a time limit) and read the
// answer, so other apps can reuse this instead of re-deriving the
// query/parsing boilerplate.

#ifndef TAGKIT_MUSIC_BRAINZ_SEARCH_H
#define TAGKIT_MUSIC_BRAINZ_SEARCH_H

#include <vector>

#include <String.h>

#include "RecordingMatch.h"
#include "ReleaseInfo.h"


namespace tagkit {

// What to look for in a recording search. Every text field is optional (an
// empty one is left out of the search), but at least one must be filled in.
struct RecordingQuery {
	RecordingQuery()
		:
		offset(0),
		limit(0),
		targetSeconds(-1),
		oneRowPerRelease(false)
	{
	}

	BString	artist;
	BString	title;
	BString	album;			// a release (album) the recording is on
	BString	year;			// release date: "1999", or "1999-03"
	BString	track;			// position on its disc, e.g. "7"
	BString	comment;		// MusicBrainz's disambiguation note

	int32	offset;			// how many results to skip (paging); 0 == from
							// the start
	int32	limit;			// how many to ask for, 1-100; 0 == 25

	// When >= 0, the page of results is re-sorted by how close each
	// recording's length is to this many seconds (closest first, ties
	// keeping MusicBrainz's order, recordings with no known length last).
	// Only that page is sorted, not the whole search.
	int32	targetSeconds;

	// false: one RecordingMatch per recording, with the release that
	// matches album (else the first one) filled in and every release listed
	// in .releases for cover art lookups. true: one RecordingMatch per
	// recording AND release -- the same song on three albums gives three
	// matches, each with that release's album, year and track and only that
	// release in .releases.
	bool	oneRowPerRelease;
};


// One page of a recording search.
struct RecordingSearchResult {
	RecordingSearchResult()
		:
		status(B_OK),
		totalCount(-1),
		nextOffset(0)
	{
	}

	// B_OK, or why the search failed: B_TIMED_OUT (a time limit ran out),
	// B_BUSY (MusicBrainz said "too many requests" even after waiting and
	// trying again), B_BAD_VALUE (nothing to search for) or B_ERROR.
	status_t					status;
	std::vector<RecordingMatch>	matches;

	// How many recordings MusicBrainz says match in all, or -1 if unknown.
	int32						totalCount;

	// The offset to give the next RecordingQuery to get the following page.
	int32						nextOffset;

	// True if there is probably another page after this one.
	bool						HasMore() const;
};


class MusicBrainzSearch {
public:
	// Looks up one page of recordings. Performs network I/O -- always call
	// this from a worker thread, never the UI thread. Failures are not
	// thrown: they come back in the result's status, with no matches.
	//
	// Each search is given network_timeout() seconds (NetworkTimeout.h).
	// MusicBrainz allows about one request a second; if it answers "too
	// many requests" the search waits a moment and tries once more before
	// giving up with B_BUSY.
	static RecordingSearchResult Search(const RecordingQuery& query);

	// The simple form of Search() for artist + song (+ album): the first
	// page only, cut down to maxResults matches. Returns an empty vector if
	// there were no matches or the lookup failed for any reason (network
	// error, malformed response, ...) -- a failed search just looks like
	// zero results; timedOut (if given) is set to true if a time limit ran
	// out, false for every other outcome.
	//
	// album is optional (pass an empty string to leave it out): when given
	// it's added to the query so only recordings that appear on a matching
	// release come back, and that release is the one reported as each
	// match's album/year/track (and listed first among its releases for
	// cover art). Handy when the plain artist + song search buries the
	// album you're after among live versions and compilations.
	//
	// Without a targetSeconds (< 0) matches come back in MusicBrainz's own
	// relevance order. With one, everything MusicBrainz returned is
	// re-sorted by how close its length is to targetSeconds (closest
	// first, ties keeping MusicBrainz's relevance order, recordings with
	// no known length last) before being cut down to maxResults -- the
	// closer the length, the likelier it's the same recording.
	static std::vector<RecordingMatch> SearchRecording(const BString& artist,
		const BString& song, const BString& album, int32 maxResults = 10,
		int32 targetSeconds = -1, bool* timedOut = NULL);

	// For finding cover art without a song: releases by artist, narrowed
	// to titles matching album if one is given (pass an empty string to
	// leave it out), in MusicBrainz's relevance order and with repeats of
	// the same title and year dropped. Same rules as SearchRecording():
	// network I/O on a worker thread, failures look like an empty result.
	static std::vector<ReleaseRef> SearchReleases(const BString& artist,
		const BString& album, int32 maxResults = 25, bool* timedOut = NULL);

	// Looks up the releases that have this disc ID (what libdiscid
	// computes from a CD's table of contents), with each release's album
	// details and the track list of the matching disc. For a release of
	// several discs only the one with this ID is listed (discNumber says
	// which). status is B_ENTRY_NOT_FOUND when MusicBrainz has no release
	// for that disc, which is common: try SearchReleases() with the
	// artist and album instead, then LookupRelease(). Same rules as
	// Search(): network I/O on a worker thread, failures come back in
	// status, one retry if MusicBrainz says it is busy.
	static ReleaseLookupResult LookupDiscId(const BString& discId);

	// Looks up one release by its MusicBrainz ID: album details and a
	// track list. With a discId the tracks are those of the disc with that
	// ID (when the release has it, else of its only disc if it has just
	// one); without one, or when no disc matches a release of several
	// discs, the tracks of all discs are listed in order (TrackInfo::disc
	// says which disc each is on). Use it to get the track list of a
	// release found by SearchReleases(). result.releases has one entry on
	// success.
	static ReleaseLookupResult LookupRelease(const BString& releaseId,
		const BString& discId = BString());
};

} // namespace tagkit

#endif // TAGKIT_MUSIC_BRAINZ_SEARCH_H

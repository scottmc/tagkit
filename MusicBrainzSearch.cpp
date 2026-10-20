/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "MusicBrainzSearch.h"
#include "HttpFetch.h"
#include "JsonValue.h"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include <Debug.h>
#include <OS.h>



namespace tagkit {

namespace {

// How many of a recording's releases to keep for cover art lookup. Popular
// recordings can appear on dozens of compilations; the first few (MusicBrainz
// lists the most relevant first) are plenty to choose between.
const int32 kMaxReleasesPerMatch = 15;

// How many recordings to ask MusicBrainz for when the caller doesn't say
// (its own default), and the most it allows in one request.
const int32 kRecordingSearchLimit = 25;
const int32 kMaxRecordingSearchLimit = 100;

// How long to wait before trying again when MusicBrainz says "too many
// requests" (it allows about one a second).
const bigtime_t kBusyRetryDelay = 1500000;

// How many releases to ask MusicBrainz for.
const int32 kReleaseSearchLimit = 50;


BString
EscapeLuceneQuoted(const BString& text)
{
	BString result(text);
	result.ReplaceAll("\\", "\\\\");
	result.ReplaceAll("\"", "\\\"");
	return result;
}


// Orders matches by how far their length is from a target, closest first.
// Recordings with no known length sort after every one that has one.
struct CloserToTarget {
	CloserToTarget(int32 target) : fTarget(target) {}

	bool operator()(const RecordingMatch& a, const RecordingMatch& b) const
	{
		if (a.durationSeconds < 0 || b.durationSeconds < 0)
			return a.durationSeconds >= 0 && b.durationSeconds < 0;
		return abs(a.durationSeconds - fTarget)
			< abs(b.durationSeconds - fTarget);
	}

private:
	int32	fTarget;
};


// Joins an "artist-credit" array (each entry has a "name" and the
// "joinphrase" that goes after it) into the single display string
// MusicBrainz itself uses for an artist credit (handles multi-artist
// collaborations, e.g. "Artist One feat. Artist Two").
BString
JoinArtistCredit(const JsonValue& credit)
{
	BString result;
	for (int32 i = 0; i < credit.Count(); i++) {
		result << credit.At(i).Get("name").String();
		result << credit.At(i).Get("joinphrase").String();
	}
	return result;
}


// "YYYY", "YYYY-MM" or "YYYY-MM-DD" -- or empty when MusicBrainz doesn't
// know the date. Returns the year, 0 if unknown.
int32
YearOf(const JsonValue& date)
{
	const BString& text = date.String();
	return text.Length() >= 4 ? atoi(text.String()) : 0;
}


// Adds field:"value" to a Lucene query, joined to what is there by AND;
// nothing is added for an empty value.
void
AddTerm(BString& query, const char* field, const BString& value)
{
	if (value.Length() == 0)
		return;
	if (query.Length() > 0)
		query << " AND ";
	query << field << ":\"" << EscapeLuceneQuoted(value) << "\"";
}


// Downloads a MusicBrainz web service search (type is "recording" or
// "release") and reads the JSON answer. Returns B_OK with the parsed answer
// in answer; B_TIMED_OUT if a time limit ran out; B_BUSY if MusicBrainz
// kept saying "too many requests"; another error otherwise.
status_t
RunSearch(const char* type, const BString& luceneQuery, int32 limit,
	int32 offset, JsonValue& answer)
{
	BString url("https://musicbrainz.org/ws/2/");
	url << type << "?fmt=json&limit=" << limit << "&offset=" << offset
		<< "&query=" << url_encode(luceneQuery);

	PRINT(("MusicBrainzSearch: %s\n", luceneQuery.String()));

	std::vector<unsigned char> response;
	BString error;
	status_t status = http_get(url, response, &error);
	if (status == B_BUSY) {
		// Asked too soon after the last request: wait a moment, try once
		// more.
		fprintf(stderr, "TagView: MusicBrainz is busy; trying again\n");
		snooze(kBusyRetryDelay);
		error = "";
		status = http_get(url, response, &error);
	}
	if (status != B_OK) {
		fprintf(stderr, "TagView: MusicBrainz %s search failed: %s\n", type,
			error.String());
		return status;
	}

	if (response.empty()
			|| !JsonValue::Parse((const char*)&response[0], response.size(),
				answer)) {
		fprintf(stderr, "TagView: MusicBrainz %s search: unreadable "
			"answer\n", type);
		return B_BAD_DATA;
	}

	return B_OK;
}


// Where the recording sits on a release: the first track of the first disc
// that lists it, 0 if the answer didn't carry a track list.
int32
TrackPosition(const JsonValue& release)
{
	const JsonValue& media = release.Get("media");
	for (int32 m = 0; m < media.Count(); m++) {
		const JsonValue& medium = media.At(m);
		const JsonValue& tracks = medium.Get("track");
		if (tracks.Count() == 0)
			continue;

		// The track's own number is a string ("3", or "A1" on vinyl); when
		// it isn't a plain number, count from where the answer's list of
		// tracks starts on the disc.
		int32 position = atoi(tracks.At(0).Get("number").String().String());
		if (position <= 0 && medium.Get("track-offset").IsNumber())
			position = (int32)medium.Get("track-offset").Number() + 1;
		return position > 0 ? position : 0;
	}
	return 0;
}


ReleaseRef
ReleaseRefOf(const JsonValue& release)
{
	ReleaseRef ref;
	ref.id = release.Get("id").String();
	ref.title = release.Get("title").String();
	ref.year = YearOf(release.Get("date"));
	return ref;
}

} // namespace


bool
RecordingSearchResult::HasMore() const
{
	return status == B_OK && !matches.empty() && totalCount >= 0
		&& nextOffset < totalCount;
}


/*static*/ RecordingSearchResult
MusicBrainzSearch::Search(const RecordingQuery& query)
{
	RecordingSearchResult result;

	BString luceneQuery;
	AddTerm(luceneQuery, "artist", query.artist);
	AddTerm(luceneQuery, "release", query.album);
	AddTerm(luceneQuery, "recording", query.title);
	AddTerm(luceneQuery, "date", query.year);
	AddTerm(luceneQuery, "comment", query.comment);
	AddTerm(luceneQuery, "tnum", query.track);
	if (luceneQuery.Length() == 0) {
		result.status = B_BAD_VALUE;
		return result;
	}

	int32 offset = query.offset > 0 ? query.offset : 0;
	int32 limit = query.limit > 0 ? query.limit : kRecordingSearchLimit;
	if (limit > kMaxRecordingSearchLimit)
		limit = kMaxRecordingSearchLimit;
	result.nextOffset = offset;

	JsonValue answer;
	result.status = RunSearch("recording", luceneQuery, limit, offset, answer);
	if (result.status != B_OK)
		return result;

	const JsonValue& recordings = answer.Get("recordings");
	result.totalCount = answer.Get("count").IsNumber()
		? (int32)answer.Get("count").Number() : -1;
	result.nextOffset = offset + recordings.Count();
	if (recordings.Count() == 0)
		PRINT(("MusicBrainzSearch: no recordings found\n"));

	for (int32 i = 0; i < recordings.Count(); i++) {
		const JsonValue& recording = recordings.At(i);

		RecordingMatch match;
		match.id = recording.Get("id").String();
		match.title = recording.Get("title").String();
		match.artist = JoinArtistCredit(recording.Get("artist-credit"));
		match.disambiguation = recording.Get("disambiguation").String();

		// "length" is in milliseconds.
		double length = recording.Get("length").Number();
		match.durationSeconds = length > 0 ? (int32)(length / 1000) : -1;

		// MusicBrainz's relevance score, 0-100. The results come back
		// in this order anyway.
		match.score = recording.Get("score").IsNumber()
			? (int32)recording.Get("score").Number() : -1;

		const JsonValue& releases = recording.Get("releases");

		if (query.oneRowPerRelease) {
			if (releases.Count() == 0)
				result.matches.push_back(match);

			for (int32 r = 0; r < releases.Count(); r++) {
				const JsonValue& release = releases.At(r);
				RecordingMatch row = match;
				row.releases.push_back(ReleaseRefOf(release));
				row.album = release.Get("title").String();
				row.year = YearOf(release.Get("date"));
				row.track = TrackPosition(release);
				result.matches.push_back(row);
			}
			continue;
		}

		if (releases.Count() > 0) {
			// With an album given, the release whose title contains it
			// is the one to report (and to try for cover art first);
			// otherwise it's MusicBrainz's first release.
			int32 chosen = 0;
			if (query.album.Length() > 0) {
				for (int32 r = 0; r < releases.Count(); r++) {
					BString eachTitle = releases.At(r).Get("title").String();
					if (eachTitle.IFindFirst(query.album) >= 0) {
						chosen = r;
						break;
					}
				}
			}

			// Remember every release (capped) for cover art lookup, the
			// chosen one first.
			for (int32 n = 0; n < releases.Count()
					&& (int32)match.releases.size() < kMaxReleasesPerMatch;
					n++) {
				int32 r = n == 0 ? chosen : (n <= chosen ? n - 1 : n);
				match.releases.push_back(ReleaseRefOf(releases.At(r)));
			}

			const JsonValue& release = releases.At(chosen);
			match.album = release.Get("title").String();
			match.year = YearOf(release.Get("date"));
			match.track = TrackPosition(release);
		}

		result.matches.push_back(match);
	}

	if (query.targetSeconds >= 0) {
		// stable_sort so equally-close matches keep MusicBrainz's
		// relevance order.
		std::stable_sort(result.matches.begin(), result.matches.end(),
			CloserToTarget(query.targetSeconds));
	}

	return result;
}


std::vector<RecordingMatch>
MusicBrainzSearch::SearchRecording(const BString& artist, const BString& song,
	const BString& album, int32 maxResults, int32 targetSeconds,
	bool* timedOut)
{
	if (timedOut != NULL)
		*timedOut = false;

	if (artist.Length() == 0 || song.Length() == 0)
		return std::vector<RecordingMatch>();

	RecordingQuery query;
	query.artist = artist;
	query.title = song;
	query.album = album;
	query.targetSeconds = targetSeconds;

	RecordingSearchResult result = Search(query);
	if (timedOut != NULL)
		*timedOut = result.status == B_TIMED_OUT;

	if ((int32)result.matches.size() > maxResults)
		result.matches.resize(maxResults);
	return result.matches;
}


std::vector<ReleaseRef>
MusicBrainzSearch::SearchReleases(const BString& artist, const BString& album,
	int32 maxResults, bool* timedOut)
{
	std::vector<ReleaseRef> releases;
	if (timedOut != NULL)
		*timedOut = false;

	if (artist.Length() == 0)
		return releases;

	BString luceneQuery;
	AddTerm(luceneQuery, "artist", artist);
	AddTerm(luceneQuery, "release", album);

	JsonValue answer;
	status_t status = RunSearch("release", luceneQuery, kReleaseSearchLimit,
		0, answer);
	if (status != B_OK) {
		if (status == B_TIMED_OUT && timedOut != NULL)
			*timedOut = true;
		return releases;
	}

	const JsonValue& list = answer.Get("releases");
	for (int32 i = 0; i < list.Count()
			&& (int32)releases.size() < maxResults; i++) {
		ReleaseRef ref = ReleaseRefOf(list.At(i));

		// Reissues of one album share a cover; keep the first.
		bool repeat = false;
		for (size_t r = 0; r < releases.size(); r++) {
			if (releases[r].year == ref.year
					&& releases[r].title.ICompare(ref.title) == 0) {
				repeat = true;
				break;
			}
		}
		if (!repeat)
			releases.push_back(ref);
	}

	return releases;
}

} // namespace tagkit

/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// TagRecord.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// TagRecord is a plain data holder for the tag fields we know how to show
// and edit. It intentionally has no dependency on any particular tagging
// library (TagLib, MusicBrainz, etc.) so it can be passed around freely
// and reused by other apps (programs like Hare or ArmyKnife could).

#ifndef TAGKIT_TAG_RECORD_H
#define TAGKIT_TAG_RECORD_H

#include <memory>

#include <String.h>
#include <SupportDefs.h>

#include "CoverArtImage.h"


namespace tagkit {

enum audio_format {
	AUDIO_FORMAT_UNKNOWN = 0,
	AUDIO_FORMAT_MP3,
	AUDIO_FORMAT_OGG,
	AUDIO_FORMAT_FLAC,
	AUDIO_FORMAT_OPUS,
	AUDIO_FORMAT_SPEEX,
	AUDIO_FORMAT_MP4,		// .m4a, .m4b, .mp4, .aac (AAC / ALAC)
	AUDIO_FORMAT_WAV,
	AUDIO_FORMAT_AIFF,
	AUDIO_FORMAT_WAVPACK,
	AUDIO_FORMAT_MUSEPACK,
	AUDIO_FORMAT_APE,		// Monkey's Audio
	AUDIO_FORMAT_ASF,		// .wma
	AUDIO_FORMAT_TTA		// True Audio
};


// Returns the audio_format implied by a file's extension (".mp3", ".ogg",
// ".flac", ".opus", ".m4a" ... case-insensitive). Returns
// AUDIO_FORMAT_UNKNOWN for anything else.
audio_format format_from_extension(const BString& path);

// Returns the audio_format for a MIME type such as "audio/mpeg" or
// "audio/x-flac" (case-insensitive; any ";parameters" are ignored). Returns
// AUDIO_FORMAT_UNKNOWN for a type that isn't one of the supported audio
// types, including NULL and "".
audio_format format_from_mime_type(const char* mimeType);

// The best guess from a file's MIME type and its name: the MIME type wins,
// and the extension decides when the type is missing or unhelpful (such as
// "application/octet-stream") or only says "Ogg" for a file that is really
// Opus or Speex. Takes the type as a string so a Tracker file panel filter,
// which is handed the type, doesn't need to look it up again.
audio_format format_from_type_and_name(const char* mimeType,
	const BString& fileName);

// Looks up the file's MIME type from its attributes (asking the system to
// work it out if the file has none yet) and combines it with the file name
// as format_from_type_and_name() does. Returns AUDIO_FORMAT_UNKNOWN if the
// file can't be opened.
audio_format format_from_file(const BString& path);

// Human readable label for an audio_format ("MP3", "Ogg Vorbis", "FLAC",
// "Opus", ... "Unknown").
const char* format_label(audio_format format);


struct TagRecord {
	TagRecord();

	BString		path;			// full path of the source file
	BString		fileName;		// leaf name, for display
	audio_format	format;

	BString		artist;
	BString		title;
	BString		album;
	BString		genre;
	BString		comment;

	int32		track;			// 0 == unknown
	int32		year;			// 0 == unknown
	int32		durationSeconds;	// -1 == unknown
	int32		bitRateKbps;		// -1 == unknown
	int32		sampleRateHz;		// -1 == unknown
	int32		channels;		// -1 == unknown

	bool		tagsLoaded;		// true once a reader has filled this in
	bool		modified;		// tag fields changed in memory (e.g. a
								// MusicBrainz match was applied) but not
								// yet written to the file
	bool		hasCoverArt;		// the file already has embedded art

	// Cover art chosen to be written into the file on the next save, or
	// NULL for "leave the file's art alone". Shared (not copied) between
	// the copies of a record, since the image can be hundreds of KB.
	std::shared_ptr<const CoverArtImage>	newCoverArt;

	// True when the user removed the file's cover art; the next save then
	// deletes the embedded pictures. Never set together with newCoverArt.
	bool		removeCoverArt;

	// True when neither artist nor title could be read from the file's
	// tags. This is the trigger for offering a MusicBrainz lookup based
	// on the file name instead.
	bool IsUntagged() const;
};


// Best-effort split of a music file's leaf name into a plausible artist
// and song title, e.g. "Artist - Song.mp3" -> ("Artist", "Song"), or
// "03 - Artist - Song.mp3" (leading track number stripped first) ->
// ("Artist", "Song"). Meant as a starting point for a MusicBrainz search
// on an untagged file, not a reliable parse -- returns false (leaving
// artist/song untouched) if no usable separator was found at all.
bool guess_artist_song_from_file_name(const BString& fileName,
	BString& artist, BString& song);

} // namespace tagkit

#endif // TAGKIT_TAG_RECORD_H

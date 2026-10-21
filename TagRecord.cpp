/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagRecord.h"

#include <ctype.h>
#include <string.h>

#include <Mime.h>
#include <Node.h>
#include <NodeInfo.h>


namespace tagkit {


static bool
ends_with_ci(const BString& str, const char* suffix)
{
	int32 suffixLen = (int32)strlen(suffix);
	int32 strLen = str.Length();
	if (strLen < suffixLen)
		return false;

	BString tail;
	str.CopyInto(tail, strLen - suffixLen, suffixLen);
	tail.ToLower();
	return tail == suffix;
}


namespace {

struct FormatEntry {
	const char*		extension;
	audio_format	format;
};

const FormatEntry kExtensions[] = {
	{ ".mp3", AUDIO_FORMAT_MP3 },
	{ ".ogg", AUDIO_FORMAT_OGG },
	{ ".oga", AUDIO_FORMAT_OGG },
	{ ".flac", AUDIO_FORMAT_FLAC },
	{ ".opus", AUDIO_FORMAT_OPUS },
	{ ".spx", AUDIO_FORMAT_SPEEX },
	{ ".m4a", AUDIO_FORMAT_MP4 },
	{ ".m4b", AUDIO_FORMAT_MP4 },
	{ ".mp4", AUDIO_FORMAT_MP4 },
	{ ".aac", AUDIO_FORMAT_MP4 },
	{ ".wav", AUDIO_FORMAT_WAV },
	{ ".aif", AUDIO_FORMAT_AIFF },
	{ ".aiff", AUDIO_FORMAT_AIFF },
	{ ".wv", AUDIO_FORMAT_WAVPACK },
	{ ".mpc", AUDIO_FORMAT_MUSEPACK },
	{ ".ape", AUDIO_FORMAT_APE },
	{ ".wma", AUDIO_FORMAT_ASF },
	{ ".tta", AUDIO_FORMAT_TTA }
};

// Haiku's own type names and the older / alternative spellings seen in
// the wild. The Ogg-based ones come first in the lookup order so
// "audio/x-opus+ogg" is Opus, not just Ogg.
const FormatEntry kMimeTypes[] = {
	{ "audio/x-opus+ogg", AUDIO_FORMAT_OPUS },
	{ "audio/opus", AUDIO_FORMAT_OPUS },
	{ "audio/x-opus", AUDIO_FORMAT_OPUS },
	{ "audio/x-speex+ogg", AUDIO_FORMAT_SPEEX },
	{ "audio/x-speex", AUDIO_FORMAT_SPEEX },
	{ "audio/speex", AUDIO_FORMAT_SPEEX },
	{ "audio/x-vorbis+ogg", AUDIO_FORMAT_OGG },
	{ "audio/ogg", AUDIO_FORMAT_OGG },
	{ "audio/x-ogg", AUDIO_FORMAT_OGG },
	{ "audio/vorbis", AUDIO_FORMAT_OGG },
	{ "audio/x-vorbis", AUDIO_FORMAT_OGG },
	{ "application/ogg", AUDIO_FORMAT_OGG },
	{ "application/x-ogg", AUDIO_FORMAT_OGG },
	{ "application/vorbis", AUDIO_FORMAT_OGG },
	{ "application/x-vorbis", AUDIO_FORMAT_OGG },
	{ "audio/mpeg", AUDIO_FORMAT_MP3 },
	{ "audio/x-mpeg", AUDIO_FORMAT_MP3 },
	{ "audio/mp3", AUDIO_FORMAT_MP3 },
	{ "audio/x-mp3", AUDIO_FORMAT_MP3 },
	{ "audio/mpg", AUDIO_FORMAT_MP3 },
	{ "audio/x-mpg", AUDIO_FORMAT_MP3 },
	{ "audio/flac", AUDIO_FORMAT_FLAC },
	{ "audio/x-flac", AUDIO_FORMAT_FLAC },
	{ "application/flac", AUDIO_FORMAT_FLAC },
	{ "application/x-flac", AUDIO_FORMAT_FLAC },
	{ "audio/mp4", AUDIO_FORMAT_MP4 },
	{ "audio/x-m4a", AUDIO_FORMAT_MP4 },
	{ "audio/m4a", AUDIO_FORMAT_MP4 },
	{ "audio/aac", AUDIO_FORMAT_MP4 },
	{ "audio/x-aac", AUDIO_FORMAT_MP4 },
	{ "audio/x-wav", AUDIO_FORMAT_WAV },
	{ "audio/wav", AUDIO_FORMAT_WAV },
	{ "audio/wave", AUDIO_FORMAT_WAV },
	{ "audio/vnd.wave", AUDIO_FORMAT_WAV },
	{ "audio/x-aiff", AUDIO_FORMAT_AIFF },
	{ "audio/aiff", AUDIO_FORMAT_AIFF },
	{ "audio/x-wavpack", AUDIO_FORMAT_WAVPACK },
	{ "audio/x-musepack", AUDIO_FORMAT_MUSEPACK },
	{ "audio/musepack", AUDIO_FORMAT_MUSEPACK },
	{ "audio/x-ape", AUDIO_FORMAT_APE },
	{ "audio/x-monkeys-audio", AUDIO_FORMAT_APE },
	{ "audio/x-ms-wma", AUDIO_FORMAT_ASF },
	{ "audio/x-tta", AUDIO_FORMAT_TTA }
};

}	// namespace


audio_format
format_from_extension(const BString& path)
{
	for (size_t i = 0; i < sizeof(kExtensions) / sizeof(kExtensions[0]); i++) {
		if (ends_with_ci(path, kExtensions[i].extension))
			return kExtensions[i].format;
	}
	return AUDIO_FORMAT_UNKNOWN;
}


audio_format
format_from_mime_type(const char* mimeType)
{
	if (mimeType == NULL || mimeType[0] == '\0')
		return AUDIO_FORMAT_UNKNOWN;

	// Lower-case, and drop anything from a ';' on ("audio/ogg; codecs=...").
	BString type(mimeType);
	int32 semicolon = type.FindFirst(';');
	if (semicolon >= 0)
		type.Truncate(semicolon);
	type.Trim();
	type.ToLower();

	for (size_t i = 0; i < sizeof(kMimeTypes) / sizeof(kMimeTypes[0]); i++) {
		if (type == kMimeTypes[i].extension)
			return kMimeTypes[i].format;
	}
	return AUDIO_FORMAT_UNKNOWN;
}


audio_format
format_from_type_and_name(const char* mimeType, const BString& fileName)
{
	audio_format byType = format_from_mime_type(mimeType);
	audio_format byName = format_from_extension(fileName);

	if (byType == AUDIO_FORMAT_UNKNOWN)
		return byName;

	// "Ogg" is only a container: the name can say it holds Opus or Speex.
	if (byType == AUDIO_FORMAT_OGG && (byName == AUDIO_FORMAT_OPUS
			|| byName == AUDIO_FORMAT_SPEEX)) {
		return byName;
	}
	return byType;
}


audio_format
format_from_file(const BString& path)
{
	BNode node(path.String());
	if (node.InitCheck() != B_OK)
		return AUDIO_FORMAT_UNKNOWN;

	BNodeInfo info(&node);
	char type[B_MIME_TYPE_LENGTH];
	type[0] = '\0';
	if (info.GetType(type) != B_OK) {
		// No type yet (a file just copied in, say): have the system work it
		// out from the name and contents, then ask again.
		type[0] = '\0';
		update_mime_info(path.String(), false, true, false);
		if (info.GetType(type) != B_OK)
			type[0] = '\0';
	}

	return format_from_type_and_name(type, path);
}


const char*
format_label(audio_format format)
{
	switch (format) {
		case AUDIO_FORMAT_MP3:
			return "MP3";
		case AUDIO_FORMAT_OGG:
			return "Ogg Vorbis";
		case AUDIO_FORMAT_FLAC:
			return "FLAC";
		case AUDIO_FORMAT_OPUS:
			return "Opus";
		case AUDIO_FORMAT_SPEEX:
			return "Speex";
		case AUDIO_FORMAT_MP4:
			return "MP4 / AAC";
		case AUDIO_FORMAT_WAV:
			return "WAV";
		case AUDIO_FORMAT_AIFF:
			return "AIFF";
		case AUDIO_FORMAT_WAVPACK:
			return "WavPack";
		case AUDIO_FORMAT_MUSEPACK:
			return "Musepack";
		case AUDIO_FORMAT_APE:
			return "Monkey's Audio";
		case AUDIO_FORMAT_ASF:
			return "WMA";
		case AUDIO_FORMAT_TTA:
			return "True Audio";
		default:
			return "Unknown";
	}
}


bool
MusicBrainzIds::IsEmpty() const
{
	return discId.IsEmpty() && releaseId.IsEmpty()
		&& releaseGroupId.IsEmpty() && artistId.IsEmpty()
		&& recordingId.IsEmpty();
}


TagRecord::TagRecord()
	:
	format(AUDIO_FORMAT_UNKNOWN),
	track(0),
	year(0),
	durationSeconds(-1),
	bitRateKbps(-1),
	sampleRateHz(-1),
	channels(-1),
	tagsLoaded(false),
	modified(false),
	hasCoverArt(false),
	writable(true),
	removeCoverArt(false)
{
}


bool
TagRecord::IsUntagged() const
{
	return artist.IsEmpty() && title.IsEmpty();
}


bool
guess_artist_song_from_file_name(const BString& fileName, BString& artist,
	BString& song)
{
	BString name(fileName);

	// drop the extension
	int32 dot = name.FindLast('.');
	if (dot > 0)
		name.Truncate(dot);

	// drop a leading track number ("03 ", "03.", "03 - ", "03_", ...)
	int32 i = 0;
	while (i < name.Length() && isdigit((unsigned char)name[i]))
		i++;
	if (i > 0 && i < name.Length()) {
		int32 j = i;
		while (j < name.Length()
				&& (name[j] == ' ' || name[j] == '.' || name[j] == '-'
					|| name[j] == '_')) {
			j++;
		}
		if (j > i)
			name.Remove(0, j);
	}

	// try common artist/song separators, most specific first
	static const char* kSeparators[] = { " - ", " \xE2\x80\x93 " /* en dash */,
		" \xE2\x80\x94 " /* em dash */, "-", NULL };

	for (int32 s = 0; kSeparators[s] != NULL; s++) {
		int32 pos = name.FindFirst(kSeparators[s]);
		if (pos <= 0)
			continue;

		name.CopyInto(artist, 0, pos);
		song = name;
		song.Remove(0, pos + (int32)strlen(kSeparators[s]));

		artist.Trim();
		song.Trim();
		artist.ReplaceAll('_', ' ');
		song.ReplaceAll('_', ' ');

		if (artist.Length() > 0 && song.Length() > 0)
			return true;
	}

	return false;
}


} // namespace tagkit

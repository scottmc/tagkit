/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagAttributes.h"

#include <stdlib.h>
#include <string.h>

#include <fs_attr.h>

#include <InterfaceDefs.h>
#include <Message.h>
#include <Mime.h>
#include <MimeType.h>
#include <Node.h>
#include <NodeInfo.h>
#include <TypeConstants.h>

#include <taglib/fileref.h>
#include <taglib/tfile.h>
#include <taglib/tpropertymap.h>
#include <taglib/tstring.h>
#include <taglib/tstringlist.h>


namespace tagkit {

namespace {

enum source {
	SOURCE_ARTIST,
	SOURCE_ALBUM,
	SOURCE_TITLE,
	SOURCE_TRACK,
	SOURCE_YEAR,
	SOURCE_GENRE,
	SOURCE_COMMENT,
	SOURCE_DISC_ID,
	SOURCE_RELEASE_ID,
	SOURCE_RELEASE_GROUP_ID,
	SOURCE_ARTIST_ID,
	SOURCE_RECORDING_ID
};


struct AttributeSpec {
	const char*	mediaName;	// the name with ATTRIBUTE_NAMES_MEDIA
	const char*	audioName;	// the name with ATTRIBUTE_NAMES_AUDIO
	const char*	publicName;	// the column title Tracker shows
	type_code	type;		// B_STRING_TYPE or B_INT32_TYPE
	source		from;
	bool		musicBrainz;	// only with AttributeOptions::musicBrainzIds
};


// Haiku's names (as ArmyKnife uses them on Haiku), and Hare's.
const AttributeSpec kAttributes[] = {
	{ "Audio:Artist", "Audio:Artist", "Artist", B_STRING_TYPE,
		SOURCE_ARTIST, false },
	{ "Audio:Album", "Audio:Album", "Album", B_STRING_TYPE,
		SOURCE_ALBUM, false },
	{ "Media:Title", "Audio:Title", "Title", B_STRING_TYPE,
		SOURCE_TITLE, false },
	{ "Audio:Track", "Audio:Track", "Track", B_INT32_TYPE,
		SOURCE_TRACK, false },
	{ "Media:Year", "Audio:Year", "Year", B_INT32_TYPE,
		SOURCE_YEAR, false },
	{ "Media:Genre", "Audio:Genre", "Genre", B_STRING_TYPE,
		SOURCE_GENRE, false },
	{ "Media:Comment", "Audio:Comment", "Comment", B_STRING_TYPE,
		SOURCE_COMMENT, false },

	// The MusicBrainz ids use MusicBrainz Picard's tag names, under the
	// "Audio:" prefix.
	{ "Audio:MUSICBRAINZ_DISCID", "Audio:MUSICBRAINZ_DISCID",
		"MusicBrainz Disc ID", B_STRING_TYPE, SOURCE_DISC_ID, true },
	{ "Audio:MUSICBRAINZ_ALBUMID", "Audio:MUSICBRAINZ_ALBUMID",
		"MusicBrainz Album ID", B_STRING_TYPE, SOURCE_RELEASE_ID, true },
	{ "Audio:MUSICBRAINZ_RELEASEGROUPID", "Audio:MUSICBRAINZ_RELEASEGROUPID",
		"MusicBrainz Release Group ID", B_STRING_TYPE,
		SOURCE_RELEASE_GROUP_ID, true },
	{ "Audio:MUSICBRAINZ_ARTISTID", "Audio:MUSICBRAINZ_ARTISTID",
		"MusicBrainz Artist ID", B_STRING_TYPE, SOURCE_ARTIST_ID, true },
	{ "Audio:MUSICBRAINZ_TRACKID", "Audio:MUSICBRAINZ_TRACKID",
		"MusicBrainz Track ID", B_STRING_TYPE, SOURCE_RECORDING_ID, true }
};

const int32 kAttributeCount = sizeof(kAttributes) / sizeof(kAttributes[0]);


const char*
name_of(const AttributeSpec& spec, const AttributeOptions& options)
{
	return options.naming == ATTRIBUTE_NAMES_AUDIO
		? spec.audioName : spec.mediaName;
}


// One id from the file's own tags, or "" if it has none.
BString
file_id(const TagLib::PropertyMap& properties, const char* key)
{
	TagLib::PropertyMap::ConstIterator it
		= properties.find(TagLib::String(key));
	if (it == properties.end() || it->second.isEmpty())
		return BString();
	return BString(it->second.front().to8Bit(true).c_str());
}


// The record's MusicBrainz ids, with any it lacks filled in from the
// file's tags (only when options ask for the ids at all).
MusicBrainzIds
ids_for(const TagRecord& record, const AttributeOptions& options)
{
	MusicBrainzIds ids = record.musicBrainz;
	if (!options.musicBrainzIds || record.path.Length() == 0
			|| (!ids.discId.IsEmpty() && !ids.releaseId.IsEmpty()
				&& !ids.releaseGroupId.IsEmpty()
				&& !ids.artistId.IsEmpty() && !ids.recordingId.IsEmpty())) {
		return ids;
	}

	TagLib::FileRef file(record.path.String());
	if (file.isNull() || file.file() == NULL)
		return ids;

	TagLib::PropertyMap properties = file.file()->properties();
	if (ids.discId.IsEmpty())
		ids.discId = file_id(properties, "MUSICBRAINZ_DISCID");
	if (ids.releaseId.IsEmpty())
		ids.releaseId = file_id(properties, "MUSICBRAINZ_ALBUMID");
	if (ids.releaseGroupId.IsEmpty()) {
		ids.releaseGroupId
			= file_id(properties, "MUSICBRAINZ_RELEASEGROUPID");
	}
	if (ids.artistId.IsEmpty())
		ids.artistId = file_id(properties, "MUSICBRAINZ_ARTISTID");
	if (ids.recordingId.IsEmpty())
		ids.recordingId = file_id(properties, "MUSICBRAINZ_TRACKID");
	return ids;
}


BString
text_for(const TagRecord& record, const MusicBrainzIds& ids, source from)
{
	switch (from) {
		case SOURCE_ARTIST:		return record.artist;
		case SOURCE_ALBUM:		return record.album;
		case SOURCE_TITLE:		return record.title;
		case SOURCE_GENRE:		return record.genre;
		case SOURCE_COMMENT:	return record.comment;
		case SOURCE_DISC_ID:	return ids.discId;
		case SOURCE_RELEASE_ID:	return ids.releaseId;
		case SOURCE_RELEASE_GROUP_ID:	return ids.releaseGroupId;
		case SOURCE_ARTIST_ID:	return ids.artistId;
		case SOURCE_RECORDING_ID:	return ids.recordingId;
		default:				return BString();
	}
}


int32
number_for(const TagRecord& record, source from)
{
	return from == SOURCE_TRACK ? record.track : record.year;
}


// True if the attribute is already on the node with exactly this value
// (and type).
bool
attribute_matches(BNode& node, const char* name, const AttributeSpec& spec,
	const BString& text, int32 number)
{
	attr_info info;
	if (node.GetAttrInfo(name, &info) != B_OK || info.type != spec.type)
		return false;

	if (spec.type == B_INT32_TYPE) {
		int32 current = 0;
		if (info.size != (off_t)sizeof(current)
				|| node.ReadAttr(name, spec.type, 0, &current,
					sizeof(current)) != (ssize_t)sizeof(current)) {
			return false;
		}
		return current == number;
	}

	// Strings are stored with their terminating NUL.
	if (info.size != (off_t)text.Length() + 1)
		return false;
	BString current;
	char* buffer = current.LockBuffer(info.size);
	if (buffer == NULL)
		return false;
	ssize_t got = node.ReadAttr(name, spec.type, 0, buffer, info.size);
	current.UnlockBuffer(got > 0 ? (int32)got - 1 : 0);
	return got == (ssize_t)info.size && current == text;
}


// Whether the record has something to store for this attribute.
bool
has_value(const AttributeSpec& spec, const BString& text, int32 number)
{
	return spec.type == B_INT32_TYPE ? number > 0 : text.Length() > 0;
}


// Lists the attribute in the attribute info of the file's MIME type (once),
// which is what makes Tracker offer it in its Attributes menu. Best effort:
// a file without a type, or a type whose attribute info can't be read or
// changed, simply isn't listed.
void
register_attribute(BNode& node, const char* name, const AttributeSpec& spec)
{
	BNodeInfo nodeInfo(&node);
	char type[B_MIME_TYPE_LENGTH];
	if (nodeInfo.InitCheck() != B_OK || nodeInfo.GetType(type) != B_OK
			|| type[0] == '\0') {
		return;
	}

	BMimeType mimeType(type);
	BMessage attributes;
	if (mimeType.InitCheck() != B_OK
			|| mimeType.GetAttrInfo(&attributes) != B_OK) {
		return;
	}

	const char* listed;
	for (int32 i = 0; attributes.FindString("attr:name", i, &listed) == B_OK;
			i++) {
		if (strcmp(listed, name) == 0)
			return;
	}

	attributes.AddString("attr:name", name);
	attributes.AddString("attr:public_name", spec.publicName);
	attributes.AddInt32("attr:type", (int32)spec.type);
	attributes.AddBool("attr:public", true);
	attributes.AddBool("attr:viewable", true);
	attributes.AddBool("attr:editable", true);
	attributes.AddInt32("attr:width", 30);
	attributes.AddInt32("attr:alignment", (int32)B_ALIGN_LEFT);
	attributes.AddBool("attr:extra", false);
	mimeType.SetAttrInfo(&attributes);
}

} // namespace


AttributeOptions::AttributeOptions()
	:
	naming(ATTRIBUTE_NAMES_MEDIA),
	musicBrainzIds(false)
{
}


bool
tag_attributes_need_update(const TagRecord& record,
	const AttributeOptions& options)
{
	if (record.path.Length() == 0)
		return false;

	BNode node(record.path.String());
	if (node.InitCheck() != B_OK)
		return false;

	MusicBrainzIds ids = ids_for(record, options);
	for (int32 i = 0; i < kAttributeCount; i++) {
		const AttributeSpec& spec = kAttributes[i];
		if (spec.musicBrainz && !options.musicBrainzIds)
			continue;
		BString text = text_for(record, ids, spec.from);
		int32 number = number_for(record, spec.from);
		if (has_value(spec, text, number)
				&& !attribute_matches(node, name_of(spec, options), spec,
					text, number)) {
			return true;
		}
	}
	return false;
}


status_t
copy_tags_to_attributes(const TagRecord& record, int32* written,
	const AttributeOptions& options)
{
	if (written != NULL)
		*written = 0;

	BNode node(record.path.String());
	status_t status = node.InitCheck();
	if (status != B_OK)
		return status;

	MusicBrainzIds ids = ids_for(record, options);
	for (int32 i = 0; i < kAttributeCount; i++) {
		const AttributeSpec& spec = kAttributes[i];
		if (spec.musicBrainz && !options.musicBrainzIds)
			continue;
		const char* name = name_of(spec, options);
		BString text = text_for(record, ids, spec.from);
		int32 number = number_for(record, spec.from);
		if (!has_value(spec, text, number)
				|| attribute_matches(node, name, spec, text, number)) {
			continue;
		}

		// Replace rather than overwrite, so a shorter value (or one that
		// was stored as another type) doesn't leave old data behind.
		node.RemoveAttr(name);

		ssize_t result;
		ssize_t expected;
		if (spec.type == B_INT32_TYPE) {
			expected = sizeof(number);
			result = node.WriteAttr(name, spec.type, 0, &number,
				expected);
		} else {
			expected = text.Length() + 1;
			result = node.WriteAttr(name, spec.type, 0, text.String(),
				expected);
		}
		if (result != expected)
			return result < 0 ? (status_t)result : B_IO_ERROR;

		register_attribute(node, name, spec);

		if (written != NULL)
			(*written)++;
	}
	return B_OK;
}

} // namespace tagkit

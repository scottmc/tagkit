/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagField.h"

#include <stdlib.h>


namespace tagkit {

namespace {

// Blank means unknown (0); otherwise digits only, at most four of them.
bool
ParseOptionalNumber(const BString& text, int32& number)
{
	if (text.Length() == 0) {
		number = 0;
		return true;
	}
	if (text.Length() > 4)
		return false;

	for (int32 i = 0; i < text.Length(); i++) {
		if (text.ByteAt(i) < '0' || text.ByteAt(i) > '9')
			return false;
	}

	number = atoi(text.String());
	return true;
}

} // namespace


const char*
tag_field_label(tag_field field)
{
	switch (field) {
		case TAG_FIELD_ARTIST:	return "Artist";
		case TAG_FIELD_TITLE:	return "Title";
		case TAG_FIELD_ALBUM:	return "Album";
		case TAG_FIELD_TRACK:	return "Track";
		case TAG_FIELD_YEAR:	return "Year";
		case TAG_FIELD_GENRE:	return "Genre";
		case TAG_FIELD_COMMENT:	return "Comment";
		default:				return "";
	}
}


BString
tag_field_value(const TagRecord& record, tag_field field)
{
	BString text;
	switch (field) {
		case TAG_FIELD_ARTIST:	text = record.artist; break;
		case TAG_FIELD_TITLE:	text = record.title; break;
		case TAG_FIELD_ALBUM:	text = record.album; break;
		case TAG_FIELD_GENRE:	text = record.genre; break;
		case TAG_FIELD_COMMENT:	text = record.comment; break;
		case TAG_FIELD_TRACK:
			if (record.track > 0)
				text << record.track;
			break;
		case TAG_FIELD_YEAR:
			if (record.year > 0)
				text << record.year;
			break;
		default:
			break;
	}
	return text;
}


bool
set_tag_field_value(TagRecord& record, tag_field field, const BString& text)
{
	BString trimmed(text);
	trimmed.Trim();

	int32 number = 0;
	switch (field) {
		case TAG_FIELD_ARTIST:	record.artist = trimmed; return true;
		case TAG_FIELD_TITLE:	record.title = trimmed; return true;
		case TAG_FIELD_ALBUM:	record.album = trimmed; return true;
		case TAG_FIELD_GENRE:	record.genre = trimmed; return true;
		case TAG_FIELD_COMMENT:	record.comment = trimmed; return true;
		case TAG_FIELD_TRACK:
			if (!ParseOptionalNumber(trimmed, number))
				return false;
			record.track = number;
			return true;
		case TAG_FIELD_YEAR:
			if (!ParseOptionalNumber(trimmed, number))
				return false;
			record.year = number;
			return true;
		default:
			return false;
	}
}


BString
common_tag_field_value(const std::vector<const TagRecord*>& records,
	tag_field field, bool* mixed)
{
	if (mixed != NULL)
		*mixed = false;
	if (records.empty())
		return BString();

	BString first = tag_field_value(*records[0], field);
	for (size_t i = 1; i < records.size(); i++) {
		if (tag_field_value(*records[i], field) != first) {
			if (mixed != NULL)
				*mixed = true;
			return BString();
		}
	}
	return first;
}


} // namespace tagkit

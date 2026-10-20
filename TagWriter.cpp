/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "TagWriter.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tbytevector.h>
#include <taglib/tlist.h>
#include <taglib/tmap.h>
#include <taglib/tstring.h>
#include <taglib/tvariant.h>

#include "FileAccess.h"


namespace tagkit {


static TagLib::String
to_taglib_string(const BString& string)
{
	return TagLib::String(string.String(), TagLib::String::UTF8);
}


namespace {

// The seven fields write_tags() sets, as TagLib holds them.
struct TagFields {
	TagLib::String	artist;
	TagLib::String	title;
	TagLib::String	album;
	TagLib::String	genre;
	TagLib::String	comment;
	unsigned int	track;
	unsigned int	year;

	explicit TagFields(const TagLib::Tag* tag)
		:
		artist(tag->artist()),
		title(tag->title()),
		album(tag->album()),
		genre(tag->genre()),
		comment(tag->comment()),
		track(tag->track()),
		year(tag->year())
	{
	}

	bool operator==(const TagFields& other) const
	{
		return artist == other.artist && title == other.title
			&& album == other.album && genre == other.genre
			&& comment == other.comment && track == other.track
			&& year == other.year;
	}
};

}	// namespace


bool
write_tags(const TagRecord& record, BString* errorMessage)
{
	if (record.path.IsEmpty()) {
		if (errorMessage != NULL)
			*errorMessage = "no file path";
		return false;
	}

	// Say why, in plain words, before TagLib can only fail to save.
	file_write_problem problem = check_file_writable(record.path);
	if (problem != FILE_WRITABLE) {
		if (errorMessage != NULL) {
			*errorMessage = file_write_problem_text(problem);
		}
		return false;
	}

	TagLib::FileRef file(record.path.String());
	if (file.isNull() || file.tag() == NULL) {
		if (errorMessage != NULL)
			*errorMessage = "couldn't open the file's tags";
		return false;
	}

	TagLib::Tag* tag = file.tag();

	// What the file holds now, to tell afterwards whether the save changed
	// anything.
	const TagFields before(tag);

	tag->setArtist(to_taglib_string(record.artist));
	tag->setTitle(to_taglib_string(record.title));
	tag->setAlbum(to_taglib_string(record.album));
	tag->setGenre(to_taglib_string(record.genre));
	tag->setComment(to_taglib_string(record.comment));
	tag->setTrack(record.track > 0 ? (unsigned int)record.track : 0);
	tag->setYear(record.year > 0 ? (unsigned int)record.year : 0);
	const bool tagsChanged = !(TagFields(tag) == before);

	if (record.newCoverArt != NULL && !record.newCoverArt->data.empty()) {
		// Replace the file's front cover (if it has one) with the chosen
		// image, keeping any other embedded pictures (back cover, ...).
		const CoverArtImage& art = *record.newCoverArt;
		const TagLib::String kPictureKey("PICTURE");
		const TagLib::String kFrontCover("Front Cover");

		TagLib::List<TagLib::VariantMap> pictures;
		TagLib::List<TagLib::VariantMap> existing
			= file.complexProperties(kPictureKey);
		for (TagLib::List<TagLib::VariantMap>::ConstIterator it
				= existing.begin(); it != existing.end(); ++it) {
			if (it->value("pictureType").toString() != kFrontCover)
				pictures.append(*it);
		}

		TagLib::VariantMap picture;
		picture["data"] = TagLib::ByteVector(
			reinterpret_cast<const char*>(&art.data[0]),
			(unsigned int)art.data.size());
		picture["pictureType"] = kFrontCover;
		picture["mimeType"] = to_taglib_string(art.mimeType);
		picture["description"] = TagLib::String();
		pictures.append(picture);

		if (!file.setComplexProperties(kPictureKey, pictures)) {
			if (errorMessage != NULL)
				*errorMessage = "this file type can't hold cover art";
			return false;
		}
	}

	if (record.removeCoverArt && record.newCoverArt == NULL) {
		// Drop every embedded picture (the reader shows any of them when
		// there's no front cover, so a partial removal would just show
		// another one).
		if (!file.setComplexProperties(TagLib::String("PICTURE"),
				TagLib::List<TagLib::VariantMap>())) {
			if (errorMessage != NULL)
				*errorMessage = "couldn't remove the cover art";
			return false;
		}
	}

	if (!file.save()) {
		if (errorMessage != NULL) {
			*errorMessage = "couldn't write to the file "
				"(is it read-only?)";
		}
		return false;
	}

	// Some places (an audio CD's files, say) accept a write and quietly throw
	// it away. If the tags were meant to change but read back exactly as they
	// were, say so instead of claiming success. (Only "nothing changed at
	// all" counts: a format may legitimately shorten or tidy a value.)
	if (tagsChanged) {
		TagLib::FileRef saved(record.path.String());
		if (!saved.isNull() && saved.tag() != NULL
			&& TagFields(saved.tag()) == before) {
			if (errorMessage != NULL) {
				*errorMessage = "the file didn't keep the changes "
					"(is it on a read-only volume?)";
			}
			return false;
		}
	}

	return true;
}


} // namespace tagkit

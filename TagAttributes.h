/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// TagAttributes.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// Haiku keeps a copy of a music file's main tags as file attributes (the
// columns Tracker shows, and what queries search): Audio:Artist,
// Audio:Album, Media:Title, Audio:Track, Media:Year, Media:Genre and
// Media:Comment -- the same set ArmyKnife uses. These helpers compare a
// TagRecord with the attributes on its file and write the tags out as
// attributes.
//
// AttributeOptions picks between that naming and the older "Audio:" one
// Hare has always used (Audio:Title, Audio:Year, Audio:Genre,
// Audio:Comment), and can add the MusicBrainz identifiers
// (Audio:MUSICBRAINZ_DISCID and friends) as attributes too.
//
// A blank tag (empty text, or a Track/Year of 0) never touches the file's
// attribute: an attribute is added or updated only when the tag has a
// value to put there.

#ifndef TAGKIT_TAG_ATTRIBUTES_H
#define TAGKIT_TAG_ATTRIBUTES_H

#include <String.h>
#include <SupportDefs.h>

#include "TagRecord.h"


namespace tagkit {


// Which names the title, year, genre and comment attributes get. Artist,
// album and track are Audio:Artist, Audio:Album and Audio:Track either way.
enum attribute_naming {
	ATTRIBUTE_NAMES_MEDIA = 0,	// Media:Title, Media:Year, Media:Genre,
								// Media:Comment (what ArmyKnife uses)
	ATTRIBUTE_NAMES_AUDIO		// Audio:Title, Audio:Year, Audio:Genre,
								// Audio:Comment (what Hare has always used)
};


struct AttributeOptions {
	AttributeOptions();

	attribute_naming	naming;			// default ATTRIBUTE_NAMES_MEDIA

	// Also store the MusicBrainz ids as the string attributes
	// Audio:MUSICBRAINZ_DISCID, _ALBUMID, _RELEASEGROUPID, _ARTISTID and
	// _TRACKID. Each comes from record.musicBrainz; one the record doesn't
	// have is taken from the file's own tags when it has it there. An id
	// found nowhere is left alone. Default false.
	bool				musicBrainzIds;
};


// True if writing the record's tags as attributes would add or change at
// least one attribute on its file (so false when the file already has
// them all, or the file can't be opened).
bool tag_attributes_need_update(const TagRecord& record,
	const AttributeOptions& options = AttributeOptions());

// Adds the attributes the file doesn't have yet and updates the ones whose
// value differs from the record's tags; ones that already match are left
// alone. *written (if given) is set to how many were added or changed.
// A new attribute is also listed in its file type's attribute info, so
// Tracker offers it as a column. Returns B_OK, or the error that stopped it.
status_t copy_tags_to_attributes(const TagRecord& record,
	int32* written = NULL,
	const AttributeOptions& options = AttributeOptions());

} // namespace tagkit

#endif // TAGKIT_TAG_ATTRIBUTES_H

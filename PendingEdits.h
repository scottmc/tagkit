/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// PendingEdits.h
//
// Part of "tagkit".
// Keeps track of hand edits to the tags of rows in a list, from the moment
// they are made until the user either keeps them (Apply) or takes them back
// (Discard Changes): each row's record as it was before its first edit is
// remembered, so the edit can be undone.
//
// The rows belong to the program, whatever kind it uses -- a TagView's
// TagRows, Hare's BRefRows -- so the class only knows them as opaque
// pointers and asks an EditHost to read and show their records. It does no
// drawing and has no windows; the program decides when to call it (from a
// FieldEditorWindow's result, say) and keeps its Apply and Discard buttons
// in step with HasPending().

#ifndef TAGKIT_PENDING_EDITS_H
#define TAGKIT_PENDING_EDITS_H

#include <map>
#include <vector>

#include <String.h>
#include <SupportDefs.h>

#include "TagField.h"
#include "TagRecord.h"


namespace tagkit {

// What PendingEdits needs from the program that owns the rows.
class EditHost {
public:
	virtual						~EditHost();

	// Whether the row is still in the list (it may have been removed while
	// an editor was open).
	virtual	bool				EditRowExists(void* row) = 0;

	// The record the row currently shows.
	virtual	TagRecord			EditRowRecord(void* row) = 0;

	// Makes the row show record (and hold it, so it is what gets saved).
	virtual	void				EditSetRowRecord(void* row,
									const TagRecord& record) = 0;
};


class PendingEdits {
public:
								PendingEdits(EditHost* host);

	// Sets field to value in every one of rows that still exists, marking
	// their records modified and remembering how each row was before its
	// first edit. A row the value would not change is left alone. Returns
	// how many rows changed, or -1 if value isn't acceptable for the field
	// (see set_tag_field_value()), in which case nothing was changed.
			int32				SetField(const std::vector<void*>& rows,
									tag_field field, const BString& value);

	// Whether there is anything to apply or discard, and how many rows.
			bool				HasPending() const;
			int32				Count() const;

	// Keeps the edits: the rows already show them, so this just forgets how
	// they were. Returns how many rows had edits.
			int32				Apply();

	// Puts every row that still exists back as it was. Returns how many
	// rows had edits.
			int32				Discard();

	// Forgets a row that is going away, without touching it.
			void				Forget(void* row);

private:
			EditHost*			fHost;
			std::map<void*, TagRecord>	fOriginals;
};

} // namespace tagkit

#endif // TAGKIT_PENDING_EDITS_H

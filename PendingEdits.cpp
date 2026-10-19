/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// PendingEdits.cpp
//
// Part of "tagkit". See PendingEdits.h.

#include "PendingEdits.h"


namespace tagkit {


EditHost::~EditHost()
{
}


PendingEdits::PendingEdits(EditHost* host)
	:
	fHost(host)
{
}


int32
PendingEdits::SetField(const std::vector<void*>& rows, tag_field field,
	const BString& value)
{
	// Only rows that are still there.
	std::vector<void*> live;
	for (size_t i = 0; i < rows.size(); i++) {
		if (rows[i] != NULL && fHost->EditRowExists(rows[i]))
			live.push_back(rows[i]);
	}
	if (live.empty())
		return 0;

	// Check the text once, before anything is changed.
	TagRecord check = fHost->EditRowRecord(live[0]);
	if (!set_tag_field_value(check, field, value))
		return -1;

	int32 changed = 0;
	for (size_t i = 0; i < live.size(); i++) {
		TagRecord before = fHost->EditRowRecord(live[i]);
		TagRecord record = before;
		set_tag_field_value(record, field, value);

		// Nothing actually changed for this file.
		if (tag_field_value(record, field) == tag_field_value(before, field))
			continue;

		// The first edit to a row remembers how it was.
		if (fOriginals.find(live[i]) == fOriginals.end())
			fOriginals[live[i]] = before;

		record.modified = true;
		fHost->EditSetRowRecord(live[i], record);
		changed++;
	}
	return changed;
}


bool
PendingEdits::HasPending() const
{
	return !fOriginals.empty();
}


int32
PendingEdits::Count() const
{
	return (int32)fOriginals.size();
}


int32
PendingEdits::Apply()
{
	int32 count = (int32)fOriginals.size();
	fOriginals.clear();
	return count;
}


int32
PendingEdits::Discard()
{
	int32 count = (int32)fOriginals.size();

	for (std::map<void*, TagRecord>::iterator it = fOriginals.begin();
			it != fOriginals.end(); ++it) {
		if (fHost->EditRowExists(it->first))
			fHost->EditSetRowRecord(it->first, it->second);
	}
	fOriginals.clear();
	return count;
}


void
PendingEdits::Forget(void* row)
{
	fOriginals.erase(row);
}


} // namespace tagkit

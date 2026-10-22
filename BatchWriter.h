/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// BatchWriter.h
//
// Part of "tagkit" -- shared tagging code, first written for TagView.
// Saves the tags of many files in one go, telling the caller after each file
// so a program can show progress ("Saved 3 of 12") and stop early. It is
// write_tags() in a loop with a report, so one bad file never stops the rest.

#ifndef TAGKIT_BATCH_WRITER_H
#define TAGKIT_BATCH_WRITER_H

#include <vector>

#include <String.h>
#include <SupportDefs.h>

#include "TagRecord.h"


namespace tagkit {

// What happened to one file.
struct SaveResult {
	BString		path;		// the record's path
	BString		fileName;	// the record's fileName, for messages
	bool		saved;		// false if write_tags() failed
	BString		error;		// why, when saved is false
};


// Called after each file is done (saved or failed). done is how many files
// have been tried so far, counting this one; total is how many there are.
// Return false to stop: the files after this one are left alone.
//
// It runs on the thread that called write_tags_batch(), which is usually a
// worker thread: don't touch views from it, send a message to the window
// instead.
typedef bool (*save_progress_function)(int32 done, int32 total,
	const SaveResult& result, void* cookie);


// Writes each record with write_tags(), in order. Returns one SaveResult for
// every file that was tried, in the same order as records (fewer than
// records.size() if progress asked to stop). The records are not changed, so
// the caller decides what to do with the ones that were saved, usually
// reading them back with read_tags().
//
// Like write_tags() this modifies files and blocks, so call it from a worker
// thread unless the list is short.
std::vector<SaveResult> write_tags_batch(const std::vector<TagRecord>& records,
	save_progress_function progress = NULL, void* cookie = NULL);

} // namespace tagkit

#endif // TAGKIT_BATCH_WRITER_H

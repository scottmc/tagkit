/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// FileAccess.h
//
// Part of "tagkit" -- the reusable pieces of TagView.
// Whether a file's tags can be saved, and if not, why: the file is gone, is
// not a plain file, is on a read-only volume (a CD, a mounted image), or is
// not writable by the user.

#ifndef TAGKIT_FILE_ACCESS_H
#define TAGKIT_FILE_ACCESS_H

#include <String.h>
#include <SupportDefs.h>


namespace tagkit {

enum file_write_problem {
	FILE_WRITABLE = 0,			// no problem
	FILE_MISSING,				// no such file (or a broken symbolic link)
	FILE_NOT_A_FILE,			// a folder or other non-file
	FILE_READ_ONLY_VOLUME,		// the volume it is on is read-only
	FILE_NO_PERMISSION			// the user may not write to the file
};


// Looks at the file at path, following symbolic links to the file they
// point to (that is the one a save changes). Does not modify anything.
file_write_problem check_file_writable(const BString& path);

// Short, translated reason for a problem ("the file is on a read-only volume"), for
// showing to the user. FILE_WRITABLE gives an empty string.
const char* file_write_problem_text(file_write_problem problem);

} // namespace tagkit

#endif // TAGKIT_FILE_ACCESS_H

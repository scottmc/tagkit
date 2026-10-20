/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "FileAccess.h"

#include <sys/stat.h>
#include <unistd.h>

#include <Entry.h>
#include <Volume.h>

#include "TagKitStrings.h"


namespace tagkit {


file_write_problem
check_file_writable(const BString& path)
{
	if (path.IsEmpty())
		return FILE_MISSING;

	// true == follow symbolic links, as the tag writer's own open does.
	BEntry entry(path.String(), true);
	if (entry.InitCheck() != B_OK || !entry.Exists())
		return FILE_MISSING;

	if (!entry.IsFile())
		return FILE_NOT_A_FILE;

	BVolume volume;
	if (entry.GetVolume(&volume) == B_OK && volume.IsReadOnly())
		return FILE_READ_ONLY_VOLUME;

	// Haiku's usual user is the superuser as far as access() is concerned,
	// which says "yes" to a file with no write bit at all (the files on an
	// audio CD are -r--r--r--). A file nobody may write to is read-only.
	struct stat st;
	if (stat(path.String(), &st) == 0
		&& (st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0) {
		return FILE_NO_PERMISSION;
	}

	if (access(path.String(), W_OK) != 0)
		return FILE_NO_PERMISSION;

	return FILE_WRITABLE;
}


const char*
file_write_problem_text(file_write_problem problem)
{
	switch (problem) {
		case FILE_MISSING:
			return TAGKIT_TR("the file is missing");
		case FILE_NOT_A_FILE:
			return TAGKIT_TR("that isn't a file");
		case FILE_READ_ONLY_VOLUME:
			return TAGKIT_TR("the file is on a read-only volume");
		case FILE_NO_PERMISSION:
			return TAGKIT_TR("the file is read-only");
		default:
			return "";
	}
}


} // namespace tagkit

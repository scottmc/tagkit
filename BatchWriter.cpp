/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "BatchWriter.h"

#include "TagWriter.h"


namespace tagkit {


std::vector<SaveResult>
write_tags_batch(const std::vector<TagRecord>& records,
	save_progress_function progress, void* cookie)
{
	std::vector<SaveResult> results;
	results.reserve(records.size());

	const int32 total = (int32)records.size();
	for (int32 i = 0; i < total; i++) {
		SaveResult result;
		result.path = records[i].path;
		result.fileName = records[i].fileName;
		result.saved = write_tags(records[i], &result.error);
		if (result.saved)
			result.error = "";
		results.push_back(result);

		if (progress != NULL && !progress(i + 1, total, result, cookie))
			break;
	}

	return results;
}


} // namespace tagkit

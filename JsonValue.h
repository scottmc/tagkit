/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// JsonValue.h
//
// Part of "tagkit" -- shared tagging code, first written for TagView.
// A small JSON reader, just enough for the answers of web services such as
// MusicBrainz. Reading never fails halfway: a missing key or an out of
// range index gives back a "null" value that reads as empty / 0 / false,
// so lookups can be chained without checking each step.

#ifndef TAGKIT_JSON_VALUE_H
#define TAGKIT_JSON_VALUE_H

#include <vector>

#include <String.h>
#include <SupportDefs.h>


namespace tagkit {

class JsonValue {
public:
	enum Type {
		JSON_NULL,
		JSON_BOOL,
		JSON_NUMBER,
		JSON_STRING,
		JSON_ARRAY,
		JSON_OBJECT
	};

								JsonValue();

	// Parses text (length bytes, UTF-8) into a value. Returns false, with
	// the value left as null, if text isn't valid JSON, is nested more
	// than 64 levels deep, or has anything but spaces after the value.
	static	bool				Parse(const char* text, size_t length,
									JsonValue& value);

			Type				GetType() const { return fType; }
			bool				IsNull() const { return fType == JSON_NULL; }
			bool				IsString() const { return fType == JSON_STRING; }
			bool				IsNumber() const { return fType == JSON_NUMBER; }
			bool				IsArray() const { return fType == JSON_ARRAY; }
			bool				IsObject() const { return fType == JSON_OBJECT; }

	// The value as a string / number / boolean; empty, 0 and false when it
	// is another type.
			const BString&		String() const;
			double				Number() const;
			bool				Bool() const;

	// How many items an array has, or members an object has (0 for the
	// other types).
			int32				Count() const;

	// An array's item, or an object's member value, by position; null if
	// out of range or not an array / object.
			const JsonValue&	At(int32 index) const;

	// An object's member by name; null if there is none (or this isn't an
	// object). With repeated names the first one counts.
			const JsonValue&	Get(const char* key) const;

private:
	friend class JsonReader;

			Type				fType;
			BString				fString;
			double				fNumber;
			bool				fBool;
			std::vector<JsonValue>	fItems;
			std::vector<BString>	fKeys;
};

} // namespace tagkit

#endif // TAGKIT_JSON_VALUE_H

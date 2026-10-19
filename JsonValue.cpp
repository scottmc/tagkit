/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
#include "JsonValue.h"

#include <stdlib.h>
#include <string.h>
#include <string>


namespace tagkit {

namespace {

const int kMaxDepth = 64;


const JsonValue&
NullValue()
{
	static const JsonValue null;
	return null;
}

} // namespace


class JsonReader {
public:
	JsonReader(const char* text, size_t length)
		:
		fCurrent(text),
		fEnd(text + length)
	{
	}

	bool ReadDocument(JsonValue& value)
	{
		if (!ReadValue(value, 0))
			return false;
		SkipSpace();
		return fCurrent == fEnd;
	}

private:
	void SkipSpace()
	{
		while (fCurrent < fEnd && (*fCurrent == ' ' || *fCurrent == '\t'
				|| *fCurrent == '\n' || *fCurrent == '\r')) {
			fCurrent++;
		}
	}

	bool Expect(const char* word)
	{
		size_t length = strlen(word);
		if ((size_t)(fEnd - fCurrent) < length
				|| memcmp(fCurrent, word, length) != 0) {
			return false;
		}
		fCurrent += length;
		return true;
	}

	bool ReadValue(JsonValue& value, int depth)
	{
		if (depth > kMaxDepth)
			return false;

		SkipSpace();
		if (fCurrent >= fEnd)
			return false;

		switch (*fCurrent) {
			case '{':
				return ReadObject(value, depth);
			case '[':
				return ReadArray(value, depth);
			case '"':
			{
				std::string text;
				if (!ReadString(text))
					return false;
				value.fType = JsonValue::JSON_STRING;
				value.fString.SetTo(text.c_str(), (int32)text.size());
				return true;
			}
			case 't':
				value.fType = JsonValue::JSON_BOOL;
				value.fBool = true;
				return Expect("true");
			case 'f':
				value.fType = JsonValue::JSON_BOOL;
				value.fBool = false;
				return Expect("false");
			case 'n':
				value.fType = JsonValue::JSON_NULL;
				return Expect("null");
			default:
				return ReadNumber(value);
		}
	}

	bool ReadNumber(JsonValue& value)
	{
		const char* start = fCurrent;
		while (fCurrent < fEnd && strchr("+-.eE0123456789", *fCurrent) != NULL)
			fCurrent++;
		if (fCurrent == start || fCurrent - start > 64)
			return false;

		std::string text(start, fCurrent - start);
		char* parsedEnd = NULL;
		double number = strtod(text.c_str(), &parsedEnd);
		if (parsedEnd == NULL || *parsedEnd != '\0')
			return false;

		value.fType = JsonValue::JSON_NUMBER;
		value.fNumber = number;
		return true;
	}

	static void AppendUtf8(std::string& text, uint32 character)
	{
		if (character < 0x80) {
			text += (char)character;
		} else if (character < 0x800) {
			text += (char)(0xC0 | (character >> 6));
			text += (char)(0x80 | (character & 0x3F));
		} else if (character < 0x10000) {
			text += (char)(0xE0 | (character >> 12));
			text += (char)(0x80 | ((character >> 6) & 0x3F));
			text += (char)(0x80 | (character & 0x3F));
		} else {
			text += (char)(0xF0 | (character >> 18));
			text += (char)(0x80 | ((character >> 12) & 0x3F));
			text += (char)(0x80 | ((character >> 6) & 0x3F));
			text += (char)(0x80 | (character & 0x3F));
		}
	}

	bool ReadHex4(uint32& result)
	{
		if (fEnd - fCurrent < 4)
			return false;
		result = 0;
		for (int i = 0; i < 4; i++) {
			char c = *fCurrent++;
			result <<= 4;
			if (c >= '0' && c <= '9')
				result |= c - '0';
			else if (c >= 'a' && c <= 'f')
				result |= c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				result |= c - 'A' + 10;
			else
				return false;
		}
		return true;
	}

	// fCurrent is at the opening quote.
	bool ReadString(std::string& text)
	{
		fCurrent++;
		while (fCurrent < fEnd) {
			char c = *fCurrent++;
			if (c == '"')
				return true;
			if ((unsigned char)c < 0x20)
				return false;
			if (c != '\\') {
				text += c;
				continue;
			}

			if (fCurrent >= fEnd)
				return false;
			char escape = *fCurrent++;
			switch (escape) {
				case '"': text += '"'; break;
				case '\\': text += '\\'; break;
				case '/': text += '/'; break;
				case 'b': text += '\b'; break;
				case 'f': text += '\f'; break;
				case 'n': text += '\n'; break;
				case 'r': text += '\r'; break;
				case 't': text += '\t'; break;
				case 'u':
				{
					uint32 character;
					if (!ReadHex4(character))
						return false;
					// A high surrogate must be followed by a \uXXXX low
					// one; together they are one character above U+FFFF.
					if (character >= 0xD800 && character < 0xDC00) {
						uint32 low;
						if (fEnd - fCurrent < 2 || fCurrent[0] != '\\'
								|| fCurrent[1] != 'u') {
							return false;
						}
						fCurrent += 2;
						if (!ReadHex4(low) || low < 0xDC00 || low > 0xDFFF)
							return false;
						character = 0x10000 + ((character - 0xD800) << 10)
							+ (low - 0xDC00);
					} else if (character >= 0xDC00 && character <= 0xDFFF) {
						return false;
					}
					AppendUtf8(text, character);
					break;
				}
				default:
					return false;
			}
		}
		return false;	// no closing quote
	}

	bool ReadArray(JsonValue& value, int depth)
	{
		fCurrent++;
		value.fType = JsonValue::JSON_ARRAY;

		SkipSpace();
		if (fCurrent < fEnd && *fCurrent == ']') {
			fCurrent++;
			return true;
		}

		while (true) {
			JsonValue item;
			if (!ReadValue(item, depth + 1))
				return false;
			value.fItems.push_back(item);

			SkipSpace();
			if (fCurrent >= fEnd)
				return false;
			if (*fCurrent == ',') {
				fCurrent++;
				continue;
			}
			if (*fCurrent == ']') {
				fCurrent++;
				return true;
			}
			return false;
		}
	}

	bool ReadObject(JsonValue& value, int depth)
	{
		fCurrent++;
		value.fType = JsonValue::JSON_OBJECT;

		SkipSpace();
		if (fCurrent < fEnd && *fCurrent == '}') {
			fCurrent++;
			return true;
		}

		while (true) {
			SkipSpace();
			if (fCurrent >= fEnd || *fCurrent != '"')
				return false;
			std::string key;
			if (!ReadString(key))
				return false;

			SkipSpace();
			if (fCurrent >= fEnd || *fCurrent != ':')
				return false;
			fCurrent++;

			JsonValue member;
			if (!ReadValue(member, depth + 1))
				return false;
			value.fKeys.push_back(BString(key.c_str(), (int32)key.size()));
			value.fItems.push_back(member);

			SkipSpace();
			if (fCurrent >= fEnd)
				return false;
			if (*fCurrent == ',') {
				fCurrent++;
				continue;
			}
			if (*fCurrent == '}') {
				fCurrent++;
				return true;
			}
			return false;
		}
	}

	const char*	fCurrent;
	const char*	fEnd;
};


JsonValue::JsonValue()
	:
	fType(JSON_NULL),
	fNumber(0),
	fBool(false)
{
}


/*static*/ bool
JsonValue::Parse(const char* text, size_t length, JsonValue& value)
{
	JsonValue parsed;
	JsonReader reader(text, length);
	if (!reader.ReadDocument(parsed)) {
		value = JsonValue();
		return false;
	}

	value = parsed;
	return true;
}


const BString&
JsonValue::String() const
{
	static const BString empty;
	return fType == JSON_STRING ? fString : empty;
}


double
JsonValue::Number() const
{
	return fType == JSON_NUMBER ? fNumber : 0;
}


bool
JsonValue::Bool() const
{
	return fType == JSON_BOOL && fBool;
}


int32
JsonValue::Count() const
{
	if (fType != JSON_ARRAY && fType != JSON_OBJECT)
		return 0;
	return (int32)fItems.size();
}


const JsonValue&
JsonValue::At(int32 index) const
{
	if ((fType != JSON_ARRAY && fType != JSON_OBJECT) || index < 0
			|| index >= (int32)fItems.size()) {
		return NullValue();
	}
	return fItems[index];
}


const JsonValue&
JsonValue::Get(const char* key) const
{
	if (fType != JSON_OBJECT)
		return NullValue();
	for (size_t i = 0; i < fKeys.size(); i++) {
		if (fKeys[i] == key)
			return fItems[i];
	}
	return NullValue();
}

} // namespace tagkit

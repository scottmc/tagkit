/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// TagKitStrings.h
//
// Part of "tagkit".
// How TagKit's own words -- column titles, field names, buttons, window
// titles, the "(multiple)" shown for files that disagree -- get translated.
//
// By default they are English, and nothing here needs any library. A program
// that has a Haiku catalog (a Locale Kit one, like Hare's) defines
// TAGKIT_USE_CATALOG when it builds, in DEFINES next to the TAGKIT_CLIENT_*
// ones. Every string in TagKit then goes through B_TRANSLATE_CONTEXT() with
// the context "TagKit", so:
//   * the program's own catalog is searched for them at run time -- no setup
//     in the code, only the catalog;
//   * Haiku's catkeys collection (`make catkeys`) finds them in TagKit's
//     source files as it does the program's own, because TagKit is compiled
//     into the program, so they land in the program's .catkeys file under
//     the context "TagKit".
// A string without a translation is shown in English. The words a program
// gives TagKit itself (an EmptyArtButton's label, say) are the program's to
// translate.
//
// Messages with something in them use a %placeholder% that is replaced with
// the value after translating ("Edit %field%"), and counts use ICU plural
// rules ("{0, plural, one{# file selected} other{# files selected}}"),
// which a translator can adapt to the language's own plural forms.

#ifndef TAGKIT_STRINGS_H
#define TAGKIT_STRINGS_H

#include <String.h>
#include <SupportDefs.h>

#ifdef TAGKIT_USE_CATALOG
#	include <Catalog.h>
#	include <StringFormat.h>
#	define TAGKIT_TR(text) B_TRANSLATE_CONTEXT(text, "TagKit")
#else
#	define TAGKIT_TR(text) (text)
#endif


namespace tagkit {

// text with the first occurrence of placeholder (such as "%field%") replaced
// by value.
inline BString
fill_in(const char* text, const char* placeholder, const char* value)
{
	BString result(text);
	result.ReplaceFirst(placeholder, value);
	return result;
}


// "1 file selected", "3 files selected", in the program's language.
inline BString
files_selected_text(int32 count)
{
	BString result;
#ifdef TAGKIT_USE_CATALOG
	BStringFormat format(B_TRANSLATE_CONTEXT(
		"{0, plural, one{# file selected} other{# files selected}}",
		"TagKit"));
	if (format.Format(result, (int64)count) == B_OK)
		return result;
	result = "";
#endif
	result << count << (count == 1 ? " file selected" : " files selected");
	return result;
}

} // namespace tagkit

#endif // TAGKIT_STRINGS_H

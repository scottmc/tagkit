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
// A string without a translation is shown in English. Link the static
// "localestub" library as well (tagkit.mk's TAGKIT_CATALOG_LIBS), which every
// program using B_TRANSLATE needs. The words a program
// gives TagKit itself (an EmptyArtButton's label, say) are the program's to
// translate.
//
// A program that already has translations for the same words -- ArmyKnife
// has its own "Artist", "Album" and the rest in its own catalog contexts --
// can have TagKit use those instead, or give them different wording, with
// set_string_override() (below). It works with or without
// TAGKIT_USE_CATALOG.
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
#	define TAGKIT_TR(text) \
		::tagkit::apply_string_override(text, B_TRANSLATE_CONTEXT(text, "TagKit"))
#else
#	define TAGKIT_TR(text) ::tagkit::apply_string_override(text, text)
#endif


namespace tagkit {

// Called for every string TagKit shows. english is the string as written in
// TagKit's source (also the lookup key); translated is what TagKit would
// show (the same as english when there is no catalog or no translation).
// Return the text to show instead, or NULL to keep translated. The returned
// pointer has to stay valid for as long as the string is used, which is
// true of what B_TRANSLATE() returns; it is not copied.
typedef const char* (*string_override_function)(const char* english,
	const char* translated, void* cookie);

struct StringOverride {
	string_override_function	function;
	void*						cookie;
};


inline StringOverride&
string_override()
{
	static StringOverride current = { NULL, NULL };
	return current;
}


// Installs (or, with NULL, removes) the override. Do it once at start-up,
// before any TagKit window or view is made: the strings are looked up as they
// are used, and this is not locked.
inline void
set_string_override(string_override_function function, void* cookie = NULL)
{
	string_override().function = function;
	string_override().cookie = cookie;
}


// What TAGKIT_TR() uses; not meant to be called directly.
inline const char*
apply_string_override(const char* english, const char* translated)
{
	const StringOverride& current = string_override();
	if (current.function != NULL) {
		const char* replacement
			= current.function(english, translated, current.cookie);
		if (replacement != NULL)
			return replacement;
	}
	return translated;
}


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
	BStringFormat format(apply_string_override(
		"{0, plural, one{# file selected} other{# files selected}}",
		B_TRANSLATE_CONTEXT(
			"{0, plural, one{# file selected} other{# files selected}}",
			"TagKit")));
	if (format.Format(result, (int64)count) == B_OK)
		return result;
	result = "";
#endif
	result << count << (count == 1 ? " file selected" : " files selected");
	return result;
}

} // namespace tagkit

#endif // TAGKIT_STRINGS_H

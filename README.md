# TagKit

Reusable music-tag pieces for Haiku programs: a tag data model, reading and
writing tags with TagLib (MP3, Ogg Vorbis, FLAC), cover art lookup and
handling, MusicBrainz search, and a few views to show it all.

TagKit began as the `src/tagkit` folder of
[TagView](https://github.com/scottmc/TagView) and was split out into its own
repository (with its history) so other programs, such as Hare, can share it.

## What's in it

- `TagRecord`, `TagField` -- a plain data holder for one file's tags (no
  dependency on any tagging library) and the editable fields.
- `TagReader`, `TagWriter`, `TagAttributes` -- read and write tags with
  TagLib; copy tags to Haiku file attributes.
- `CoverArtImage`, `CoverArtFetch`, `ITunesArtwork`, `HttpFetch` -- cover art
  kept as its original bytes, found on the Cover Art Archive (libcoverart)
  with an iTunes fallback.
- `MusicBrainzSearch`, `RecordingMatch` -- recording and release search.
- `CompactView`, `CoverArtView`, `CoverArtCandidatesView`,
  `CoverArtDragDrop`, `EmptyArtButton`, `TagView`, `RecordingMatchView` --
  views.
- `GenreList`, `ClientInfo` -- the genre names, and the User-Agent that web
  services get told (the program using TagKit supplies its own name).

Everything is in the `tagkit` C++ namespace. Reference documentation is in
[`docs/index.html`](docs/index.html).

## Using it

Add the repository to your program as a git submodule named `tagkit`:

    git submodule add https://github.com/scottmc/tagkit tagkit

then use `tagkit.mk` from your Makefile (its comments show the few lines to
add) and include headers as `#include "tagkit/CompactView.h"`. TagKit's
sources are compiled into your program, not built as a separate library,
because the web services want to know which program is calling: define
`TAGKIT_CLIENT_NAME` and `TAGKIT_CLIENT_VERSION` (and, ideally,
`TAGKIT_CLIENT_CONTACT`) in your Makefile's `DEFINES`. See `ClientInfo.h`.

## Requirements

Haiku with the TagLib, libmusicbrainz5 and libcoverart development packages
(`taglib_devel`, `libmusicbrainz5_devel`, `libcoverart_devel`), and `curl` at
run time for the iTunes lookups.

## License

MIT License; see [LICENSE](LICENSE).

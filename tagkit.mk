## tagkit.mk -- build fragment for apps that use TagKit.
##
## TagKit is a collection of source files meant to be compiled into the app
## that uses it (not a prebuilt library): tagkit::user_agent() and friends
## use the TAGKIT_CLIENT_* defines of the app being built (see ClientInfo.h).
##
## In the app's Haiku generic Makefile, put the repository somewhere in the
## app's tree (a git submodule named "tagkit" next to the Makefile works
## well), then before the "include .../makefile-engine" line add:
##
##	TAGKIT_DIR = tagkit
##	include $(TAGKIT_DIR)/tagkit.mk
##
##	SRCS = src/main.cpp ... $(TAGKIT_SRCS)
##	LIBS = $(STDCPPLIBS) be $(TAGKIT_LIBS)
##	SYSTEM_INCLUDE_PATHS = $(TAGKIT_SYSTEM_INCLUDE_PATHS)
##	LOCAL_INCLUDE_PATHS = src $(TAGKIT_LOCAL_INCLUDE_PATHS)
##	DEFINES = TAGKIT_CLIENT_NAME=MyApp TAGKIT_CLIENT_VERSION=1.0 \
##		TAGKIT_CLIENT_CONTACT=example.org/myapp
##
## Add TAGKIT_USE_CATALOG to DEFINES, and $(TAGKIT_CATALOG_LIBS) to LIBS, if the
## app has a Haiku catalog and should translate TagKit's own words too (see
## TagKitStrings.h); `make catkeys` then collects them along with the app's
## own, under the context "TagKit".
##
## and include TagKit's headers as #include "tagkit/CompactView.h".
##
## Note that the Haiku makefile-engine puts every object file in one
## directory, named after its source file, so the app must not have a source
## file with the same name as one of TagKit's (CoverArtView.cpp, say).
## An app that only needs part of TagKit can list just the files it wants
## instead of using TAGKIT_SRCS (the headers say what each one needs).
##
## Two ready-made parts: TAGKIT_CORE_SRCS is everything without a window or
## view (tags, files, web lookups, cover art images, translation hooks), and
## TAGKIT_VIEW_SRCS is the views and windows. TAGKIT_SRCS is both. An app
## moving to TagKit step by step can start with the core and add the views
## when it has retired its own views of the same names.

TAGKIT_DIR ?= tagkit

# Without windows or views: needs "be", TagLib and the Translation Kit.
TAGKIT_CORE_SRCS = $(addprefix $(TAGKIT_DIR)/, \
	BatchWriter.cpp \
	CoverArtFetch.cpp \
	CoverArtImage.cpp \
	FileAccess.cpp \
	GenreList.cpp \
	HttpFetch.cpp \
	ITunesArtwork.cpp \
	JsonValue.cpp \
	MusicBrainzSearch.cpp \
	NetworkTimeout.cpp \
	PendingEdits.cpp \
	TagAttributes.cpp \
	TagField.cpp \
	TagReader.cpp \
	TagRecord.cpp \
	TagWriter.cpp)

# The views and windows: need "be", the tracker library and columnlistview as
# well.
TAGKIT_VIEW_SRCS = $(addprefix $(TAGKIT_DIR)/, \
	CompactView.cpp \
	CoverArtCandidatesView.cpp \
	CoverArtDragDrop.cpp \
	CoverArtPickerWindow.cpp \
	CoverArtView.cpp \
	EmptyArtButton.cpp \
	FieldEditorWindow.cpp \
	RecordingMatchView.cpp \
	TagView.cpp)

TAGKIT_SRCS = $(TAGKIT_CORE_SRCS) $(TAGKIT_VIEW_SRCS)

# Libraries TagKit links against (add "be" yourself): for everything, and for
# just the core.
TAGKIT_LIBS = tag translation tracker columnlistview
TAGKIT_CORE_LIBS = tag translation

# Also needed, after TAGKIT_LIBS, if the app defines TAGKIT_USE_CATALOG: the
# static stub that lets a program use catalogs (what B_TRANSLATE needs).
TAGKIT_CATALOG_LIBS = localestub

# Headers: TagKit's own include each other by their plain names, so the app
# only needs the directory that CONTAINS the tagkit folder.
TAGKIT_LOCAL_INCLUDE_PATHS = $(or $(patsubst %/,%,$(dir $(TAGKIT_DIR))),.)

# BColumnListView lives under the private interface headers, and TagLib's
# headers are looked up (not hard-coded) so a secondary architecture build
# finds its own copy.
TAGKIT_SYSTEM_INCLUDE_PATHS = \
	$(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/interface) \
	$(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY taglib)

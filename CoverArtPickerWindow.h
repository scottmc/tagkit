/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// CoverArtPickerWindow.h
//
// Part of "tagkit".

#ifndef TAGKIT_COVER_ART_PICKER_WINDOW_H
#define TAGKIT_COVER_ART_PICKER_WINDOW_H

#include <vector>

#include <Messenger.h>
#include <Rect.h>
#include <String.h>
#include <Window.h>

#include "CoverArtImage.h"

class BButton;
class BStringView;


namespace tagkit {

class CoverArtCandidatesView;

// The BMessage `what` codes the picker uses. A program using it handles
// kMsgCoverArtPickerChosen and kMsgCoverArtPickerClosed; the third is the
// picker's own.
enum {
	kMsgCoverArtPickerSelection	= 'cASc',	// (inside) another thumbnail picked
	kMsgCoverArtPickerChosen	= 'cACh',	// sent to the target: "index"
	kMsgCoverArtPickerClosed	= 'cAWc'	// sent to the target when the
										// window is going away: "frame"
};


// Shown when a lookup turned up cover art on more than one release: a
// scrollable strip of thumbnails (like Hare's cover art picker) with the
// selected release's title and year underneath. Non-modal. "Use Selected
// Cover" (or double-clicking a thumbnail) sends kMsgCoverArtPickerChosen
// with the chosen candidate's int32 "index" -- its position in the vector
// given at construction, and in the order added since -- to the BMessenger
// given, then closes.
//
// However it closes (Use Selected Cover, Cancel, the close box) the target
// then gets kMsgCoverArtPickerClosed, with the window's last frame as
// "frame" (a BRect) so the program can remember where the user left it, and
// can drop its pointer to the window. (Calling Quit() on the window
// yourself sends nothing.) Where the window first opens is up to the
// program: move it after constructing and before Show().
class CoverArtPickerWindow : public BWindow {
public:
								CoverArtPickerWindow(BMessenger target,
									const BString& fileName,
									const std::vector<CoverArtImage>& images);

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				MoveToFrontAndFocus();

	// Adds a candidate that turned up after the window opened (covers keep
	// arriving while the lookup is still working through the releases).
	// Safe to call from another window's thread.
			void				AddImage(const CoverArtImage& image);

	// Shows/hides the "still looking" note in the prompt. Safe to call from
	// another window's thread.
			void				SetLoading(bool loading);

private:
			void				_UpdateCaption();
			void				_Choose();

			BMessenger			fTarget;
			CoverArtCandidatesView*	fCandidatesView;
			BStringView*		fCaptionView;
			BStringView*		fPromptView;
			BString				fPromptText;
			std::vector<BString>	fCaptions;
};

} // namespace tagkit

#endif // TAGKIT_COVER_ART_PICKER_WINDOW_H

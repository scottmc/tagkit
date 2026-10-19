/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// CoverArtPickerWindow.cpp
//
// Part of "tagkit". See CoverArtPickerWindow.h.

#include "CoverArtPickerWindow.h"

#include <Autolock.h>
#include <Bitmap.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <ScrollView.h>
#include <StringView.h>

#include "CoverArtCandidatesView.h"
#include "TagKitStrings.h"


namespace tagkit {


static const float kWindowWidth = 480;
static const float kWindowHeight = 250;

// Thumbnails visible at once in the picker; more covers scroll.
static const int32 kImagesWide = 4;


CoverArtPickerWindow::CoverArtPickerWindow(BMessenger target,
	const BString& fileName, const std::vector<CoverArtImage>& images)
	:
	BWindow(BRect(180, 180, 180 + kWindowWidth, 180 + kWindowHeight),
		TAGKIT_TR("Choose Cover Art"), B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target)
{
	fPromptText = fill_in(TAGKIT_TR("Cover art for \"%file%\""), "%file%",
		fileName.String());
	fPromptView = new BStringView("prompt", "");
	fPromptView->SetText(fPromptText.String());

	fCandidatesView = new CoverArtCandidatesView(kImagesWide);
	fCandidatesView->SetSelectionMessage(
		new BMessage(kMsgCoverArtPickerSelection));
	fCandidatesView->SetInvocationMessage(new BMessage(kMsgCoverArtPickerChosen));

	// Decode every candidate; the view takes ownership of the bitmaps. A
	// candidate that won't decode stays in the list as a placeholder so
	// indexes keep matching the caller's vector.
	BBitmap** bitmaps = new BBitmap*[images.size()];
	for (size_t i = 0; i < images.size(); i++) {
		bitmaps[i] = decode_cover_art(images[i]);

		BString caption(images[i].releaseTitle);
		if (images[i].year > 0)
			caption << " (" << images[i].year << ")";
		fCaptions.push_back(caption);
	}
	fCandidatesView->SetCandidates(bitmaps, (int32)images.size());
	delete[] bitmaps;

	BScrollView* scrollView = new BScrollView("candidatesScroll",
		fCandidatesView, B_WILL_DRAW | B_FRAME_EVENTS, true /* horizontal */,
		false /* vertical */, B_FANCY_BORDER);

	fCaptionView = new BStringView("caption", "");

	BButton* cancelButton = new BButton("cancel", TAGKIT_TR("Cancel"),
		new BMessage(B_QUIT_REQUESTED));
	BButton* useButton = new BButton("use", TAGKIT_TR("Use Selected Cover"),
		new BMessage(kMsgCoverArtPickerChosen));
	useButton->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fPromptView)
		.Add(scrollView)
		.Add(fCaptionView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(useButton)
		.End()
		.End();

	cancelButton->SetTarget(this);
	useButton->SetTarget(this);

	_UpdateCaption();

	// Fit the four-thumbnail strip rather than the guessed size above.
	ResizeToPreferred();
}


void
CoverArtPickerWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgCoverArtPickerSelection:
			_UpdateCaption();
			break;

		case kMsgCoverArtPickerChosen:
			_Choose();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
CoverArtPickerWindow::QuitRequested()
{
	// Let the program remember where we were and drop its pointer to us
	// instead of holding a stale one.
	BMessage closed(kMsgCoverArtPickerClosed);
	closed.AddRect("frame", Frame());
	fTarget.SendMessage(&closed);
	return true;
}


void
CoverArtPickerWindow::AddImage(const CoverArtImage& image)
{
	// Called from another window's thread, so lock this window first.
	BAutolock locker(this);
	if (!locker.IsLocked())
		return;

	BString caption(image.releaseTitle);
	if (image.year > 0)
		caption << " (" << image.year << ")";
	fCaptions.push_back(caption);

	fCandidatesView->AddCandidate(decode_cover_art(image));
}


void
CoverArtPickerWindow::SetLoading(bool loading)
{
	BAutolock locker(this);
	if (!locker.IsLocked())
		return;

	BString text(fPromptText);
	if (loading)
		text << " " << TAGKIT_TR("(still looking" B_UTF8_ELLIPSIS ")");
	else
		text << ":";
	fPromptView->SetText(text.String());
}


void
CoverArtPickerWindow::MoveToFrontAndFocus()
{
	if (IsHidden())
		Show();
	Activate();
}


void
CoverArtPickerWindow::_UpdateCaption()
{
	int32 index = fCandidatesView->SelectedIndex();
	if (index >= 0 && index < (int32)fCaptions.size())
		fCaptionView->SetText(fCaptions[index].String());
	else
		fCaptionView->SetText("");
}


void
CoverArtPickerWindow::_Choose()
{
	int32 index = fCandidatesView->SelectedIndex();
	if (index < 0 || index >= fCandidatesView->CountCandidates())
		return;

	BMessage chosen(kMsgCoverArtPickerChosen);
	chosen.AddInt32("index", index);
	fTarget.SendMessage(&chosen);

	PostMessage(B_QUIT_REQUESTED);
}


} // namespace tagkit

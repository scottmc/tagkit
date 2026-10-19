/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// EmptyArtButton.h
//
// Part of "tagkit".
// A button -- an icon over a label, on a translucent chip -- that a cover
// art view draws in its middle while it has no cover to show, such as Hare's
// "Load CD". Pressing it posts a message you supply to the view's window.
// Whether it is shown is up to the program (Hare shows it only while a CD is
// mounted), so it can be hidden and shown again without being set up twice.
//
// CoverArtView and CompactView use one of these inside; a program sets it
// through their SetEmptyButton() and SetEmptyButtonVisible(). A view of your
// own can own one as well and forward Draw(), MouseDown() and MouseUp().

#ifndef TAGKIT_EMPTY_ART_BUTTON_H
#define TAGKIT_EMPTY_ART_BUTTON_H

#include <Point.h>
#include <Rect.h>
#include <String.h>

class BBitmap;
class BMessage;
class BView;


namespace tagkit {

class EmptyArtButton {
public:
								EmptyArtButton();
								~EmptyArtButton();

	// Sets the label, the icon (NULL for none; drawn at up to 32 x 32) and
	// the message posted to the window when the button is pressed. Takes
	// ownership of icon and message. A NULL message removes the button.
	// The button starts out visible.
			void				SetButton(const char* label, BBitmap* icon,
									BMessage* message);

	// Hides or shows the button without forgetting it.
			void				SetVisible(bool visible);

	// True if there is a button and it is visible.
			bool				IsActive() const;

	// Draws the button centered in square (in view's coordinates), using
	// the view's font. Does nothing unless IsActive(). Call it after the
	// background has been drawn.
			void				Draw(BView* view, BRect square);

	// For a primary-button press at where in view; returns true if the
	// press landed on the button (and so is not for anything else).
			bool				MouseDown(BView* view, BPoint where,
									BRect square);

	// For the matching release; returns true if a press on the button was
	// being tracked. The message is posted to view->Window() only if the
	// pointer is still on the button.
			bool				MouseUp(BView* view, BPoint where,
									BRect square);

private:
			BRect				_ButtonRect(BView* view, BRect square,
									float* iconSize) const;

			BString				fLabel;
			BBitmap*			fIcon;
			BMessage*			fMessage;
			bool				fVisible;
			bool				fPressed;
};

} // namespace tagkit

#endif // TAGKIT_EMPTY_ART_BUTTON_H

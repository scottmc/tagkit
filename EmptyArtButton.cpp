/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// EmptyArtButton.cpp
//
// Part of "tagkit". See EmptyArtButton.h.

#include "EmptyArtButton.h"

#include <math.h>

#include <Bitmap.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <View.h>
#include <Window.h>


namespace tagkit {

namespace {

const float kIconSize = 32.0f;
const float kSmallestIcon = 16.0f;
const float kPadding = 8.0f;
const float kLabelGap = 4.0f;
const float kCornerRadius = 8.0f;

} // namespace


EmptyArtButton::EmptyArtButton()
	:
	fIcon(NULL),
	fMessage(NULL),
	fVisible(true),
	fPressed(false)
{
}


EmptyArtButton::~EmptyArtButton()
{
	delete fIcon;
	delete fMessage;
}


void
EmptyArtButton::SetButton(const char* label, BBitmap* icon, BMessage* message)
{
	if (icon != fIcon)
		delete fIcon;
	if (message != fMessage)
		delete fMessage;

	fLabel = label != NULL ? label : "";
	fIcon = icon;
	fMessage = message;
	fVisible = true;
	fPressed = false;
}


void
EmptyArtButton::SetVisible(bool visible)
{
	fVisible = visible;
	if (!visible)
		fPressed = false;
}


bool
EmptyArtButton::IsActive() const
{
	return fVisible && fMessage != NULL;
}


BRect
EmptyArtButton::_ButtonRect(BView* view, BRect square, float* iconSize) const
{
	font_height fontHeight;
	view->GetFontHeight(&fontHeight);
	float labelHeight = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);
	float labelWidth = view->StringWidth(fLabel.String());

	// In a small square the icon shrinks, and below a point goes away.
	float size = 0;
	if (fIcon != NULL) {
		size = fminf(kIconSize,
			floorf((fminf(square.Width(), square.Height()) + 1.0f) / 3.0f));
		if (size < kSmallestIcon)
			size = 0;
	}
	if (iconSize != NULL)
		*iconSize = size;

	float width = fmaxf(labelWidth, size) + (2 * kPadding);
	float height = (2 * kPadding) + labelHeight
		+ (size > 0 ? size + kLabelGap : 0);
	if (width > square.Width() + 1.0f)
		width = square.Width() + 1.0f;

	BRect rect(0, 0, width - 1.0f, height - 1.0f);
	rect.OffsetBy(floorf(square.left + (square.Width() + 1.0f - width) / 2.0f),
		floorf(square.top + (square.Height() + 1.0f - height) / 2.0f));
	return rect;
}


void
EmptyArtButton::Draw(BView* view, BRect square)
{
	if (!IsActive())
		return;

	float iconSize = 0;
	BRect rect = _ButtonRect(view, square, &iconSize);

	// A translucent white chip keeps the icon and label readable on
	// whatever is behind them; darker while pressed.
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	view->SetHighColor(fPressed ? make_color(255, 255, 255, 110)
		: make_color(255, 255, 255, 170));
	view->FillRoundRect(rect, kCornerRadius, kCornerRadius);
	view->SetHighColor(make_color(40, 40, 40, 90));
	view->StrokeRoundRect(rect, kCornerRadius, kCornerRadius);

	float top = rect.top + kPadding;
	if (iconSize > 0) {
		BRect iconRect(0, 0, iconSize - 1.0f, iconSize - 1.0f);
		iconRect.OffsetTo(floorf(rect.left + (rect.Width() + 1.0f - iconSize)
			/ 2.0f), top);
		view->DrawBitmap(fIcon, fIcon->Bounds(), iconRect,
			B_FILTER_BITMAP_BILINEAR);
		top += iconSize + kLabelGap;
	}
	view->SetDrawingMode(B_OP_COPY);

	BString label(fLabel);
	BFont font;
	view->GetFont(&font);
	font.TruncateString(&label, B_TRUNCATE_END,
		rect.Width() + 1.0f - (2 * kPadding));

	font_height fontHeight;
	view->GetFontHeight(&fontHeight);
	view->SetHighColor(make_color(40, 40, 40, 255));
	view->DrawString(label.String(), BPoint(
		floorf(rect.left + (rect.Width() + 1.0f
			- view->StringWidth(label.String())) / 2.0f),
		top + fontHeight.ascent));
}


bool
EmptyArtButton::MouseDown(BView* view, BPoint where, BRect square)
{
	if (!IsActive() || !_ButtonRect(view, square, NULL).Contains(where))
		return false;

	fPressed = true;
	// So the release arrives here even if the pointer has left the view.
	view->SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	view->Invalidate();
	return true;
}


bool
EmptyArtButton::MouseUp(BView* view, BPoint where, BRect square)
{
	if (!fPressed)
		return false;

	fPressed = false;
	view->Invalidate();

	if (IsActive() && view->Window() != NULL
			&& _ButtonRect(view, square, NULL).Contains(where)) {
		view->Window()->PostMessage(new BMessage(*fMessage));
	}
	return true;
}


} // namespace tagkit

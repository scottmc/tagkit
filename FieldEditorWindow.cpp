/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// FieldEditorWindow.cpp
//
// Part of "tagkit". See FieldEditorWindow.h.

#include "FieldEditorWindow.h"

#include <GroupLayout.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <Screen.h>
#include <TextControl.h>
#include <TextView.h>

#include "GenreList.h"
#include "TagKitStrings.h"
#include "TagRecord.h"


namespace tagkit {

static const float kMinWidth = 280;


FieldEditorWindow::FieldEditorWindow(BMessenger target, const BMessage& result,
	const char* label, const char* initialText, BPoint screenPosition,
	float width, const std::vector<BString>* choices,
	const std::vector<BString>* recent, bool mixed)
	:
	BWindow(BRect(0, 0, 100, 50), TAGKIT_TR("Edit"), B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS),
	fTarget(target),
	fResult(result),
	fLabel(label),
	fInitialText(initialText != NULL ? initialText : ""),
	fTextControl(NULL),
	fMenuField(NULL),
	fChoice(initialText != NULL ? initialText : ""),
	fWantedWidth(width > kMinWidth ? width : kMinWidth),
	fUsesChoices(choices != NULL),
	fMixed(mixed),
	fFinished(false)
{
	SetTitle(fill_in(TAGKIT_TR("Edit %field%"), "%field%", label).String());

	BString controlLabel(label);
	controlLabel << ":";

	if (choices != NULL) {
		BPopUpMenu* menu = new BPopUpMenu("choices");
		menu->SetLabelFromMarked(true);
		_BuildChoiceMenu(menu, *choices, recent, initialText);

		fMenuField = new BMenuField("fieldEditor", controlLabel.String(),
			menu);
		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.Add(fMenuField)
			.End();
	} else {
		fTextControl = new BTextControl("fieldEditor", controlLabel.String(),
			fInitialText.String(), new BMessage(kMsgFieldEditorCommit));
		fTextControl->SetTarget(this);

		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.Add(fTextControl)
			.End();
	}

	_PlaceWindow(screenPosition);

	if (fTextControl != NULL) {
		fTextControl->MakeFocus(true);
		fTextControl->TextView()->SelectAll();
	}
}


void
FieldEditorWindow::_PlaceWindow(BPoint screenPosition)
{
	// Wide enough for the cell it replaces (but never cramped), placed
	// with the text field around the pointer and kept on screen.
	ResizeTo(fWantedWidth, Bounds().Height());
	ResizeToPreferred();
	if (Bounds().Width() < fWantedWidth)
		ResizeTo(fWantedWidth, Bounds().Height());

	BRect screenFrame = BScreen(this).Frame();
	BPoint position(screenPosition.x - 24, screenPosition.y - Bounds().Height()
		/ 2.0f - 8);
	if (position.x + Bounds().Width() > screenFrame.right)
		position.x = screenFrame.right - Bounds().Width();
	if (position.y + Bounds().Height() > screenFrame.bottom)
		position.y = screenFrame.bottom - Bounds().Height();
	if (position.x < screenFrame.left)
		position.x = screenFrame.left;
	if (position.y < screenFrame.top)
		position.y = screenFrame.top;
	MoveTo(position);
}


void
FieldEditorWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgFieldEditorCommit:
			_Accept(true);
			break;

		case kMsgFieldEditorPick:
		{
			const char* value = "";
			message->FindString("value", &value);
			fChoice = value;
			_Accept(true);
			break;
		}

		case kMsgFieldEditorOther:
			_SwitchToText();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
FieldEditorWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	// Escape cancels. (The text field would otherwise just swallow it.)
	if (message->what == B_KEY_DOWN) {
		int8 byte = 0;
		if (message->FindInt8("byte", &byte) == B_OK && byte == B_ESCAPE) {
			_Cancel();
			return;
		}
	}

	BWindow::DispatchMessage(message, handler);
}


void
FieldEditorWindow::WindowActivated(bool active)
{
	BWindow::WindowActivated(active);

	if (active)
		return;

	if (fUsesChoices) {
		// Nothing was picked: leave the value alone.
		_Cancel();
		return;
	}

	// Clicking anywhere else accepts what was typed.
	_Accept(false);
}


bool
FieldEditorWindow::QuitRequested()
{
	BMessage closed(kMsgFieldEditorClosed);
	closed.AddPointer("editor", this);
	fTarget.SendMessage(&closed);
	return true;
}


void
FieldEditorWindow::_Cancel()
{
	if (fFinished)
		return;
	fFinished = true;
	PostMessage(B_QUIT_REQUESTED);
}


void
FieldEditorWindow::_Accept(bool explicitly)
{
	if (fFinished)
		return;

	BString text(fUsesChoices ? fChoice.String() : fTextControl->Text());

	// A blank for a field the files disagree on is almost surely just the
	// editor losing focus untouched, not a wish to clear it everywhere.
	if (!explicitly && fMixed && text.Length() == 0) {
		_Cancel();
		return;
	}

	fFinished = true;

	BMessage result(fResult);
	result.AddString("value", text.String());
	fTarget.SendMessage(&result);

	PostMessage(B_QUIT_REQUESTED);
}


void
FieldEditorWindow::_SwitchToText()
{
	// Swap the drop-down for a text field, keeping the window's place.
	BGroupLayout* layout = dynamic_cast<BGroupLayout*>(GetLayout());
	if (fMenuField == NULL || layout == NULL)
		return;

	layout->RemoveView(fMenuField);
	if (fMenuField->Parent() != NULL)
		fMenuField->RemoveSelf();
	delete fMenuField;
	fMenuField = NULL;
	fUsesChoices = false;

	BString controlLabel(fLabel);
	controlLabel << ":";
	fTextControl = new BTextControl("fieldEditor", controlLabel.String(),
		fInitialText.String(), new BMessage(kMsgFieldEditorCommit));
	fTextControl->SetTarget(this);
	layout->AddView(fTextControl);

	BRect frame = Frame();
	InvalidateLayout();
	ResizeTo(fWantedWidth, Bounds().Height());
	ResizeToPreferred();
	if (Bounds().Width() < fWantedWidth)
		ResizeTo(fWantedWidth, Bounds().Height());
	MoveTo(frame.LeftTop());

	fTextControl->MakeFocus(true);
	fTextControl->TextView()->SelectAll();
}


void
FieldEditorWindow::_BuildChoiceMenu(BMenu* menu,
	const std::vector<BString>& choices, const std::vector<BString>* recent,
	const char* initialText)
{
	BString current(initialText != NULL ? initialText : "");
	bool marked = false;

	BMessage* noneMessage = new BMessage(kMsgFieldEditorPick);
	noneMessage->AddString("value", "");
	BMenuItem* none = new BMenuItem(TAGKIT_TR("(none)"), noneMessage);
	menu->AddItem(none);
	if (current.Length() == 0) {
		none->SetMarked(true);
		marked = true;
	}
	menu->AddSeparatorItem();

	// A current value that isn't in the list stays selectable.
	bool inList = false;
	for (size_t i = 0; i < choices.size(); i++) {
		if (choices[i].ICompare(current) == 0) {
			inList = true;
			break;
		}
	}
	if (!marked && !inList) {
		BMessage* message = new BMessage(kMsgFieldEditorPick);
		message->AddString("value", current.String());
		BMenuItem* item = new BMenuItem(current.String(), message);
		item->SetMarked(true);
		menu->AddItem(item);
		menu->AddSeparatorItem();
		marked = true;
	}

	// The recent picks, newest first, then a line before the full list.
	if (recent != NULL && !recent->empty()) {
		for (size_t i = 0; i < recent->size(); i++) {
			BMessage* message = new BMessage(kMsgFieldEditorPick);
			message->AddString("value", (*recent)[i].String());
			BMenuItem* item = new BMenuItem((*recent)[i].String(), message);
			if (!marked && (*recent)[i].ICompare(current) == 0) {
				item->SetMarked(true);
				marked = true;
			}
			menu->AddItem(item);
		}
		menu->AddSeparatorItem();
	}

	for (size_t i = 0; i < choices.size(); i++) {
		BMessage* message = new BMessage(kMsgFieldEditorPick);
		message->AddString("value", choices[i].String());
		BMenuItem* item = new BMenuItem(choices[i].String(), message);
		if (!marked && choices[i].ICompare(current) == 0) {
			item->SetMarked(true);
			marked = true;
		}
		menu->AddItem(item);
	}

	// For a value that isn't in any list.
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(TAGKIT_TR("Other" B_UTF8_ELLIPSIS),
		new BMessage(kMsgFieldEditorOther)));

	menu->SetTargetForItems(this);
}


FieldEditorWindow*
new_field_editor(BMessenger target, const BMessage& result, tag_field field,
	const std::vector<const TagRecord*>& records, BPoint screenPosition,
	float width, const std::vector<BString>* recentGenres)
{
	// If the files disagree the editor starts out blank (and knows not to
	// accept that blank when the user just clicks away).
	bool mixed = false;
	BString value = common_tag_field_value(records, field, &mixed);

	bool isGenre = field == TAG_FIELD_GENRE;
	return new FieldEditorWindow(target, result, tag_field_label(field),
		value.String(), screenPosition, width,
		isGenre ? &genre_names() : NULL, isGenre ? recentGenres : NULL, mixed);
}


} // namespace tagkit

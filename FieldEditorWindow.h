/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott McCreary
 *		Claude (Anthropic), coding
 */
// FieldEditorWindow.h
//
// Part of "tagkit".

#ifndef TAGKIT_FIELD_EDITOR_WINDOW_H
#define TAGKIT_FIELD_EDITOR_WINDOW_H

#include <vector>

#include <Messenger.h>
#include <Point.h>
#include <String.h>
#include <Window.h>

class BGroupLayout;
class BMenu;
class BMenuField;
class BTextControl;


namespace tagkit {

// The BMessage `what` codes the editor uses. Only kMsgFieldEditorClosed
// is of interest to the program using it; the others are the editor's own.
enum {
	kMsgFieldEditorCommit	= 'fECm',	// (inside) Return pressed
	kMsgFieldEditorPick		= 'fEPk',	// (inside) menu entry chosen
	kMsgFieldEditorOther	= 'fEOt',	// (inside) "Other..." chosen
	kMsgFieldEditorClosed	= 'fECl'	// sent to the target when the window
									// is going away ("editor": the window)
};


// A small window with one text field, opened where the user
// right-clicked a tag field (in either view) to edit its value. Return
// (or clicking anywhere else) accepts the text; Escape cancels.
//
// With a list of choices (the genre list, say) the text field is replaced
// by a drop-down menu of them: picking one accepts it straight away.
// Recently picked choices (if given) come first, set off from the full list
// by a separator line. A "(none)" entry at the top clears the value, and a
// current value that isn't in the list is kept as an entry of its own so
// opening the editor never loses it. "Other..." at the bottom turns the
// drop-down into a text field, for a value that isn't in the list; Return
// or clicking elsewhere then accepts it as usual.
//
// When the value being edited stands for several files that don't agree
// (mixed), the editor starts out blank. Then clicking elsewhere with
// nothing typed cancels instead of accepting a blank, so that a stray click
// can't clear the field of every file; pressing Return in the blank field,
// or picking "(none)", still does.
//
// Accepting sends a copy of the message given at construction to the
// target, with the text added as "value" -- the caller puts whatever it
// needs to find its way back (which row, which field) in that message.
// Cancelling sends nothing. Either way, once the window is gone it sends
// kMsgFieldEditorClosed (with this window as "editor") so the caller can
// drop its pointer.
class FieldEditorWindow : public BWindow {
public:
								FieldEditorWindow(BMessenger target,
									const BMessage& result,
									const char* label,
									const char* initialText,
									BPoint screenPosition, float width,
									const std::vector<BString>* choices
										= NULL,
									const std::vector<BString>* recent
										= NULL,
									bool mixed = false);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				DispatchMessage(BMessage* message,
									BHandler* handler);
	virtual	void				WindowActivated(bool active);
	virtual	bool				QuitRequested();

private:
			// explicitly: Return or a menu pick, as opposed to clicking
			// elsewhere.
			void				_Accept(bool explicitly);
			void				_Cancel();
			void				_SwitchToText();
			void				_PlaceWindow(BPoint screenPosition);
			void				_BuildChoiceMenu(BMenu* menu,
									const std::vector<BString>& choices,
									const std::vector<BString>* recent,
									const char* initialText);

			BMessenger			fTarget;
			BMessage			fResult;
			BString				fLabel;
			BString				fInitialText;
			BTextControl*		fTextControl;
			BMenuField*			fMenuField;
			BString				fChoice;	// drop-down mode: current pick
			float				fWantedWidth;
			bool				fUsesChoices;
			bool				fMixed;
			bool				fFinished;	// accepted or cancelled already
};

} // namespace tagkit

#endif // TAGKIT_FIELD_EDITOR_WINDOW_H

/*
 * Copyright 2015 Vale Tolpegin <valetolpegin@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GnuplotViewer.h"

#include <View.h>
#include <Application.h>
#include <Button.h>
#include <BitmapStream.h>
#include <TextView.h>
#include <Bitmap.h>
#include <Alert.h>
#include <Directory.h>
#include <Path.h>
#include <File.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <stdlib.h>
#include <Roster.h>
#include <NodeMonitor.h>
#include <SupportDefs.h>
#include <NodeInfo.h>
#include <LayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ControlLook.h>
#include <private/interface/AboutWindow.h>

enum
{
	GENERATE_PLOT = 'gnpt',
	MSG_SAVE_PANEL = 'mgsp',
	MSG_OUTPUT_TYPE = 'BTMN',
	SAVE_PLOT = 'svas',
	SHOW_ABOUT = 'swat'
};

const char* kTypeField = "be:type";
const char* kTranslatorField = "be:translator";


GnuplotViewer::GnuplotViewer(void)
	: BWindow(BRect(50,50,1000,600), "HaikuPlot", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fSavePanel(NULL)
{
	_BuildLayout();

	BMessenger msgr(NULL, this);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &msgr, NULL, 0, false);
}


void
GnuplotViewer::_BuildLayout()
{
	BRect r(Bounds());
	r.bottom = 20;

	BMenuBar *fMenuBar = new BMenuBar(r, "menubar");
	BMenu *fFileMenu = new BMenu("File");
	BMenu *fMenuSaveAs = new BMenu("Export plot as", B_ITEMS_IN_COLUMN);
	BMenu *fSettingsMenu = new BMenu("Settings");

	BTranslationUtils::AddTranslationItems(fMenuSaveAs,
		B_TRANSLATOR_BITMAP);
	fFileMenu->AddItem(fMenuSaveAs);

	fMenuBar->AddItem(fFileMenu);

	fSettingsMenu->AddItem(new BMenuItem("About", new BMessage(SHOW_ABOUT),
		'A', B_COMMAND_KEY));

	fMenuBar->AddItem(fSettingsMenu);

	fPictureView = new BView(BRect(0,20,650,600), "picture_view",
		B_FOLLOW_NONE, B_WILL_DRAW);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fMenuBar)
		.Add(fPictureView);
}


bool
GnuplotViewer::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
GnuplotViewer::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped())
	{
		entry_ref ref;
		if (msg->FindRef("refs", &ref) != B_OK)
			return;

		GeneratePlot(ref);
		return;
	}

	switch (msg->what)
	{
		case GENERATE_PLOT:
		{
			fOpenPanel->Show();
			loading_plot = false;

			break;
		}
		case MSG_OUTPUT_TYPE:
		{
			if (!fSavePanel)
				_SaveAs(msg);

			break;
		}
		case MSG_SAVE_PANEL:
		{
			_SaveToFile(msg);

			break;
		}
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (msg->FindRef("refs", &ref) != B_OK)
				break;

			if (loading_plot)
				LoadPlot(ref);
			else
				GeneratePlot(ref);

			break;
		}
		case B_NODE_MONITOR:
		{
			HandleNodeMonitoring(msg);

			break;
		}
		case SHOW_ABOUT:
		{
			BAboutWindow *about = new BAboutWindow("HaikuPlot",
				"application/x-vnd.haikuplot");

			about->AddDescription("A GUI interface to the popular command line graphing tool gnuplot");
			about->AddCopyright(2015, "Vale Tolpegin");

			about->Show();

			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}


void
GnuplotViewer::HandleNodeMonitoring(BMessage *msg)
{
	int32 opcode;
	if (msg->FindInt32("opcode", &opcode) != B_OK)
		return;

	switch (opcode)
	{
		case B_STAT_CHANGED:
		{
			GeneratePlot(fRef);

			break;
		}
		default:
		{
			break;
		}
	}
}


void
GnuplotViewer::GeneratePlot(const entry_ref &ref)
{
	PrepareNodeMonitoring(ref);

	BEntry entry(&ref, true);
	entry_ref real_ref;
	entry.GetRef(&real_ref);

	BFile file(&real_ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	BPath path(&real_ref);

	BString *command = new
		BString("gnuplot-x86 -e 'set output \"outpic.png\"' ");
	command->Append(path.Path());

	if (system(command->String()) == 0)
	{
		BEntry entry("outpic.png");
		entry_ref pic_ref;
		entry.GetRef(&pic_ref);

		LoadPlot(pic_ref);
	}
}


void
GnuplotViewer::PrepareNodeMonitoring(const entry_ref &ref)
{
	BFile file(&ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	BEntry entry(&ref, true);
	entry.GetRef(&fRef);
	entry.GetNodeRef(&fNodeRef);

	watch_node(&fNodeRef, B_WATCH_STAT, this);
}


void
GnuplotViewer::LoadPlot(const entry_ref &ref)
{
	fPictureView->ClearViewBitmap();

	BEntry entry(&ref, true);
	entry_ref real_ref;
	entry.GetRef(&real_ref);

	BFile file(&real_ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	BPath path(&real_ref);
	fPictureBitmap = BTranslationUtils::GetBitmap(path.Path());

	fPictureView->SetViewBitmap(fPictureBitmap,
		B_FOLLOW_TOP | B_FOLLOW_LEFT, 0);
	fPictureView->ResizeTo(fPictureBitmap->Bounds().Width(),
		fPictureBitmap->Bounds().Height());
}


void
GnuplotViewer::_SaveAs(BMessage* message)
{
	// Read the translator and output type the user chose
	translator_id outTranslator;
	uint32 outType;
	if (message->FindInt32(kTranslatorField,
			reinterpret_cast<int32 *>(&outTranslator)) != B_OK
		|| message->FindInt32(kTypeField,
			reinterpret_cast<int32 *>(&outType)) != B_OK)
		return;

	// Add the chosen translator and output type to the
	// message that the save panel will send back
	BMessage panelMsg(MSG_SAVE_PANEL);
	panelMsg.AddInt32(kTranslatorField, outTranslator);
	panelMsg.AddInt32(kTypeField, outType);

	// Create save panel and show it
	BMessenger target(this);
	fSavePanel = new (std::nothrow) BFilePanel(B_SAVE_PANEL,
		&target, NULL, 0, false, &panelMsg);
	if (!fSavePanel)
		return;

	fSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
	fSavePanel->Show();
}


void
GnuplotViewer::_SaveToFile(BMessage* message)
{
	// Read in where the file should be saved
	entry_ref dirRef;
	if (message->FindRef("directory", &dirRef) != B_OK)
		return;

	const char* filename;
	if (message->FindString("name", &filename) != B_OK)
		return;

	// Read in the translator and type to be used
	// to save the output image
	translator_id outTranslator;
	uint32 outType;
	if (message->FindInt32(kTranslatorField,
			reinterpret_cast<int32 *>(&outTranslator)) != B_OK
		|| message->FindInt32(kTypeField,
			reinterpret_cast<int32 *>(&outType)) != B_OK)
		return;

	// Find the translator_format information needed to
	// write a MIME attribute for the image file
	BTranslatorRoster* roster = BTranslatorRoster::Default();
	const translation_format* outFormat = NULL;
	int32 outCount = 0;
	if (roster->GetOutputFormats(outTranslator, &outFormat, &outCount) != B_OK
		|| outCount < 1)
		return;

	int32 i;
	for (i = 0; i < outCount; i++) {
		if (outFormat[i].group == B_TRANSLATOR_BITMAP && outFormat[i].type
				== outType)
			break;
	}
	if (i == outCount)
		return;

		// Write out the image file
	BDirectory dir(&dirRef);
	SaveToFile(&dir, filename, NULL, &outFormat[i]);
}


void
GnuplotViewer::SaveToFile(BDirectory* dir, const char* name,
	BBitmap* bitmap, const translation_format* format)
{
	if (bitmap == NULL) {
		// If no bitmap is supplied, write out the whole image
		bitmap = fPictureBitmap;
	}

	BBitmapStream stream(bitmap);

	bool loop = true;
	while (loop) {
		BTranslatorRoster* roster = BTranslatorRoster::Default();
		if (!roster)
			break;
		// write data
		BFile file(dir, name, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() != B_OK)
			break;
		if (roster->Translate(&stream, NULL, NULL, &file, format->type) < B_OK)
			break;
		// set mime type
		BNodeInfo info(&file);
		if (info.InitCheck() == B_OK)
			info.SetType(format->MIME);

		loop = false;
			// break out of loop gracefully (indicates no errors)
	}

	stream.DetachBitmap(&bitmap);
		// Don't allow the bitmap to be deleted, this is
		// especially important when using fBitmap as the bitmap
}

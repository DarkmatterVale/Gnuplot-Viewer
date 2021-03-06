/*
 * Copyright 2015 Vale Tolpegin <valetolpegin@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef GNUPLOTVIEWER_H
#define GNUPLOTVIEWER_H

#include <Window.h>

#include <MenuBar.h>
#include <String.h>
#include <FilePanel.h>
#include <Entry.h>
#include <Directory.h>
#include <Bitmap.h>
#include <Node.h>
#include <TranslationUtils.h>
#include <TranslationDefs.h>
#include <TranslatorRoster.h>

class GnuplotViewer : public BWindow
{
	public:
		GnuplotViewer(void);

		bool QuitRequested(void);
		void MessageReceived(BMessage *msg);

	private:
		void _BuildLayout(void);

		void LoadPlot(const entry_ref &ref);
		void LoadPlot(void);
		void GeneratePlot(const entry_ref &ref);

		void PrepareNodeMonitoring(const entry_ref &ref);
		void HandleNodeMonitoring(BMessage *msg);

		void _SaveAs(BMessage *message);
		void _SaveToFile(BMessage *message);
		void SaveToFile(BDirectory* dir, const char* name, BBitmap* bitmap,
			const translation_format* format);

		BFilePanel *fOpenPanel;
		BFilePanel *fSavePanel;
		BBitmap *fPictureBitmap;
		BView *fPictureView;

		bool loading_plot;

		entry_ref fRef;
		node_ref fNodeRef;
};

#endif

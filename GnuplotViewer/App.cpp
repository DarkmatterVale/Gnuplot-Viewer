/*
 * Copyright 2015 Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Application.h>
#include <OS.h>
#include <stdlib.h>

#include "GnuplotViewer.h"

class App : public BApplication
{
	public:
		App(void);
};

App::App(void)
	: BApplication("application/x-vnd.gnuplotviewer")
{
	GnuplotViewer *mainWin = new GnuplotViewer();
	mainWin->Show();
}

int main(void)
{
	App app;
	app.Run();
	return 0;
}

/*
 *
 * $Id$
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kstdguiitem.h>

#include <qfile.h>
#include <qtimer.h>

#include <stdlib.h>

#include "k3bapplication.h"
#include "k3bsplash.h"
#include <k3bglobals.h>



static const char *description =
    I18N_NOOP("K3b is a CD burning program that has two aims:\nusability and as many features as possible.");


static KCmdLineOptions options[] =
    {
        { "+[File]", I18N_NOOP("file to open"), 0 },
        { "data", I18N_NOOP("Create a new data project and add all given files"), 0 },
        { "audio", I18N_NOOP("Create a new audio project and add all given files"), 0 },
        { "vcd", I18N_NOOP("Create a new video cd project and add all given files"), 0 },
        { "mixed", I18N_NOOP("Create a new mixed mode project and add all given files"), 0 },
        { "emovix", I18N_NOOP("Create a new eMovix project and add all given files"), 0 },
        { "copy", I18N_NOOP("Open the cd copy dialog"), 0 },
        { "isoimage", I18N_NOOP("Write an ISO image to cd"), 0 },
        { "binimage", I18N_NOOP("Write an Bin/Cue image to cd"), 0 },
	{ "erase", I18N_NOOP("Erase a CDRW"), 0 },
        { 0, 0, 0 }
        // INSERT YOUR COMMANDLINE OPTIONS HERE
    };

int main(int argc, char *argv[]) {

    KAboutData aboutData( "k3b", I18N_NOOP("K3b"),
                          "0.9", description, KAboutData::License_GPL,
                          I18N_NOOP("(c) 1999 - 2003, Sebastian Trueg and the K3b Team"), 0, "http://www.k3b.org" );
    aboutData.addAuthor("Sebastian Trueg",I18N_NOOP("Maintainer"), "trueg@k3b.org");
    aboutData.addAuthor("Thomas Froescher",I18N_NOOP("Video-ripping and encoding"), "tfroescher@k3b.org");
    aboutData.addAuthor("Christian Kvasny",I18N_NOOP("VCD Project"), "chris@k3b.org");
    aboutData.addAuthor("Klaus-Dieter Krannich", I18N_NOOP("Developer "), "kd@k3b.org" );

    aboutData.addCredit("Ayo",
			I18N_NOOP("For his bombastic artwork."),
			"73lab@free.fr" );
    aboutData.addCredit("Christoph Thielecke",
			I18N_NOOP("For extensive testing and the first German translation."),
			"crissi99@gmx.de");
    aboutData.addCredit("Joerg Schilling",
			I18N_NOOP("For his great cdrtools and the patient answers to all my questions.") );


    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    K3bApplication app;


    //   if (app.isRestored())
    //     {
    //       RESTORE(K3bMainWindow);
    //     }
    //   else
    //     {

    if( !QFile::exists( K3b::globalConfig() ) ) {
      if( KMessageBox::warningYesNo( 0, i18n("It appears that you have not run K3bSetup yet. "
					     "It is recommended to do so. "
					     "Should K3bSetup be started?"),
				     i18n("K3b Setup"), KStdGuiItem::yes(), KStdGuiItem::no(),
				     i18n("Don't prompt me again.") ) == KMessageBox::Yes ) {
	KProcess p;
	p << "kdesu" << "k3bsetup --lang " + KGlobal::locale()->language();
	if( !p.start( KProcess::DontCare ) )
	  KMessageBox::error( 0, i18n("Could not find kdesu to run K3bSetup with root privileges. "
				      "Please run it manually as root.") );
	exit(0);
      }
    }

    app.config()->setGroup( "General Options" );
    K3bSplash* splash = 0;
    if( app.config()->readBoolEntry("Show splash", true) ) {
        splash = new K3bSplash( 0 );
        splash->connect( &app, SIGNAL(initializationInfo(const QString&)), SLOT(addInfo(const QString&)) );

        // kill the splash after 5 seconds
        QTimer::singleShot( 5000, splash, SLOT(close()) );

        splash->show();
    }

    // this will init everything
    app.init();

    return app.exec();
}

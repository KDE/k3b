
/*
 *
 * $Id$
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include <k3process.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ksimpleconfig.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <kurl.h>

#include <qfile.h>
#include <q3cstring.h>
#include <qdatastream.h>
#include <qtimer.h>

#include <stdlib.h>

#include "k3bapplication.h"
#include <k3bglobals.h>
#include <k3bcore.h>


#include <config-k3b.h>

static const char* description = I18N_NOOP("A CD and DVD burning application");


static KCmdLineOptions options[] =
    {
        { "+[URL(s)]", I18N_NOOP("file(s) to open"), 0 },
        { "data", I18N_NOOP("Create a new data CD project and add all given files"), 0 },
        { "datacd", I18N_NOOP("Create a new data CD project and add all given files (DEPRECATED. Use --data)"), 0 },
        { "audiocd", I18N_NOOP("Create a new audio CD project and add all given files"), 0 },
        { "videocd", I18N_NOOP("Create a new video CD project and add all given files"), 0 },
        { "mixedcd", I18N_NOOP("Create a new mixed mode CD project and add all given files"), 0 },
        { "emovix", I18N_NOOP("Create a new eMovix CD project and add all given files"), 0 },
        { "emovixcd", I18N_NOOP("Create a new eMovix CD project and add all given files (DEPRECATED. Use --emovix)"), 0 },
        { "datadvd", I18N_NOOP("Create a new data DVD project and add all given files (DEPRECATED. Use --data)"), 0 },
        { "emovixdvd", I18N_NOOP("Create a new eMovix DVD project and add all given files (DEPRECATED. Use --emovix)"), 0 },
        { "videodvd", I18N_NOOP("Create a new Video DVD project and add all given files"), 0 },
        { "burn", I18N_NOOP("Open the project burn dialog for the current project"), 0 },
        { "copy <device>", I18N_NOOP("Open the copy dialog, optionally specify the source device"), 0 },
        { "copycd <device>", I18N_NOOP("Open the CD copy dialog, optionally specify the source device (DEPRECATED: Use --copy)"), 0 },
        { "copydvd <device>", I18N_NOOP("Open the DVD copy dialog (DEPRECATED: Use --copy)"), 0 },
        { "cdimage <url>", I18N_NOOP("Write a CD image to a CD-R(W)"), 0 },
        { "dvdimage <url>", I18N_NOOP("Write a DVD ISO9660 image to a DVD"), 0 },
        { "image <url>", I18N_NOOP("Write a CD or DVD image to a CD-R(W) or DVD depending on the size"), 0 },
	{ "format <device>", I18N_NOOP("Format a rewritable medium"), 0 },
	{ "erasecd <device>", I18N_NOOP("Erase a CDRW (DEPRECATED: Use --format)"), 0 },
	{ "formatdvd <device>", I18N_NOOP("Format a DVD-RW or DVD+RW (DEPRECATED: Use --format)"), 0 },
	{ "cddarip <device>", I18N_NOOP("Extract Audio tracks digitally (+encoding)"), 0 },
	{ "videodvdrip <device>", I18N_NOOP("Rip Video DVD Titles (+transcoding)"), 0 },
	{ "videocdrip <device>", I18N_NOOP("Rip Video CD Tracks"), 0 },
	{ "lang <language>", I18N_NOOP("Set the GUI language"), 0 },
	{ "nosplash", I18N_NOOP("Disable the splash screen"), 0 },
	{ "ao <method>", I18N_NOOP("Set the audio output method (like arts or alsa depending on the installed plugins)"), 0 },
        { "device <device>", I18N_NOOP( "Set the device to be used for new projects (This option has no effect. "
                                        "Its main purpose is to enable handling of empty media from the KDE Media Manager)." ), 0 }
    };

int main( int argc, char* argv[] )
{
  KAboutData aboutData( "k3b", ki18n("K3b"),
			LIBK3B_VERSION, description, KAboutData::License_GPL,
			ki18n("(c) 1999 - 2007, Sebastian Trüg"), KLocalizedString(), I18N_NOOP("http://www.k3b.org" ));
  aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Maintainer and Lead Developer"), "trueg@k3b.org");
  aboutData.addAuthor(ki18n("Christian Kvasny"),ki18n("VideoCD Project and VideoCD ripping"), "chris@k3b.org");
  aboutData.addCredit(ki18n("Klaus-Dieter Krannich"), ki18n("Advanced Cdrdao integration"), "kd@k3b.org" );

  aboutData.addCredit(ki18n("Thomas Froescher"),
		      ki18n("VideoDVD ripping and video encoding in pre-1.0 versions."),
		      "tfroescher@k3b.org");
  aboutData.addCredit(ki18n("Alexis Younes aka Ayo"),
		      ki18n("For his bombastic artwork."),
		      "73lab@free.fr" );
  aboutData.addCredit(ki18n("Christoph Thielecke"),
		      ki18n("For extensive testing and the first German translation."),
		      "crissi99@gmx.de");
  aboutData.addCredit(ki18n("Andy Polyakov"),
		      ki18n("For the great dvd+rw-tools and the nice cooperation."),
		      "appro@fy.chalmers.se" );
  aboutData.addCredit(ki18n("Roberto De Leo"),
		      ki18n("For the very cool eMovix package and his accommodating work."),
		      "peggish@users.sf.net" );
  aboutData.addCredit(ki18n("John Steele Scott"),
		      ki18n("For the flac decoding plugin."),
		      "toojays@toojays.net" );
  aboutData.addCredit(ki18n("György Szombathelyi"),
		      ki18n("For the very useful isofslib."),
		      "gyurco@users.sourceforge.net" );
  aboutData.addCredit(ki18n("Erik de Castro Lopo"),
		      ki18n("For libsamplerate which is used for generic resampling in the audio decoder framework."),
		      "erikd@mega-nerd.com" );
  aboutData.addCredit(ki18n("Jakob Petsovits"),
		      ki18n("For the very cool conditional audio ripping pattern."),
		      "jpetso@gmx.at" );
  aboutData.addCredit(ki18n("Heiner Eichmann"),
		      ki18n("For his work on the BSD port and some great patches."),
		      "h.eichmann@gmx.de" );
  aboutData.addCredit(ki18n("Adriaan De Groot"),
		      ki18n("For his work on the BSD port."),
		      "" );
  aboutData.addCredit(ki18n("Thiago Macieira"),
		      ki18n("For his help with the many invalid k3b entries on bugs.kde.org."),
		      "thiago@kde.org" );
  aboutData.addCredit(ki18n("Marcel Dierkes"),
		      ki18n("For the great K3b icon eyecandy."),
		      "marcel.dierkes@gmx.de" );
  aboutData.addCredit(ki18n("Christoph Burger-Scheidlin"),
		      ki18n("For his neverending help cleaning out the K3b bug database."),
		      "andersin@freenet.de" );
  aboutData.addCredit( ki18n("Robert Wadley"),
                       ki18n( "Rob created a great theme and came up with the idea for transparent themes." ),
                       "rob@robntina.fastmail.us" );
  aboutData.addCredit( ki18n("Dmitry Novikov"),
                       ki18n( "For the amazing K3b 1.0 theme." ),
                       "quant@trktvs.ru" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  if( K3bApplication::start() ) {
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    if( args->isSet("lang") )
    {
      QStringList lst;
      lst << args->getOption("lang");
      if( !KGlobal::locale()->setLanguage(lst) )
	kDebug() << "Unable to set to language " << args->getOption("lang")
		  << " current is: " << KGlobal::locale()->language() << endl;
    }
    K3bApplication app;

    // we need a running app for the init method
    QTimer::singleShot( 0, &app, SLOT(init()) );

    return app.exec();
  }
}

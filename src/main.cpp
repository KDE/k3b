
/*
 *
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


int main( int argc, char* argv[] )
{
  KAboutData aboutData( "k3b",0, ki18n("K3b"),
			LIBK3B_VERSION, ki18n(description), KAboutData::License_GPL,
			ki18n("(c) 1998 - 2009, Sebastian Trüg"), KLocalizedString(), I18N_NOOP("http://www.k3b.org" ));

  aboutData.addAuthor(ki18n("Sebastian Trüg"),ki18n("Maintainer and Lead Developer"), "trueg@k3b.org");
  aboutData.addAuthor(ki18n("Christian Kvasny"),ki18n("VideoCD Project and VideoCD ripping"), "chris@k3b.org");
  aboutData.addAuthor(ki18n("Montel Laurent"), ki18n("Port to kde4"), "montel@kde.org");
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

  KCmdLineOptions options;
  options.add("+[URL(s)]", ki18n("file(s) to open"));
  options.add("data", ki18n("Create a new data CD project and add all given files"));
  options.add("datacd", ki18n("Create a new data CD project and add all given files (DEPRECATED. Use --data)"));
  options.add("audiocd", ki18n("Create a new audio CD project and add all given files"));
  options.add("videocd", ki18n("Create a new video CD project and add all given files"));
  options.add("mixedcd", ki18n("Create a new mixed mode CD project and add all given files"));
  options.add("emovix", ki18n("Create a new eMovix CD project and add all given files"));
  options.add("emovixcd", ki18n("Create a new eMovix CD project and add all given files (DEPRECATED. Use --emovix)"));
  options.add("datadvd", ki18n("Create a new data DVD project and add all given files (DEPRECATED. Use --data)"));
  options.add("emovixdvd", ki18n("Create a new eMovix DVD project and add all given files (DEPRECATED. Use --emovix)"));
  options.add("videodvd", ki18n("Create a new Video DVD project and add all given files"));
  options.add("burn", ki18n("Open the project burn dialog for the current project"));
  options.add("copy <device>", ki18n("Open the copy dialog, optionally specify the source device"));
  options.add("copycd <device>", ki18n("Open the CD copy dialog, optionally specify the source device (DEPRECATED: Use --copy)"));
  options.add("copydvd <device>", ki18n("Open the DVD copy dialog (DEPRECATED: Use --copy)"));
  options.add("image <url>", ki18n("Write an image to a CD or DVD"));
  options.add("cdimage <url>", ki18n("Write a CD image to a CD (DEPRECATED: Use --image)"));
  options.add("dvdimage <url>", ki18n("Write a DVD ISO9660 image to a DVD (DEPRECATED: Use --image)"));
  options.add("format <device>", ki18n("Format a rewritable medium"));
  options.add("erasecd <device>", ki18n("Erase a CDRW (DEPRECATED: Use --format)"));
  options.add("formatdvd <device>", ki18n("Format a DVD-RW or DVD+RW (DEPRECATED: Use --format)"));
  options.add("cddarip <device>", ki18n("Extract Audio tracks digitally (+encoding)"));
  options.add("videodvdrip <device>", ki18n("Rip Video DVD Titles (+transcoding)"));
  options.add("videocdrip <device>", ki18n("Rip Video CD Tracks"));
  options.add("lang <language>", ki18n("Set the GUI language"));
  options.add("nosplash", ki18n("Disable the splash screen"));
  options.add("ao <method>", ki18n("Set the audio output method (like arts or alsa depending on the installed plugins)"));
  options.add("device <device>", ki18n( "Set the device to be used for new projects (This option has no effect. "
                                        "Its main purpose is to enable handling of empty media from the KDE Media Manager)." ));


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

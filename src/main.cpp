/*
 *
 * $Id$
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
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
#include <ksimpleconfig.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <kurl.h>

#include <qfile.h>
#include <qcstring.h>
#include <qdatastream.h>

#include <stdlib.h>

#include "k3bapplication.h"
#include <k3bglobals.h>


#include <config.h>


static const char *description = I18N_NOOP("A CD and DVD burning application");


static KCmdLineOptions options[] =
    {
        { "+[URL(s)]", I18N_NOOP("file(s) to open"), 0 },
        { "datacd", I18N_NOOP("Create a new data CD project and add all given files"), 0 },
        { "audiocd", I18N_NOOP("Create a new audio CD project and add all given files"), 0 },
        { "videocd", I18N_NOOP("Create a new video CD project and add all given files"), 0 },
        { "mixedcd", I18N_NOOP("Create a new mixed mode CD project and add all given files"), 0 },
        { "emovixcd", I18N_NOOP("Create a new eMovix CD project and add all given files"), 0 },
        { "datadvd", I18N_NOOP("Create a new data DVD project and add all given files"), 0 },
        { "emovixdvd", I18N_NOOP("Create a new eMovix DVD project and add all given files"), 0 },
        { "videodvd", I18N_NOOP("Create a new Video DVD project and add all given files"), 0 },
        { "copycd", I18N_NOOP("Open the CD copy dialog"), 0 },
        { "copydvd", I18N_NOOP("Open the DVD copy dialog"), 0 },
        { "cdimage", I18N_NOOP("Write a CD image to a CD-R(W)"), 0 },
        { "dvdimage", I18N_NOOP("Write a DVD ISO9660 image to a DVD±R(W)"), 0 },
        { "image", I18N_NOOP("Write a CD or DVD image to a CD-R(W) or DVD±R(W) depending on the size"), 0 },
	{ "erasecd", I18N_NOOP("Erase a CDRW"), 0 },
	{ "formatdvd", I18N_NOOP("Format a DVD-RW or DVD+RW"), 0 },
	{ "lang <language>", I18N_NOOP("Set the GUI language"), 0 },
	{ "nosplash", I18N_NOOP("Disable the splash screen"), 0 },
	{ "ao <method>", I18N_NOOP("Set the audio output method (like arts or alsa depending on the installed plugins)"), 0 },
        KCmdLineLastOption
    };

int main( int argc, char* argv[] )
{
  KAboutData aboutData( "k3b", I18N_NOOP("K3b"),
			"0.11.98", description, KAboutData::License_GPL,
			I18N_NOOP("(c) 1999 - 2005, Sebastian Trüg and the K3b Team"), 0, "http://www.k3b.org" );
  aboutData.addAuthor("Sebastian Trüg",I18N_NOOP("Maintainer"), "trueg@k3b.org");
  aboutData.addAuthor("Thomas Froescher",I18N_NOOP("VideoDVD ripping and video encoding"), "tfroescher@k3b.org");
  aboutData.addAuthor("Christian Kvasny",I18N_NOOP("VideoCD Project and VideoCD ripping"), "chris@k3b.org");
  aboutData.addAuthor("Klaus-Dieter Krannich", I18N_NOOP("Developer"), "kd@k3b.org" );

  aboutData.addCredit("Ayo",
		      I18N_NOOP("For his bombastic artwork."),
		      "73lab@free.fr" );
  aboutData.addCredit("Christoph Thielecke",
		      I18N_NOOP("For extensive testing and the first German translation."),
		      "crissi99@gmx.de");
  aboutData.addCredit("Andy Polyakov",
		      I18N_NOOP("For the great dvd+rw-tools and the nice cooperation."),
		      "appro@fy.chalmers.se" );
  aboutData.addCredit("Roberto De Leo",
		      I18N_NOOP("For the very cool eMovix package and his accommodating work."),
		      "peggish@users.sf.net" );
  aboutData.addCredit("John Steele Scott",
		      I18N_NOOP("For the flac decoding plugin."),
		      "toojays@toojays.net" );
  aboutData.addCredit("György Szombathelyi",
		      I18N_NOOP("For the very useful isofslib."),
		      "gyurco@users.sourceforge.net" );
  aboutData.addCredit("Erik de Castro Lopo",
		      I18N_NOOP("For libsamplerate which is used for generic resampling in the audio decoder framework."),
		      "erikd@mega-nerd.com" );
  aboutData.addCredit("Jakob Petsovits",
		      I18N_NOOP("For the very cool conditional audio ripping pattern."),
		      "jpetso@gmx.at" );


  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  if( K3bApplication::start() ) {
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    if( args->isSet("lang") )
      if( !KGlobal::locale()->setLanguage(args->getOption("lang")) )
	kdDebug() << "Unable to set to language " << args->getOption("lang") 
		  << " current is: " << KGlobal::locale()->language() << endl;
  
    K3bApplication app;
    app.init();

    return app.exec();
  }
}

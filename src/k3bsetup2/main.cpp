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

/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Sun Aug 25 13:19:59 CEST 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kapplication.h>
#include <kmessagebox.h>

#include <unistd.h>
#include <stdlib.h>

#include "k3bsetup2.h"


static const char *description = I18N_NOOP("K3bSetup prepares the system for CD writing with K3b.");


static KCmdLineOptions options[] =
{
  { "lang <language>", I18N_NOOP("Specify a particular language"), 0 },
  { 0, 0, 0 }
  // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{
  KLocale::setMainCatalogue( "k3b" );

  KAboutData aboutData( "k3bsetup", I18N_NOOP("K3bSetup"),
    "2.0", description, KAboutData::License_GPL,
    "(c) 2002, Sebastian Trueg", 0, 0, "trueg@informatik.uni-freiburg.de");
  aboutData.addAuthor("Sebastian Trueg",0, "trueg@informatik.uni-freiburg.de");
  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KLocale::setMainCatalogue( "k3b" );

  KApplication a;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  if( args->isSet("lang") )
    KGlobal::locale()->setLanguage(args->getOption("lang"));

  if (getuid()) {
    KMessageBox::error(0, i18n("K3b Setup must be run as root!"));
    exit(1);
  }

  K3bSetup2 setup;

  a.setMainWidget(&setup);
  setup.show();
  setup.init();

  return a.exec();
}

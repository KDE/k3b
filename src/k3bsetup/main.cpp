/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Sat Dec  1 12:45:27 CET 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include "k3bsetup.h"
#include "k3bsetupwizard.h"

static const char *description =
	I18N_NOOP("K3bSetup");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE


static KCmdLineOptions options[] =
{
  { 0, 0, 0 }
  // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{
  KAboutData aboutData( "k3bsetup", I18N_NOOP("K3bSetup"),
    "0.1", description, KAboutData::License_GPL,
    "(c) 2001, Sebastian Trueg", 0, 0, "trueg@informatik.uni-freiburg.de");
  aboutData.addAuthor("Sebastian Trueg",0, "trueg@informatik.uni-freiburg.de");
  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication a;
  KLocale::setMainCatalogue( "k3b" );

  if (getuid()) {
    KMessageBox::error(0, i18n("K3b Setup must be run as root!"));
    exit(1);
  }

  K3bSetup setup;

  K3bSetupWizard *k3bsetup = new K3bSetupWizard( &setup );
  a.setMainWidget(k3bsetup);
  k3bsetup->show();

  return a.exec();
}

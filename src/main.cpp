/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
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
#include <krun.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kstdguiitem.h>

#include <qfile.h>
#include <qtimer.h>

#include <stdlib.h>

#include "k3b.h"
#include "k3bsplash.h"
#include "tools/k3bglobals.h"


static const char *description = 
I18N_NOOP("K3b is a CD burning program that has two aims:\nusability and as many features as possible.");
	
	
static KCmdLineOptions options[] =
  {
    { "+[File]", I18N_NOOP("file to open"), 0 },
    { 0, 0, 0 }
    // INSERT YOUR COMMANDLINE OPTIONS HERE
  };

int main(int argc, char *argv[])
{

  KAboutData aboutData( "k3b", I18N_NOOP("K3b"),
			"0.7", description, KAboutData::License_GPL,
			"(c) 2001, Sebastian Trueg", 0, 0, "trueg@informatik.uni-freiburg.de");
  aboutData.addAuthor("Sebastian Trueg",I18N_NOOP("Maintainer"), "trueg@informatik.uni-freiburg.de");
  aboutData.addAuthor("Thomas Froescher",I18N_NOOP("Developer"), "tfroescher@gmx.de");
  aboutData.addCredit("Ayo", I18N_NOOP("For his bombastic artwork."), "73lab@free.fr" );
  aboutData.addCredit("Crissi", I18N_NOOP("For extensive testing and the first German translation."), "crissi99@gmx.de");
  aboutData.addCredit("Joerg Schilling", I18N_NOOP("For his great cdrtools and the patient answers to all my questions.") );
	
  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication app;


  if( !QFile::exists( K3b::globalConfig() ) ) {
    if( KMessageBox::warningYesNo( 0, i18n("It seems as if you have not run K3bSetup yet. It is recommended to do so. "
					   "Should K3bSetup be started?"),
				   i18n("K3b Setup"), KStdGuiItem::yes(), KStdGuiItem::no(), 
				   i18n("Don't bother me again.") ) == KMessageBox::Yes ) {
      KRun::runCommand( "kdesu k3bsetup" );
      exit(0);
    }
  }
  


  if (app.isRestored())
    {
      RESTORE(K3bMainWindow);
    }
  else 
    {
      K3bMainWindow *k3bMainWidget = new K3bMainWindow();
      app.setMainWidget( k3bMainWidget );
      k3bMainWidget->initView();  // needs a kapp instance

      k3bMainWidget->config()->setGroup( "General Options" );
      if( k3bMainWidget->config()->readBoolEntry("Show splash", true) ) {
	K3bSplash* splash = new K3bSplash( k3bMainWidget );
	splash->connect( k3bMainWidget, SIGNAL(initializationInfo(const QString&)), SLOT(addInfo(const QString&)) );

	// kill the splash after 15 seconds
	QTimer::singleShot( 15000, splash, SLOT(close()) );

	splash->show();
      }

      k3bMainWidget->init();
      k3bMainWidget->show();

      KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		
      if (args->count())
	{
	  for(int i=0;i<args->count();i++)
	    {
	      k3bMainWidget->openDocumentFile(args->arg(i));
	    }
	}
		
      args->clear();
    }

  return app.exec();
}  

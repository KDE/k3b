/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bcore.h"
#include "k3bsystemproblemdialog.h"
#include "tools/k3btitlelabel.h"
#include <k3bexternalbinmanager.h>
#include <k3bstdguiitems.h>
#include <k3bexternalbinmanager.h>
#include <device/k3bdevicemanager.h>
#include <k3bversion.h>

#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <ktextedit.h>
#include <kconfig.h>
#include <kapplication.h>



K3bSystemProblem::K3bSystemProblem( int t,
				    const QString& p,
				    const QString& d,
				    const QString& s,
				    bool k )
  : type(t),
    problem(p),
    details(d),
    solution(s),
    solvableByK3bSetup(k)
{
}


K3bSystemProblemDialog::K3bSystemProblemDialog( const QValueList<K3bSystemProblem>& problems,
						QWidget* parent, 
						const char* name )
  : KDialog( parent, name )
{
  setCaption( i18n("System Configuration Problems") );

  // setup the title
  // ---------------------------------------------------------------------------------------------------
  QFrame* headerFrame = K3bStdGuiItems::purpleFrame( this );
  QHBoxLayout* layout4 = new QHBoxLayout( headerFrame ); 
  layout4->setMargin( 2 ); // to make sure the frame gets displayed
  layout4->setSpacing( 0 );
  QLabel* pixmapLabelLeft = new QLabel( headerFrame, "pixmapLabelLeft" );
  pixmapLabelLeft->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  pixmapLabelLeft->setPixmap( QPixmap(locate( "appdata", "pics/diskinfo_left.png" )) );
  pixmapLabelLeft->setScaledContents( FALSE );
  layout4->addWidget( pixmapLabelLeft );
  K3bTitleLabel* labelTitle = new K3bTitleLabel( headerFrame, "m_labelTitle" );
  labelTitle->setTitle( i18n("System Configuration Problems"), i18n("1 problem", "%n problems", problems.count() ) );
  labelTitle->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  layout4->addWidget( labelTitle );
  layout4->setStretchFactor( labelTitle, 1 );
  QLabel* pixmapLabelRight = new QLabel( headerFrame, "pixmapLabelRight" );
  pixmapLabelRight->setPaletteBackgroundColor( QColor( 205, 210, 255 ) );
  pixmapLabelRight->setPixmap( QPixmap(locate( "appdata", "pics/diskinfo_right.png" )) );
  pixmapLabelRight->setScaledContents( FALSE );
  layout4->addWidget( pixmapLabelRight );


  m_closeButton = new QPushButton( i18n("Close"), this );
  connect( m_closeButton, SIGNAL(clicked()), this, SLOT(close()) );
  m_checkDontShowAgain = new QCheckBox( i18n("Don't show again"), this );


  // setup the problem view
  // ---------------------------------------------------------------------------------------------------
  KTextEdit* view = new KTextEdit( this );
  view->setReadOnly(true);
  view->setTextFormat(RichText);


  // layout everything
  QGridLayout* grid = new QGridLayout( this );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );
  grid->addMultiCellWidget( headerFrame, 0, 0, 0, 1 );
  grid->addMultiCellWidget( view, 1, 1, 0, 1 );
  grid->addWidget( m_checkDontShowAgain, 2, 0 );
  grid->addWidget( m_closeButton, 2, 1 );
  grid->setColStretch( 0, 1 );
  grid->setRowStretch( 1, 1 );

  QString text = "<html>";

  for( QValueList<K3bSystemProblem>::const_iterator it = problems.begin();
       it != problems.end(); ++it ) {
    const K3bSystemProblem& p = *it;

    text.append( "<p><b>" );
    if( p.type == K3bSystemProblem::CRITICAL )
      text.append( "<span style=\"color:red\">" );
    text.append( p.problem );
    if( p.type == K3bSystemProblem::CRITICAL )
      text.append( "</span>" );
    text.append( "</b><br>" );
    text.append( p.details + "<br>" );
    text.append( "<i>" + i18n("Solution") + "</i>: " + p.solution );
    text.append( "</p>" );
  }

  text.append( "</html>" );

  view->setText(text);
  view->setCursorPosition(0,0);
  view->ensureCursorVisible();
}


void K3bSystemProblemDialog::closeEvent( QCloseEvent* e )
{
  if( m_checkDontShowAgain->isChecked() ) {
    kapp->config()->setGroup( "General Options" );
    kapp->config()->writeEntry( "check system config", false );
  }

  e->accept();
}


void K3bSystemProblemDialog::checkSystem()
{
  QValueList<K3bSystemProblem> problems;

  // 1. cdrecord, cdrdao
  if( !k3bcore->externalBinManager()->foundBin( "cdrecord" ) ) {
    problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
				       i18n("Unable to find %1 executable").arg("cdrecord"),
				       i18n("K3b uses cdrecord to actually write cds. "
					    "Without cdrecord K3b won't be able to properly "
					    "initialize the writing devices."),
				       i18n("Install the cdrtools package which contains "
					    "cdrecord."),
				       false ) );
  }
  else {
    if( k3bcore->externalBinManager()->binObject( "cdrecord" )->version < K3bVersion( 2, 0 ) )
      problems.append( K3bSystemProblem( K3bSystemProblem::NON_CRITICAL,
					 i18n("Used %1 version %2 is outdated").arg("cdrecord").arg(k3bcore->externalBinManager()->binObject( "cdrecord" )->version),
					 i18n("Although K3b supports all cdrtools versions since "
					      "1.10 it is highly recommended to at least use "
					      "version 2.0."),
					 i18n("Install a more recent version of the cdrtools."),
					 false ) );
    
    if( !k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "suidroot" ) )
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("%1 does not run with root privileges").arg("cdrecord"),
					 i18n("cdrecord needs to run with root privileges "
					      "to be able to access the cd devices, "
					      "use real time scheduling, and "
					      "set a non-standard fifo buffer. This is also "
					      "true when using SuSE's resmgr."),
					 i18n("Use K3bSetup to solve this problem."),
					 true ) );
  }
  
  if( !k3bcore->externalBinManager()->foundBin( "cdrdao" ) ) {
    problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
				       i18n("Unable to find %1 executable").arg("cdrdao"),
				       i18n("K3b uses cdrdao to actually write cds. "
					    "Without cdrdao you won't be able to copy cds, "
					    "write cue/bin images, write CD-TEXT, and write "
					    "audio cds on-the-fly."),
				       i18n("Install the cdrdao package."),
				       false ) );
  }
  else if( !k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "suidroot" ) ) {
    problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
				       i18n("%1 does not run with root privileges").arg("cdrdao"),
				       i18n("cdrdao needs to run with root privileges "
					    "to be able to access the cd devices and "
					    "use real time scheduling."
					    "This is also true when using SuSE's resmgr."),
				       i18n("Use K3bSetup to solve this problem."),
				       true ) );
  }

  if( !k3bcore->deviceManager()->dvdWriter().isEmpty() ) {
    if( !k3bcore->externalBinManager()->foundBin( "growisofs" ) ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("Unable to find %1 executable").arg("growisofs"),
					 i18n("K3b uses growisofs to actually write dvds. "
					      "Without growisofs you won't be able to write dvds. "
					      "Make sure to install at least version 5.10."),
					 i18n("Install the growisofs package."),
					 false ) );
    }
    else if( k3bcore->externalBinManager()->binObject( "growisofs" )->version < K3bVersion( 5, 10 ) ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("Used %1 version %2 is outdated").arg("growisofs").arg(k3bcore->externalBinManager()->binObject( "growisofs" )->version),
					 i18n("K3b needs at least growisofs version 5.10 to write dvds. "
					      "All older versions will not work and K3b will refuse to use them."),
					 i18n("Install a more recent version of growisofs."),
					 false ) );
    }
  }

  // 2. ATAPI devices
  bool atapiReader = false;
  bool atapiWriter = false;
  for( QPtrListIterator<K3bCdDevice::CdDevice> it( k3bcore->deviceManager()->readingDevices() );
       it.current(); ++it ) {
    if( it.current()->interfaceType() == K3bCdDevice::CdDevice::IDE ) {
      atapiReader = true;
      break;
    }
  }
  for( QPtrListIterator<K3bCdDevice::CdDevice> it( k3bcore->deviceManager()->burningDevices() );
       it.current(); ++it ) {
    if( it.current()->interfaceType() == K3bCdDevice::CdDevice::IDE ) {
      atapiWriter = true;
      break;
    }
  }

  if( atapiWriter ) {
    if( !K3bCdDevice::plainAtapiSupport() &&
	!K3bCdDevice::hackedAtapiSupport() ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("No ATAPI writing support in kernel"),
					 i18n("Your kernel does not support writing without "
					      "SCSI emulation but there is at least one "
					      "writer in your system not configured to use "
					      "SCSI emulation."),
					 i18n("The best and recommended solution is to enable "
					      "ide-scsi (SCSI emulation) for all writer devices. "
					      "This way you won't have any problems."),
					 false ) );
    }
    else {
      // we have atapi support in some way in the kernel

      if( k3bcore->externalBinManager()->foundBin( "cdrecord" ) ) {

	if( !( k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "hacked-atapi" ) &&
	       K3bCdDevice::hackedAtapiSupport() ) &&
	    !( k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "plain-atapi" ) &&
	       K3bCdDevice::plainAtapiSupport() ) ) {
	  problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					     i18n("%1 %2 does not support ATAPI").arg("cdrecord").arg(k3bcore->externalBinManager()->binObject("cdrecord")->version),
					     i18n("The configured version of %1 does not "
						  "support writing to ATAPI devices without "
						  "SCSI emulation and there is at least one writer "
						  "in your system not configured to use "
						  "SCSI emulation.").arg("cdrecord"),
					     i18n("The best and recommended solution is to enable "
						  "ide-scsi (SCSI emulation) for all writer devices. "
						  "This way you won't have any problems. Or you install "
						  "(or select as the default) a more recent version of %1.").arg("cdrtools"),
					     false ) );
	}
      }

      if( k3bcore->externalBinManager()->foundBin( "cdrdao" ) ) {

	if( !( k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "hacked-atapi" ) &&
	       K3bCdDevice::hackedAtapiSupport() ) &&
	    !( k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "plain-atapi" ) &&
	       K3bCdDevice::plainAtapiSupport() ) ) {
	  problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					     i18n("%1 %2 does not support ATAPI").arg("cdrdao").arg(k3bcore->externalBinManager()->binObject("cdrdao")->version),
					     i18n("The configured version of %1 does not "
						  "support writing to ATAPI devices without "
						  "SCSI emulation and there is at least one writer "
						  "in your system not configured to use "
						  "SCSI emulation.").arg("cdrdao"),
					     i18n("The best and recommended solution is to enable "
						  "ide-scsi (SCSI emulation) for all writer devices. "
						  "This way you won't have any problems. Or you install "
						  "(or select as the default) a more recent version of %1.").arg("cdrdao"),
					     false ) );
	}
	else {
	  problems.append( K3bSystemProblem( K3bSystemProblem::NON_CRITICAL,
					     i18n("cdrdao has problems with ATAPI writers"),
					     i18n("When K3b %1 was released no version of cdrdao "
						  "was able to write without SCSI emulation. "
						  "Although it is possible that there actually "
						  "is a version with ATAPI support it is unlikely.").arg(k3bcore->version()),
					     i18n("The best and recommended solution is to enable "
						  "ide-scsi (SCSI emulation) for all writer devices. "
						  "This way you won't have any problems."),
					     false ) );
	}
      }
    }
  }

  if( atapiReader ) { 

    if( k3bcore->externalBinManager()->foundBin( "cdrdao" ) ) {
      
      if( !( k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "hacked-atapi" ) &&
	     K3bCdDevice::hackedAtapiSupport() ) &&
	  !( k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "plain-atapi" ) &&
	     K3bCdDevice::plainAtapiSupport() ) ) {
	problems.append( K3bSystemProblem( K3bSystemProblem::WARNING,
					   i18n("No support for ATAPI with cdrdao"),
					   i18n("You will not be able to use all your reading devices "
						"as copy sources since there is at least one not "
						"configured to use SCSI emulation and your system does "
						"not support ATAPI with cdrdao."),
					   i18n("The best and recommended solution is to enable "
						"ide-scsi (SCSI emulation) for all writer devices. "
						"This way you won't have any problems."),
					   false ) );
      }
    }
  }

  kdDebug() << "(K3bCore) System problems:" << endl;
  for( QValueList<K3bSystemProblem>::const_iterator it = problems.begin();
       it != problems.end(); ++it ) {
    const K3bSystemProblem& p = *it;

    switch( p.type ) {
    case K3bSystemProblem::CRITICAL:
      kdDebug() << " CRITICAL" << endl;
      break;
    case K3bSystemProblem::NON_CRITICAL:
      kdDebug() << " NON_CRITICAL" << endl;
      break;
    case K3bSystemProblem::WARNING:
      kdDebug() << " WARNING" << endl;
      break;
    }
    kdDebug() << " PROBLEM:  " << p.problem << endl
	      << " DETAILS:  " << p.details << endl
	      << " SOLUTION: " << p.solution << endl << endl;

  }
  if( problems.isEmpty() )
    kdDebug() << "          - none - " << endl;
  else {
    K3bSystemProblemDialog( problems ).exec();
  }
}

#include "k3bsystemproblemdialog.moc"

/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include <config.h>


#include "k3bapplication.h"
#include "k3bsystemproblemdialog.h"
#include "k3bpassivepopup.h"
#include <k3btitlelabel.h>
#include <k3bexternalbinmanager.h>
#include <k3bstdguiitems.h>
#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bversion.h>
#include <k3bglobals.h>
#include <k3bpluginmanager.h>
#include <k3bplugin.h>
#include <k3bprocess.h>
#include <k3bthememanager.h>

#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qfileinfo.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <ktextedit.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kglobal.h>





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
  pixmapLabelLeft->setScaledContents( FALSE );
  layout4->addWidget( pixmapLabelLeft );
  K3bTitleLabel* labelTitle = new K3bTitleLabel( headerFrame, "m_labelTitle" );
  labelTitle->setTitle( i18n("System Configuration Problems"), 
			i18n("1 problem", "%n problems", problems.count() ) );
  layout4->addWidget( labelTitle );
  layout4->setStretchFactor( labelTitle, 1 );
  QLabel* pixmapLabelRight = new QLabel( headerFrame, "pixmapLabelRight" );
  pixmapLabelRight->setScaledContents( FALSE );
  layout4->addWidget( pixmapLabelRight );


  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    pixmapLabelLeft->setPaletteBackgroundColor( theme->backgroundColor() );
    pixmapLabelLeft->setPixmap( theme->pixmap( K3bTheme::MEDIA_LEFT ) );
    pixmapLabelRight->setPaletteBackgroundColor( theme->backgroundColor() );
    pixmapLabelRight->setPixmap( theme->pixmap( K3bTheme::MEDIA_NONE ) );
    labelTitle->setPaletteBackgroundColor( theme->backgroundColor() );
    labelTitle->setPaletteForegroundColor( theme->foregroundColor() );
  }


  m_closeButton = new QPushButton( i18n("Close"), this );
  connect( m_closeButton, SIGNAL(clicked()), this, SLOT(close()) );
  m_checkDontShowAgain = new QCheckBox( i18n("Don't show again"), this );

#ifdef HAVE_K3BSETUP
  m_k3bsetupButton = new QPushButton( i18n("Start K3bSetup2"), this );
  connect( m_k3bsetupButton, SIGNAL(clicked()), this, SLOT(slotK3bSetup()) );
#endif

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
  QHBoxLayout* buttonBox = new QHBoxLayout;
  buttonBox->setSpacing( spacingHint() );
  buttonBox->setMargin( 0 );
#ifdef HAVE_K3BSETUP
  buttonBox->addWidget( m_k3bsetupButton );
#endif
  buttonBox->addWidget( m_closeButton );
  grid->addLayout( buttonBox, 2, 1 );
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


void K3bSystemProblemDialog::checkSystem( QWidget* parent, 
					  const char* name )
{
  QValueList<K3bSystemProblem> problems;

  // 1. cdrecord, cdrdao
  if( !k3bcore->externalBinManager()->foundBin( "cdrecord" ) ) {
    problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
				       i18n("Unable to find %1 executable").arg("cdrecord"),
				       i18n("K3b uses cdrecord to actually write CDs."),
				       i18n("Install the cdrtools package which contains "
					    "cdrecord."),
				       false ) );
  }
  else {
    if( k3bcore->externalBinManager()->binObject( "cdrecord" )->version < K3bVersion( 2, 0 ) ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::NON_CRITICAL,
					 i18n("Used %1 version %2 is outdated").arg("cdrecord").arg(k3bcore->externalBinManager()->binObject( "cdrecord" )->version),
					 i18n("Although K3b supports all cdrtools versions since "
					      "1.10 it is highly recommended to at least use "
					      "version 2.0."),
					 i18n("Install a more recent version of the cdrtools."),
					 false ) );
    }
    
#ifdef Q_OS_LINUX

    //
    // Since kernel 2.6.8 older cdrecord versions are not able to use the SCSI subsystem when running suid root anymore
    // So far we ignore the suid root issue with kernel >= 2.6.8 and cdrecord < 2.01.01a02
    //
    // Kernel 2.6.16.something seems to introduce another problem which was apparently worked around in cdrecord 2.01.01a05
    //
    if( K3b::simpleKernelVersion() >= K3bVersion( 2, 6, 8 ) &&
	k3bcore->externalBinManager()->binObject( "cdrecord" )->version < K3bVersion( 2, 1, 1, "a05" ) ) {
      if( k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "suidroot" ) )
	problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					   i18n("%1 will be run with root privileges on kernel >= 2.6.8").arg("cdrecord <= 2.01.01a05"),
					   i18n("Since Linux kernel 2.6.8 %1 will not work when run suid "
						"root for security reasons anymore.").arg("cdrecord <= 2.01.01a05"),
					   i18n("Use K3bSetup to solve this problem."),
					   true ) );
    }
    else if( !k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "suidroot" ) && getuid() != 0 ) // not root
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("%1 will be run without root privileges").arg("cdrecord"),
					 i18n("It is highly recommended to configure cdrecord "
					      "to run with root privileges. Only then cdrecord "
					      "runs with high priority which increases the overall "
					      "stability of the burning process. Apart from that "
					      "it allows changing the size of the used burning buffer. "
					      "A lot of user problems could be solved this way. This is also "
					      "true when using SuSE's resmgr."),
					 i18n("Use K3bSetup to solve this problem."),
					 true ) );
#endif
  }

  if( !k3bcore->externalBinManager()->foundBin( "cdrdao" ) ) {
    problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
				       i18n("Unable to find %1 executable").arg("cdrdao"),
				       i18n("K3b uses cdrdao to actually write CDs."),
				       i18n("Install the cdrdao package."),
				       false ) );
  }
  else {
#ifdef Q_OS_LINUX
    if( !k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "suidroot" ) && getuid() != 0 )
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("%1 will be run without root privileges").arg("cdrdao"),
					 i18n("It is highly recommended to configure cdrdao "
					      "to run with root privileges to increase the "
					      "overall stability of the burning process."),
					 i18n("Use K3bSetup to solve this problem."),
					 true ) );
#endif
  }

  if( !k3bcore->deviceManager()->dvdWriter().isEmpty() ) {
    if( !k3bcore->externalBinManager()->foundBin( "growisofs" ) ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("Unable to find %1 executable").arg("growisofs"),
					 i18n("K3b uses growisofs to actually write dvds. "
					      "Without growisofs you won't be able to write dvds. "
					      "Make sure to install at least version 5.10."),
					 i18n("Install the dvd+rw-tools package."),
					 false ) );
    }
    else {
      if( k3bcore->externalBinManager()->binObject( "growisofs" )->version < K3bVersion( 5, 10 ) ) {
	problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					   i18n("Used %1 version %2 is outdated").arg("growisofs").arg(k3bcore->externalBinManager()->binObject( "growisofs" )->version),
					   i18n("K3b needs at least growisofs version 5.10 to write dvds. "
						"All older versions will not work and K3b will refuse to use them."),
					   i18n("Install a more recent version of %1.").arg("growisofs"),
					   false ) );
      }
      else if( k3bcore->externalBinManager()->binObject( "growisofs" )->version < K3bVersion( 5, 12 ) ) {
	problems.append( K3bSystemProblem( K3bSystemProblem::NON_CRITICAL,
					   i18n("Used %1 version %2 is outdated").arg("growisofs").arg(k3bcore->externalBinManager()->binObject( "growisofs" )->version),
					   i18n("K3b won't be able to copy DVDs on-the-fly using a growisofs "
						"version older than 5.12."),
					   i18n("Install a more recent version of %1.").arg("growisofs"),
					   false ) );
      }
      else if( k3bcore->externalBinManager()->binObject( "growisofs" )->version < K3bVersion( 6, 0 ) ) {
	problems.append( K3bSystemProblem( K3bSystemProblem::NON_CRITICAL,
					   i18n("Used %1 version %2 is outdated").arg("growisofs").arg(k3bcore->externalBinManager()->binObject( "growisofs" )->version),
					   i18n("It is highly recommended to use growisofs 6.0 or higher."),
					   i18n("Install a more recent version of %1.").arg("growisofs"),
					   false ) );
      }
      // for now we ignore the suid root bit becasue of the memorylocked issue
//       else if( !k3bcore->externalBinManager()->binObject( "growisofs" )->hasFeature( "suidroot" ) ) {
// 	problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
// 					   i18n("%1 will be run without root privileges").arg("growisofs"),
// 					   i18n("It is highly recommended to configure growisofs "
// 						"to run with root privileges. Only then growisofs "
// 						"runs with high priority which increases the overall "
// 						"stability of the burning process."),
// 					   i18n("Use K3bSetup to solve this problem."),
// 					   true ) );
//       }
    }

    if( !k3bcore->externalBinManager()->foundBin( "dvd+rw-format" ) ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("Unable to find %1 executable").arg("dvd+rw-format"),
					 i18n("K3b uses dvd+rw-format to format DVD-RWs and DVD+RWs."),
					 i18n("Install the dvd+rw-tools package."),
					 false ) );
    }
  }

  if( !k3bcore->externalBinManager()->foundBin( "mkisofs" ) ) {

  }
  else if( k3bcore->externalBinManager()->binObject( "mkisofs" )->version < K3bVersion( 1, 14 ) ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("Used %1 version %2 is outdated")
					 .arg("mkisofs")
					 .arg(k3bcore->externalBinManager()->binObject( "mkisofs" )->version),
					 i18n("K3b needs at least mkisofs version 1.14. Older versions may introduce problems "
					      "when creating data projects."),
					 i18n("Install a more recent version of %1.").arg("mkisofs"),
					 false ) );
  }

  // 2. device check
  bool atapiReader = false;
  bool atapiWriter = false;
  bool dvd_r_dl = false;
  for( QPtrListIterator<K3bDevice::Device> it( k3bcore->deviceManager()->readingDevices() );
       it.current(); ++it ) {
    if( it.current()->interfaceType() == K3bDevice::IDE ) {
      atapiReader = true;
      break;
    }
  }
  for( QPtrListIterator<K3bDevice::Device> it( k3bcore->deviceManager()->burningDevices() );
       it.current(); ++it ) {
    if( it.current()->interfaceType() == K3bDevice::IDE )
      atapiWriter = true;
    if( it.current()->type() & K3bDevice::DEVICE_DVD_R_DL )
      dvd_r_dl = true;

#if 0
    if( it.current()->automount() ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::NON_CRITICAL,
					 i18n("Writing device %1 - %2 is automounted.")
					 .arg(it.current()->vendor()).arg(it.current()->description()),
					 i18n("Automounting can cause problems with CD/DVD writing, especially "
					      "with rewritable media. Although it might all work just fine it "
					      "is recommended to disable automounting completely for now."),
					 i18n("Replace the automounting entries in /etc/fstab with old-fashioned "
					      "ones."),
					 false ) );
    }
#endif
  }

  if( atapiWriter ) {
    if( !K3b::plainAtapiSupport() &&
	!K3b::hackedAtapiSupport() ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("No ATAPI writing support in kernel"),
					 i18n("Your kernel does not support writing without "
					      "SCSI emulation but there is at least one "
					      "writer in your system not configured to use "
					      "SCSI emulation."),
					 i18n("The best and recommended solution is to enable "
					      "ide-scsi (SCSI emulation) for all devices. "
					      "This way you won't have any problems."
					      "Be aware that you may still enable DMA on ide-scsi "
					      "emulated drives."),
					 false ) );
    }
    else {
      // we have atapi support in some way in the kernel

      if( k3bcore->externalBinManager()->foundBin( "cdrecord" ) ) {

	if( !( k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "hacked-atapi" ) &&
	       K3b::hackedAtapiSupport() ) &&
	    !( k3bcore->externalBinManager()->binObject( "cdrecord" )->hasFeature( "plain-atapi" ) &&
	       K3b::plainAtapiSupport() ) ) {
	  problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					     i18n("%1 %2 does not support ATAPI").arg("cdrecord").arg(k3bcore->externalBinManager()->binObject("cdrecord")->version),
					     i18n("The configured version of %1 does not "
						  "support writing to ATAPI devices without "
						  "SCSI emulation and there is at least one writer "
						  "in your system not configured to use "
						  "SCSI emulation.").arg("cdrecord"),
					     i18n("The best and recommended solution is to enable "
						  "ide-scsi (SCSI emulation) for all devices. "
						  "This way you won't have any problems. Or you install "
						  "(or select as the default) a more recent version of %1.").arg("cdrtools"),
					     false ) );
	}
      }

      if( k3bcore->externalBinManager()->foundBin( "cdrdao" ) ) {

	if( !k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "hacked-atapi" ) &&
	    !k3bcore->externalBinManager()->binObject( "cdrdao" )->hasFeature( "plain-atapi") ) {
	  problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					     i18n("%1 %2 does not support ATAPI")
					     .arg("cdrdao").arg(k3bcore->externalBinManager()->binObject("cdrdao")->version),
					     i18n("The configured version of %1 does not "
						  "support writing to ATAPI devices without "
						  "SCSI emulation and there is at least one writer "
						  "in your system not configured to use "
						  "SCSI emulation.").arg("cdrdao"),
					     K3b::simpleKernelVersion() > K3bVersion( 2, 5, 0 )
					     ? i18n("Install cdrdao >= 1.1.8 which supports writing to "
						    "ATAPI devices directly.")
					     : i18n("The best, and recommended, solution is to use "
						    "ide-scsi (SCSI emulation) for all writer devices: "
						    "this way you will not have any problems; or, you can install "
						    "(or select as the default) a more recent version of %1.").arg("cdrdao"),
					     false ) );
	}
      }
    }
  }

  if( dvd_r_dl && k3bcore->externalBinManager()->foundBin( "growisofs" ) ) {
    if( k3bcore->externalBinManager()->binObject( "growisofs" )->version < K3bVersion( 6, 0 ) )
      problems.append( K3bSystemProblem( K3bSystemProblem::NON_CRITICAL,
					 i18n("Used %1 version %2 is outdated").arg("growisofs").arg(k3bcore->externalBinManager()->binObject( "growisofs" )->version),
					 i18n("K3b won't be able to write DVD-R Dual Layer media using a growisofs "
					      "version older than 6.0."),
					 i18n("Install a more recent version of growisofs."),
					 false ) );
  }

  for( QPtrListIterator<K3bDevice::Device> it( k3bcore->deviceManager()->allDevices() );
       it.current(); ++it ) {
    K3bDevice::Device* dev = it.current();

    if( !QFileInfo( dev->blockDeviceName() ).isWritable() )
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("No write access to device %1").arg(dev->blockDeviceName()),
					 i18n("K3b needs write access to all the devices to perform certain tasks. "
					      "Without you might encounter problems with %1 - %2").arg(dev->vendor()).arg(dev->description()),
					 i18n("Make sure you have write access to %1. In case you are not using "
					      "devfs or udev K3bSetup is able to do this for you.").arg(dev->blockDeviceName()),
					 false ) );


    if( !dev->genericDevice().isEmpty() &&
	!QFileInfo( dev->genericDevice() ).isWritable() )
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("No write access to generic SCSI device %1").arg(dev->genericDevice()),
					 i18n("Without write access to the generic device you might "
					      "encounter problems with Audio CD ripping from %1 - %2").arg(dev->vendor()).arg(dev->description()),
					 i18n("Make sure you have write access to %1. In case you are not using "
					      "devfs or udev K3bSetup is able to do this for you.").arg(dev->genericDevice()),
					 false ) );

    if( !dmaActivated( dev ) )
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("DMA disabled on device %1 - %2").arg(dev->vendor()).arg(dev->description()),
					 i18n("With most modern CD/DVD devices enabling DMA highly increases "
					      "read/write performance. If you experience very low writing speeds "
					      "this is probably the cause."),
					 i18n("Enable DMA temporarily as root with 'hdparm -d 1 %1'.").arg(dev->blockDeviceName()) ) );
  }


  //
  // Check if the user specified some user parameters and warn about it
  //
  const QMap<QString, K3bExternalProgram*>& programMap = k3bcore->externalBinManager()->programs();
  for( QMap<QString, K3bExternalProgram*>::const_iterator it = programMap.constBegin();
       it != programMap.constEnd(); ++it ) {
    const K3bExternalProgram* p = it.data();
    if( !p->userParameters().isEmpty() ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::WARNING,
					 i18n("User parameters specified for external program %1").arg(p->name()),
					 i18n("Sometimes it may be nessessary to specify user parameters in addition to "
					      "the parameters generated by K3b. This is simply a warning to make sure that "
					      "these parameters are really wanted and won't be pat of some bug report."),
					 i18n("To remove the user parameters for the external program %1 open the "
					      "K3b settings page 'Programs' and choose the tab 'User Parameters'."),
					 false ) );
    }
  }

  //
  // Way too many users are complaining about K3b not being able to decode mp3 files. So just warn them about
  // the legal restrictions with many distros
  //
  QPtrList<K3bPlugin> plugins = k3bcore->pluginManager()->plugins( "AudioDecoder" );
  bool haveMp3Decoder = false;
  for( QPtrListIterator<K3bPlugin> it( plugins ); *it; ++it ) {
    if( it.current()->pluginInfo().name() == "K3b MAD Decoder" ) {
      haveMp3Decoder = true;
      break;
    }
  }
  if( !haveMp3Decoder ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::WARNING,
					 i18n("Mp3 Audio Decoder plugin not found."),
					 i18n("K3b could not load or find the Mp3 decoder plugin. This means that you will not "
					      "be able to create Audio CDs from Mp3 files. Many Linux distributions do not "
					      "include Mp3 support for legal reasons."),
					 i18n("To enable Mp3 support, please install the MAD Mp3 decoding library as well as the "
					      "K3b MAD Mp3 decoder plugin (the latter may already be installed but not functional "
					      "due to the missing libmad). Some distributions allow installation of Mp3 support "
					      "via an online update tool (i.e. SuSE's YOU)."),
					 false ) );
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
  if( problems.isEmpty() ) {
    kdDebug() << "          - none - " << endl;
    K3bPassivePopup::showPopup( i18n("No problems found in system configuration."), i18n("System Problems") );
  }
  else {
    K3bSystemProblemDialog( problems, parent, name ).exec();
  }
}

void K3bSystemProblemDialog::slotK3bSetup()
{
  KProcess p;
  p << "kdesu" << "kcmshell k3bsetup2 --lang " + KGlobal::locale()->language();
  if( !p.start( KProcess::DontCare ) )
    KMessageBox::error( 0, i18n("Unable to start K3bSetup2.") );
}


int K3bSystemProblemDialog::dmaActivated( K3bDevice::Device* dev )
{
  QString hdparm = K3b::findExe( "hdparm" );
  if( hdparm.isEmpty() )
    return -1;

  K3bProcess p;
  K3bProcessOutputCollector out( &p );
  p << hdparm << "-d" << dev->blockDeviceName();
  if( !p.start( KProcess::Block, KProcess::AllOutput ) )
    return -1;

  // output is something like:
  //
  // /dev/hda:
  //  using_dma    =  1 (on)
  //
  // But we ignore the on/off since it might be translated
  //
  if( out.output().contains( "1 (" ) )
    return 1;
  else if( out.output().contains( "0 (" ) )
    return 0;
  else
    return -1;
}

#include "k3bsystemproblemdialog.moc"

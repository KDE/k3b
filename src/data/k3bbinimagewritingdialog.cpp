/*
 *
 * $Id$
 * Copyright (C) 2003 Klaus-Dieter Krannich <kd@k3b.org>
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


#include "k3bbinimagewritingdialog.h"
#include "k3bbinimagewritingjob.h"
#include <tools/k3bglobals.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3bburnprogressdialog.h>
#include <k3bstdguiitems.h>
#include <k3bcore.h>
#include <tools/k3bmd5job.h>

#include <kconfig.h>
#include <kstandarddirs.h>
#include <kcutlabel.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kio/global.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kurlrequester.h>
#include <kactivelabel.h>

#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qtabwidget.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>
#include <qspinbox.h>



    // ------------------------------------------------
    // Test for cue/bin
    // ------------------------------------------------

    // TODO: check if cue-file has only one FILE-statement (otherwise say: unusabe cue-file: xxx.cue)

//     if( info.extension( false ) == "bin" || info.extension( false ) == "cue" ) {
//       // search the corresponding file
//       QString basename = path.left( path.length() - 3 );

//       // cdrdao does not use the FILE statement but the basename + the bin extension
//       bool binAvail = QFile::exists( basename + "bin" );
//       bool cueAvail = QFile::exists( basename + "cue" );

//       if( binAvail && cueAvail ) {
// 	// TODO: check if the cue-file has only one FILE-statement as needed for cdrdao
// 	m_bCueBinAvailable = true;
// 	m_cuePath = basename + "cue";
// 	m_binPath = basename + "bin";

// 	imageSize = QFileInfo( m_binPath ).size();
//       }
//     }


K3bBinImageWritingDialog::K3bBinImageWritingDialog( QWidget* parent, const char* name, bool modal )
  : K3bInteractionDialog( parent, name, i18n("Write Bin/Cue Image to CD"), QString::null,
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  modal )
{
   m_job = 0;

   setupGui();
   m_writerSelectionWidget->setSupportedWritingApps( K3b::CDRDAO );

   slotLoadUserDefaults();

   kapp->config()->setGroup("General Options");
   m_editTocPath->setURL( kapp->config()->readEntry( "last written bin/cue image", "" ) );
}


K3bBinImageWritingDialog::~K3bBinImageWritingDialog()
{
   if (m_job) 
      delete m_job;
}


void K3bBinImageWritingDialog::setupGui()
{
  QWidget* frame = mainWidget();

  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );

  // image group box
  // -----------------------------------------------------------------------
  QGroupBox* groupImage = new QGroupBox( i18n("CUE/TOC File"), frame );
  groupImage->setColumnLayout(0, Qt::Vertical );
  groupImage->layout()->setSpacing( 0 );
  groupImage->layout()->setMargin( 0 );
  QGridLayout* groupImageLayout = new QGridLayout( groupImage->layout() );
  groupImageLayout->setAlignment( Qt::AlignTop );
  groupImageLayout->setSpacing( spacingHint() );
  groupImageLayout->setMargin( marginHint() );

  m_editTocPath = new KURLRequester( groupImage );
  m_editTocPath->setCaption( i18n("Choose TOC/CUE file") );
  
  groupImageLayout->addWidget( m_editTocPath, 0, 0 );

  // options
  // -----------------------------------------------------------------------
  QTabWidget* tabOptions = new QTabWidget( frame );

  QWidget* optionTab = new QWidget( tabOptions );
  QGridLayout* optionTabLayout = new QGridLayout( optionTab );
  optionTabLayout->setMargin(marginHint());
  optionTabLayout->setSpacing(spacingHint());

  QGroupBox* groupOptions = new QGroupBox( 5, Qt::Vertical, i18n("Options"), optionTab );
  groupOptions->setInsideSpacing( spacingHint() );
  groupOptions->setInsideMargin( marginHint() );

  m_checkSimulate = K3bStdGuiItems::simulateCheckbox( groupOptions );
  m_checkMulti    = K3bStdGuiItems::startMultisessionCheckBox( groupOptions );

  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), optionTab );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );

  QLabel* pixLabel = new QLabel( groupCopies );
  pixLabel->setPixmap( locate( "appdata", "pics/k3b_cd_copy.png" ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( groupCopies );
  m_spinCopies->setMinValue( 1 );
  m_spinCopies->setMaxValue( 99 );

  optionTabLayout->addWidget( groupOptions, 0, 0 );
  optionTabLayout->addWidget( groupCopies, 0, 1 );

  tabOptions->addTab( optionTab, i18n("&Options") );

  // advanced tab
  // ------------
  QWidget* advancedTab = new QWidget( tabOptions );
  QGridLayout* advancedTabLayout = new QGridLayout( advancedTab );
  advancedTabLayout->setMargin(marginHint());
  advancedTabLayout->setSpacing(spacingHint());

  m_checkForce = new QCheckBox( i18n("Force writing"), advancedTab );

  advancedTabLayout->addWidget( m_checkForce, 0, 0 );
  advancedTabLayout->setRowStretch( 1, 1 );

  tabOptions->addTab( advancedTab, i18n("&Advanced") );


  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  grid->addWidget( m_writerSelectionWidget, 0, 0 );
  grid->addWidget( groupImage, 1, 0 );
  grid->addWidget( tabOptions, 2, 0 );

  grid->setRowStretch( 2, 1 );

  QToolTip::add( m_checkForce, i18n("Force writing") );
  QToolTip::add( m_spinCopies, i18n("Number of copies") );

  QWhatsThis::add( m_checkForce, i18n("<p>Forces the execution of an operation that otherwise would not be "
				      "performed. ") );
  QWhatsThis::add( m_spinCopies, i18n("<p>Select how many copies you want K3b to create from the Image.") );

  slotWriterChanged();  
}


void K3bBinImageWritingDialog::slotStartClicked()
{
  if( m_job == 0 )
    m_job = new K3bBinImageWritingJob();

  m_job->setWriter( m_writerSelectionWidget->writerDevice() );
  m_job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  m_job->setTocFile(m_editTocPath->url());
  m_job->setSimulate(m_checkSimulate->isChecked());
  m_job->setMulti(m_checkMulti->isChecked());
  m_job->setForce(m_checkForce->isChecked());
  m_job->setCopies(m_spinCopies->value());

  if (!m_editTocPath->url().isEmpty()) {

    // save the path
    kapp->config()->setGroup("General Options");
    kapp->config()->writeEntry( "last written bin/cue image", m_editTocPath->url() );

    K3bBurnProgressDialog d( kapp->mainWidget(), "burnProgress", true );
    
    hide();
    d.startJob(m_job);
    close();
  } else
     KMessageBox::error( this, i18n("Please select a TOC File"), i18n("No TOC File"));
}


void K3bBinImageWritingDialog::setTocFile( const KURL& url )
{
  m_tocPath = url.path();
  m_editTocPath->setURL(m_tocPath);
}


void K3bBinImageWritingDialog::slotWriterChanged()
{
}

void K3bBinImageWritingDialog::slotLoadUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "CueBin image writing" );

  m_checkSimulate->setChecked( c->readBoolEntry( "simulate", false ) );
  m_checkMulti->setChecked( c->readBoolEntry( "multisession", false ) );
  m_checkForce->setChecked( c->readBoolEntry( "force", false ) );
  m_spinCopies->setValue( c->readNumEntry( "copies", 1 ) );

  m_writerSelectionWidget->loadConfig( c );
}


void K3bBinImageWritingDialog::slotSaveUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "CueBin image writing" );

  c->writeEntry( "simulate", m_checkSimulate->isChecked() );
  c->writeEntry( "multisession", m_checkMulti->isChecked() );
  c->writeEntry( "force", m_checkForce->isChecked() );
  c->writeEntry( "copies", m_spinCopies->value() );

  m_writerSelectionWidget->saveConfig( c );
}

void K3bBinImageWritingDialog::slotLoadK3bDefaults()
{
  m_checkSimulate->setChecked( false );
  m_checkMulti->setChecked( false );
  m_checkForce->setChecked( false );
  m_spinCopies->setValue(1);
}


#include "k3bbinimagewritingdialog.moc"

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

#include "k3bblankingdialog.h"

#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
#include "k3bblankingjob.h"
#include "k3bwriterselectionwidget.h"
#include "k3bdiskerasinginfodialog.h"
#include <k3bglobals.h>
#include <k3bcore.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kguiitem.h>
#include <kstdguiitem.h>
#include <kconfig.h>
#include <kapplication.h>

#include <qgroupbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qptrlist.h>
#include <qframe.h>
#include <qtextview.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qheader.h>


K3bBlankingDialog::K3bBlankingDialog( QWidget* parent, const char* name )
  : K3bInteractionDialog( parent, name, i18n("Erase CD-RW") )
{
  setCancelButtonText( i18n("Close") );
  setupGui();

  m_groupBlankType->setButton( 0 );

  m_job = 0;

  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotWriterChanged()) );
  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)), this, SLOT(slotWritingAppChanged(int)) );
  slotLoadUserDefaults();
  slotWriterChanged();
}


K3bBlankingDialog::~K3bBlankingDialog()
{
  if( m_job )
    delete m_job;
}


void K3bBlankingDialog::setupGui()
{
  QWidget* frame = mainWidget();

  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );


  // --- setup the blanking type button group -----------------------------
  m_groupBlankType = new QButtonGroup( i18n("&Erase Type"), frame );
  m_groupBlankType->setExclusive( true );
  m_groupBlankType->setColumnLayout(0, Qt::Vertical );
  m_groupBlankType->layout()->setSpacing( 0 );
  m_groupBlankType->layout()->setMargin( 0 );
  QVBoxLayout* groupBlankTypeLayout = new QVBoxLayout( m_groupBlankType->layout() );
  groupBlankTypeLayout->setAlignment( Qt::AlignTop );
  groupBlankTypeLayout->setSpacing( spacingHint() );
  groupBlankTypeLayout->setMargin( marginHint() );

  m_radioFastBlank = new QRadioButton( i18n("&Fast"), m_groupBlankType );
  m_radioCompleteBlank = new QRadioButton( i18n("Co&mplete"), m_groupBlankType );
  m_radioBlankTrack = new QRadioButton( i18n("Erase last &track"), m_groupBlankType );
  m_radioUncloseSession = new QRadioButton( i18n("&Unclose last session"), m_groupBlankType );
  m_radioBlankSession = new QRadioButton( i18n("Erase last &session"), m_groupBlankType );

  groupBlankTypeLayout->addWidget( m_radioFastBlank );
  groupBlankTypeLayout->addWidget( m_radioCompleteBlank );
  groupBlankTypeLayout->addWidget( m_radioBlankTrack);
  groupBlankTypeLayout->addWidget( m_radioUncloseSession );
  groupBlankTypeLayout->addWidget( m_radioBlankSession );
  // ----------------------------------------------------------------------


  // ----- setup the putput group ------------------------------------------
  m_groupOutput = new QGroupBox( i18n("Output"), frame );
  m_groupOutput->setColumnLayout(0, Qt::Vertical );
  m_groupOutput->layout()->setSpacing( 0 );
  m_groupOutput->layout()->setMargin( 0 );
  QGridLayout* groupOutputLayout = new QGridLayout( m_groupOutput->layout() );
  groupOutputLayout->setAlignment( Qt::AlignTop );
  groupOutputLayout->setSpacing( spacingHint() );
  groupOutputLayout->setMargin( marginHint() );

  m_viewOutput = new KListView( m_groupOutput );
  m_viewOutput->setSorting(-1);
  m_viewOutput->addColumn( i18n("Type") );
  m_viewOutput->addColumn( i18n("Message") );
  m_viewOutput->header()->hide();
  groupOutputLayout->addWidget( m_viewOutput, 0, 0 );
  // ------------------------------------------------------------------------

  // -- setup option group --------------------------------------------------
  m_groupOptions = new QGroupBox( i18n("Options"), frame );
  m_groupOptions->setColumnLayout(0, Qt::Vertical );
  m_groupOptions->layout()->setSpacing( 0 );
  m_groupOptions->layout()->setMargin( 0 );
  QVBoxLayout* groupOptionsLayout = new QVBoxLayout( m_groupOptions->layout() );
  groupOptionsLayout->setAlignment( Qt::AlignTop );
  groupOptionsLayout->setSpacing( spacingHint() );
  groupOptionsLayout->setMargin( marginHint() );

  m_checkForce = new QCheckBox( m_groupOptions );
  m_checkForce->setText( i18n("F&orce\n(Try this if K3b\nis not able to\nblank a CD-RW in\nnormal mode)") );

  groupOptionsLayout->addWidget( m_checkForce );
  // ------------------------------------------------------------------------


  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  grid->addMultiCellWidget( m_writerSelectionWidget, 0, 0, 0, 1 );
  grid->addWidget( m_groupBlankType, 1, 0 );
  grid->addWidget( m_groupOptions, 1, 1 );
  grid->addMultiCellWidget( m_groupOutput, 2, 2, 0, 1 );
}


void K3bBlankingDialog::slotStartClicked()
{
  // start the blankingjob and connect to the info-signal
  // disable the user1 button and enable the cancel button
  m_viewOutput->clear();

  if( m_job == 0 ) {
    m_job = new K3bBlankingJob();
    connect( m_job, SIGNAL(infoMessage(const QString&,int)), 
	     this, SLOT(slotInfoMessage(const QString&,int)) );
  }

  m_job->setDevice( m_writerSelectionWidget->writerDevice() );
  m_job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  m_job->setForce( m_checkForce->isChecked() );
  m_job->setWritingApp(m_writerSelectionWidget->writingApp());

  if( m_radioCompleteBlank->isChecked() )
    m_job->setMode( K3bBlankingJob::Complete );
  else if( m_radioFastBlank->isChecked() )
    m_job->setMode( K3bBlankingJob::Fast );
  else if( m_radioBlankTrack->isChecked() )
    m_job->setMode( K3bBlankingJob::Track );
  else if( m_radioUncloseSession->isChecked() )
    m_job->setMode( K3bBlankingJob::Unclose );
  else // m_radioBlankSession->isChecked()
    m_job->setMode( K3bBlankingJob::Session );


  K3bErasingInfoDialog d;

  connect( m_job, SIGNAL(finished(bool)), &d, SLOT(slotFinished(bool)) );
  connect( &d, SIGNAL(cancelClicked()), m_job, SLOT(cancel()) );

  m_job->start();
  d.exec();
}


void K3bBlankingDialog::slotInfoMessage( const QString& str, int type )
{
  QListViewItem* item = new QListViewItem( m_viewOutput, m_viewOutput->lastItem(), QString::null, str );

  // set the icon
  switch( type ) {
  case K3bJob::ERROR:
    item->setPixmap( 0, SmallIcon( "stop" ) );
    break;
  case K3bJob::PROCESS:
    item->setPixmap( 0, SmallIcon( "cdwriter_unmount" ) );
    break;
  case K3bJob::STATUS:
  default:
    item->setPixmap( 0, SmallIcon( "ok" ) );
  }
}


void K3bBlankingDialog::slotWriterChanged()
{
  // check if it is a cdrw writer
  K3bDevice* dev = m_writerSelectionWidget->writerDevice();

  if( !dev )
    return;

  if( dev->writesCdrw() )
    m_buttonStart->setEnabled( true );
  else {
    m_buttonStart->setEnabled( false );
    QListViewItem* item = new QListViewItem( m_viewOutput, m_viewOutput->lastItem(),
					     i18n("%1 does not support CD-RW writing.").arg(dev->devicename()) );
    item->setPixmap( 0, SmallIcon( "stop" ) );
  }
}

void K3bBlankingDialog::slotWritingAppChanged(int app)
{
  if ( app == K3b::CDRDAO ) {
    m_radioBlankTrack->setEnabled(false);
    m_radioUncloseSession->setEnabled(false);
    m_radioBlankSession->setEnabled(false);
  } else {
    m_radioBlankTrack->setEnabled(true);
    m_radioUncloseSession->setEnabled(true);
    m_radioBlankSession->setEnabled(true);
  }
}


void K3bBlankingDialog::slotLoadK3bDefaults()
{
  m_radioFastBlank->setChecked(true);
  m_checkForce->setChecked(false);
}

void K3bBlankingDialog::slotLoadUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "CDRW Erasing" );

  QString mode = c->readEntry( "erase_mode" );
  if( mode == "complete" )
    m_radioCompleteBlank->setChecked(true);
  else if( mode == "session" )
    m_radioBlankSession->setChecked(true);
  else if( mode == "track" )
    m_radioBlankTrack->setChecked(true);
  else if( mode == "unclose_session" )
    m_radioUncloseSession->setChecked(true);
  else
    m_radioFastBlank->setChecked(true);

  m_checkForce->setChecked( c->readBoolEntry( "force", false ) );

  m_writerSelectionWidget->loadConfig( c );
}

void K3bBlankingDialog::slotSaveUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "CDRW Erasing" );

  QString mode;
  if( m_radioCompleteBlank->isChecked() )
    mode = "complete";
  else if( m_radioBlankSession->isChecked() )
    mode = "session";
  else if( m_radioBlankTrack->isChecked() )
    mode = "track";
  else if( m_radioUncloseSession->isChecked() )
    mode = "unclose_session";
  else
    mode = "fast";
  c->writeEntry( "erase_mode", mode );

  c->writeEntry( "force", m_checkForce->isChecked() );

  m_writerSelectionWidget->saveConfig( c );
}

#include "k3bblankingdialog.moc"

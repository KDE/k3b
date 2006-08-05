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

#include "k3bblankingdialog.h"
#include "k3bdebuggingoutputdialog.h"
#include "k3bdebuggingoutputfile.h"

#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include "k3bblankingjob.h"
#include "k3bwriterselectionwidget.h"
#include <k3bprogressdialog.h>
#include <k3bglobals.h>
#include <k3bcore.h>
#include <k3bemptydiscwaiter.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <kiconloader.h>
#include <kguiitem.h>
#include <kstdguiitem.h>
#include <kconfig.h>

#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qtextview.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qheader.h>
#include <qmap.h>
#include <qtooltip.h>
#include <qwhatsthis.h>



class K3bBlankingDialog::Private
{
public:
  Private()
    : job(0),
      erasingDlg(0) {
  }

  K3bBlankingJob* job;
  K3bProgressDialog* erasingDlg;
  K3bDebuggingOutputDialog* debugDialog;
  K3bDebuggingOutputFile debugFile;
  QMap<int, int> comboTypeMap;
  QMap<int, int> typeComboMap;

  bool jobRunning;
};


K3bBlankingDialog::K3bBlankingDialog( QWidget* parent, const char* name )
  : K3bInteractionDialog( parent, name, 
			  i18n("Erase CD-RW"),
			  QString::null,
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  "CDRW Erasing" )
{
  d = new Private();
  d->debugDialog = new K3bDebuggingOutputDialog( this );

  setCancelButtonText( i18n("Close") );
  setupGui();

  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotWriterChanged()) );
  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)), this, SLOT(slotWritingAppChanged(int)) );
  slotWriterChanged();
}


K3bBlankingDialog::~K3bBlankingDialog()
{
  delete d;
}


void K3bBlankingDialog::setDevice( K3bDevice::Device* dev )
{
  m_writerSelectionWidget->setWriterDevice( dev );
}


void K3bBlankingDialog::setupGui()
{
  QWidget* frame = mainWidget();

  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );
  m_writerSelectionWidget->setWantedMediumType( K3bDevice::MEDIA_CD_RW );
  m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_COMPLETE|K3bDevice::STATE_INCOMPLETE );

  // --- setup the blanking type button group -----------------------------
  QGroupBox* groupBlankType = new QGroupBox( 1, Qt::Vertical, i18n("&Erase Type"), frame );
  groupBlankType->layout()->setSpacing( spacingHint() );
  groupBlankType->layout()->setMargin( marginHint() );

  m_comboEraseMode = new QComboBox( groupBlankType );
  // ----------------------------------------------------------------------

  QGridLayout* grid = new QGridLayout( frame );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );
  grid->addWidget( m_writerSelectionWidget, 0, 0 );
  grid->addWidget( groupBlankType, 1, 0 );
}


void K3bBlankingDialog::slotStartClicked()
{
  // start the blankingjob and connect to the info-signal
  // disable the user1 button and enable the cancel button
  d->debugDialog->clear();
  d->debugFile.open();

  if( d->job == 0 ) {
    d->job = new K3bBlankingJob( this, this );
    connect( d->job, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     d->debugDialog, SLOT(addOutput(const QString&, const QString&)) );
    connect( d->job, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     &d->debugFile, SLOT(addOutput(const QString&, const QString&)) );
    connect( d->job, SIGNAL(finished(bool)), 
	     this, SLOT(slotJobFinished(bool)) );
  }

  d->job->setDevice( m_writerSelectionWidget->writerDevice() );
  d->job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  // why should one ever not want to force?
  d->job->setForce( true );
  d->job->setWritingApp(m_writerSelectionWidget->writingApp());
  d->job->setMode( d->comboTypeMap[m_comboEraseMode->currentItem()] );

  if( !d->erasingDlg )
    d->erasingDlg = new K3bProgressDialog( i18n("Erasing CD-RW"), this );

  connect( d->erasingDlg, SIGNAL(cancelClicked()), d->job, SLOT(cancel()) );

  if( !exitLoopOnHide() )
    hide();

  d->jobRunning = true;
  d->job->start();
  if( d->jobRunning ) // in case the job already finished in the start slot
    d->erasingDlg->exec(false);

  if( !exitLoopOnHide() )
    show();
  else
    close();
}


void K3bBlankingDialog::slotJobFinished( bool success )
{
  d->jobRunning = false;
  d->erasingDlg->hide();
  d->debugFile.close();

  if( success )
    KMessageBox::information( this, i18n("Successfully erased CD-RW."),
			      i18n("Success") );
  else if( KMessageBox::warningYesNo( this, 
				      i18n("The Erasing process failed. Do you want to see the debugging output?"),
				      i18n("Erasing failed.") ) == KMessageBox::Yes )
    d->debugDialog->exec();
}


void K3bBlankingDialog::slotWriterChanged()
{
  // check if it is a cdrw writer
  K3bDevice::Device* dev = m_writerSelectionWidget->writerDevice();

  if( !dev )
    m_buttonStart->setEnabled( false );
  else if( dev->writesCdrw() )
    m_buttonStart->setEnabled( true );
  else {
    m_buttonStart->setEnabled( false );
    KMessageBox::sorry( this, i18n("%1 does not support CD-RW writing.").arg(dev->devicename()) );
  }
}

void K3bBlankingDialog::slotWritingAppChanged(int app)
{
  QWhatsThis::remove( m_comboEraseMode );
  QString whatsThisInfo;

  static QString wsComplete = i18n("Erases the complete disk. This takes as long "
				   "as writing the complete CD.");
  static QString wsFast = i18n("Erases just the TOC, the PMA, and the pregap.");
  static QString wsTrack = i18n("Erases just the last track.");
  static QString wsUnclose = i18n("Reopen the last session to make it possible to append "
				  "further data.");
  static QString wsSession = i18n("Erases the last session of a multisession CD.");

  int lastMode = d->comboTypeMap[m_comboEraseMode->currentItem()];

  m_comboEraseMode->clear();
  d->comboTypeMap.clear();
  d->typeComboMap.clear();

  m_comboEraseMode->insertItem( i18n("Fast") );
  d->comboTypeMap[0] = K3bBlankingJob::Fast;
  d->typeComboMap[K3bBlankingJob::Fast] = 0;
  m_comboEraseMode->insertItem( i18n("Complete") );
  d->comboTypeMap[1] = K3bBlankingJob::Complete;
  d->typeComboMap[K3bBlankingJob::Complete] = 1;

  whatsThisInfo = "<p>" + i18n("Blanking mode:") +
    "<p><b>" + i18n("Fast") + "</b><br>" + wsFast;
  whatsThisInfo += "<p><b>" + i18n("Complete") + "</b><br>" + wsComplete;

  if ( app != K3b::CDRDAO ) {
    m_comboEraseMode->insertItem( i18n("Erase Last Track") );
    d->comboTypeMap[2] = K3bBlankingJob::Track;
    d->typeComboMap[K3bBlankingJob::Track] = 2;
    whatsThisInfo += "<p><b>" + i18n("Erase Last Track") + "</b><br>" + wsTrack;
    m_comboEraseMode->insertItem( i18n("Reopen Last Session") );
    d->comboTypeMap[3] = K3bBlankingJob::Unclose;
    d->typeComboMap[K3bBlankingJob::Unclose] = 3;
    whatsThisInfo += "<p><b>" + i18n("Reopen Last Session") + "</b><br>" + wsUnclose;
    m_comboEraseMode->insertItem( i18n("Erase Last Session") );
    d->comboTypeMap[4] = K3bBlankingJob::Session;
    d->typeComboMap[K3bBlankingJob::Session] = 4;
    whatsThisInfo += "<p><b>" + i18n("Erase Last Session") + "</b><br>" + wsSession;
  }

  QWhatsThis::add( m_comboEraseMode, whatsThisInfo );

  // try to reset last mode
  if( d->typeComboMap.contains( lastMode ) )
    m_comboEraseMode->setCurrentItem( d->typeComboMap[lastMode] );
  else
    m_comboEraseMode->setCurrentItem( d->typeComboMap[K3bBlankingJob::Fast] );
}


void K3bBlankingDialog::loadK3bDefaults()
{
  m_writerSelectionWidget->loadDefaults();
  m_comboEraseMode->setCurrentItem( d->typeComboMap[K3bBlankingJob::Fast] );
}

void K3bBlankingDialog::loadUserDefaults( KConfigBase* c )
{
  m_writerSelectionWidget->loadConfig( c );
  slotWritingAppChanged( m_writerSelectionWidget->writingApp() );

  QString mode = c->readEntry( "erase_mode" );
  kdDebug() << "(K3bBlankingDialog) slotWritingAppChanged mode: " << mode << endl;
  m_comboEraseMode->setCurrentItem( d->typeComboMap[K3bBlankingJob::Fast] );
  if( mode == "complete" )
    m_comboEraseMode->setCurrentItem( d->typeComboMap[K3bBlankingJob::Complete] );
  else if( d->typeComboMap.size() > 2 ) {
    if( mode == "session" )
      m_comboEraseMode->setCurrentItem( d->typeComboMap[K3bBlankingJob::Session] );
    else if( mode == "track" )
      m_comboEraseMode->setCurrentItem( d->typeComboMap[K3bBlankingJob::Track] );
    else if( mode == "unclose_session" )
      m_comboEraseMode->setCurrentItem( d->typeComboMap[K3bBlankingJob::Unclose] );
  }
}

void K3bBlankingDialog::saveUserDefaults( KConfigBase* c )
{
  QString mode;
  switch( d->comboTypeMap[m_comboEraseMode->currentItem()] ) {
  case K3bBlankingJob::Complete:
    mode = "complete";
    break;
  case K3bBlankingJob::Session:
    mode = "session";
    break;
  case K3bBlankingJob::Track:
    mode = "track";
    break;
  case K3bBlankingJob::Unclose:
    mode = "unclose_session";
    break;
  default:
    mode = "fast";
    break;
  }
  c->writeEntry( "erase_mode", mode );

  m_writerSelectionWidget->saveConfig( c );
}


int K3bBlankingDialog::waitForMedia( K3bDevice::Device* device,
				     int mediaState,
				     int mediaType,
				     const QString& message )
{
  // this is only needed for the formatting
  return K3bEmptyDiscWaiter::wait( device, mediaState, mediaType, message, this );
}

  
bool K3bBlankingDialog::questionYesNo( const QString& text,
				       const QString& caption,
				       const QString& yesText,
				       const QString& noText )
{
  return ( KMessageBox::questionYesNo( this, 
				       text, 
				       caption, 
				       yesText.isEmpty() ? KStdGuiItem::yes() : KGuiItem(yesText),
				       noText.isEmpty() ? KStdGuiItem::no() : KGuiItem(noText) ) == KMessageBox::Yes );
}


void K3bBlankingDialog::blockingInformation( const QString& text,
					     const QString& caption )
{
  KMessageBox::information( this, text, caption );
}

#include "k3bblankingdialog.moc"

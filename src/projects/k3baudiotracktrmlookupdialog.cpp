/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include <config.h>

#ifdef HAVE_MUSICBRAINZ

#include "k3baudiotracktrmlookupdialog.h"
#include "k3bmusicbrainzjob.h"

#include <k3bbusywidget.h>
#include <k3baudiotrack.h>
#include <k3baudiofile.h>

#include <kmessagebox.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kdebug.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qframe.h>
#include <qeventloop.h>
#include <qpushbutton.h>
#include <qapplication.h>


K3bAudioTrackTRMLookupDialog::K3bAudioTrackTRMLookupDialog( QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, 
		 i18n("MusicBrainz Query"), 
		 KDialogBase::Cancel,
		 KDialogBase::Cancel,
		 parent, 
		 name,
		 true,
		 true )
{
  QGridLayout* grid = new QGridLayout( plainPage() );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );

  m_infoLabel = new QLabel( plainPage() );
  QLabel* pixLabel = new QLabel( plainPage() );
  pixLabel->setPixmap( KGlobal::iconLoader()->loadIcon( "musicbrainz", KIcon::NoGroup, 64 ) );
  pixLabel->setScaledContents( false );

  m_busyWidget = new K3bBusyWidget( plainPage() );

  grid->addMultiCellWidget( pixLabel, 0, 1, 0, 0 );
  grid->addWidget( m_infoLabel, 0, 1 );
  grid->addWidget( m_busyWidget, 1, 1 );

  m_inLoop = false;
  m_mbJob = new K3bMusicBrainzJob( this );
  connect( m_mbJob, SIGNAL(infoMessage(const QString&, int)), 
	   this, SLOT(slotMbJobInfoMessage(const QString&, int)) );
  connect( m_mbJob, SIGNAL(finished(bool)), this, SLOT(slotMbJobFinished(bool)) );
}


K3bAudioTrackTRMLookupDialog::~K3bAudioTrackTRMLookupDialog()
{
}


int K3bAudioTrackTRMLookupDialog::lookup( const QPtrList<K3bAudioTrack>& tracks )
{
  m_mbJob->setTracks( tracks );
  m_mbJob->start();

  m_busyWidget->showBusy(true);
  setModal( true );
  show();
  m_inLoop = true;
  QApplication::eventLoop()->enterLoop();

  return 0;
}


void K3bAudioTrackTRMLookupDialog::slotMbJobInfoMessage( const QString& message, int )
{
  m_infoLabel->setText( message );
}


void K3bAudioTrackTRMLookupDialog::slotMbJobFinished( bool )
{
  m_busyWidget->showBusy(false);
  hide();
  if( m_inLoop )
    QApplication::eventLoop()->exitLoop();
}


void K3bAudioTrackTRMLookupDialog::slotCancel()
{
  actionButton( Cancel )->setEnabled( false );
  m_mbJob->cancel();
}

#include "k3baudiotracktrmlookupdialog.moc"

#endif

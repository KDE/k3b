/* 
 *
 * $Id: $
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


#include "k3bemptydiscwaiter.h"
#include "k3bdevice.h"
#include "k3bblankingjob.h"
#include "tools/k3bbusywidget.h"
#include "k3bdiskerasinginfodialog.h"
#include "k3b.h"

#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>


K3bEmptyDiscWaiter::K3bEmptyDiscWaiter( K3bDevice* device, QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, i18n("Waiting for disk"), KDialogBase::Cancel | KDialogBase::User1, 
		 KDialogBase::Cancel, parent, name, true, true, i18n("Force") )
{
  m_timer = new QTimer( this );
  m_device = device;

  m_label = new QLabel( plainPage() );
  m_label->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

  QLabel* pixLabel = new QLabel( plainPage() );
  pixLabel->setAlignment( Qt::AlignCenter | Qt::AlignVCenter );
  pixLabel->setPixmap( KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", KIcon::NoGroup, KIcon::SizeMedium ) );

  QHBoxLayout* box = new QHBoxLayout( plainPage() );
  box->setSpacing( 20 );
  box->setMargin( marginHint() );
  box->addWidget( pixLabel );
  box->addWidget( m_label );
  box->setStretchFactor( m_label, 1 );

  QToolTip::add( actionButton(KDialogBase::User1), i18n("Force K3b to continue if it seems not to detect your empty CDR.") );
}


K3bEmptyDiscWaiter::~K3bEmptyDiscWaiter()
{
}

int K3bEmptyDiscWaiter::waitForEmptyDisc( bool appendable )
{
  m_apppendable = appendable;

  if( appendable )
    m_label->setText( i18n("Please insert an appendable CDR medium into drive<p><b>%1 %2 (%3)</b>.").arg(m_device->vendor()).arg(m_device->description()).arg(m_device->devicename()) );
  else
    m_label->setText( i18n("Please insert an empty CDR medium into drive<p><b>%1 %2 (%3)</b>.").arg(m_device->vendor()).arg(m_device->description()).arg(m_device->devicename()) );

  adjustSize();

  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotTestForEmptyCd()) );
  m_timer->start(1000);

  return exec();
}


void K3bEmptyDiscWaiter::slotTestForEmptyCd()
{
  int x = m_device->isEmpty();
  if( x == K3bDevice::EMPTY || ( x == K3bDevice::APPENDABLE && m_apppendable ) ) {
    m_timer->stop();
    
    done( DISK_READY );
  }
  else if( x != -1 ) {
    if( m_device->rewritable() ) {
      m_timer->stop();

      if( KMessageBox::questionYesNo( this, i18n("K3b found a rewritable disk. Should it be erased?"),
				      i18n("Found CD-RW") ) == KMessageBox::Yes ) {
	// start a k3bblankingjob
	K3bErasingInfoDialog d;

	K3bBlankingJob job;
	job.setDevice( m_device );
	job.setMode( K3bBlankingJob::Fast );
	connect( &job, SIGNAL(finished(bool)), &d, SLOT(slotFinished(bool)) );
	connect( &d, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
	job.start();
	d.exec();

	if( !k3bMain()->eject() )  // the job ejected the cd otherwise
	  m_device->eject();
      }
      else
	m_device->eject();

      m_timer->start(1000);
    }
  }
}


void K3bEmptyDiscWaiter::slotCancel()
{
  m_timer->stop();

  done( CANCELED );
}


void K3bEmptyDiscWaiter::slotUser1()
{
  m_timer->stop();

  done( DISK_READY );
}


#include "k3bemptydiscwaiter.moc"

/***************************************************************************
                          k3bemptydiscwaiter.cpp  -  description
                             -------------------
    begin                : Mon Oct 22 2001
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

#include "k3bemptydiscwaiter.h"
#include "k3bdevice.h"
#include "../k3bblankingjob.h"
#include "../k3bbusywidget.h"

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

  QToolTip::add( actionButton(KDialogBase::User1), i18n("Force K3b to continue if it seems not to detect your empty cdr") );
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

  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotTestForEmptyCd()) );
  m_timer->start(1000);

  return exec();
}


void K3bEmptyDiscWaiter::slotTestForEmptyCd()
{
  int x = m_device->isEmpty();
  if( x == 0 || ( x == 1 && m_apppendable ) ) {
    m_timer->stop();
    
    done( DISK_READY );
  }
  else if( x != -1 ) {
    if( m_device->rewritable() ) {
      m_timer->stop();

      if( KMessageBox::questionYesNo( this, i18n("K3b found a rewritable disk. Should it be erased?"),
				      i18n("Found CD-RW") ) == KMessageBox::Yes ) {
	// start a k3bblankingjob
	ErasingInfoDialog d;

	K3bBlankingJob job;
	job.setDevice( m_device );
	job.setMode( K3bBlankingJob::Fast );
	job.setForce( true );
	connect( &job, SIGNAL(finished(bool)), &d, SLOT(slotFinished(bool)) );
	connect( &d, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
	job.start();
	d.exec();

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




K3bEmptyDiscWaiter::ErasingInfoDialog::ErasingInfoDialog( QWidget* parent, const char* name ) 
  : KDialogBase( parent, name, true, i18n("Erasing"), Cancel|Ok, Ok, true )
{
  QFrame* main = makeMainWidget();
  QGridLayout* mainLayout = new QGridLayout( main );
  mainLayout->setMargin( marginHint() );
  mainLayout->setSpacing( spacingHint() );

  m_label = new QLabel( i18n("Erasing CD-RW"), main );
  m_busyWidget = new K3bBusyWidget( main );

  mainLayout->addWidget( m_label, 0, 0 );
  mainLayout->addWidget( m_busyWidget, 1, 0 );

  showButtonOK( false );
  m_busyWidget->showBusy( true );
}


K3bEmptyDiscWaiter::ErasingInfoDialog::~ErasingInfoDialog()
{}


void K3bEmptyDiscWaiter::ErasingInfoDialog::slotFinished( bool success )
{
  m_busyWidget->showBusy( false );

  if( success )
    slotClose();
  else {
    showButtonOK( true );
    showButtonCancel( false );
    m_label->setText( i18n("Sorry, K3b was not able to erase the disk.") );
  }
}


#include "k3bemptydiscwaiter.moc"

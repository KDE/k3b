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


#include "k3bemptydiscwaiter.h"
#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3bblankingjob.h"
#include "tools/k3bbusywidget.h"
#include "k3bdiskerasinginfodialog.h"
#include "k3b.h"

#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <qeventloop.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>


class K3bEmptyDiscWaiter::Private
{
public:
  Private() {
    dialogVisible = false;
    inLoop = false;
  }

  K3bDevice* device;
  QPushButton* buttonCancel;
  QPushButton* buttonForce;

  bool appendable;

  int result;
  int dialogVisible;
  bool inLoop;

  bool forced;
  bool canceled;

  QLabel* label;

  K3bCdDevice::DeviceHandler* deviceHandler;
};



K3bEmptyDiscWaiter::K3bEmptyDiscWaiter( K3bDevice* device, QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, i18n("Waiting for disk"), KDialogBase::Cancel | KDialogBase::User1, 
		 KDialogBase::Cancel, parent, name, true, true, i18n("Force") )
{
  d = new Private();
  d->device = device;

  d->deviceHandler = new K3bCdDevice::DeviceHandler( device, this );
  connect( d->deviceHandler, SIGNAL(finished(bool)),
	   this, SLOT(slotDeviceHandlerFinished(bool)) );

  // setup the gui
  // -----------------------------
  d->label = new QLabel( plainPage() );
  d->label->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
  QLabel* pixLabel = new QLabel( plainPage() );
  pixLabel->setAlignment( Qt::AlignCenter | Qt::AlignVCenter );
  pixLabel->setPixmap( KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", 
								    KIcon::NoGroup, KIcon::SizeMedium ) );
  QHBoxLayout* box = new QHBoxLayout( plainPage() );
  box->setSpacing( 20 );
  box->setMargin( marginHint() );
  box->addWidget( pixLabel );
  box->addWidget( d->label );
  box->setStretchFactor( d->label, 1 );
  // -----------------------------


  QToolTip::add( actionButton(KDialogBase::User1), 
		 i18n("Force K3b to continue if it seems not to detect your empty CDR.") );
}


K3bEmptyDiscWaiter::~K3bEmptyDiscWaiter()
{
  delete d;
}


int K3bEmptyDiscWaiter::waitForEmptyDisc( bool appendable )
{
  if ( d->inLoop ) {
    kdError() << "(K3bEmptyDiscWaiter) Recursive call detected." << endl;
    return -1;
  }

  d->appendable = appendable;
  d->dialogVisible = false;
  d->forced = false;
  d->canceled = false;

  if( appendable )
    d->label->setText( i18n("Please insert an appendable CDR medium into drive<p><b>%1 %2 (%3)</b>.").arg(d->device->vendor()).arg(d->device->description()).arg(d->device->devicename()) );
  else
    d->label->setText( i18n("Please insert an empty CDR medium into drive<p><b>%1 %2 (%3)</b>.").arg(d->device->vendor()).arg(d->device->description()).arg(d->device->devicename()) );

  adjustSize();

  startDeviceHandler();

  d->inLoop = true;
  QApplication::eventLoop()->enterLoop();
  
  return d->result;
}


int K3bEmptyDiscWaiter::exec()
{
  return waitForEmptyDisc( false );
}


void K3bEmptyDiscWaiter::startDeviceHandler()
{
  d->deviceHandler->sendCommand( K3bCdDevice::DeviceHandler::MEDIUM_STATE );
}


void K3bEmptyDiscWaiter::slotDeviceHandlerFinished( bool success )
{
  if( d->forced || d->canceled )
    return;

  if( success ) {
    int x = d->deviceHandler->errorCode();
    if( x == K3bDevice::EMPTY || ( x == K3bDevice::APPENDABLE && d->appendable ) ) {
      
      finishWaiting( DISK_READY );
    }
    else {
      if( x == K3bDevice::COMPLETE || x == K3bDevice::APPENDABLE ) {
	
	// this should not block for long since the device has been opened recently
	if( d->device->rewritable() ) {
	  if( KMessageBox::questionYesNo( this, i18n("K3b found a rewritable disk. Should it be erased?"),
					  i18n("Found CD-RW") ) == KMessageBox::Yes ) {
	    // start a k3bblankingjob
	    K3bErasingInfoDialog infoDialog;
	    
	    K3bBlankingJob job;
	    job.setDevice( d->device );
	    job.setMode( K3bBlankingJob::Fast );
	    connect( &job, SIGNAL(finished(bool)), &infoDialog, SLOT(slotFinished(bool)) );
	    connect( &infoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
	    job.start();
	    infoDialog.exec();
	    
	    K3bCdDevice::eject( d->device );
	  }
	  else
	    K3bCdDevice::eject( d->device );
	}
      }
      else {
	// we need to show the dialog if not done already
	if( !d->dialogVisible ) {
	  d->dialogVisible = true;
	  clearWFlags( WDestructiveClose );
	  setWFlags( WShowModal );
	  show();
	}
      }
      
      QTimer::singleShot( 1000, this, SLOT(startDeviceHandler()) );
    }
  }
  else { // success == false
    QTimer::singleShot( 1000, this, SLOT(startDeviceHandler()) );
  }
}


void K3bEmptyDiscWaiter::slotCancel()
{
  d->canceled = true;
  finishWaiting( CANCELED );
}


void K3bEmptyDiscWaiter::slotUser1()
{
  d->forced = true;
  finishWaiting( DISK_READY );
}


void K3bEmptyDiscWaiter::finishWaiting( int code )
{
  d->deviceHandler->cancel();

  d->result = code;
  if( d->dialogVisible )
    hide();

  if( d->inLoop ) {
    d->inLoop = false;
    QApplication::eventLoop()->exitLoop();
  }
}


int K3bEmptyDiscWaiter::wait( K3bDevice* device, bool appendable )
{
  K3bEmptyDiscWaiter d( device );
  return d.waitForEmptyDisc( appendable );
}

#include "k3bemptydiscwaiter.moc"

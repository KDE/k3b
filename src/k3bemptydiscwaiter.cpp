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
#include <device/k3bdevice.h>
#include <device/k3bdevicehandler.h>
#include "k3bblankingjob.h"
#include "tools/k3bbusywidget.h"
#include "k3bdiskerasinginfodialog.h"

#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <qeventloop.h>
#include <qfont.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kactivelabel.h>


class K3bEmptyDiscWaiter::Private
{
public:
  Private() {
    dialogVisible = false;
    inLoop = false;
  }

  K3bCdDevice::CdDevice* device;
  QPushButton* buttonCancel;
  QPushButton* buttonForce;

  bool appendable;

  int wantedMediaType;

  int result;
  int dialogVisible;
  bool inLoop;

  bool forced;
  bool canceled;

  QLabel* labelRequest;
  QLabel* labelFoundMedia;
  QLabel* pixLabel;

  K3bCdDevice::DeviceHandler* deviceHandler;
};



K3bEmptyDiscWaiter::K3bEmptyDiscWaiter( K3bDevice* device, QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, i18n("Waiting for disk"), KDialogBase::Cancel | KDialogBase::User1, 
		 KDialogBase::Cancel, parent, name, true, true, i18n("Force") )
{
  d = new Private();
  d->device = device;

  d->deviceHandler = new K3bCdDevice::DeviceHandler( device, this );
  connect( d->deviceHandler, SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
	   this, SLOT(slotDeviceHandlerFinished(K3bCdDevice::DeviceHandler*)) );

  // setup the gui
  // -----------------------------
  d->labelRequest = new QLabel( plainPage() );
  d->labelRequest->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
  d->labelFoundMedia = new QLabel( plainPage() );
  d->pixLabel = new QLabel( plainPage() );
  d->pixLabel->setAlignment( Qt::AlignHCenter | Qt::AlignTop );

  QFont f( d->labelFoundMedia->font() );
  f.setBold(true);
  d->labelFoundMedia->setFont( f );

  QGridLayout* grid = new QGridLayout( plainPage() );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );

  grid->addMultiCellWidget( d->pixLabel, 0, 2, 0, 0 );
  grid->addColSpacing( 1, 20 );
  grid->addWidget( new QLabel( i18n("Found media:"), plainPage() ), 0, 2 );
  grid->addWidget( d->labelFoundMedia, 0, 3 );
  grid->addMultiCellWidget( d->labelRequest, 1, 1, 2, 3 );
  grid->setRowStretch( 2, 1 );
  grid->setColStretch( 3, 1 );
  // -----------------------------


  QToolTip::add( actionButton(KDialogBase::User1), 
		 i18n("Force K3b to continue if it seems not to detect your empty CD-R(W)/DVD±R(W).") );
}


K3bEmptyDiscWaiter::~K3bEmptyDiscWaiter()
{
  delete d;
}


int K3bEmptyDiscWaiter::waitForEmptyDisc( bool appendable, int mediaType )
{
  if ( d->inLoop ) {
    kdError() << "(K3bEmptyDiscWaiter) Recursive call detected." << endl;
    return -1;
  }

  d->appendable = appendable;
  d->wantedMediaType = mediaType;
  d->dialogVisible = false;
  d->forced = false;
  d->canceled = false;

  QString m;
  if( (d->wantedMediaType & K3bCdDevice::MEDIA_WRITABLE_DVD) &&
      (d->wantedMediaType & K3bCdDevice::MEDIA_WRITABLE_CD) )
    m = i18n("CDR or DVDR");
  else if( d->wantedMediaType & K3bCdDevice::MEDIA_WRITABLE_DVD )
    m = i18n("DVDR(W)");
  else
    m = i18n("CDR(W)");

  if( appendable )
    d->labelRequest->setText( i18n("Please insert an appendable medium (%4) into drive<p><b>%1 %2 (%3)</b>.").arg(d->device->vendor()).arg(d->device->description()).arg(d->device->devicename()).arg( m ) );
  else
    d->labelRequest->setText( i18n("Please insert an empty medium (%4) into drive<p><b>%1 %2 (%3)</b>.").arg(d->device->vendor()).arg(d->device->description()).arg(d->device->devicename()).arg( m ) );

  if( d->wantedMediaType & K3bCdDevice::MEDIA_WRITABLE_DVD )
    d->pixLabel->setPixmap( KGlobal::instance()->iconLoader()->loadIcon( "dvd_unmount", 
									 KIcon::NoGroup, KIcon::SizeMedium ) );
  else
    d->pixLabel->setPixmap( KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount", 
									 KIcon::NoGroup, KIcon::SizeMedium ) );

  adjustSize();

  startDeviceHandler();

  d->inLoop = true;
  QApplication::eventLoop()->enterLoop();
  
  kdDebug() << "(K3bEmptyDiscWaiter) waitForEmptyDisc() finished" << endl;

  return d->result;
}


int K3bEmptyDiscWaiter::exec()
{
  return waitForEmptyDisc( false );
}


void K3bEmptyDiscWaiter::startDeviceHandler()
{
  //
  // For some reason utilizing the DeviceHandler more than once introduces problems.
  //

  connect( K3bCdDevice::sendCommand( K3bCdDevice::DeviceHandler::NG_DISKINFO, d->device ), 
	   SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
	   this, 
	   SLOT(slotDeviceHandlerFinished(K3bCdDevice::DeviceHandler*)) );
  //  d->deviceHandler->sendCommand( K3bCdDevice::DeviceHandler::NG_DISKINFO );
}


void K3bEmptyDiscWaiter::slotDeviceHandlerFinished( K3bCdDevice::DeviceHandler* dh )
{
  kdDebug() << "(K3bEmptyDiscWaiter) slotDeviceHandlerFinished() " << endl;
  if( d->forced || d->canceled )
    return;

  QString mediaState;
  if( dh->ngDiskInfo().diskState() == K3bCdDevice::STATE_COMPLETE )
    mediaState = i18n("complete");
  else if( dh->ngDiskInfo().diskState() == K3bCdDevice::STATE_INCOMPLETE )
    mediaState = i18n("appendable");
  else if( dh->ngDiskInfo().diskState() == K3bCdDevice::STATE_EMPTY )
    mediaState = i18n("empty");

  if( !mediaState.isEmpty() )
    mediaState = " (" + mediaState +")";

  d->labelFoundMedia->setText( K3bCdDevice::mediaTypeString( d->deviceHandler->ngDiskInfo().mediaType() ) 
			       + mediaState );

  if( dh->success() ) {
    if( d->wantedMediaType & dh->ngDiskInfo().mediaType() 
      &&
	( (dh->ngDiskInfo().empty() && !d->appendable)
	  ||
	  (dh->ngDiskInfo().appendable() && d->appendable)
	  ) )
	finishWaiting( dh->ngDiskInfo().mediaType() );
    else {
      if( dh->ngDiskInfo().rewritable() ) {
	
	// this should not block for long since the device has been opened recently
	if( d->device->rewritable() ) {
	  if( KMessageBox::questionYesNo( qApp->activeWindow(),
					  i18n("K3b found rewritable disk in %1 - %2. "
					       "Should it be erased?").arg(d->device->vendor()).arg(d->device->description()),
					  i18n("Found rewritable disk") ) == KMessageBox::Yes ) {
	    // start a k3bblankingjob
	    K3bErasingInfoDialog infoDialog( qApp->activeWindow() );
	    
	    //
	    // TODO: check for DVD and use a DVDErasingJob if so
	    //

	    K3bBlankingJob job;
	    job.setDevice( d->device );
	    job.setMode( K3bBlankingJob::Fast );
	    connect( &job, SIGNAL(finished(bool)), &infoDialog, SLOT(slotFinished(bool)) );
	    connect( &infoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
	    job.start();
	    // if the Job did not eject we do it...
	    K3bCdDevice::reload( d->device );
	    infoDialog.exec();
	  }
	  else
	    K3bCdDevice::eject( d->device );
	}
      }
      else {
	showDialog();
      }
      
      QTimer::singleShot( 1000, this, SLOT(startDeviceHandler()) );
    }
  }
  else { // success == false
    kdDebug() << "(K3bEmptyDiscWaiter) devicehandler error." << endl;
    showDialog();
    QTimer::singleShot( 1000, this, SLOT(startDeviceHandler()) );
  }
}


void K3bEmptyDiscWaiter::showDialog()
{
  // we need to show the dialog if not done already
  if( !d->dialogVisible ) {
    d->dialogVisible = true;
    clearWFlags( WDestructiveClose );
    setWFlags( WShowModal );
    setResult( 0 );
    show();
  }
}


void K3bEmptyDiscWaiter::slotCancel()
{
  kdDebug() << "(K3bEmptyDiscWaiter) slotCancel() " << endl;
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
  kdDebug() << "(K3bEmptyDiscWaiter) finishWaiting() " << endl;
  d->deviceHandler->cancel();

  d->result = code;
  if( d->dialogVisible )
    hide();

  if( d->inLoop ) {
    d->inLoop = false;
    kdDebug() << "(K3bEmptyDiscWaiter) exitLoop " << endl;
    QApplication::eventLoop()->exitLoop();
  }
}


int K3bEmptyDiscWaiter::wait( K3bDevice* device, bool appendable, int mediaType )
{
  K3bEmptyDiscWaiter d( device, qApp->activeWindow() );
  return d.waitForEmptyDisc( appendable, mediaType );
}

#include "k3bemptydiscwaiter.moc"

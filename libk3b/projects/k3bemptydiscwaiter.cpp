/*
 *
 * $Id$
 * Copyright (C) 2003-2004 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bemptydiscwaiter.h"
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
#include <k3bdevicehandler.h>
#include <k3bglobals.h>
#include <k3bcore.h>
#include <k3biso9660.h>
#include "k3bblankingjob.h"
#include <k3bbusywidget.h>
#include <k3bprogressdialog.h>
#include "datadvd/k3bdvdformattingjob.h"

#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <qeventloop.h>
#include <qfont.h>

#include <klocale.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kactivelabel.h>
#include <knotifyclient.h>


class K3bEmptyDiscWaiter::Private
{
public:
  Private()
    : erasingInfoDialog(0) {
    dialogVisible = false;
    inLoop = false;
  }

  K3bDevice::Device* device;

  int wantedMediaType;
  int wantedMediaState;

  int result;
  int dialogVisible;
  bool inLoop;

  bool forced;
  bool canceled;

  QLabel* labelRequest;
  QLabel* labelFoundMedia;
  QLabel* pixLabel;

  K3bProgressDialog* erasingInfoDialog;
};



K3bEmptyDiscWaiter::K3bEmptyDiscWaiter( K3bDevice::Device* device, QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, i18n("Waiting for disk"), 
		 KDialogBase::Cancel|KDialogBase::User1|KDialogBase::User2|KDialogBase::User3, 
		 KDialogBase::Cancel, parent, name, true, true, i18n("Force"), i18n("Eject"), i18n("Load") )
{
  d = new Private();
  d->device = device;

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


int K3bEmptyDiscWaiter::waitForDisc( int mediaState, int mediaType, const QString& message )
{
  if ( d->inLoop ) {
    kdError() << "(K3bEmptyDiscWaiter) Recursive call detected." << endl;
    return -1;
  }

  d->wantedMediaState = mediaState;
  d->wantedMediaType = mediaType;
  d->dialogVisible = false;
  d->forced = false;
  d->canceled = false;

  QString m;
  if( (d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_DVD) &&
	   (d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_CD) )
    m = i18n("CD-R(W) or DVD±R(W)");
  else if( d->wantedMediaType == K3bDevice::MEDIA_DVD_PLUS_R_DL )
    m = i18n("Dual-Layer DVD+R");
  else if( d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_DVD )
    m = i18n("DVD±R(W)");
  else
    m = i18n("CD-R(W)");

  if( message.isEmpty() ) {
    //
    // We do not cover every case here but just the once that really make sense
    //
    if( (d->wantedMediaState & K3bDevice::STATE_COMPLETE) && (d->wantedMediaState & K3bDevice::STATE_INCOMPLETE) )
      d->labelRequest->setText( i18n("Please insert a complete or appendable medium (%4) "
				     "into drive<p><b>%1 %2 (%3)</b>.")
				.arg(d->device->vendor())
				.arg(d->device->description())
				.arg(d->device->devicename())
				.arg( m ) );
    else if( d->wantedMediaState & K3bDevice::STATE_COMPLETE )
      d->labelRequest->setText( i18n("Please insert a complete medium (%4) "
				     "into drive<p><b>%1 %2 (%3)</b>.")
				.arg(d->device->vendor())
				.arg(d->device->description())
				.arg(d->device->devicename())
				.arg( m ) );
    else if( d->wantedMediaState & K3bDevice::STATE_INCOMPLETE )
      d->labelRequest->setText( i18n("Please insert an appendable medium (%4) "
				     "into drive<p><b>%1 %2 (%3)</b>.")
				.arg(d->device->vendor())
				.arg(d->device->description())
				.arg(d->device->devicename())
				.arg( m ) );
    else
      d->labelRequest->setText( i18n("Please insert an empty medium (%4) "
				     "into drive<p><b>%1 %2 (%3)</b>.")
				.arg(d->device->vendor())
				.arg(d->device->description())
				.arg(d->device->devicename())
				.arg( m ) );
  }
  else
    d->labelRequest->setText( message );

  if( d->wantedMediaType & K3bDevice::MEDIA_WRITABLE_DVD )
    d->pixLabel->setPixmap( KGlobal::instance()->iconLoader()->loadIcon( "dvd_unmount",
									 KIcon::NoGroup, KIcon::SizeMedium ) );
  else
    d->pixLabel->setPixmap( KGlobal::instance()->iconLoader()->loadIcon( "cdwriter_unmount",
									 KIcon::NoGroup, KIcon::SizeMedium ) );

  adjustSize();

  kdDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: startup." << endl;
  startDeviceHandler();

  d->inLoop = true;
  QApplication::eventLoop()->enterLoop();

  kdDebug() << "(K3bEmptyDiscWaiter) waitForEmptyDisc() finished" << endl;

  return d->result;
}


int K3bEmptyDiscWaiter::exec()
{
  return waitForDisc();
}


void K3bEmptyDiscWaiter::startDeviceHandler()
{
  //
  // For some reason utilizing the DeviceHandler more than once introduces problems.
  //

  kdDebug() << "(K3bEmptyDiscWaiter) STARTING DEVCEHANDLER." << endl << endl;

  connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::NG_DISKINFO, d->device ), 
	   SIGNAL(finished(K3bDevice::DeviceHandler*)),
	   this, 
	   SLOT(slotDeviceHandlerFinished(K3bDevice::DeviceHandler*)) );
}


void K3bEmptyDiscWaiter::slotDeviceHandlerFinished( K3bDevice::DeviceHandler* dh )
{
  kdDebug() << "(K3bEmptyDiscWaiter) slotDeviceHandlerFinished() " << endl;
  if( d->forced || d->canceled )
    return;

  KConfig* c = k3bcore->config();
  c->setGroup( "General Options" );
  bool formatWithoutAsking = c->readBoolEntry( "auto rewritable erasing", false );

  if( dh->diskInfo().diskState() == K3bDevice::STATE_NO_MEDIA ) {
    d->labelFoundMedia->setText( i18n("No media") );
    showDialog();
    QTimer::singleShot( 1000, this, SLOT(startDeviceHandler()) );
    return;
  }

  QString mediaState;
  if( dh->diskInfo().diskState() == K3bDevice::STATE_COMPLETE )
    mediaState = i18n("complete");
  else if( dh->diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE )
    mediaState = i18n("appendable");
  else if( dh->diskInfo().diskState() == K3bDevice::STATE_EMPTY )
    mediaState = i18n("empty");

  if( !mediaState.isEmpty() )
    mediaState = " (" + mediaState +")";

  d->labelFoundMedia->setText( K3bDevice::mediaTypeString( dh->diskInfo().mediaType() ) 
			       + mediaState );

  if( dh->success() ) {

    // /////////////////////////////////////////////////////////////
    //
    // DVD+RW handling
    //
    // /////////////////////////////////////////////////////////////

    // DVD+RW: if empty we need to preformat. Although growisofs does it before writing doing it here
    //         allows better control and a progress bar. If it's not empty we shoud check if there is 
    //         already a filesystem on the media. 
    if( (d->wantedMediaType & K3bDevice::MEDIA_DVD_PLUS_RW) &&
	(dh->diskInfo().mediaType() & K3bDevice::MEDIA_DVD_PLUS_RW) ) {

      kdDebug() << "(K3bEmptyDiscWaiter) ------ found DVD+RW as wanted." << endl;

      if( dh->diskInfo().diskState() == K3bDevice::STATE_EMPTY ) {

	// special case for the formatting job which wants to preformat itself!
	if( d->wantedMediaState & K3bDevice::STATE_COMPLETE &&
	    d->wantedMediaState & K3bDevice::STATE_EMPTY ) {
	  kdDebug() << "(K3bEmptyDiscWaiter) special case: DVD*RW for the formatting job." << endl;
	  finishWaiting( K3bDevice::MEDIA_DVD_PLUS_RW );
	}
	else {
	  // empty - preformat without asking
	  prepareErasingDialog();
	  
	  K3bDvdFormattingJob job( this );
	  job.setDevice( d->device );
	  job.setQuickFormat( true );
	  job.setForce( false );
	  job.setForceNoEject( true );
	  
	  d->erasingInfoDialog->setText( i18n("Preformatting DVD+RW") );
	  connect( &job, SIGNAL(finished(bool)), this, SLOT(slotErasingFinished(bool)) );
	  connect( &job, SIGNAL(percent(int)), d->erasingInfoDialog, SLOT(setProgress(int)) );
	  connect( d->erasingInfoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
	  job.start(dh);
	  d->erasingInfoDialog->exec(true);
	}
      }
      else {
	if( d->wantedMediaState == K3bDevice::STATE_EMPTY ) {
	  // check if the media contains a filesystem
	  d->device->open();
	  K3bIso9660 isoF( d->device );
	  bool hasIso = isoF.open();
	  d->device->close();
	  
	  if( formatWithoutAsking || 
	      !hasIso || 
	      KMessageBox::questionYesNo( parentWidgetToUse(),
					  i18n("Found %1 media in %2 - %3. "
					       "Should it be overwritten?")
					  .arg("DVD+RW")
					  .arg(d->device->vendor())
					  .arg(d->device->description()),
					  i18n("Found %1").arg("DVD+RW") ) == KMessageBox::Yes ) {
	    finishWaiting( K3bDevice::MEDIA_DVD_PLUS_RW );
	  }
	  else {
	    kdDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: no DVD+RW overwrite" << endl;
	    connect( K3bDevice::eject( d->device ), 
		     SIGNAL(finished(K3bDevice::DeviceHandler*)),
		     this, 
		     SLOT(startDeviceHandler()) );
	  }
	}
	else {  // complete or appendable media wanted
	  // the isofs will be grown
	  finishWaiting( K3bDevice::MEDIA_DVD_PLUS_RW );
	}
      }
    } // --- DVD+RW --------


    // /////////////////////////////////////////////////////////////
    //
    // DVD-RW handling
    //
    // /////////////////////////////////////////////////////////////

    //
    // DVD-RW in sequential mode can be empty. DVD-RW in restricted overwrite mode is always complete.
    //
    else if( (d->wantedMediaType & (K3bDevice::MEDIA_DVD_RW|
				    K3bDevice::MEDIA_DVD_RW_SEQ|
				    K3bDevice::MEDIA_DVD_RW_OVWR) ) &&
	     (dh->diskInfo().mediaType() & (K3bDevice::MEDIA_DVD_RW|
					      K3bDevice::MEDIA_DVD_RW_SEQ|
					      K3bDevice::MEDIA_DVD_RW_OVWR) ) ) {

      kdDebug() << "(K3bEmptyDiscWaiter) ------ found DVD-R(W) as wanted." << endl;

      // we format in the following cases:
      // seq. incr. and not empty and empty requested
      // seq. incr. and restr. overwri. reqested
      // restr. ovw. and seq. incr. requested

      // we have exactly what was requested
      if( (d->wantedMediaType & dh->diskInfo().currentProfile()) &&
	  (d->wantedMediaState & dh->diskInfo().diskState()) ) {
	finishWaiting( dh->diskInfo().currentProfile() );
      }

      // DVD-RW in restr. overwrite may just------  be overwritten
      else if( (dh->diskInfo().currentProfile() & K3bDevice::MEDIA_DVD_RW_OVWR) &&
	  (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_OVWR) &&
	  (d->wantedMediaState == K3bDevice::STATE_EMPTY) ) {

	kdDebug() << "(K3bEmptyDiscWaiter) ------ DVD-RW restricted overwrite." << endl;

	// check if the media contains a filesystem
	d->device->open();
	K3bIso9660 isoF( d->device );
	bool hasIso = isoF.open();
	d->device->close();

	if( formatWithoutAsking ||
	    !hasIso || 
	    KMessageBox::questionYesNo( parentWidgetToUse(),
					i18n("Found %1 media in %2 - %3. "
					     "Should it be overwritten?")
					.arg(K3bDevice::mediaTypeString(dh->diskInfo().currentProfile()))
					.arg(d->device->vendor())
					.arg(d->device->description()),
					i18n("Found %1").arg("DVD-RW") ) == KMessageBox::Yes ) {
	  finishWaiting( K3bDevice::MEDIA_DVD_RW_OVWR );
	}
	else {
	  kdDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: no DVD-RW overwrite." << endl;
	  connect( K3bDevice::eject( d->device ), 
		   SIGNAL(finished(K3bDevice::DeviceHandler*)),
		   this, 
		   SLOT(startDeviceHandler()) );
	}
      }
      // formatting
      else if( ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_OVWR) &&
		 (dh->diskInfo().currentProfile() & K3bDevice::MEDIA_DVD_RW_SEQ) &&
		 !(d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ) ) ||

	       ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ) &&
		 (dh->diskInfo().currentProfile() & K3bDevice::MEDIA_DVD_RW_OVWR) &&
		 !(d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_OVWR) ) ||

	       ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ) &&
		 (dh->diskInfo().currentProfile() & K3bDevice::MEDIA_DVD_RW_SEQ) &&
		 (d->wantedMediaState == K3bDevice::STATE_EMPTY) &&
		 (dh->diskInfo().diskState() != K3bDevice::STATE_EMPTY) ) ) {

	kdDebug() << "(K3bEmptyDiscWaiter) ------ DVD-RW needs to be formated." << endl;

	if( formatWithoutAsking ||
	    KMessageBox::questionYesNo( parentWidgetToUse(),
					i18n("Found %1 media in %2 - %3. "
					     "Should it be formatted?")
					.arg( K3bDevice::mediaTypeString(dh->diskInfo().currentProfile()) )
					.arg(d->device->vendor())
					.arg(d->device->description()),
					i18n("Found %1").arg("DVD-RW") ) == KMessageBox::Yes ) {

	  kdDebug() << "(K3bEmptyDiscWaiter) ------ formatting DVD-RW." << endl;	  

	  prepareErasingDialog();

	  K3bDvdFormattingJob job( this );
	  job.setDevice( d->device );
	  // we prefere the current mode of the media if no special mode has been requested
	  job.setMode( ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ) &&
			 (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_OVWR) )
		       ? ( dh->diskInfo().currentProfile() == K3bDevice::MEDIA_DVD_RW_OVWR
			   ? K3b::WRITING_MODE_RES_OVWR
			   : K3b::WRITING_MODE_INCR_SEQ )
		       : ( (d->wantedMediaType & K3bDevice::MEDIA_DVD_RW_SEQ) 
			   ? K3b::WRITING_MODE_INCR_SEQ 
			   : K3b::WRITING_MODE_RES_OVWR ) );
	  job.setQuickFormat( true );
	  job.setForce( false );
	  job.setForceNoEject(true);

	  d->erasingInfoDialog->setText( i18n("Formatting DVD-RW") );
	  connect( &job, SIGNAL(finished(bool)), this, SLOT(slotErasingFinished(bool)) );
	  connect( &job, SIGNAL(percent(int)), d->erasingInfoDialog, SLOT(setProgress(int)) );
	  connect( d->erasingInfoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
	  job.start(dh);
	  d->erasingInfoDialog->exec(true);
	}
	else {
	  kdDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: no DVD-RW formatting." << endl;
	  connect( K3bDevice::eject( d->device ), 
		   SIGNAL(finished(K3bDevice::DeviceHandler*)),
		   this, 
		   SLOT(startDeviceHandler()) );
	}
      }
      else {
	kdDebug() << "(K3bEmptyDiscWaiter) ------ nothing useful found." << endl;
	
	showDialog();
	QTimer::singleShot( 1000, this, SLOT(startDeviceHandler()) );
      }
    } // --- DVD-RW ------


    // /////////////////////////////////////////////////////////////
    //
    // CD handling (and DVD-R and DVD+R)
    //
    // /////////////////////////////////////////////////////////////

    // we have exactly what was requested
    else if( (d->wantedMediaType & dh->diskInfo().mediaType()) &&
	     (d->wantedMediaState & dh->diskInfo().diskState()) )
      finishWaiting( dh->diskInfo().mediaType() );

    else if( (dh->diskInfo().currentProfile() != K3bDevice::MEDIA_UNKNOWN) &&
	     (d->wantedMediaType & dh->diskInfo().currentProfile()) &&
	     (d->wantedMediaState & dh->diskInfo().diskState()) )
      finishWaiting( dh->diskInfo().mediaType() );

    // this is for CD drives that are not able to determine the state of a disk
    else if( dh->diskInfo().diskState() == K3bDevice::STATE_UNKNOWN && 
	     dh->diskInfo().mediaType() == K3bDevice::MEDIA_CD_ROM &&
	     d->wantedMediaType & K3bDevice::MEDIA_CD_ROM )
      finishWaiting( dh->diskInfo().mediaType() );

    // format CD-RW
    else if( (d->wantedMediaType & dh->diskInfo().mediaType()) &&
	     (d->wantedMediaState & K3bDevice::STATE_EMPTY) &&
	     dh->diskInfo().rewritable() ) {
	
      if( formatWithoutAsking ||
	  KMessageBox::questionYesNo( parentWidgetToUse(),
				      i18n("Found rewritable media in %1 - %2. "
					   "Should it be erased?").arg(d->device->vendor()).arg(d->device->description()),
				      i18n("Found rewritable disk") ) == KMessageBox::Yes ) {
	

	prepareErasingDialog();

	// start a k3bblankingjob
	d->erasingInfoDialog->setText( i18n("Erasing CD-RW") );

	// the user may be using cdrdao for erasing as cdrecord does not work
	int erasingApp = K3b::DEFAULT;
	c->setGroup( "General Options" );
	if( c->readBoolEntry( "Manual writing app selection", false ) ) {
	  c->setGroup( "CDRW Erasing" );
	  erasingApp = K3b::writingAppFromString( c->readEntry( "writing_app" ) );
	}
	  
	K3bBlankingJob job( this );
	job.setDevice( d->device );
	job.setMode( K3bBlankingJob::Fast );
	job.setForce(true);
	job.setForceNoEject(true);
	job.setSpeed( 0 ); // Auto
	job.setWritingApp( erasingApp );
	connect( &job, SIGNAL(finished(bool)), this, SLOT(slotErasingFinished(bool)) );
	connect( d->erasingInfoDialog, SIGNAL(cancelClicked()), &job, SLOT(cancel()) );
	job.start();
	d->erasingInfoDialog->exec(false);
      }
      else {
	kdDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: no CD-RW overwrite." << endl;
	connect( K3bDevice::eject( d->device ), 
		 SIGNAL(finished(K3bDevice::DeviceHandler*)),
		 this, 
		 SLOT(startDeviceHandler()) );
      }
    }
    else {

      kdDebug() << "(K3bEmptyDiscWaiter) ------ nothing useful found." << endl;

      showDialog();
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

    KNotifyClient::event( "WaitingForMedium" );

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


void K3bEmptyDiscWaiter::slotUser2()
{
  K3bDevice::eject( d->device );
}


void K3bEmptyDiscWaiter::slotUser3()
{
  K3bDevice::load( d->device );
}


void K3bEmptyDiscWaiter::finishWaiting( int code )
{
  kdDebug() << "(K3bEmptyDiscWaiter) finishWaiting() " << endl;

  d->result = code;
  if( d->dialogVisible )
    hide();

  if( d->inLoop ) {
    d->inLoop = false;
    kdDebug() << "(K3bEmptyDiscWaiter) exitLoop " << endl;
    QApplication::eventLoop()->exitLoop();
  }
}


void K3bEmptyDiscWaiter::slotErasingFinished( bool success )
{
  if( success ) {
    connect( K3bDevice::reload( d->device ), 
	     SIGNAL(finished(K3bDevice::DeviceHandler*)),
	     this, 
	     SLOT(slotReloadingAfterErasingFinished(K3bDevice::DeviceHandler*)) );
  }
  else {
    K3bDevice::eject( d->device );
    d->erasingInfoDialog->hide();
    KMessageBox::error( parentWidgetToUse(), i18n("Erasing failed.") );
    kdDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: erasing finished." << endl;
    QTimer::singleShot( 0, this, SLOT(startDeviceHandler()) );
  }
}


void K3bEmptyDiscWaiter::slotReloadingAfterErasingFinished( K3bDevice::DeviceHandler* dh )
{
  d->erasingInfoDialog->hide();

  if( !dh->success() ) {
    KMessageBox::error( parentWidgetToUse(), i18n("Unable to reload media. Please reload manually."),
			i18n("Reload failed") );
  }
  
  // now just check the disk for the last time
  kdDebug() << "(K3bEmptyDiscWaiter) starting devicehandler: reloading after erasing finished." << endl;
  QTimer::singleShot( 1000, this, SLOT(startDeviceHandler()) );
}


int K3bEmptyDiscWaiter::wait( K3bDevice::Device* device, bool appendable, int mediaType, QWidget* parent )
{
  K3bEmptyDiscWaiter d( device, parent ? parent : qApp->activeWindow() );
  int mediaState = K3bDevice::STATE_EMPTY;
  if( appendable ) mediaState |= K3bDevice::STATE_INCOMPLETE;
  return d.waitForDisc( mediaState, mediaType );
}


int K3bEmptyDiscWaiter::wait( K3bDevice::Device* device,
			      int mediaState,
			      int mediaType,
			      const QString& message,
			      QWidget* parent )
{
  K3bEmptyDiscWaiter d( device, parent ? parent : qApp->activeWindow() );
  return d.waitForDisc( mediaState, mediaType, message );
}


void K3bEmptyDiscWaiter::prepareErasingDialog()
{
  // we hide the emptydiskwaiter so the info dialog needs to have the same parent
  if( !d->erasingInfoDialog )
    d->erasingInfoDialog = new K3bProgressDialog( QString::null, parentWidget() );

  //
  // hide the dialog 
  //
  if( d->dialogVisible ) {
    hide();
    d->dialogVisible = false;
  }
}


QWidget* K3bEmptyDiscWaiter::parentWidgetToUse()
{
  // we might also show dialogs if the discwaiter widget is not visible yet
  if( d->dialogVisible )
    return this;
  else
    return parentWidget();
}


int K3bEmptyDiscWaiter::waitForMedia( K3bDevice::Device* device,
				      int mediaState,
				      int mediaType,
				      const QString& message )
{
  // this is only needed for the formatting
  return wait( device, mediaState, mediaType, message, d->erasingInfoDialog );
}

  
bool K3bEmptyDiscWaiter::questionYesNo( const QString& text,
					const QString& caption )
{
  return ( KMessageBox::questionYesNo( parentWidgetToUse(), text, caption ) == KMessageBox::Yes );
}


#include "k3bemptydiscwaiter.moc"

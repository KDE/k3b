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

#include "k3bdvdformattingjob.h"

#include <k3bglobals.h>
#include <k3bprocess.h>
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
#include <k3bdevicehandler.h>
#include <k3bdiskinfo.h>
#include <k3bexternalbinmanager.h>
#include <k3bcore.h>
#include <k3bversion.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <qvaluelist.h>
#include <qregexp.h>

#include <errno.h>
#include <string.h>


class K3bDvdFormattingJob::Private
{
public:
  Private()
    : quick(false),
      force(false),
      mode(K3b::WRITING_MODE_AUTO),
      device(0),
      process(0),
      dvdFormatBin(0),
      lastProgressValue(0),
      running(false),
      forceNoEject(false) {
  }

  bool quick;
  bool force;
  int mode;

  K3bDevice::Device* device;
  K3bProcess* process;
  const K3bExternalBin* dvdFormatBin;

  int lastProgressValue;

  bool success;
  bool canceled;
  bool running;

  bool forceNoEject;
};


K3bDvdFormattingJob::K3bDvdFormattingJob( K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bJob( jh, parent, name )
{
  d = new Private;
}


K3bDvdFormattingJob::~K3bDvdFormattingJob()
{
  delete d->process;
  delete d;
}


void K3bDvdFormattingJob::setForceNoEject( bool b )
{
  d->forceNoEject = b;
}


QString K3bDvdFormattingJob::jobDescription() const
{
  return i18n("Formatting DVDÂ±RW");
}


QString K3bDvdFormattingJob::jobDetails() const
{
  if( d->quick )
    return i18n("Quick Format");
  else
    return QString::null;
}


void K3bDvdFormattingJob::start()
{
  d->canceled = false;
  d->running = true;

  emit started();

  if( !d->device ) {
    emit infoMessage( i18n("No device set"), ERROR );
    d->running = false;
    emit finished(false);
    return;
  }

  //
  // first wait for a dvd+rw or dvd-rw
  // Be aware that an empty DVD-RW might be reformatted to another writing mode
  // so we also wait for empty dvds
  //
  if( waitForMedia( d->device,  
		    K3bDevice::STATE_COMPLETE|K3bDevice::STATE_INCOMPLETE|K3bDevice::STATE_EMPTY,
		    K3bDevice::MEDIA_WRITABLE_DVD,
		    i18n("Please insert a rewritable DVD medium into drive<p><b>%1 %2 (%3)</b>.").arg(d->device->vendor()).arg(d->device->description()).arg(d->device->devicename()) ) == -1 ) {
    emit canceled();
    d->running = false;
    emit finished(false);
    return;
  }

  emit infoMessage( i18n("Checking media..."), INFO );
  emit newTask( i18n("Checking media") );

  connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::NG_DISKINFO, d->device ), 
	   SIGNAL(finished(K3bDevice::DeviceHandler*)),
	   this, 
	   SLOT(slotDeviceHandlerFinished(K3bDevice::DeviceHandler*)) );
}


void K3bDvdFormattingJob::start( K3bDevice::DeviceHandler* dh )
{
  d->canceled = false;
  d->running = true;

  emit started();

  slotDeviceHandlerFinished( dh );
}


void K3bDvdFormattingJob::cancel()
{
  if( d->running ) {
    d->canceled = true;
    if( d->process )
      d->process->kill();
  }
  else {
    kdDebug() << "(K3bDvdFormattingJob) not running." << endl;
  }
}


void K3bDvdFormattingJob::setDevice( K3bDevice::Device* dev )
{
  d->device = dev;
}


void K3bDvdFormattingJob::setMode( int m )
{
  d->mode = m;
}


void K3bDvdFormattingJob::setQuickFormat( bool b )
{
  d->quick = b;
}


void K3bDvdFormattingJob::setForce( bool b )
{
  d->force = b;
}


void K3bDvdFormattingJob::slotStderrLine( const QString& line )
{
// * DVD±RW format utility by <appro@fy.chalmers.se>, version 4.4.
// * 4.7GB DVD-RW media in Sequential mode detected.
// * blanking 100.0|

// * formatting 100.0|

  emit debuggingOutput( "dvd+rw-format", line );

  // parsing for the -gui mode (since dvd+rw-format 4.6)
  int pos = line.find( "blanking" );
  if( pos < 0 )
    pos = line.find( "formatting" );
  if( pos >= 0 ) {
    pos = line.find( QRegExp( "\\d" ), pos );
  }
  // parsing for \b\b... stuff 
  else if( !line.startsWith("*") ) {
    pos = line.find( QRegExp( "\\d" ) );
  }

  if( pos >= 0 ) {
    int endPos = line.find( QRegExp("[^\\d\\.]"), pos ) - 1;
    bool ok;
    int progress = (int)(line.mid( pos, endPos - pos ).toDouble(&ok));
    if( ok ) {
      d->lastProgressValue = progress;
      emit percent( progress );
    }
    else {
      kdDebug() << "(K3bDvdFormattingJob) parsing error: '" << line.mid( pos, endPos - pos ) << "'" << endl;
    }
  }
}


void K3bDvdFormattingJob::slotProcessFinished( KProcess* p )
{
  if( d->canceled ) {
    emit canceled();
    d->success = false;
  }
  else if( p->normalExit() ) {
    if( p->exitStatus() == 0 ) {
      emit infoMessage( i18n("Formatting successfully finished"), K3bJob::SUCCESS );

      if( d->lastProgressValue < 100 ) {
	emit infoMessage( i18n("Do not bother with the progress stopping before 100%."), INFO );
	emit infoMessage( i18n("The formatting will continue in the background while writing."), INFO );
      }

      d->success = true;
    }
    else {
      emit infoMessage( i18n("%1 returned an unknown error (code %2).").arg(d->dvdFormatBin->name()).arg(p->exitStatus()), 
			K3bJob::ERROR );
      emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
      emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );
      
      d->success = false;
    }
  }
  else {
    emit infoMessage( i18n("%1 did not exit cleanly.").arg(d->dvdFormatBin->name()), 
		      ERROR );
    d->success = false;
  }

  k3bcore->config()->setGroup("General Options");
  if( d->forceNoEject ||
      k3bcore->config()->readBoolEntry( "No cd eject", false ) ) {
    d->running = false;
    emit finished(d->success);
  }
  else {
    emit infoMessage( i18n("Ejecting DVD..."), INFO );
    connect( K3bDevice::eject( d->device ), 
	     SIGNAL(finished(K3bDevice::DeviceHandler*)),
	     this, 
	     SLOT(slotEjectingFinished(K3bDevice::DeviceHandler*)) );
  }
}


void K3bDvdFormattingJob::slotEjectingFinished( K3bDevice::DeviceHandler* dh )
{
  if( !dh->success() )
    emit infoMessage( i18n("Unable to eject media."), ERROR );

  d->running = false;
  emit finished(d->success);
}


void K3bDvdFormattingJob::slotDeviceHandlerFinished( K3bDevice::DeviceHandler* dh )
{
  if( d->canceled ) {
    emit canceled();
    emit finished(false);
    d->running = false;
  }

  if( dh->success() ) {


    //
    // Now check the media type:
    // if DVD-RW: use d->mode
    //            emit warning if formatting is full and stuff
    //
    // in overwrite mode: emit info that progress might stop before 100% since formatting will continue
    //                    in the background once the media gets rewritten (only DVD+RW?)
    //

    // emit info about what kind of media has been found

    if( !(dh->diskInfo().mediaType() & (K3bDevice::MEDIA_DVD_RW|
					  K3bDevice::MEDIA_DVD_RW_SEQ|
					  K3bDevice::MEDIA_DVD_RW_OVWR|
					  K3bDevice::MEDIA_DVD_PLUS_RW)) ) {
      emit infoMessage( i18n("No rewritable DVD media found. Unable to format."), ERROR );
      d->running = false;
      emit finished(false);
      return;
    }


    bool format = true;  // do we need to format
    bool blank = false;  // blank is for DVD-RW sequential incremental
                         // DVD-RW restricted overwrite and DVD+RW uses the force option (or no option at all)



    //
    // DVD+RW is quite easy to handle. There is only one possible mode and it is always recommended to not
    // format it more than once but to overwrite it once it is formatted
    // Once the initial formatting has been done it's always "appendable" (or "complete"???)
    //


    if( dh->diskInfo().mediaType() == K3bDevice::MEDIA_DVD_PLUS_RW ) {
      emit infoMessage( i18n("Found %1 media.").arg(K3bDevice::mediaTypeString(K3bDevice::MEDIA_DVD_PLUS_RW)),
						    INFO );

      // mode is ignored

      if( dh->diskInfo().empty() ) {
	// 
	// The DVD+RW is blank and needs to be initially formatted
	//
	blank = false;
      }
      else {
	emit infoMessage( i18n("No need to format %1 media more than once."). arg(K3bDevice::mediaTypeString(K3bDevice::MEDIA_DVD_PLUS_RW)), INFO );
	emit infoMessage( i18n("It may simply be overwritten."), INFO );

	if( d->force ) {
	  emit infoMessage( i18n("Forcing formatting anyway."), INFO );
	  emit infoMessage( i18n("It is not recommended to force formatting of DVD+RW media."), INFO );
	  emit infoMessage( i18n("Already after 10-20 reformats the media might be unusable."), INFO );
	  blank = false;
	}
	else {
	  format = false;
	}
      }

      if( format )
	emit newSubTask( i18n("Formatting DVD+RW") );
    }



    //
    // DVD-RW has two modes: incremental sequential (the default which is also needed for DAO writing)
    // and restricted overwrite which compares to the DVD+RW mode.
    //
    
    else {  // MEDIA_DVD_RW
      emit infoMessage( i18n("Found %1 media.").arg(K3bDevice::mediaTypeString(K3bDevice::MEDIA_DVD_RW)),
			INFO );

      if( dh->diskInfo().currentProfile() != K3bDevice::MEDIA_UNKNOWN ) {
	emit infoMessage( i18n("Formatted in %1 mode.").arg(K3bDevice::mediaTypeString(dh->diskInfo().currentProfile())), INFO );	
	

	//
	// Is it possible to have an empty DVD-RW in restricted overwrite mode???? I don't think so.
	//
	
	if( dh->diskInfo().empty() &&
	    (d->mode == K3b::WRITING_MODE_AUTO ||
	     (d->mode == K3b::WRITING_MODE_INCR_SEQ && 
	      dh->diskInfo().currentProfile() == K3bDevice::MEDIA_DVD_RW_SEQ) ||
	     (d->mode == K3b::WRITING_MODE_RES_OVWR && 
	      dh->diskInfo().currentProfile() == K3bDevice::MEDIA_DVD_RW_OVWR) )
	    ) {
	  emit infoMessage( i18n("Media is already empty."), INFO );
	  if( d->force )
	    emit infoMessage( i18n("Forcing formatting anyway."), INFO );
	  else
	    format = false;
	}
	else if( dh->diskInfo().currentProfile() == K3bDevice::MEDIA_DVD_RW_OVWR &&
		 d->mode != K3b::WRITING_MODE_INCR_SEQ ) {
	  emit infoMessage( i18n("No need to format %1 media more than once.")
			    .arg(K3bDevice::mediaTypeString(dh->diskInfo().currentProfile())), INFO );
	  emit infoMessage( i18n("It may simply be overwritten."), INFO );

	  if( d->force )
	    emit infoMessage( i18n("Forcing formatting anyway."), INFO );
	  else
	    format = false;
	}
	

	if( format ) {
	  if( d->mode == K3b::WRITING_MODE_AUTO ) {
	    // just format in the same mode as the media is currently formatted
	    blank = (dh->diskInfo().currentProfile() == K3bDevice::MEDIA_DVD_RW_SEQ);
	  }
	  else {
	    blank = (d->mode == K3b::WRITING_MODE_INCR_SEQ);
	  }
	  
	  emit newSubTask( i18n("Formatting"
				" DVD-RW in %1 mode.").arg(K3bDevice::mediaTypeString( blank ? 
											 K3bDevice::MEDIA_DVD_RW_SEQ :
											 K3bDevice::MEDIA_DVD_RW_OVWR )) );
	}
      }
      else {
	emit infoMessage( i18n("Unable to determine the current formatting state of the DVD-RW media."), ERROR );
	d->running = false;
	emit finished(false);
	return;
      }
    }

    
    if( format ) {
      delete d->process;
      d->process = new K3bProcess();
      d->process->setRunPrivileged(true);
      //      d->process->setSuppressEmptyLines(false);
      connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStderrLine(const QString&)) );
      connect( d->process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessFinished(KProcess*)) );
      
      d->dvdFormatBin = k3bcore->externalBinManager()->binObject( "dvd+rw-format" );
      if( !d->dvdFormatBin ) {
	emit infoMessage( i18n("Could not find %1 executable.").arg("dvd+rw-format"), ERROR );
	d->running = false;
	emit finished(false);
	return;
      }
      
      if( !d->dvdFormatBin->copyright.isEmpty() )
	emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg(d->dvdFormatBin->name()).arg(d->dvdFormatBin->version).arg(d->dvdFormatBin->copyright), INFO );
      
      
      *d->process << d->dvdFormatBin;

      if( d->dvdFormatBin->version >= K3bVersion( 4, 6 ) )
	*d->process << "-gui";

      QString p;
      if( blank )
	p = "-blank";
      else
	p = "-force";
      if( !d->quick )
	p += "=full";

      *d->process << p;

      *d->process << d->device->blockDeviceName();

      // additional user parameters from config
      const QStringList& params = d->dvdFormatBin->userParameters();
      for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
	*d->process << *it;

      kdDebug() << "***** dvd+rw-format parameters:\n";
      const QValueList<QCString>& args = d->process->args();
      QString s;
      for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
	s += *it + " ";
      }
      kdDebug() << s << endl << flush;
      emit debuggingOutput( "dvd+rw-format comand:", s );


      if( !d->process->start( KProcess::NotifyOnExit, KProcess::All ) ) {
	// something went wrong when starting the program
	// it "should" be the executable
	kdDebug() << "(K3bDvdFormattingJob) could not start " << d->dvdFormatBin->path << endl;
	emit infoMessage( i18n("Could not start %1.").arg(d->dvdFormatBin->name()), K3bJob::ERROR );
	d->running = false;
	emit finished(false);
      }
      else {
	emit newTask( i18n("Formatting") );
      }
    }
    else {
      // already formatted :)
      d->running = false;
      emit finished(true);
    }
  }
  else {
    emit infoMessage( i18n("Unable to determine media state."), ERROR );
    d->running = false;
    emit finished(false);
  }
}


#include "k3bdvdformattingjob.moc"

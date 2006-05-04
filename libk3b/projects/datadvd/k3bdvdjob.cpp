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


#include "k3bdvdjob.h"
#include "k3bdvddoc.h"
#include "k3bgrowisofsimager.h"

#include <k3bcore.h>
#include <k3bisoimager.h>
#include <k3bdataverifyingjob.h>
#include <k3bgrowisofswriter.h>
#include <k3bglobals.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bdiskinfo.h>
#include <k3bdeviceglobals.h>
#include <k3bglobalsettings.h>
#include <k3biso9660.h>

#include <klocale.h>
#include <kapplication.h>


class K3bDvdJob::Private
{
public:
};


K3bDvdJob::K3bDvdJob( K3bDataDoc* doc, K3bJobHandler* hdl, QObject* parent )
  : K3bDataJob( doc, hdl, parent ),
    m_doc( doc )
{
  d = new Private();
}


K3bDvdJob::~K3bDvdJob()
{
  delete d;
}


void K3bDvdJob::prepareData()
{
  // nothing to prepare here I think
}


bool K3bDvdJob::prepareWriterJob()
{
  K3bGrowisofsWriter* writer = new K3bGrowisofsWriter( m_doc->burner(), this, this );
  
  // these do only make sense with DVD-R(W)
  writer->setSimulate( m_doc->dummy() );
  writer->setBurnSpeed( m_doc->speed() );

  // Andy said incremental sequential is the default mode and it seems uses have more problems with DAO anyway
  // BUT: I also had a report that incremental sequential produced unreadable media!
  if( m_doc->writingMode() == K3b::DAO )
//     || ( m_doc->writingMode() == K3b::WRITING_MODE_AUTO &&
// 	 usedMultiSessionMode() == K3bDataDoc::NONE ) )
    writer->setWritingMode( K3b::DAO );

  writer->setMultiSession( usedMultiSessionMode() == K3bDataDoc::CONTINUE ||
  		           usedMultiSessionMode() == K3bDataDoc::FINISH );

  writer->setCloseDvd( usedMultiSessionMode() == K3bDataDoc::NONE ||
		       usedMultiSessionMode() == K3bDataDoc::FINISH );

  if( m_doc->onTheFly() ) {
    writer->setImageToWrite( QString::null );  // read from stdin
    writer->setTrackSize( m_isoImager->size() );
  }
  else
    writer->setImageToWrite( m_doc->tempDir() );

  setWriterJob( writer );

  return true;
}


void K3bDvdJob::determineMultiSessionMode()
{
  int m = requestMedia( K3bDevice::STATE_INCOMPLETE|K3bDevice::STATE_EMPTY );

  if( m < 0 ) {
    cancel();
  }
  else {
     connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::NG_DISKINFO, m_doc->burner() ), 
	     SIGNAL(finished(K3bDevice::DeviceHandler*)),
	     this, 
	     SLOT(slotDetermineMultiSessionMode(K3bDevice::DeviceHandler*)) );
  }
}


K3bDataDoc::MultiSessionMode K3bDvdJob::getMultiSessionMode( const K3bDevice::DiskInfo& info )
{
  K3bDataDoc::MultiSessionMode mode = K3bDataDoc::NONE;

  if( info.mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) ) {
    //
    // we need to handle DVD+RW and DVD-RW overwrite media differently since remainingSize() is not valid
    // in both cases
    // Since one never closes a DVD+RW we only differ between CONTINUE and NONE
    //

    // try to check the filesystem size
    K3bIso9660 iso( m_doc->burner() );
    if( iso.open() && info.capacity() - iso.primaryDescriptor().volumeSpaceSize >= m_doc->burningLength() ) {
      mode = K3bDataDoc::CONTINUE;
    }
    else {
      mode = K3bDataDoc::NONE;
    }
  }
  else if( info.appendable() ) {
    //
    // 3 cases:
    //  1. the project does not fit -> no multisession (resulting in asking for another media above)
    //  2. the project does fit and fills up the CD -> finish multisession
    //  3. the project does fit and does not fill up the CD -> continue multisession
    //
    if( m_doc->size() > info.remainingSize().mode1Bytes() && !m_doc->sessionImported() )
      mode = K3bDataDoc::NONE;
    else if( m_doc->size() >= info.remainingSize().mode1Bytes()*9/10 )
      mode = K3bDataDoc::FINISH;
    else
      mode = K3bDataDoc::CONTINUE;
  }
  else {
    //
    // We only close the DVD if the project fills it up almost completely (90%)
    //
    if( m_doc->size() >= info.capacity().mode1Bytes()*9/10 ||
	m_doc->writingMode() == K3b::DAO )
      mode = K3bDataDoc::NONE;
    else
      mode = K3bDataDoc::START;
  }

  return mode;
}


int K3bDvdJob::requestMedia( int state )
{
  int mt = 0;
//   if( m_doc->writingMode() == K3b::WRITING_MODE_INCR_SEQ || m_doc->writingMode() == K3b::DAO )
//     mt = K3bDevice::MEDIA_DVD_RW_SEQ|K3bDevice::MEDIA_DVD_R_SEQ;
//  else
  if( m_doc->writingMode() == K3b::WRITING_MODE_RES_OVWR ) // we treat DVD+R(W) as restricted overwrite media
    mt = K3bDevice::MEDIA_DVD_RW_OVWR|K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_PLUS_R;
  else
    mt = K3bDevice::MEDIA_WRITABLE_DVD_SL;

  // double layer media
  if( m_doc->size() > 4700372992LL )
    mt = K3bDevice::MEDIA_WRITABLE_DVD_DL;

  return waitForMedia( m_doc->burner(),
		       state, 
		       mt );
}


bool K3bDvdJob::waitForMedium()
{
  emit infoMessage( i18n("Waiting for media") + "...", INFO );

  int foundMedium = requestMedia( usedMultiSessionMode() == K3bDataDoc::CONTINUE ||
				  usedMultiSessionMode() == K3bDataDoc::FINISH ?
				  K3bDevice::STATE_INCOMPLETE :
				  K3bDevice::STATE_EMPTY );

  if( foundMedium < 0 ) {
    return false;
  }
  
  if( foundMedium == 0 ) {
    emit infoMessage( i18n("Forced by user. Growisofs will be called without further tests."), INFO );
  }

  else {
    // -------------------------------
    // DVD Plus
    // -------------------------------
    if( foundMedium & K3bDevice::MEDIA_DVD_PLUS_ALL ) {
      if( m_doc->dummy() ) {
	if( !questionYesNo( i18n("K3b does not support simulation with DVD+R(W) media. "
				 "Do you really want to continue? The media will be written "
				 "for real."),
			    i18n("No Simulation with DVD+R(W)") ) ) {
	  return false;
	}

	m_doc->setDummy( false );
	emit newTask( i18n("Writing") );
      }
      
      if( m_doc->writingMode() != K3b::WRITING_MODE_AUTO && m_doc->writingMode() != K3b::WRITING_MODE_RES_OVWR )
	emit infoMessage( i18n("Writing mode ignored when writing DVD+R(W) media."), INFO );

      if( foundMedium & K3bDevice::MEDIA_DVD_PLUS_RW ) {
	  if( usedMultiSessionMode() == K3bDataDoc::NONE ||
	      usedMultiSessionMode() == K3bDataDoc::START )
	    emit infoMessage( i18n("Writing DVD+RW."), INFO );
	  else
	    emit infoMessage( i18n("Growing ISO9660 filesystem on DVD+RW."), INFO );
      }
      else if( foundMedium & K3bDevice::MEDIA_DVD_PLUS_R_DL )
	emit infoMessage( i18n("Writing Double Layer DVD+R."), INFO );
      else
	emit infoMessage( i18n("Writing DVD+R."), INFO );
    }

    // -------------------------------
    // DVD Minus
    // -------------------------------
    else {
      if( m_doc->dummy() && !m_doc->burner()->dvdMinusTestwrite() ) {
	if( !questionYesNo( i18n("Your writer (%1 %2) does not support simulation with DVD-R(W) media. "
				 "Do you really want to continue? The media will be written "
				 "for real.")
			    .arg(m_doc->burner()->vendor())
			    .arg(m_doc->burner()->description()),
			    i18n("No Simulation with DVD-R(W)") ) ) {
	  return false;
	}

	m_doc->setDummy( false );
      }

      // RESTRICTED OVERWRITE
      // --------------------
      if( foundMedium & K3bDevice::MEDIA_DVD_RW_OVWR ) {
	if( usedMultiSessionMode() == K3bDataDoc::NONE ||
	    usedMultiSessionMode() == K3bDataDoc::START )
	  emit infoMessage( i18n("Writing DVD-RW in restricted overwrite mode."), INFO );
	else
	  emit infoMessage( i18n("Growing ISO9660 filesystem on DVD-RW in restricted overwrite mode."), INFO );
      }

      // NORMAL
      // ------
      else {

	// FIXME: DVD-R DL jump and stuff

	if( m_doc->writingMode() == K3b::DAO )
	    // || ( m_doc->writingMode() == K3b::WRITING_MODE_AUTO &&
// 	      usedMultiSessionMode() == K3bDataDoc::NONE ) )
	  emit infoMessage( i18n("Writing %1 in DAO mode.").arg( K3bDevice::mediaTypeString(foundMedium, true) ), INFO );

	else {
	  // check if the writer supports writing sequential and thus multisession (on -1 the burner cannot handle 
	  // features and we simply ignore it and hope for the best)
	  if( m_doc->burner()->featureCurrent( K3bDevice::FEATURE_INCREMENTAL_STREAMING_WRITABLE ) == 0 ) {
	    if( !questionYesNo( i18n("Your writer (%1 %2) does not support Incremental Streaming with %3 "
				     "media. Multisession will not be possible. Continue anyway?")
				.arg(m_doc->burner()->vendor())
				.arg(m_doc->burner()->description())
				.arg( K3bDevice::mediaTypeString(foundMedium, true) ),
				i18n("No Incremental Streaming") ) ) {
	      return false;
	    }
	    else {
	      emit infoMessage( i18n("Writing %1 in DAO mode.").arg( K3bDevice::mediaTypeString(foundMedium, true) ), INFO );
	    }
	  }
	  else {
	    if( !(foundMedium & (K3bDevice::MEDIA_DVD_RW|K3bDevice::MEDIA_DVD_RW_OVWR|K3bDevice::MEDIA_DVD_RW_SEQ)) &&
		m_doc->writingMode() == K3b::WRITING_MODE_RES_OVWR )
	      emit infoMessage( i18n("Restricted Overwrite is not possible with DVD-R media."), INFO );
	    
	    emit infoMessage( i18n("Writing %1 in incremental mode.").arg( K3bDevice::mediaTypeString(foundMedium, true) ), INFO );
	  }
	}
      }
    }
  }

  return true;
}


QString K3bDvdJob::jobDescription() const
{
  if( m_doc->onlyCreateImages() ) {
    return i18n("Creating Data Image File");
  }
  else if( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
	   m_doc->multiSessionMode() == K3bDataDoc::AUTO ) {
    return i18n("Writing Data DVD")
      + ( m_doc->isoOptions().volumeID().isEmpty()
	  ? QString::null
	  : QString( " (%1)" ).arg(m_doc->isoOptions().volumeID()) );
  }
  else {
    return i18n("Writing Multisession DVD")
      + ( m_doc->isoOptions().volumeID().isEmpty()
	  ? QString::null
	  : QString( " (%1)" ).arg(m_doc->isoOptions().volumeID()) );
  }
}


QString K3bDvdJob::jobDetails() const
{
  if( m_doc->copies() > 1 && 
      !m_doc->dummy() &&
      !(m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
	m_doc->multiSessionMode() == K3bDataDoc::FINISH) )
    return i18n("ISO9660 Filesystem (Size: %1) - %n copy",
		"ISO9660 Filesystem (Size: %1) - %n copies",
		m_doc->copies())
      .arg(KIO::convertSize( m_doc->size() ));
  else
    return i18n("ISO9660 Filesystem (Size: %1)")
      .arg(KIO::convertSize( m_doc->size() ));
}

#include "k3bdvdjob.moc"

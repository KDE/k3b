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

#include "k3bgrowisofshandler.h"

#include <k3bjob.h>
#include <k3bcore.h>

#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kconfig.h>

#include <errno.h>
#include <string.h>


K3bGrowisofsHandler::K3bGrowisofsHandler( QObject* parent, const char* name )
  : QObject( parent, name )
{
  reset();
}


K3bGrowisofsHandler::~K3bGrowisofsHandler()
{
}


void K3bGrowisofsHandler::reset( bool dao )
{
  m_error = ERROR_UNKNOWN;
  m_dao = dao;
}

void K3bGrowisofsHandler::handleLine( const QString& line )
{
  int pos = 0;

  if( line.contains( "flushing cache" ) ) {
    emit newSubTask( i18n("Flushing Cache")  );
    emit infoMessage( i18n("Flushing the cache may take some time."), K3bJob::INFO );
  }
  else if( line.contains( "updating RMA" ) ) {
    emit newSubTask( i18n("Updating RMA") );
    emit infoMessage( i18n("Updating RMA") + "...", K3bJob::INFO );
  }
  else if( line.contains( "closing session" ) ) {
    emit newSubTask( i18n("Closing Session") );
    emit infoMessage( i18n("Closing Session") + "...", K3bJob::INFO );
  }
  else if( line.contains( "writing lead-out" ) ) {
    emit newSubTask( i18n("Writing Lead-out") );
    emit infoMessage( i18n("Writing the lead-out may take some time."), K3bJob::INFO );
  }
  else if( line.contains( "Quick Grow" ) ) {
    emit infoMessage( i18n("Removing reference to lead-out."), K3bJob::INFO );
  }
  else if( line.contains( "copying volume descriptor" ) ) {
    emit infoMessage( i18n("Modifying Iso9660 volume descriptor"), K3bJob::INFO );
  }
  else if( line.contains( "FEATURE 21h is not on" ) ) {
    if( !m_dao ) {
      emit infoMessage( i18n("Writer does not support Incremental Streaming"), K3bJob::WARNING );
      emit infoMessage( i18n("Engaging DAO"), K3bJob::WARNING );
    }
  }
  else if( ( pos = line.find( "Current Write Speed" ) ) > 0 ) {
    // parse write speed
    // /dev/sr0: "Current Write Speed" is 2.4x1385KBps

    pos += 24;
    int endPos = line.find( "x", pos );
    bool ok = true;
    double speed = line.mid( pos, endPos-pos ).toDouble(&ok);
    if( ok )
      emit infoMessage( i18n("Writing speed: %1 kb/s (%2x)")
			.arg((int)(speed*1385.0))
			.arg(KGlobal::locale()->formatNumber(speed)), K3bJob::INFO );
    else
      kdDebug() << "(K3bGrowisofsHandler) parsing error: '" << line.mid( pos, endPos-pos ) << "'" << endl;
  }
  else if( line.startsWith( ":-[" ) ) {
    // Error

    if( line.contains( "ASC=30h" ) )
      m_error = ERROR_MEDIA;

    // :-[ PERFORM OPC failed with SK=3h/ASC=73h/ASCQ=03h
    else if( line.startsWith( ":-[ PERFORM OPC failed" ) )
      emit infoMessage( i18n("OPC failed. Please try writing speed 1x."), K3bJob::ERROR );

    // :-[ attempt -blank=full or re-run with -dvd-compat -dvd-compat to engage DAO ]
    else if( !m_dao && 
	     ( line.contains( "engage DAO" ) || line.contains( "media is not formatted or unsupported" ) ) )
      emit infoMessage( i18n("Please try again with writing mode DAO."), K3bJob::ERROR );

    else if( line.startsWith( ":-[ Failed to change write speed" ) ) {
      m_error = ERROR_SPEED_SET_FAILED;
    }
  }
  else if( line.startsWith( ":-(" ) ) {
    if( line.contains( "No space left on device" ) )
      m_error = ERROR_OVERSIZE;

    else if( line.contains( "blocks are free" ) && line.contains( "to be written" ) ) {
      m_error = ERROR_OVERSIZE;
      k3bcore->config()->setGroup( "General Options" );
      if( k3bcore->config()->readBoolEntry( "Allow overburning", false ) )
	emit infoMessage( i18n("Trying to write more than the official disk capacity"), K3bJob::WARNING );
    }

    else  
      emit infoMessage( line, K3bJob::ERROR );
  }
  else {
    kdDebug() << "(growisofs) " << line << endl;
  }
}


void K3bGrowisofsHandler::handleExit( int exitCode )
{
  switch( m_error ) {
  case ERROR_MEDIA:
    emit infoMessage( i18n("K3b detected a problem with the media."), K3bJob::ERROR );
    emit infoMessage( i18n("Please try another media brand, preferably one explicitly recommended by your writer's vendor."), K3bJob::ERROR );
    emit infoMessage( i18n("Report the problem if it persists anyway."), K3bJob::ERROR );
    break;

  case ERROR_OVERSIZE:
    k3bcore->config()->setGroup( "General Options" );
    if( k3bcore->config()->readBoolEntry( "Allow overburning", false ) )
      emit infoMessage( i18n("Data did not fit on disk."), K3bJob::ERROR );
    else
      emit infoMessage( i18n("Data does not fit on disk."), K3bJob::ERROR );
    break;

  case ERROR_SPEED_SET_FAILED:
    emit infoMessage( i18n("Unable to set writing speed."), K3bJob::ERROR );
    break;

  default:

    //
    // The growisofs error codes:
    //
    // 128 + errno: fatal error upon program startup
    // errno      : fatal error during recording
    //

    if( exitCode > 128 ) {
      // for now we just emit a message with the error
      // in the future when I know more about what kinds of errors may occure
      // we will enhance this
      emit infoMessage( i18n("Fatal error at startup: %1").arg(strerror(exitCode-128)), 
			K3bJob::ERROR );
    }
    else if( exitCode == 1 ) {
      // Doku says: warning at exit
      // Example: mkisofs error
      //          unable to reload
      // So basically this is just for mkisofs failure since we do not let growisofs reload the media
      emit infoMessage( i18n("Warning at exit: (1)"), K3bJob::ERROR );
      emit infoMessage( i18n("Most likely mkisofs failed in some way."), K3bJob::ERROR );
    }
    else {
      emit infoMessage( i18n("Fatal error during recording: %1").arg(strerror(exitCode)), 
			K3bJob::ERROR );
    }
  }
}

#include "k3bgrowisofshandler.moc"

/***************************************************************************
                          k3bcdrdaoparser.cpp  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg and
                                       Klaus-Dieter Krannich
    email                : trueg@informatik.uni-freiburg.de
                           kd@math.tu-cottbus.de
***************************************************************************/

#include <k3bcdrdaoparser.h>
#include "remote.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>



inline bool operator<( const ProgressMsg& m1, const ProgressMsg& m2 )
{
  return m1.track < m2.track 
    || ( m1.track == m2.track 
	 && m1.trackProgress/10 < m2.trackProgress/10 )
    || m1.totalProgress/10 < m2.totalProgress/10;
}


inline bool operator==( const ProgressMsg& m1, const ProgressMsg& m2 )
{
  return m1.status == m2.status 
    && m1.track == m2.track 
    && m1.totalTracks == m2.totalTracks
    && m1.trackProgress/10 == m2.trackProgress/10
    && m1.totalProgress/10 == m2.totalProgress/10
    && m1.bufferFillRate == m2.bufferFillRate;
}

inline bool operator!=( const ProgressMsg& m1, const ProgressMsg& m2 )
{
  return !( m1 == m2 );
}


K3bCdrdaoParser::K3bCdrdaoParser( QObject* parent, const char* name )
  :QObject( parent, name ),
   m_currentTrack(0)
{
  m_oldMsg = new ProgressMsg;
  m_newMsg = new ProgressMsg;

  m_oldMsg->track = 0;
  m_oldMsg->trackProgress = 0;
  m_oldMsg->totalProgress = 0;
}


K3bCdrdaoParser::~K3bCdrdaoParser()
{
  delete m_oldMsg;
  delete m_newMsg;
}


void K3bCdrdaoParser::parseCdrdaoLine( const QString& str )
{
  emit debuggingOutput( "cdrdao", str );
  //  kdDebug() << "(cdrdaoparse)" << str << endl;
  // find some messages from cdrdao
  // -----------------------------------------------------------------------------------------
  if( (str).startsWith( "Warning" ) || (str).startsWith( "WARNING" ) || (str).startsWith( "ERROR" ) ) {
    parseCdrdaoError( str );
  }
  else if( (str).startsWith( "Executing power" ) ) {
    emit newSubTask( i18n("Executing Power calibration") );
  }
  else if( (str).startsWith( "Power calibration successful" ) ) {
    emit infoMessage( i18n("Power calibration successful"), K3bJob::PROCESS );
    emit newSubTask( i18n("Preparing burn process...") );
  }
  else if( (str).startsWith( "Flushing cache" ) ) {
    emit newSubTask( i18n("Flushing cache") );
  }
  else if( (str).startsWith( "Writing CD-TEXT lead" ) ) {
    emit newSubTask( i18n("Writing CD-Text lead-in...") );
  }
  else if( (str).startsWith( "Turning BURN-Proof on" ) ) {
    emit infoMessage( i18n("Turning BURN-Proof on"), K3bJob::PROCESS );
  }
  else if( str.startsWith( "Copying" ) ) {
    emit infoMessage( str, K3bJob::PROCESS );
  }
  else if( str.startsWith( "Found ISRC" ) ) {
    emit infoMessage( i18n("Found ISRC code"), K3bJob::PROCESS );
  }
  else if( str.startsWith( "Found pre-gap" ) ) {
    emit infoMessage( i18n("Found pregap: %1").arg( str.mid(str.find(":")+1) ), K3bJob::PROCESS );
  }
  else
    emit unknownCdrdaoLine(str);
}

void K3bCdrdaoParser::parseCdrdaoError( const QString& line )
{
  if( line.contains( "No driver found" ) ) {
    emit infoMessage( i18n("No cdrdao driver found."), K3bJob::ERROR );
    emit infoMessage( i18n("Please select one manually in the device settings."), K3bJob::ERROR );
    emit infoMessage( i18n("For most current drives this would be 'generic-mmc'."), K3bJob::ERROR );
  }
  else if( line.contains( "Cannot setup device" ) ) {
    // no nothing...
  }
  else if( line.contains( "not ready") ) {
    emit infoMessage( i18n("Device not ready, waiting."),K3bJob::PROCESS );
  }
  else if( !line.contains( "remote progress message" ) )
    emit infoMessage( line, K3bJob::ERROR );
}

void K3bCdrdaoParser::parseCdrdaoMessage(int fd)
{
  char msgSync[] = { 0xff, 0x00, 0xff, 0x00 };
  char buf[1024*8];
  int  state;
  QString task;

  state = 0;
  while (state < 4) {
    if( ::read(fd, buf, 1) != 1 ) {
      kdDebug() << "(K3bCdrdaoParser) remote message sync not found" << endl;
      return;
    }

    if (buf[0] == msgSync[state])
      state++;
    else
      state = 0;
  }

  if( ::read( fd, (void *)m_newMsg, sizeof(struct ProgressMsg) ) < (int)sizeof(struct ProgressMsg) ) {
    kdDebug() << "(K3bCdrdaoParser) could not read complete remote message." << endl;
    return;
  }

  // read a lot of data just to clear the buffer
  int size = ::read( fd, buf, sizeof(buf) );
  if( size > 0 )
    kdDebug() << "(K3bCdrdaoParser) dropped " 
	      << size << " bytes of message data" << endl;

//   kdDebug() << "Status: " << msg.status
// 	    << " Total:  " << msg.totalTracks
// 	    << " Track:  " << msg.track
// 	    << " TrackProgress: " << msg.trackProgress/10
// 	    << " TotalProgress: " << msg.totalProgress/10
// 	    << endl;

// sometimes the progress takes one step back (on my system when using paranoia-level 3)
// so we just use messages that are greater than the previous or first messages
  if( *m_oldMsg < *m_newMsg
      || ( m_newMsg->track == 1 &&
	   m_newMsg->trackProgress <= 10 )) {
    emit subPercent( m_newMsg->trackProgress/10 );
    emit percent( m_newMsg->totalProgress/10 );
    emit buffer(m_newMsg->bufferFillRate);
    
    if ( m_newMsg->track != m_currentTrack) {
      switch (m_newMsg->status) {
      case PGSMSG_RCD_EXTRACTING: 
	//	task = i18n("Reading (Track %1 of %2)").arg(m_newMsg->track).arg(m_newMsg->totalTracks);
	emit nextTrack( m_newMsg->track, m_newMsg->totalTracks );
	break;
      case PGSMSG_WCD_LEADIN:
	task = i18n("Writing leadin ");
	emit newSubTask( task );
	break;
      case PGSMSG_WCD_DATA:
	//	task = i18n("Writing (Track %1 of %2)").arg(m_newMsg->track).arg(m_newMsg->totalTracks);
	emit nextTrack( m_newMsg->track, m_newMsg->totalTracks );
	break;
      case PGSMSG_WCD_LEADOUT:
	task = i18n("Writing leadout ");
	emit newSubTask( task );
	break;
      }

      m_currentTrack = m_newMsg->track;
    }

    struct ProgressMsg* m = m_newMsg;
    m_newMsg = m_oldMsg;
    m_oldMsg = m;
  }
}


#include "k3bcdrdaoparser.moc"

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


inline bool operator<( const ProgressMsg& m1, const ProgressMsg& m2 ) {
    return m1.track < m2.track
           || ( m1.track == m2.track
                && m1.trackProgress < m2.trackProgress )
           || m1.totalProgress < m2.totalProgress;
}


inline bool operator==( const ProgressMsg& m1, const ProgressMsg& m2 ) {
    return m1.status == m2.status
           && m1.track == m2.track
           && m1.totalTracks == m2.totalTracks
           && m1.trackProgress == m2.trackProgress
           && m1.totalProgress == m2.totalProgress
           && m1.bufferFillRate == m2.bufferFillRate;
}

inline bool operator!=( const ProgressMsg& m1, const ProgressMsg& m2 ) {
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
    m_isStarted = false;
}


K3bCdrdaoParser::~K3bCdrdaoParser() {
    delete m_oldMsg;
    delete m_newMsg;
}

void K3bCdrdaoParser::reinit() {
    delete m_oldMsg;
    delete m_newMsg;
    m_oldMsg = new ProgressMsg;
    m_newMsg = new ProgressMsg;

    m_oldMsg->track = 0;
    m_oldMsg->trackProgress = 0;
    m_oldMsg->totalProgress = 0;
    m_currentTrack=0;
}

void K3bCdrdaoParser::parseCdrdaoLine( const QString& str ) {
    emit debuggingOutput( "cdrdao", str );
    //  kdDebug() << "(cdrdaoparse)" << str << endl;
    // find some messages from cdrdao
    // -----------------------------------------------------------------------------------------
    if( (str).startsWith( "Warning" ) || (str).startsWith( "WARNING" ) || (str).startsWith( "ERROR" ) ) {
        parseCdrdaoError( str );
    } else if( (str).startsWith( "Wrote" ) ) {
        parseCdrdaoWrote( str );
    } else if( (str).startsWith( "Executing power" ) ) {
        emit newSubTask( i18n("Executing Power calibration") );
    } else if( (str).startsWith( "Power calibration successful" ) ) {
        emit infoMessage( i18n("Power calibration successful"), K3bJob::PROCESS );
        emit newSubTask( i18n("Preparing burn process...") );
    } else if( (str).startsWith( "Flushing cache" ) ) {
        emit newSubTask( i18n("Flushing cache") );
    } else if( (str).startsWith( "Writing CD-TEXT lead" ) ) {
        emit newSubTask( i18n("Writing CD-Text lead-in...") );
    } else if( (str).startsWith( "Turning BURN-Proof on" ) ) {
        emit infoMessage( i18n("Turning BURN-Proof on"), K3bJob::PROCESS );
    } else if( str.startsWith( "Copying" ) ) {
        emit infoMessage( str, K3bJob::PROCESS );
    } else if( str.startsWith( "Found ISRC" ) ) {
        emit infoMessage( i18n("Found ISRC code"), K3bJob::PROCESS );
    } else if( str.startsWith( "Found pre-gap" ) ) {
        emit infoMessage( i18n("Found pregap: %1").arg( str.mid(str.find(":")+1) ), K3bJob::PROCESS );
    } else
        emit unknownCdrdaoLine(str);
}

void K3bCdrdaoParser::parseCdrdaoError( const QString& line ) {
    if( line.contains( "No driver found" ) ) {
        emit infoMessage( i18n("No cdrdao driver found."), K3bJob::ERROR );
        emit infoMessage( i18n("Please select one manually in the device settings."), K3bJob::ERROR );
        emit infoMessage( i18n("For most current drives this would be 'generic-mmc'."), K3bJob::ERROR );
    } else if( line.contains( "Cannot setup device" ) ) {
        // no nothing...
    } else if( line.contains( "not ready") ) {
        emit infoMessage( i18n("Device not ready, waiting."),K3bJob::PROCESS );
    } else if( line.contains("Drive does not accept any cue sheet") ) {
        emit infoMessage( i18n("Cue sheet not accepted."), K3bJob::ERROR );
        emit infoMessage( i18n("Try setting the first pregap to 0."), K3bJob::ERROR );
    } else if( !line.contains( "remote progress message" ) )
        emit infoMessage( line, K3bJob::ERROR );
}

void K3bCdrdaoParser::parseCdrdaoWrote( const QString& line ) {

    double speed;
    int pos, po2, elapsed;

    if( line.contains( "blocks" ) ) {
      if (m_isStarted) {
        elapsed = m_startWriteTime.secsTo( QTime::currentTime() );
        if (elapsed <= 0.0) return;
        
        speed = (m_size  * 1024) / elapsed;
        emit infoMessage( i18n("Estimated speed %1 Kb/s (%2x)").arg((int)speed).arg(speed/150.0,0,'g',2), K3bJob::INFO );
        m_isStarted = false;
      }
      return;
    }

    if (!m_isStarted) {
      m_startWriteTime = QTime::currentTime();
      m_isStarted = true;
    }
    
    elapsed = m_startWriteTime.secsTo( QTime::currentTime() );
    if (elapsed <= 0.0) return;

    pos = line.find( "Wrote" );
    po2 = line.find( " ", pos + 6 );
    int processed = line.mid( pos+6, po2-pos-6 ).toInt();

    pos = line.find( "of" );
    po2 = line.find( " ", pos + 3 );
    m_size = line.mid( pos+3, po2-pos-3 ).toInt();

    speed = (processed  * 1024) / elapsed;

    // kdDebug() << QString("Speed: %1 Kb/s (%2x) elapsed: %3s").arg((int)speed).arg(speed / 150.0,0,'g',2).arg(elapsed) << endl;
    emit processedSize( processed, m_size );
      
}

void K3bCdrdaoParser::parseCdrdaoMessage(QSocket *comSock) {
    char msgSync[] = { 0xff, 0x00, 0xff, 0x00 };
    char buf;
    int  state;
    unsigned int  avail,count;
    QString task;
    int msgs;

    avail = comSock->bytesAvailable();
    count = 0;

    msgs = avail / ( sizeof(msgSync)+sizeof(struct ProgressMsg) );
    if ( msgs < 1 )
        return;
    else if ( msgs > 1) {
        // move the read-index forward to the beginnig of the most recent message
        count = ( msgs-1 ) * ( sizeof(msgSync)+sizeof(struct ProgressMsg) );
        comSock->at(count);
        kdDebug() << "(K3bCdrdaoParser) " << msgs-1 << " message(s) skipped" << endl;
    }
    while (count < avail) {
        state = 0;
        while (state < 4) {
            buf=comSock->getch();
            count++;
            if (count == avail) {
                kdDebug() << "(K3bCdrdaoParser) remote message sync not found (" << count << ")" << endl;
                return;
            }

            if (buf == msgSync[state])
                state++;
            else
                state = 0;
        }

        if( (avail - count) < (int)sizeof(struct ProgressMsg) ) {
            kdDebug() << "(K3bCdrdaoParser) could not read complete remote message." << endl;
            return;
        }

        // read one message
        int size = comSock->readBlock((char *)m_newMsg,sizeof(struct ProgressMsg));
        if( size == -1 )
            kdDebug() << "(K3bCdrdaoParser) read error" << endl;
        count += size;

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
}


#include "k3bcdrdaoparser.moc"

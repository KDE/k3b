#include "k3bdiskinfodetector.h"

#include "../device/k3bdevice.h"
#include "../device/k3btoc.h"
#include "../rip/k3btcwrapper.h"
#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"

#include <kprocess.h>

#include <qtimer.h>

extern "C" {
#include <cdda_interface.h>
}



K3bDiskInfoDetector::K3bDiskInfoDetector( QObject* parent )
  : QObject( parent )
{
  m_tcWrapper = 0;
  m_process = new KProcess();
}


K3bDiskInfoDetector::~K3bDiskInfoDetector()
{
  delete m_process;
}


void K3bDiskInfoDetector::detect( K3bDevice* device )
{
  if( !m_device ) {
    qDebug("(K3bDiskInfoDetector) detect should really not be called with NULL!");
    return;
  }


  m_device = device;

  // reset
  m_info = K3bDiskInfo();
  m_info.device = m_device;

  QTimer::singleShot( 0, this, SLOT(fetchTocInfo()) );
}


void K3bDiskInfoDetector::fetchDiskInfo()
{
  m_process->clearArguments();
  m_process->disconnect();

  if( !k3bMain()->externalBinManager()->foundBin( "cdrdao" ) ) {
    qDebug("(K3bAudioJob) could not find cdrdao executable. no disk-info..." );
    testForDvd();
  }
  else {
    *m_process << k3bMain()->externalBinManager()->binPath( "cdrdao" );

    *m_process << "disk-info";
    *m_process << "--device" << m_device->busTargetLun();

    connect( m_process, SIGNAL(processExited(KProcess*)),
	     this, SLOT(slotDiskInfoFinished()) );
    connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	     this, SLOT(slotCollectStderr(KProcess*, char*, int)) );
    connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotCollectStdout(KProcess*, char*, int)) );

    m_collectedStdout = QString::null;
    m_collectedStderr = QString::null;

    if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
      qDebug("(K3bDiskInfoDetector) could not start cdrdao.");
      testForDvd();
    }
  }
}


void K3bDiskInfoDetector::slotDiskInfoFinished()
{
  // cdrdao finished, now parse the stdout output to check if it was successfull
  if( m_collectedStdout.isEmpty() ) {
    qDebug("(K3bDiskInfoDetector) disk-info gave no result... :-(");
  }
  else {
    // this is what we can use:
    // ------------------------
    // CD-RW
    // Total Capacity
    // CD-R medium (2 rows)
    // CD-R empty
    // Toc Type
    // Sessions
    // Appendable
    // Remaining Capacity
    // ------------------------

    // split to lines
    QStringList lines = QStringList::split( "\n", m_collectedStdout );

    for( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it ) {
      QString& str = *it;

      if( str.startsWith("CD-RW") ) {
	QString value = str.mid( str.find(":")+1 ).stripWhiteSpace();
	m_info.cdrw = (value == "yes");
      }

      else if( str.startsWith("Total Capacity") ) {
	m_info.sizeString = str.mid( str.find(":")+1 ).stripWhiteSpace();
	// now find the # blocks
	int start = m_info.sizeString.find("(")+1;
	int end = m_info.sizeString.find("blocks")-1;
	bool ok;
	int size = m_info.sizeString.mid( start, end-start ).toInt(&ok);
	if( ok )
	  m_info.size = size;
	else
	  qDebug("(K3bDiskInfoDetector) could not parse # of blocks from: " + m_info.sizeString.mid( start, end-start ) );
      }

      else if( str.startsWith("CD-R medium") ) {
	// here we have to parse two lines
	m_info.mediumManufactor = str.mid( str.find(":")+1 ).stripWhiteSpace();
	++it;
	m_info.mediumType = (*it).stripWhiteSpace();
      }

      else if( str.startsWith("CD-R empty") ) {
	// although this should be known before we parse it (just to be sure)
	QString value = str.mid( str.find(":")+1 ).stripWhiteSpace();
	m_info.empty = (value == "yes");
      }
	       
      else if( str.startsWith("Toc Type") ) {
	// FIXME
      }

      else if( str.startsWith("Sessions") ) {
	bool ok;
	int value = str.mid( str.find(":")+1 ).stripWhiteSpace().toInt(&ok);
	if( ok )
	  m_info.sessions = value;
	else
	  qDebug("(K3bDiskInfoDetector) Could not parse # of sessions: " + str.mid( str.find(":")+1 ).stripWhiteSpace() );
      }

      else if( str.startsWith("Appendable") ) {
	QString value = str.mid( str.find(":")+1 ).stripWhiteSpace();
	m_info.appendable = (value == "yes");
      }

      else if( str.startsWith("Remaining Capacity") ) {
	m_info.remainingString = str.mid( str.find(":")+1 ).stripWhiteSpace();
	// now find the # blocks
	int start = m_info.remainingString.find("(")+1;
	int end = m_info.remainingString.find("blocks")-1;
	bool ok;
	int size = m_info.remainingString.mid( start, end-start ).toInt(&ok);
	if( ok )
	  m_info.remaining = size;
	else
	  qDebug("(K3bDiskInfoDetector) could not parse # of blocks from: " + m_info.remainingString.mid( start, end-start ) );
      }

      else {
	qDebug("(K3bDiskInfoDetector) unusable cdrdao output: " + str );
      }
    }
  }

  testForDvd();
}


void K3bDiskInfoDetector::fetchTocInfo()
{
  m_process->clearArguments();
  m_process->disconnect();

  if( !k3bMain()->externalBinManager()->foundBin( "cdrecord" ) ) {
    qDebug("(K3bAudioJob) could not find cdrecord executable.. not toc info will be available" );
    m_info.valid = false;
    emit diskInfoReady( m_info );
  }
  else {
    *m_process << k3bMain()->externalBinManager()->binPath( "cdrecord" );

    *m_process << "-toc" << "-vv";   // -vv gives us atip info and cd-text (with recent cdrecord)
    *m_process << QString("dev=%1").arg( m_device->busTargetLun() );

    connect( m_process, SIGNAL(processExited(KProcess*)),
	     this, SLOT(slotTocInfoFinished()) );
    connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	     this, SLOT(slotCollectStderr(KProcess*, char*, int)) );
    connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotCollectStdout(KProcess*, char*, int)) );

    m_collectedStdout = QString::null;
    m_collectedStderr = QString::null;

    if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
      qDebug("(K3bDiskInfoDetector) could not start cdrecord. No toc info will be available");
      m_info.valid = false;
      emit diskInfoReady( m_info );
    }
  }
}


void K3bDiskInfoDetector::slotTocInfoFinished()
{
  // cdrecord finished, now parse the stdout output to check if it was successfull
  if( m_collectedStdout.isEmpty() ) {
    qDebug("(K3bDiskInfoDetector) cdrecord -toc gave no result... :-(");
    m_info.valid = false;
    emit diskInfoReady( m_info );
  }
  else if( m_collectedStderr.contains( "No disk" ) ) {
    m_info.noDisk = true;
    m_info.valid = true;
    emit diskInfoReady( m_info );
  }
  else {
    // parse the track list    
    // usable output is:
    // track:

    // we ignore the rest for now although there might be some atip info. This should be retrieved from cdrdao
    // TODO: CD-TEXT parsing (find a cd with cd-text or... :-(

    QStringList lines = QStringList::split( "\n", m_collectedStdout );

    K3bTrack lastTrack;
    int audioTracks = 0, dataTracks = 0;

    for( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it ) {
      QString& str = *it;

      if( str.startsWith("track:") ) {
	// cdrecord produces the following outout:
	// <tracknumber> lba: <startSector> (<...>) <startTime> adr: 1 control: <trackType> mode: <trackMode>
	// the last tracknumber will always be "lout", the leadout of the cd which we only use to determine the 
	// length of the last track

	// we just want the startSector, the trackType, and the trackMode
	int start = 6; // skip the "track:"
	start = str.find(":", start )+1;
	int end = str.find( "(", start )-1;

	bool ok;
	int startSec = str.mid( start, end-start ).toInt(&ok);
	if( ok ) {
	  start = str.find( "control:", start )+8; // skip the "control:"
	  end = str.find("mode:", start )-1;
	  int control = str.mid( start, end-start ).toInt(&ok);
	  if( ok ) {
	    start = end + 6;
	    int mode = str.mid( start ).toInt(&ok);
	    if( ok ) {
	      // all values have been determined
	      // since we need the start of the next track to determine the length we save the values
	      // in lastTrack and append the current lastTrack to the toc
	      if( !lastTrack.isEmpty() ) {
		m_info.toc.append( K3bTrack( lastTrack.firstSector(), startSec-1, lastTrack.type(), lastTrack.mode() ) );
		if( lastTrack.type() == K3bTrack::AUDIO )
		  audioTracks++;
		else
		  dataTracks++;
	      }

	      switch( control ) {
	      case 0:
		control = K3bTrack::AUDIO;
		break;
	      default:
		control = K3bTrack::DATA;
		break;
	      }

	      switch( mode ) {
	      case 1:
		mode = K3bTrack::MODE1;
		break;
	      case 2:
		mode = K3bTrack::MODE2;
		break;
	      default:
		mode = K3bTrack::UNKNOWN;
		break;
	      }

	      lastTrack = K3bTrack( startSec, startSec, control, mode );
	    }
	    else {
	      qDebug("(K3bDiskInfoDetector) Could not parse mode of track: " + str.mid( start ) );
	    }
	  }
	  else {
	    qDebug("(K3bDiskInfoDetector) Could not parse control of track: " + str.mid( start, end-start ) );
	  }
	}
	else {
	  qDebug("(K3bDiskInfoDetector) Could not parse start secstor of track: " + str.mid( start, end-start ) );
	}
      }

      else {
	qDebug("(K3bDiskInfoDetector) unusable cdrecord output: " + str );
      }
    }

    // now determine the toc type
    if( audioTracks > 0 ) {
      if( dataTracks > 0 )
	m_info.tocType = K3bDiskInfo::MIXED;
      else
	m_info.tocType = K3bDiskInfo::AUDIO;
    }
    else
      m_info.tocType = K3bDiskInfo::DATA;

  
    // atip is only readable on cd-writers
    if( m_device->burner() ) {
      fetchDiskInfo();
    }
    else {
      testForDvd();      
    }
  }
}


void K3bDiskInfoDetector::testForDvd()
{
  if( K3bTcWrapper::supportDvd() ) {
    // check if it is a dvd we can display

    if( !m_tcWrapper ) {
      m_tcWrapper = new K3bTcWrapper( this );
      connect( m_tcWrapper, SIGNAL(successfulDvdCheck(bool)), this, SLOT(slotIsDvd(bool)) );
    }

    m_tcWrapper->checkDvd( m_device );
  }
  else {
    // we are finished
    m_info.valid = true;
    emit diskInfoReady( m_info );
  }
}


void K3bDiskInfoDetector::slotIsDvd( bool dvd )
{
  if( dvd ) {
    m_info.empty = false;
    m_info.noDisk = false;
    m_info.tocType = K3bDiskInfo::DVD;
  }

  m_info.valid = true;
  emit diskInfoReady( m_info );
}


void K3bDiskInfoDetector::slotCollectStdout( KProcess*, char* data, int len )
{
  m_collectedStdout.append( QString::fromLatin1( data, len ) );
}

void K3bDiskInfoDetector::slotCollectStderr( KProcess*, char* data, int len )
{
  m_collectedStderr.append( QString::fromLatin1( data, len ) );
}


#include "k3bdiskinfodetector.moc"

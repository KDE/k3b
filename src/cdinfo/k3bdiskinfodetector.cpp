#include "k3bdiskinfodetector.h"

#include "../device/k3bdevice.h"
#include "../device/k3btoc.h"
#include "../rip/k3btcwrapper.h"
#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"

#include <kprocess.h>

#include <qtimer.h>
#include <qfile.h>



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
  if( !device ) {
    qDebug("(K3bDiskInfoDetector) detect should really not be called with NULL!");
    return;
  }

  cancel();

  m_bCanceled = false;

  m_device = device;

  // reset
  m_info = K3bDiskInfo();
  m_info.device = m_device;


  if( device->interfaceType() == K3bDevice::SCSI ) {
    // since fetchTocInfo could already emit the diskInfoReady signal
    // and detect needs to return before this happens use the timer
    QTimer::singleShot( 0, this, SLOT(fetchTocInfo()) );
  }
  else {
    // IDE device
    QTimer::singleShot( 0, this, SLOT(fetchIdeInformation()) );
  }
}


void K3bDiskInfoDetector::cancel()
{
  m_bCanceled = true;
  if( m_process->isRunning() )
    m_process->kill();
}


void K3bDiskInfoDetector::fetchDiskInfo()
{
  if( m_bCanceled )
    return;

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

    if( m_device->cdrdaoDriver() != "auto" )
      *m_process << "--driver" << m_device->cdrdaoDriver();

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
	  qDebug("(K3bDiskInfoDetector) could not parse # of blocks from: %s", m_info.sizeString.mid( start, end-start ).latin1() );
      }

      else if( str.startsWith("CD-R medium") ) {
	// here we have to parse two lines if not "n/a"
	m_info.mediumManufactor = str.mid( str.find(":")+1 ).stripWhiteSpace();
	if( m_info.mediumManufactor != "n/a" ) {
	  ++it;
	  m_info.mediumType = (*it).stripWhiteSpace();
	}
	else
	  m_info.mediumManufactor = "";
      }

      else if( str.startsWith("CD-R empty") ) {
	// although this should be known before we parse it (just to be sure)
	QString value = str.mid( str.find(":")+1 ).stripWhiteSpace();
	m_info.empty = (value == "yes");
      }
	       
      else if( str.startsWith("Toc Type") ) {
	// not used...
      }

      else if( str.startsWith("Sessions") ) {
	bool ok;
	int value = str.mid( str.find(":")+1 ).stripWhiteSpace().toInt(&ok);
	if( ok )
	  m_info.sessions = value;
	else
	  qDebug("(K3bDiskInfoDetector) Could not parse # of sessions: %s", str.mid( str.find(":")+1 ).stripWhiteSpace().latin1() );
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
	  qDebug("(K3bDiskInfoDetector) could not parse # of blocks from: %s", m_info.remainingString.mid( start, end-start ).latin1() );
      }

      else {
	qDebug("(K3bDiskInfoDetector) unusable cdrdao output: %s", str.latin1() );
      }
    }

    if( m_info.empty ) {
      m_info.remaining = m_info.size;
      m_info.remainingString = m_info.sizeString;
    }
  }

  testForDvd();
}


void K3bDiskInfoDetector::fetchTocInfo()
{
  if( m_bCanceled )
    return;

  m_process->clearArguments();
  m_process->disconnect();

  if( !k3bMain()->externalBinManager()->foundBin( "cdrecord" ) ) {
    qDebug("(K3bAudioJob) could not find cdrecord executable.. not toc info will be available" );
    m_info.valid = false;
    if( !m_bCanceled )
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
      if( !m_bCanceled )
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
    if( !m_bCanceled )
      emit diskInfoReady( m_info );
  }
  else if( m_collectedStderr.contains( "No disk" ) ) {
    m_info.noDisk = true;
    m_info.valid = true;
    if( !m_bCanceled )
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
	      qDebug("(K3bDiskInfoDetector) Could not parse mode of track: %s", str.mid( start ).latin1() );
	    }
	  }
	  else {
	    qDebug("(K3bDiskInfoDetector) Could not parse control of track: %s", str.mid( start, end-start ).latin1() );
	  }
	}
	else {
	  qDebug("(K3bDiskInfoDetector) Could not parse start secstor of track: %s", str.mid( start, end-start ).latin1() );
	}
      }

      else {
	qDebug("(K3bDiskInfoDetector) unusable cdrecord output: %s", str.latin1() );
      }
    }

    // now determine the toc type
    if( audioTracks > 0 ) {
      if( dataTracks > 0 )
	m_info.tocType = K3bDiskInfo::MIXED;
      else
	m_info.tocType = K3bDiskInfo::AUDIO;
    }
    else if( dataTracks > 0 ) {
      m_info.tocType = K3bDiskInfo::DATA;
      fetchIsoInfo();
    }

    if( audioTracks + dataTracks > 0 ) {
      m_info.empty = false;
      m_info.noDisk = false;

      calculateDiscId();
    }
    else {
      m_info.empty = true;
    }
  
    // atip is only readable on cd-writers
    if( m_device->burner() ) {
      fetchDiskInfo();
    }
    else {
      testForDvd();
    }
  }
}


void K3bDiskInfoDetector::fetchIsoInfo()
{
  if( m_bCanceled )
    return;

  QFile f( m_device->ioctlDevice() );
  f.open( IO_Raw | IO_ReadOnly );

  char buf[17*2048];

  if( f.readBlock( buf, 17*2048 ) == 17*2048 ) {
    m_info.isoId = QString::fromLatin1( &buf[16*2048+1], 5 ).stripWhiteSpace();
    m_info.isoSystemId = QString::fromLatin1( &buf[16*2048+8], 32 ).stripWhiteSpace();
    m_info.isoVolumeId = QString::fromLatin1( &buf[16*2048+40], 32 ).stripWhiteSpace();
    m_info.isoVolumeSetId = QString::fromLatin1( &buf[16*2048+190], 128 ).stripWhiteSpace();
    m_info.isoPublisherId = QString::fromLatin1( &buf[16*2048+318], 128 ).stripWhiteSpace();
    m_info.isoPreparerId = QString::fromLatin1( &buf[16*2048+446], 128 ).stripWhiteSpace();
    m_info.isoApplicationId = QString::fromLatin1( &buf[16*2048+574], 128 ).stripWhiteSpace();
  }

  f.close();
}


void K3bDiskInfoDetector::testForDvd()
{
  if( m_bCanceled )
    return;

  if( m_info.tocType == K3bDiskInfo::DATA && K3bTcWrapper::supportDvd() ) {
    // check if it is a dvd we can display

    if( !m_tcWrapper ) {
      qDebug("(K3bDiskInfoDetector) testForDvd");
      m_tcWrapper = new K3bTcWrapper( this );
      connect( m_tcWrapper, SIGNAL(successfulDvdCheck(bool)), this, SLOT(slotIsDvd(bool)) );
    }

    m_tcWrapper->isDvdInsert( m_device );

  }
  else {
    // we are finished
    m_info.valid = true;
    if( !m_bCanceled )
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
  if( !m_bCanceled )
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


void K3bDiskInfoDetector::fetchIdeInformation()
{
  if( m_bCanceled )
    return;

  // TODO: use cdparanoia-lib to retrieve toc

  m_info.valid = false;
  if( !m_bCanceled )
    emit diskInfoReady( m_info );
}


void K3bDiskInfoDetector::calculateDiscId()
{
  // calculate cddb-id
  unsigned int id = 0;
  for( K3bToc::iterator it = m_info.toc.begin(); it != m_info.toc.end(); ++it ) {
    unsigned int n = (*it).firstSector() + 150;
    n /= 75;
    while( n > 0 ) {
      id += n % 10;
      n /= 10;
    }
  }
  unsigned int l = m_info.toc.lastSector() - m_info.toc.firstSector();
  l /= 75;
  id = ( ( id % 0xff ) << 24 ) | ( l << 8 ) | m_info.toc.count();
  m_info.toc.setDiscId( id );
  
  qDebug("(K3bDiskInfoDetector) calculated disk id: %08x", id );
}

#include "k3bdiskinfodetector.moc"

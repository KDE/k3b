#include "k3bdiskinfodetector.h"

#include "../device/k3bdevice.h"
#include "../device/k3btoc.h"
#include "../rip/k3btcwrapper.h"
#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"

#include <kdebug.h>
#include <kprocess.h>

#include <qtimer.h>
#include <qfile.h>


#include <sys/ioctl.h>		// ioctls
#include <unistd.h>		// lseek, read. etc
#include <fcntl.h>		// O_RDONLY etc.
#include <linux/cdrom.h>	// old ioctls for cdrom
#include <stdlib.h>


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
    kdDebug() << "(K3bDiskInfoDetector) detect should really not be called with NULL!" << endl;
    return;
  }

  cancel();

  m_bCanceled = false;

  m_device = device;

  // reset
  m_info = K3bDiskInfo();
  m_info.device = m_device;


  // since fetchTocInfo could already emit the diskInfoReady signal
  // and detect needs to return before this happens use the timer
  if( device->interfaceType() == K3bDevice::SCSI )
    QTimer::singleShot( 0, this, SLOT(fetchTocInfo()) );
  else
    QTimer::singleShot( 0, this, SLOT(fetchIdeInformation()) );
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
    kdDebug() << "(K3bDiskInfoDetector) could not find cdrdao executable. no disk-info..." << endl;
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
      kdDebug() << "(K3bDiskInfoDetector) could not start cdrdao." << endl;
      testForDvd();
    }
  }
}


void K3bDiskInfoDetector::slotDiskInfoFinished()
{
  // cdrdao finished, now parse the stdout output to check if it was successfull
  if( m_collectedStdout.isEmpty() ) {
    kdDebug() << "(K3bDiskInfoDetector) disk-info gave no result... :-(" << endl;
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
	  kdDebug() << "(K3bDiskInfoDetector) could not parse # of blocks from: " << m_info.sizeString.mid( start, end-start ) << endl;
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
	  kdDebug() << "(K3bDiskInfoDetector) Could not parse # of sessions: " << str.mid( str.find(":")+1 ).stripWhiteSpace() << endl;
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
	  kdDebug() << "(K3bDiskInfoDetector) could not parse # of blocks from: " << m_info.remainingString.mid( start, end-start) << endl;
      }

      else {
	kdDebug() << "(K3bDiskInfoDetector) unusable cdrdao output: " << str << endl;
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

  if( m_device->interfaceType() == K3bDevice::IDE ||
      !k3bMain()->externalBinManager()->foundBin( "cdrecord" ) ) {
    kdDebug() << "(K3bDiskInfoDetector) using cdparanoia" << endl;
    fetchIdeInformation();
  }
  else {
    *m_process << k3bMain()->externalBinManager()->binPath( "cdrecord" );

    *m_process << "-toc";// << "-vv";   // -vv gives us atip info and cd-text (with recent cdrecord)
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
      kdDebug() << "(K3bDiskInfoDetector) could not start cdrecord. No toc info will be available" << endl;
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
    kdDebug() << "(K3bDiskInfoDetector) cdrecord -toc gave no result... :-(" << endl;
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

	      // TODO: fix endSector of last audio-track in multisession cds (-11400)

	      if( !lastTrack.isEmpty() ) {
		m_info.toc.append( K3bTrack( lastTrack.firstSector(), startSec-1, lastTrack.type(), lastTrack.mode() ) );
	      }


	      // now this is the meaning of control and mode:
	      // control (combination of the following)
	      // 0x01 - Audio with preemp
	      // 0x02 - Audio copy permitted
	      // 0x04 - Data track
	      // 0x08 - 4 channel audio

	      // mode (only for data tracks)
	      // 1 - Mode 1
	      // 2 - Mode 2 

	      int trackType = 0;
	      int trackMode = K3bTrack::UNKNOWN;
	      if( control & 0x04 ) {
		trackType = K3bTrack::DATA;
		if( mode == 1 )
		  trackMode = K3bTrack::MODE1;
		else if( mode == 2 )
		  trackMode = K3bTrack::MODE2;
	      }
	      else
		trackType = K3bTrack::AUDIO;

	      lastTrack = K3bTrack( startSec, startSec, trackType, trackMode );
	    }
	    else {
	      kdDebug() << "(K3bDiskInfoDetector) Could not parse mode of track: " << str.mid( start ) << endl;
	    }
	  }
	  else {
	    kdDebug() << "(K3bDiskInfoDetector) Could not parse control of track: " << str.mid( start, end-start ) << endl;
	  }
	}
	else {
	  kdDebug() << "(K3bDiskInfoDetector) Could not parse start secstor of track: " << str.mid( start, end-start) << endl;
	}
      }

      else {
	kdDebug() << "(K3bDiskInfoDetector) unusable cdrecord output: " << str << endl;
      }
    }

    // now determine the toc type
    determineTocType();


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
    m_info.isoId = QString::fromLocal8Bit( &buf[16*2048+1], 5 ).stripWhiteSpace();
    m_info.isoSystemId = QString::fromLocal8Bit( &buf[16*2048+8], 32 ).stripWhiteSpace();
    m_info.isoVolumeId = QString::fromLocal8Bit( &buf[16*2048+40], 32 ).stripWhiteSpace();
    m_info.isoVolumeSetId = QString::fromLocal8Bit( &buf[16*2048+190], 128 ).stripWhiteSpace();
    m_info.isoPublisherId = QString::fromLocal8Bit( &buf[16*2048+318], 128 ).stripWhiteSpace();
    m_info.isoPreparerId = QString::fromLocal8Bit( &buf[16*2048+446], 128 ).stripWhiteSpace();
    m_info.isoApplicationId = QString::fromLocal8Bit( &buf[16*2048+574], 128 ).stripWhiteSpace();
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
      kdDebug() << "(K3bDiskInfoDetector) testForDvd" << endl;
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
  m_collectedStdout.append( QString::fromLocal8Bit( data, len ) );
}

void K3bDiskInfoDetector::slotCollectStderr( KProcess*, char* data, int len )
{
  m_collectedStderr.append( QString::fromLocal8Bit( data, len ) );
}


void K3bDiskInfoDetector::fetchIdeInformation()
{
  if( m_bCanceled )
    return;

  int cdromfd = ::open( m_device->blockDeviceName().latin1(), O_RDONLY | O_NONBLOCK );
  if (cdromfd < 0) {
    kdDebug() << "(K3bDiskInfoDetector) could not open device" << endl;
    m_info.valid = false;
    emit diskInfoReady( m_info );
    return;
  }

  int ret = ::ioctl(cdromfd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
  m_info.valid = true;

  switch(ret) {
  case CDS_DISC_OK:
    // read toc
    struct cdrom_tochdr tocHdr;
    if( ::ioctl(cdromfd, CDROMREADTOCHDR, &tocHdr ) ) {
      kdDebug() << "disk empty?" << endl;
      m_info.noDisk = true;
      break;
    }
    else {
      struct cdrom_tocentry tocE;
      tocE.cdte_format = CDROM_LBA;
      int numTracks = (int)tocHdr.cdth_trk1 - (int)tocHdr.cdth_trk0 + 1;

      // we create this fields to have ending sector info later on
      int* startSectors = new int[numTracks+1];
      bool* dataTrack = new bool[numTracks+1];
      int* modes = new int[numTracks+1];

      // read info for every single track
      int j = 0;
      for( int i = (int)tocHdr.cdth_trk0; i <= (int)tocHdr.cdth_trk1; ++i ) {
	tocE.cdte_track = i;
	if( ::ioctl(cdromfd, CDROMREADTOCENTRY, &tocE ) ) {
	  m_info.valid = false;
	  break;
	}
	else {

	  // cdte_ctrl: or'ed combination of the following:
	  // 0x01 - Audio with preemp
	  // 0x02 - Audio copy permitted
	  // 0x04 - Data track
	  // 0x08 - 4 channel audio

	  startSectors[j] = (int)tocE.cdte_addr.lba;
	  dataTrack[j] = (bool)(tocE.cdte_ctrl & CDROM_DATA_TRACK);
	  modes[j] = (int)tocE.cdte_datamode; // this does not seem to provide valid data mode (1/2) info
	  j++;
	}
      }

      if( m_info.valid ) {
	// read leadout info
	tocE.cdte_track = CDROM_LEADOUT;
	::ioctl(cdromfd, CDROMREADTOCENTRY, &tocE );
	startSectors[numTracks] = (int)tocE.cdte_addr.lba;

	// now create the K3b structures	
	for( int i = 0; i < numTracks; ++i ) {
	  m_info.toc.append( K3bTrack( startSectors[i], 
				       startSectors[i+1], 
				       dataTrack[i] ? K3bTrack::DATA : K3bTrack::AUDIO, 
				       K3bTrack::UNKNOWN ) );  // no valid mode info as far as I know
	}
	
	m_info.empty = ( numTracks == 0 );
	m_info.noDisk = false;
      }

      // cleanup
      delete [] startSectors;
      delete [] dataTrack;
      delete [] modes;
    }
    break;
  case CDS_NO_DISC:
    kdDebug() << "no disk" << endl;
    m_info.noDisk = true;
    break;
  case CDS_TRAY_OPEN:
    kdDebug() << "tray open" << endl;
  case CDS_NO_INFO:
    kdDebug() << "no info" << endl;
  case CDS_DRIVE_NOT_READY:
    kdDebug() << "not ready" << endl;
  default:
    m_info.valid = false;
    break;
  }

  ::close( cdromfd );

  if( !m_info.valid || m_info.empty ) {
    emit diskInfoReady( m_info );
    return;
  }

  determineTocType();

  testForDvd();
}


void K3bDiskInfoDetector::determineTocType()
{
  int audioTracks = 0, dataTracks = 0;

  for( K3bToc::const_iterator it = m_info.toc.begin(); it != m_info.toc.end(); ++it ) {
    if( (*it).type() == K3bTrack::AUDIO )
      audioTracks++;
    else
      dataTracks++;
  }

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

  kdDebug() << "(K3bDiskInfoDetector) calculated disk id: " << id << endl;
}

#include "k3bdiskinfodetector.moc"

#include "k3bdiskinfodetector.h"

#include "../device/k3bdevice.h"
#include "../device/k3btoc.h"
#include "../rip/k3btcwrapper.h"
#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"

#include <kdebug.h>

#include <qtimer.h>
#include <qfile.h>

#include <sys/ioctl.h>		// ioctls
#include <unistd.h>		// lseek, read. etc
#include <fcntl.h>		// O_RDONLY etc.
#include <linux/cdrom.h>	// ioctls for cdrom
#include <stdlib.h>


K3bDiskInfoDetector::K3bDiskInfoDetector( QObject* parent )
  : QObject( parent )
{
}


K3bDiskInfoDetector::~K3bDiskInfoDetector()
{
}


void K3bDiskInfoDetector::detect( K3bDevice* device )
{
  if( !device ) {
    kdDebug() << "(K3bDiskInfoDetector) detect should really not be called with NULL!" << endl;
    return;
  }

  m_device = device;

  // reset
  m_info = K3bDiskInfo();
  m_info.device = m_device;

  QTimer::singleShot(0,this,SLOT(fetchTocInfo()));
}


void K3bDiskInfoDetector::finish(bool success)
{
  m_info.valid=success;
  ::close(m_cdfd);
  emit diskInfoReady(m_info);

}


void K3bDiskInfoDetector::fetchDiskInfo()
{
  struct cdrom_generic_command cmd;
// struct cdrom_generic_command
// {
//         unsigned char           cmd[CDROM_PACKET_SIZE];
//         unsigned char           *buffer;
//         unsigned int            buflen;
//         int                     stat;
//         struct request_sense    *sense;
//         unsigned char           data_direction;
//         int                     quiet;
//         int                     timeout;
//         void                    *reserved[1];
// };


  unsigned char inf[32];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(inf,0,32);
  cmd.cmd[0] = GPCMD_READ_DISC_INFO;
  cmd.cmd[8] = 32;
  cmd.buffer = inf;
  cmd.buflen = 32;
  cmd.data_direction = CGC_DATA_READ;
//
// Disc Information Block
// ======================
// Byte   0-1: Disk Information Length
// Byte     2: Bits 0-1 Disc Status: 0-empty,1-appendable,2-complete,3-other
//             Bits 2-3 State of last Session: 0-empty,1-incomplete,2-reserved,3-complete
//             Bit    4 Erasable, 1-cd-rw present
//             Bits 5-7 Reserved
// Byte     3: Number of first Track on Disk
// Byte     4: Number of Sessions (LSB)
// Byte     5: First Track in Last Session (LSB)
// Byte     6: Last Track in Last Session (LSB)
// Byte     7: Bits 0-4 Reserved
//             Bit    5 URU Unrestricted Use Bit
//             Bit    6 DBC_V Disc Bar Code Field is Valid 
//             Bit    7 DID_V Disc Id Field is valid
// Byte     8: Disc Type: 0x00 CD-DA or CD-ROM, 0x10 CD-I, 0x20 CD-ROM XA, 0xFF undefined
//                        all other values reserved
// Byte     9: Number of Sessions (MSB)
// Byte    10: First Track in Last Session (MSB)
// Byte    11: Last Track in Last Session (MSB)
// Byte 12-15: Disc Identification
// Byte 16-19: Last Session Lead-In Start Time MSF (16-MSB 19-LSB)
// Byte 20-23: Last Possible Start Time for Start of Lead-Out MSF (20-MSB 23-LSB)
// Byte 24-31: Disc Bar Code
//        
  if ( ::ioctl(m_cdfd,CDROM_SEND_PACKET,&cmd) == 0 ) {
    m_info.appendable = ( (inf[2] & 0x03) < 2 );        // disc empty or incomplete
    m_info.empty = ( (inf[2] & 0x03) == 0 );
    m_info.cdrw =( ((inf[2] >> 4 ) & 0x01) == 1 );      // erasable
    if ( inf[21] != 0xFF && inf[22] != 0xFF && inf[23] != 0xFF ) {
      m_info.size = inf[21]*4500 + inf[22]*75 +inf[23] - 150;
      m_info.sizeString = QString("%1:%2:%3").arg(inf[21]).arg(inf[22]).arg(inf[23]);
    }
    if (m_info.empty) {
      m_info.remaining = m_info.size;
      m_info.remainingString = m_info.sizeString;
    } else if ( inf[17] != 0xFF && inf[18] != 0xFF && inf[19] != 0xFF ) { // start of last leadin - 4650
      m_info.remaining = m_info.size - inf[17]*4500 - inf[18]*75 - inf[19] - 4650;
      m_info.remainingString = QString("%1:%2:%3").arg(inf[17]).arg(inf[18]).arg(inf[19]);
    }
  } else
    kdDebug() << "(K3bDiskInfoDetector) could not get disk info !" << endl;
}

void K3bDiskInfoDetector::fetchTocInfo()
{
  struct cdrom_tochdr tochdr;
  struct cdrom_tocentry tocentry;
  int status;
  if ( (m_cdfd = ::open(m_device->ioctlDevice().latin1(),O_RDONLY | O_NONBLOCK)) == -1 ) {
    kdDebug() << "(K3bDiskInfoDetector) could not open device !" << endl;
    m_info.valid=false;
    emit diskInfoReady(m_info);
    return;
  }
  
  if ( (status = ::ioctl(m_cdfd,CDROM_DISC_STATUS)) >= 0 )
    switch (status) {
      case CDS_AUDIO:   m_info.tocType = K3bDiskInfo::AUDIO;
                        break;
      case CDS_DATA_1:
      case CDS_DATA_2:  m_info.tocType = K3bDiskInfo::DATA;
                        break;
      case CDS_XA_2_1: 
      case CDS_XA_2_2: 
      case CDS_MIXED:   m_info.tocType = K3bDiskInfo::MIXED;
                        break;
      case CDS_NO_DISC: finish(true);
                        return;  
      case CDS_NO_INFO: m_info.noDisk = false;
                        if (m_info.device->burner())
                           fetchDiskInfo();
                        finish(true);
                        return;  
  } else 
     kdDebug() << "(K3bDiskInfoDetector) disc status is " << status << " !" << endl;   
  
  m_info.noDisk = false;
  
  struct cdrom_generic_command cmd;
  unsigned char dat[4];

  ::memset(&cmd,0,sizeof (struct cdrom_generic_command));
  ::memset(dat,0,4);
  cmd.cmd[0] = GPCMD_READ_TOC_PMA_ATIP; 
// Format Field: 0-TOC, 1-Session Info, 2-Full TOC, 3-PMA, 4-ATIP, 5-CD-TEXT
  cmd.cmd[2] = 1; 
  cmd.cmd[8] = 4;
  cmd.buffer = dat;
  cmd.buflen = 4;
  cmd.data_direction = CGC_DATA_READ;
//
// Session Info
// ============
// Byte 0-1: Data Length
// Byte   2: First Complete Session Number (Hex) - always 1
// Byte   3: Last Complete Session Number (Hex)
//
  if ( ::ioctl(m_cdfd,CDROM_SEND_PACKET,&cmd) == 0 )
     m_info.sessions = dat[3];
  else 
    kdDebug() << "(K3bDiskInfoDetector) could not get session info !" << endl;
// 
// CDROMREADTOCHDR ioctl returns: 
// cdth_trk0: First Track Number
// cdth_trk1: Last Track Number
//
  if ( ::ioctl(m_cdfd,CDROMREADTOCHDR,&tochdr) != 0 )
  {
     kdDebug() << "(K3bDiskInfoDetector) could not get toc header !" << endl;
     finish(false);
     return;
  }
  K3bTrack lastTrack;
  for (int i = tochdr.cdth_trk0; i <= tochdr.cdth_trk1 + 1; i++) {
    ::memset(&tocentry,0,sizeof (struct cdrom_tocentry));
// get Lead-Out Information too
    tocentry.cdte_track = (i<=tochdr.cdth_trk1) ? i : CDROM_LEADOUT;
    tocentry.cdte_format = CDROM_LBA;
//
// CDROMREADTOCENTRY ioctl returns:
// cdte_addr.lba: Start Sector Number (LBA Format requested)
// cdte_ctrl:     4 ctrl bits
//                   00x0b: 2 audio Channels(no pre-emphasis)
//                   00x1b: 2 audio Channels(pre-emphasis)
//                   10x0b: audio Channels(no pre-emphasis),reserved in cd-rw
//                   10x1b: audio Channels(pre-emphasis),reserved in cd-rw
//                   01x0b: data track, recorded uninterrupted
//                   01x1b: data track, recorded incremental
//                   11xxb: reserved
//                   xx0xb: digital copy prohibited
//                   xx1xb: digital copy permitted
// cdte_addr:     4 addr bits (type of Q-Subchannel data)
//                   0000b: no Information
//                   0001b: current position data
//                   0010b: MCN
//                   0011b: ISRC
//                   0100b-1111b:  reserved
// cdte_datamode:  0: Data Mode1
//                 1: CD-I
//                 2: CD-XA Mode2
//

    if ( ::ioctl(m_cdfd,CDROMREADTOCENTRY,&tocentry) != 0)
      kdDebug() << "(K3bDiskInfoDetector) error reading tocentry " << i << endl; 
    int startSec = tocentry.cdte_addr.lba;
    int control  = tocentry.cdte_ctrl & 0x0f;
    int mode     = tocentry.cdte_datamode;
    if( !lastTrack.isEmpty() ) {
		   m_info.toc.append( K3bTrack( lastTrack.firstSector(), startSec-1, lastTrack.type(), lastTrack.mode() ) );
	  }
    int trackType = 0;
    int trackMode = K3bTrack::UNKNOWN;
	  if( control & 0x04 ) {
	  	trackType = K3bTrack::DATA;
		  if( mode == 1 )
		    trackMode = K3bTrack::MODE1;
		  else if( mode == 2 )
		    trackMode = K3bTrack::MODE2;
	  } else
		  trackType = K3bTrack::AUDIO;

	  lastTrack = K3bTrack( startSec, startSec, trackType, trackMode );

  }
  if (m_info.device->burner())
    fetchDiskInfo();

  if (m_info.tocType != K3bDiskInfo::AUDIO)
    fetchIsoInfo();
  else
    calculateDiscId();

  int caps;

// get capabilities
  if ( (caps=::ioctl(m_cdfd,CDROM_GET_CAPABILITY)) >= 0 ) 
// is the device dvd capable ?
    if ( caps & (CDC_DVD | CDC_DVD_R | CDC_DVD_RAM) ) {
//     try to read the physical dvd-structure
//     if this fails, we probably cannot take any further (usefull) dvd-action
       dvd_struct dvdinfo;
       ::memset(&dvdinfo,0,sizeof(dvd_struct));
       dvdinfo.type = DVD_STRUCT_PHYSICAL;
       if ( ::ioctl(m_cdfd,DVD_READ_STRUCT,&dvdinfo) == 0 ) {
          m_info.empty = false;
          m_info.noDisk = false;
          m_info.tocType = K3bDiskInfo::DVD;
       }
    }

  finish(true);
}

void K3bDiskInfoDetector::fetchIsoInfo()
{
  char buf[17*2048];
  ::lseek( m_cdfd, 0, SEEK_SET );

  if( ::read( m_cdfd, buf, 17*2048 ) == 17*2048 ) {
    m_info.isoId = QString::fromLocal8Bit( &buf[16*2048+1], 5 ).stripWhiteSpace();
    m_info.isoSystemId = QString::fromLocal8Bit( &buf[16*2048+8], 32 ).stripWhiteSpace();
    m_info.isoVolumeId = QString::fromLocal8Bit( &buf[16*2048+40], 32 ).stripWhiteSpace();
    m_info.isoVolumeSetId = QString::fromLocal8Bit( &buf[16*2048+190], 128 ).stripWhiteSpace();
    m_info.isoPublisherId = QString::fromLocal8Bit( &buf[16*2048+318], 128 ).stripWhiteSpace();
    m_info.isoPreparerId = QString::fromLocal8Bit( &buf[16*2048+446], 128 ).stripWhiteSpace();
    m_info.isoApplicationId = QString::fromLocal8Bit( &buf[16*2048+574], 128 ).stripWhiteSpace();
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

/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998, 1999 Andreas Mueller <mueller@daneb.ping.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * $Log$
 * Revision 1.2  2000/12/17 10:51:23  andreasm
 * Default verbose level is now 2. Adaopted message levels to have finer
 * grained control about the amount of messages printed by cdrdao.
 * Added CD-TEXT writing support to the GenericMMCraw driver.
 * Fixed CD-TEXT cue sheet creating for the GenericMMC driver.
 *
 * Revision 1.1.1.1  2000/02/05 01:37:07  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 * Revision 1.8  1999/04/02 16:44:30  mueller
 * Removed 'revisionDate' because it is not available in general.
 *
 * Revision 1.7  1999/03/27 20:52:56  mueller
 * Adapted to slightly changed interface.
 *
 * Revision 1.6  1999/02/06 20:41:44  mueller
 * Improved error message.
 *
 * Revision 1.5  1998/09/06 13:34:22  mueller
 * Use 'message()' for printing messages.
 *
 * Revision 1.4  1998/08/13 19:13:28  mueller
 * Added member function 'timout()' to set timeout of SCSI commands.
 *
 * Revision 1.3  1998/08/07 12:36:04  mueller
 * Added enabling command transformation for emulated host adaptor (ATAPI).
 *
 */

static char rcsid[] = "$Id$";

#include <config.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <linux/../scsi/sg.h>  /* cope with silly includes */
#include <asm/param.h> // for HZ

#include "ScsiIf.h"
#include "util.h"
#include "sg_err.h"
// for qDebug
#include <qobject.h>
/* Runtime selection to obtain the best features available from the
   Linux SCSI generic (sg) driver taken from:

*  Copyright (C) 1999 D. Gilbert
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.

   This program is meant as an example to application writers who wish
   to use the Linux sg driver. Linux changed its sg driver in kernel
   2.2.6 . The new version has extra features that application writers
   may wish to use but not at the expense of backward compatibility with
   the original driver. Also if an application is distributed in binary
   code, runtime selection is needed in the application code interfacing
   to sg in order to cope with the various combinations:

   App compiled with    App binary run on     Notes
   ----------------------------------------------------------------------
   new sg.h             new driver
   new sg.h             original driver       backward compatibility mode
   original sg.h        new driver            "forward" compatibility mode
   original sg.h        original driver


   Noteworthy features:
        - forward + backward compatible from 2.0 to 2.3 series of
          kernels (tested on: 2.0.27, 2.2.10, 2.3.8). Extra features
          are used when available. This is done via run time selection.
        - handles /usr/include/scsi bug in Redhat 6.0 + other distros
        - allows device argument to be a non-sg SCSI device (eg /dev/sda)
          and shows mapping to equivalent sg device
        - internal scan (used for previous item when non-sg device) does
          not hang on O_EXCL flag on device but steps over it.
        - allows app to request reserved buffer size and shows the amount
          actually reserved **
        - outputs as much error information as is available
        - uses categorization and sense buffer decode routines in sg_err.c
        - sets SCSI command length explicitly (when available)

   ** The original reserved buffer (ie SG_BIG_BUFF) is allocated one for
      all users of the driver. From 2.2.6 onwards, the reserved buffer is
      per file descriptor.

   One assumption made here is that ioctl command numbers do not change.

*/

#ifndef SG_GET_RESERVED_SIZE
#define SG_GET_RESERVED_SIZE 0x2272
#endif

#ifndef SG_SET_RESERVED_SIZE
#define SG_SET_RESERVED_SIZE 0x2275
#endif

#ifndef SG_GET_VERSION_NUM
#define SG_GET_VERSION_NUM 0x2282
#endif

#ifndef SG_MAX_SENSE
#define SG_MAX_SENSE 16
#endif


#define SG_RT_ORIG 0
#define SG_RT_NEW32 1  /* new driver version 2.1.31 + 2.1.32 */
#define SG_RT_NEW34 2  /* new driver version 2.1.34 and after */


class ScsiIfImpl {
public:
  struct ScsiIdLun {
    int mux4;
    int hostUniqueId;
  };

  struct Sghn { /* for "forward" compatibility case */
    int pack_len;    /* [o] reply_len (ie useless), ignored as input */
    int reply_len;   /* [i] max length of expected reply (inc. sg_header) */
    int pack_id;     /* [io] id number of packet (use ints >= 0) */
    int result;      /* [o] 0==ok, else (+ve) Unix errno (best ignored) */
    unsigned int twelve_byte:1; 
           /* [i] Force 12 byte command length for group 6 & 7 commands  */
    unsigned int target_status:5;   /* [o] scsi status from target */
    unsigned int host_status:8;     /* [o] host status (see "DID" codes) */
    unsigned int driver_status:8;   /* [o] driver status+suggestion */
    unsigned int other_flags:10;    /* unused */
    unsigned char sense_buffer[SG_MAX_SENSE]; /* [o] Output in 3 cases:
           when target_status is CHECK_CONDITION or 
           when target_status is COMMAND_TERMINATED or
           when (driver_status & DRIVER_SENSE) is true. */
  };      /* This structure is 36 bytes long on i386 */

  char *filename_; // user provided device name
  char *dev_;      // actual sg device name
  int fd_;

  int driverVersion_;
  int maxSendLen_;

  char *buf_;
  Sghn *bufHd_;

  const char *makeDevName(int k, int do_numeric);
  int openScsiDevAsSg(const char *devname);
  void determineDriverVersion();
  int adjustReservedBuffer(int requestedSize);
};


ScsiIf::ScsiIf(const char *dev)
{
  impl_ = new ScsiIfImpl;

  impl_->filename_ = strdupCC(dev);
  impl_->dev_ = NULL;

  impl_->maxSendLen_ = 0;

  impl_->buf_ = NULL;
  impl_->bufHd_ = NULL;
  
  impl_->fd_ = -1;

  vendor_[0] = 0;
  product_[0] = 0;
  revision_[0] = 0;
}

ScsiIf::~ScsiIf()
{
  if (impl_->fd_ >= 0)
    close(impl_->fd_);

  delete[] impl_->filename_;
  delete[] impl_->dev_;
  delete[] impl_->buf_;
  delete impl_;
}

// opens and flushes scsi device
// return: 0: OK
//         1: device could not be opened
//         2: inquiry failed
int ScsiIf::init()
{
  int flags;

  if ((impl_->fd_ = impl_->openScsiDevAsSg(impl_->filename_)) < 0) {
    if (impl_->fd_ != -9999) {
      qDebug( "Cannot open SG device \"%s\": %s",
	      impl_->filename_, strerror(errno));
      return 1;
    }
    else {
      qDebug( "Cannot map \"%s\" to a SG device.", impl_->filename_);
      return 1;
    }
  }

  impl_->determineDriverVersion();

  impl_->maxSendLen_ = impl_->adjustReservedBuffer(64 * 1024);

  if (impl_->maxSendLen_ < 4096) {
    qDebug( "%s: Cannot reserve enough buffer space - granted size: %d",
	    impl_->dev_, impl_->maxSendLen_);
    return 1;
  }

  maxDataLen_ = impl_->maxSendLen_ - sizeof(struct sg_header) - 12;

  impl_->buf_ = new char[impl_->maxSendLen_];
  impl_->bufHd_ = (ScsiIfImpl::Sghn *)impl_->buf_;

  flags = fcntl(impl_->fd_, F_GETFL);
  fcntl(impl_->fd_, F_SETFL, flags|O_NONBLOCK);

  memset(impl_->bufHd_, 0, sizeof(ScsiIfImpl::Sghn));
  impl_->bufHd_->reply_len = sizeof(ScsiIfImpl::Sghn);

  while (read(impl_->fd_, impl_->bufHd_, impl_->maxSendLen_) >= 0 ||
	 errno != EAGAIN) ;

  fcntl(impl_->fd_, F_SETFL, flags);

#if 0 && defined(SG_EMULATED_HOST) && defined(SG_SET_TRANSFORM)
  if (ioctl(impl_->fd_, SG_EMULATED_HOST, &flags) == 0 && flags != 0) {
    // emulated host adaptor for ATAPI drives
    // switch on command transformation
    ioctl(impl_->fd_, SG_SET_TRANSFORM, NULL);
  }
#endif

  if (inquiry() != 0) {
    return 2;
  }

  return 0;
}

// Sets given timeout value in seconds and returns old timeout.
// return: old timeout
int ScsiIf::timeout(int t)
{
  int old = ioctl(impl_->fd_, SG_GET_TIMEOUT, NULL);
  int ret;

  t *= HZ;

  if ((ret = ioctl(impl_->fd_, SG_SET_TIMEOUT, &t)) != 0) {
     qDebug("Cannot set SCSI timeout: %s", strerror(ret));
  }

  return old/HZ;
}

// sends a scsi command and receives data
// return 0: OK
//        1: scsi command failed (os level, no sense data available)
//        2: scsi command failed (sense data available)
int ScsiIf::sendCmd(const unsigned char *cmd, int cmdLen,
		    const unsigned char *dataOut, int dataOutLen,
		    unsigned char *dataIn, int dataInLen, int showMessage)
{
  int status;
  int sendLen = sizeof(struct sg_header) + cmdLen + dataOutLen;
  int recLen = sizeof(struct sg_header) + dataInLen;
  //	MODIFIED for K3b
  //assert(cmdLen > 0 && cmdLen <= 12);
	if(cmdLen < 0 || cmdLen > 12){
		return 1; // scsi command error
	}
  //assert(sendLen <= impl_->maxSendLen_);
  if (sendLen > impl_->maxSendLen_){
  		return 1; // scsi command error
  }

  memset(impl_->buf_, 0, sizeof(ScsiIfImpl::Sghn));

  memcpy(impl_->buf_ + sizeof(ScsiIfImpl::Sghn), cmd, cmdLen);
  if (dataOut != NULL) {
    memcpy(impl_->buf_ + sizeof(ScsiIfImpl::Sghn) + cmdLen, dataOut,
	   dataOutLen);
  }

  impl_->bufHd_->reply_len   = recLen;
  impl_->bufHd_->twelve_byte = (cmdLen == 12);
  impl_->bufHd_->result = 0;

  /* send command */
  do {
    status = write(impl_->fd_, impl_->buf_, sendLen);
    if ((status < 0 && errno != EINTR) ||
	(status >= 0 && status != sendLen)) {
      /* some error happened */
      qDebug( "write(generic) result = %d cmd = 0x%x: %s",
	      status, cmd[0], strerror(errno) );
      return 1;
    }
  } while (status < 0);

  // read result
  do {
    status = read(impl_->fd_, impl_->buf_, recLen);
    if (status < 0) {
      if (errno != EINTR) {
	qDebug( "read(generic) failed: %s", strerror(errno));
	return 1;
      }
    }
    else if (status != recLen) {
      qDebug( "read(generic) did not return expected amount of data.");
      return 1;
    }
    else if (impl_->bufHd_->result) {
      // scsi command failed
      switch (impl_->driverVersion_) {
      case SG_RT_NEW32:
      case SG_RT_NEW34:
	sg_chk_n_print("\nSCSI command failed", impl_->bufHd_->target_status,
		       impl_->bufHd_->host_status,
		       impl_->bufHd_->driver_status,
		       impl_->bufHd_->sense_buffer);
	break;
      default:
	qDebug( "read(generic) failed: %s",
		strerror(impl_->bufHd_->result));
	break;
      }

      return 1;
    }
    else if (impl_->bufHd_->sense_buffer[2] != 0) {
      if (showMessage)
	printError();

      return 2;
    }
  } while (status < 0);

  if (dataIn != NULL && dataInLen > 0) {
    memcpy(dataIn, impl_->buf_ + sizeof(ScsiIfImpl::Sghn), dataInLen);
  }

  return 0;
}

const unsigned char *ScsiIf::getSense(int &len) const
{
  len = 15;
  return impl_->bufHd_->sense_buffer;
}

void ScsiIf::printError()
{
  switch (impl_->driverVersion_) {
  case SG_RT_NEW32:
  case SG_RT_NEW34:
    sg_chk_n_print("\nSCSI command failed", impl_->bufHd_->target_status,
		   impl_->bufHd_->host_status, impl_->bufHd_->driver_status,
		   impl_->bufHd_->sense_buffer);
    break;

  default:
    sg_print_sense("\nSCSI command failed", impl_->bufHd_->sense_buffer);
    break;
  }

  //decodeSense(impl_->bufHd_->sense_buffer, 15);
}

int ScsiIf::inquiry()
{
  unsigned char cmd[6];
  unsigned char result[0x2c];
  int i;

  cmd[0] = 0x12; // INQUIRY
  cmd[1] = cmd[2] = cmd[3] = 0;
  cmd[4] = 0x2c;
  cmd[5] = 0;

  if (sendCmd(cmd, 6, NULL, 0, result, 0x2c, 1) != 0) {
    qDebug( "Inquiry command failed on \"%s\"", impl_->dev_);
    return 1;
  }

  strncpy(vendor_, (char *)(result + 0x08), 8);
  vendor_[8] = 0;

  strncpy(product_, (char *)(result + 0x10), 16);
  product_[16] = 0;

  strncpy(revision_, (char *)(result + 0x20), 4);
  revision_[4] = 0;

  for (i = 7; i >= 0 && vendor_[i] == ' '; i--) {
    vendor_[i] = 0;
  }

  for (i = 15; i >= 0 && product_[i] == ' '; i--) {
    product_[i] = 0;
  }

  for (i = 3; i >= 0 && revision_[i] == ' '; i--) {
    revision_[i] = 0;
  }

  return 0;
}

ScsiIf::ScanData *ScsiIf::scan(int *len)
{
  *len = 0;
  return NULL;
}

#include "ScsiIf-common.cc"

void ScsiIfImpl::determineDriverVersion()
{
  int reserved_size = 0;
  int sg_version = 0;

  /* Run time selection code follows */
  if (ioctl(fd_, SG_GET_RESERVED_SIZE, &reserved_size) < 0) {
    driverVersion_ = SG_RT_ORIG;
    qDebug( "Detected old SG driver version.");
  }
  else if (ioctl(fd_, SG_GET_VERSION_NUM, &sg_version) < 0) {
    driverVersion_ = SG_RT_NEW32;
    qDebug( "Detected SG driver version: 2.1.32");
  }
  else {
    driverVersion_ = SG_RT_NEW34;
    qDebug( "Detected SG driver version: %d.%d.%d", sg_version / 10000,
	    (sg_version / 100) % 100, sg_version % 100);
  }
}

#define MAX_SG_DEVS 26

#define SCAN_ALPHA 0
#define SCAN_NUMERIC 1
#define DEF_SCAN SCAN_ALPHA

const char *ScsiIfImpl::makeDevName(int k, int do_numeric)
{
  static char filename[100];
  char buf[20];

  strcpy(filename, "/dev/sg");

  if (do_numeric) {
    sprintf(buf, "%d", k);
    strcat(filename, buf);
  }
  else {
    if (k <= 26) {
      buf[0] = 'a' + (char)k;
      buf[1] = '\0';
      strcat(filename, buf);
    }
    else {
      strcat(filename, "xxxx");
    }
  }

  return filename;
}

int ScsiIfImpl::openScsiDevAsSg(const char *devname)
{
  int fd, bus, bbus, k;
  ScsiIdLun m_idlun, mm_idlun;
  int do_numeric = DEF_SCAN;
  const char *fname = devname;

  if ((fd = open(fname, O_RDONLY | O_NONBLOCK)) < 0) {
    if (EACCES == errno) {
      if ((fd = open(fname, O_RDWR | O_NONBLOCK)) < 0)
	return fd;
    }
  }
  if (ioctl(fd, SG_GET_TIMEOUT, 0) < 0) { /* not a sg device ? */
    if (ioctl(fd, SCSI_IOCTL_GET_BUS_NUMBER, &bus) < 0) {
      qDebug( "%s: Need a filename that resolves to a SCSI device.",
	      fname);
      close(fd);
      return -9999;
    }
    if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, &m_idlun) < 0) {
      qDebug( "%s: Need a filename that resolves to a SCSI device (2).",
	      fname);
      close(fd);
      return -9999;
    }
    close(fd);

    for (k = 0; k < MAX_SG_DEVS; k++) {
      fname = makeDevName(k, do_numeric);
      if ((fd = open(fname, O_RDONLY | O_NONBLOCK)) < 0) {
	if (EACCES == errno)
	  fd = open(fname, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
	  if ((ENOENT == errno) && (0 == k) && (do_numeric == DEF_SCAN)) {
	    do_numeric = ! DEF_SCAN;
	    fname = makeDevName(k, do_numeric);
	    if ((fd = open(fname, O_RDONLY | O_NONBLOCK)) < 0) {
	      if (EACCES == errno)
		fd = open(fname, O_RDWR | O_NONBLOCK);
	    }
	  }
	  if (fd < 0) {
	    if (EBUSY == errno)
	      continue;  /* step over if O_EXCL already on it */
	    else
	      break;
	  }
	}
      }
      if (ioctl(fd, SCSI_IOCTL_GET_BUS_NUMBER, &bbus) < 0) {
	qDebug( "%s: SG: ioctl SCSI_IOCTL_GET_BUS_NUMBER failed: %s",
		fname, strerror(errno));
	close(fd);
	fd = -9999;
      }
      if (ioctl(fd, SCSI_IOCTL_GET_IDLUN, &mm_idlun) < 0) {
	qDebug( "%s: SG: ioctl SCSI_IOCTL_GET_IDLUN failed: %s",
		fname, strerror(errno));
	close(fd);
	fd = -9999;
      }
      if ((bus == bbus) &&
	  ((m_idlun.mux4 & 0xff) == (mm_idlun.mux4 & 0xff)) &&
	  (((m_idlun.mux4 >> 8) & 0xff) ==
	   ((mm_idlun.mux4 >> 8) & 0xff)) &&
	  (((m_idlun.mux4 >> 16) & 0xff) ==
	   ((mm_idlun.mux4 >> 16) & 0xff))) {
	qDebug( "Mapping %s to sg device: %s", devname, fname);
	break;
      }
      else {
	close(fd);
	fd = -9999;
      }
    }
  }

  if (fd >= 0) { /* everything ok, close and re-open read-write */
    dev_ = strdupCC(fname);
    close(fd);
    return open(dev_, O_RDWR);
  }
  else {
    return fd;
  }
}

int ScsiIfImpl::adjustReservedBuffer(int requestedSize)
{
  int maxTransferLength;

  switch (driverVersion_) {
  case SG_RT_NEW32: /* SG_SET_RESERVED_SIZE exists but does nothing in */
                    /* version 2.1.32 and 2.1.33, so harmless to try */
  case SG_RT_NEW34:
    if (ioctl(fd_, SG_SET_RESERVED_SIZE, &requestedSize) < 0) {
      qDebug( "SG_SET_RESERVED_SIZE ioctl failed: %s", strerror(errno));
      return 0;
    }
    if (ioctl(fd_, SG_GET_RESERVED_SIZE, &maxTransferLength) < 0) {
      qDebug( "SG_GET_RESERVED_SIZE ioctl failed: %s", strerror(errno));
      return 0;
    }
    break;

  default:
#ifdef SG_BIG_BUFF
    maxTransferLength = SG_BIG_BUFF;
#else
    maxTransferLength = 4096;
#endif
    break;
  }

  qDebug( "SG: Maximum transfer length: %ld", maxTransferLength);

  return maxTransferLength;
}

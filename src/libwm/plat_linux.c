/*
 * $Id$
 *
 * This file is part of WorkMan, the civilized CD player library
 * (c) 1991-1997 by Steven Grimm (original author)
 * (c) by Dirk Försterling (current 'author' = maintainer)
 * The maintainer can be contacted by his e-mail address:
 * milliByte@DeathsDoor.com 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Linux-specific drive control routines.  Very similar to the Sun module.
 */

static char plat_linux_id[] = "$Id$";

#if defined(__linux__)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdtext.h"

#if defined(BSD_MOUNTTEST)
  #include <mntent.h>
#else
  /*
   * this is for glibc 2.x which defines ust structure in
   * ustat.h not stat.h
   */
  #ifdef __GLIBC__
    #include <sys/ustat.h>
  #endif
#endif

#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include "include/wm_cdda.h"
#include "include/wm_struct.h"
#include "include/wm_platform.h"
#include "include/wm_cdrom.h"
#include "include/wm_scsi.h"
#include "include/wm_helpers.h"

#ifdef OSS_SUPPORT
#include <linux/soundcard.h>
#define CD_CHANNEL SOUND_MIXER_CD
#endif

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

#define max(a,b) ((a) > (b) ? (a) : (b))
 
#ifdef LINUX_SCSI_PASSTHROUGH
/* this is from <scsi/scsi_ioctl.h> */
# define SCSI_IOCTL_SEND_COMMAND 1
#endif

int	min_volume = 0;
int	max_volume = 255;

#ifdef OSS_SUPPORT
int mixer;
char mixer_dev_name[20] = "/dev/mixer";
#endif
extern char	*cd_device, *cddaslave_path;

int	cdda_slave = -1;

/*-------------------------------------------------------*
 *
 *
 *                   CD-ROM drive functions.
 *
 *
 *-------------------------------------------------------*/

/*--------------------------------------------------------*
 * Initialize the drive.  A no-op for the generic driver.
 *--------------------------------------------------------*/
int
gen_init( struct wm_drive *d )
{
  return (0);
} /* gen_init() */

/*---------------------------------------------------------------------------*
 * Open the CD device and figure out what kind of drive is attached.
 *---------------------------------------------------------------------------*/
int
wmcd_open( struct wm_drive *d )
{
  int		fd;
  static int	warned = 0;
  int		retval = 0;
  char vendor[32], model[32], rev[32];
  
  if (cd_device == NULL)
    cd_device = DEFAULT_CD_DEVICE;
    
  
  if (d->fd >= 0)		/* Device already open? */
    {
/*      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): [device is open (fd=%d)]\n", d->fd);*/
      return (0);
    }
  
  fd = open(cd_device, O_RDONLY | O_NONBLOCK);

  if (fd < 0)
    {
      if (errno == EACCES)
	{
          return -EACCES;
	}
      /* Hack proposed by Carey Evans, introduced by Debian maintainer :
       * treat EIO like ENXIO since some Linux drives do never return ENXIO
       * ENOMEDIUM is returned by Kernel 2.2.x unified drivers.
       */
      else if ((errno != ENXIO) && (errno != EIO) && (errno != ENOMEDIUM))
	{
          return (-6);
	}
      
      /* No CD in drive. */
      retval = 1;
    }
  
  
#ifdef LINUX_SCSI_PASSTHROUGH
  /* Can we figure out the drive type? */
  retval = wm_scsi_get_drive_type(d, vendor, model, rev);
  if (retval == WM_ERR_SCSI_INQUIRY_FAILED)
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): After failed inquiry\n");
#endif

  *d = *(find_drive_struct(vendor, model, rev));
  wm_drive_settype(vendor, model, rev);
  
  d->fd = fd;
  (d->init)(d);
  return retval;
} /* wmcd_open() */

/*
 * Re-Open the device if it is open.
 */
int
wmcd_reopen( struct wm_drive *d )
{
  int status;
  int tries = 0;
  
  do {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_reopen\n");
    if (d->fd >= 0)		/* Device really open? */
      {
	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "closing the device\n");
	if(!close( d->fd ))   /* close it! */
	  d->fd = -1;      /* closed */
      }
    wm_susleep( 1000 );
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "calls wmcd_open()\n");
    status = wmcd_open( d ); /* open it as usual */
    wm_susleep( 1000 );
    tries++;
  } while ( (status != 0) && (tries < 10)  );
  return status;
} /* wmcd_reopen() */

/*---------------------------------------------*
 * Send an arbitrary SCSI command to a device.
 *---------------------------------------------*/
int
wm_scsi( struct wm_drive *d, unsigned char *cdb, int cdblen,
	 void *retbuf, int retbuflen, int getreply )
{
#ifdef LINUX_SCSI_PASSTHROUGH
  
  char *cmd;
  int cmdsize;

  cmdsize = 2 * sizeof(int);
  if (retbuf)
    {
      if (getreply) cmdsize += max(cdblen, retbuflen);
      else cmdsize += (cdblen + retbuflen);
    }
  else cmdsize += cdblen;
  
  cmd = malloc(cmdsize);
  if (cmd == NULL)
    return (-1);
  
  ((int*)cmd)[0] = cdblen + ((retbuf && !getreply) ? retbuflen : 0);
  ((int*)cmd)[1] = ((retbuf && getreply) ? retbuflen : 0);
  
  memcpy(cmd + 2*sizeof(int), cdb, cdblen);
  if (retbuf && !getreply)
    memcpy(cmd + 2*sizeof(int) + cdblen, retbuf, retbuflen);
  
  if (ioctl(d->fd, SCSI_IOCTL_SEND_COMMAND, cmd))
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "%s: ioctl() failure\n", __FILE__);
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "command buffer is:\n");
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "%02x %02x %02x %02x %02x %02x\n",
		     cmd[0],  cmd[1],  cmd[2],  cmd[3],  cmd[4],  cmd[5]);
      free(cmd);
      return (-1);
    }
  
  if (retbuf && getreply)
    memcpy(retbuf, cmd + 2*sizeof(int), retbuflen);
  
  free(cmd);
  return 0;
  
#else /* Linux SCSI passthrough*/
/*----------------------------------------*
 * send packet over cdrom interface
 * kernel >= 2.2.16
 *----------------------------------------*/
#ifdef CDROM_SEND_PACKET	

  struct cdrom_generic_command cdc;
  struct request_sense sense;
  int ret, capability;

  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wm_scsi over CDROM_SEND_PACKET entered\n");

  capability = ioctl(d->fd, CDROM_GET_CAPABILITY);

  if(!(capability & CDC_GENERIC_PACKET))
  {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "your CDROM or/and kernel don't support CDC_GENERIC_PACKET ...\n");
    printf("your CDROM or/and kernel don't support CDC_GENERIC_PACKET ...\n");
    return -1;
  }

  memset(&cdc, 0, sizeof(struct cdrom_generic_command));
  memset(&sense, 0, sizeof(struct request_sense));
	
  memcpy(cdc.cmd, cdb, cdblen);

  cdc.buffer = retbuf;
  cdc.buflen = retbuflen;
  cdc.stat = 0;
  cdc.sense = &sense;
  cdc.data_direction = getreply;

  /* sendpacket_over_cdrom_interface() */
  return ioctl(d->fd, CDROM_SEND_PACKET, &cdc);
#endif /* CDROM_SEND_PACKET */
  printf("ERROR: this binary was compiled without CDROM GENERIC PACKET SUPPORT. kernel version < 2.2.16?\n");
  printf("ERROR: if you have a SCSI CDROM, rebuild it with a #define LINUX_SCSI_PASSTHROUGH\n");
  return (-1);
#endif
} /* wm_scsi */

/*--------------------------------*
 * Keep the CD open all the time.
 * disabled, analogous to 1.4b3
 *--------------------------------*
void
keep_cd_open( void )
{
	int	fd;
	struct flock	fl;
	extern	end;


	for (fd = 0; fd < 256; fd++)
		close(fd);

	if (fork())
		exit(0);

	if ((fd = open("/tmp/cd.lock", O_RDWR | O_CREAT, 0666)) < 0)
		exit(0);
	fl.l_type = F_WRLCK;
	fl.l_whence = 0;
	fl.l_start = 0;
	fl.l_len = 0;
	if (fcntl(fd, F_SETLK, &fl) < 0)
		exit(0);

	if (open(cd_device, 0) >= 0)
	{
		brk(&end);
		pause();
	}

	exit(0);
}
*/

/*--------------------------------------------------------------------------*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 *--------------------------------------------------------------------------*/
int
gen_get_drive_status( struct wm_drive *d, enum wm_cd_modes oldmode,
                      enum wm_cd_modes *mode, int *pos, int *track, int *index )
{
  struct cdrom_subchnl		sc;
  
#ifdef SBPCD_HACK
  static int prevpos = 0;
#endif
  
  /* If we can't get status, the CD is ejected, so default to that. */
  *mode = WM_CDM_EJECTED;
  
  /* Is the device open? */
  if (d->fd < 0)
    {
      switch (wmcd_open(d))
	{
	case -1:	/* error */
	  return (-1);
	case 1:		/* retry */
	  return (0);
	}
    }
 

  /* Try to get rid of the door locking    */
  /* Don't care about return value. If it  */
  /* works - fine. If not - ...            */
  ioctl( d->fd, CDROM_LOCKDOOR, 0 ); 
 
  sc.cdsc_format = CDROM_MSF;
 
  if (ioctl(d->fd, CDROMSUBCHNL, &sc))
    return (0);
  
  switch (sc.cdsc_audiostatus) {
  case CDROM_AUDIO_PLAY:
    *mode = WM_CDM_PLAYING;
    *track = sc.cdsc_trk;
    *index = sc.cdsc_ind;
    *pos = sc.cdsc_absaddr.msf.minute * 60 * 75 +
      sc.cdsc_absaddr.msf.second * 75 +
      sc.cdsc_absaddr.msf.frame;
#ifdef SBPCD_HACK
    if( *pos < prevpos )
      {
	if( (prevpos - *pos) < 75 )
	  {
	    *mode = WM_CDM_TRACK_DONE;
	  }
      }
    
    prevpos = *pos;
#endif
    break;
    
  case CDROM_AUDIO_PAUSED:
  case CDROM_AUDIO_NO_STATUS:
  case CDROM_AUDIO_INVALID: /**/
    if (oldmode == WM_CDM_PLAYING || oldmode == WM_CDM_PAUSED)
      {
	*mode = WM_CDM_PAUSED;
	*track = sc.cdsc_trk;
	*index = sc.cdsc_ind;
	*pos = sc.cdsc_absaddr.msf.minute * 60 * 75 +
	  sc.cdsc_absaddr.msf.second * 75 +
	  sc.cdsc_absaddr.msf.frame;
      }
    else
      *mode = WM_CDM_STOPPED;
    break;
    
  case CDROM_AUDIO_COMPLETED:
    *mode = WM_CDM_TRACK_DONE; /* waiting for next track. */
    break;
    
  default:
    *mode = WM_CDM_UNKNOWN;
    break;
  }
  
  return (0);
} /* gen_get_drive_status */

/*-------------------------------------*
 * Get the number of tracks on the CD.
 *-------------------------------------*/
int
gen_get_trackcount(struct wm_drive *d, int *tracks)
{
  struct cdrom_tochdr	hdr;
  
  if (ioctl(d->fd, CDROMREADTOCHDR, &hdr))
    return (-1);
  
  *tracks = hdr.cdth_trk1;
  return (0);
} /* gen_get_trackcount() */

/*---------------------------------------------------------*
 * Get the start time and mode (data or audio) of a track.
 *---------------------------------------------------------*/
int
gen_get_trackinfo(struct wm_drive *d, int track, int *data, int *startframe)
{
  struct cdrom_tocentry	entry;
  
  entry.cdte_track = track;
  entry.cdte_format = CDROM_MSF;
  
  if (ioctl(d->fd, CDROMREADTOCENTRY, &entry))
    return (-1);
  
  *startframe =	entry.cdte_addr.msf.minute * 60 * 75 +
    entry.cdte_addr.msf.second * 75 +
    entry.cdte_addr.msf.frame;
  *data = entry.cdte_ctrl & CDROM_DATA_TRACK ? 1 : 0;
  
  return (0);
}

/*-------------------------------------*
 * Get the number of frames on the CD.
 *-------------------------------------*/
int
gen_get_cdlen(struct wm_drive *d, int *frames)
{
  int		tmp;

  return gen_get_trackinfo( d, CDROM_LEADOUT, &tmp, frames);
} /* gen_get_cdlen() */


/*------------------------------------------------------------*
 * Play the CD from one position to another (both in frames.)
 *------------------------------------------------------------*/
int
gen_play(struct wm_drive *d, int start, int end)
{
  struct cdrom_msf		msf;

  msf.cdmsf_min0 = start / (60*75);
  msf.cdmsf_sec0 = (start % (60*75)) / 75;
  msf.cdmsf_frame0 = start % 75;
  msf.cdmsf_min1 = end / (60*75);
  msf.cdmsf_sec1 = (end % (60*75)) / 75;
  msf.cdmsf_frame1 = end % 75;
  
  if (ioctl(d->fd, CDROMPLAYMSF, &msf))
    {
      if (ioctl(d->fd, CDROMSTART))
        return (-1);
      if (ioctl(d->fd, CDROMPLAYMSF, &msf))
        return (-2);
    }
  
  /*
   * I hope no drive gets really confused after CDROMSTART
   * If so, I need to make this run-time configurable.
   * 
#ifndef FAST_IDE
  if (ioctl( d->fd, CDROMSTART))
      return (-1);
#endif
  if (ioctl( d->fd, CDROMPLAYMSF, &msf ))
      return (-2);
    */  

  return (0);
} /* gen_play() */

/*---------------*
 * Pause the CD.
 *---------------*/
int
gen_pause(struct wm_drive *d)
{
  return (ioctl(d->fd, CDROMPAUSE));
}

/*-------------------------------------------------*
 * Resume playing the CD (assuming it was paused.)
 *-------------------------------------------------*/
int
gen_resume(struct wm_drive *d)
{
  return (ioctl(d->fd, CDROMRESUME));
}

/*--------------*
 * Stop the CD.
 *--------------*/
int
gen_stop(struct wm_drive *d)
{
  return (ioctl(d->fd, CDROMSTOP));
}


/*----------------------------------------*
 * Eject the current CD, if there is one.
 *----------------------------------------*/
int
gen_eject(struct wm_drive *d)
{
  struct stat	stbuf;
#if !defined(BSD_MOUNTTEST)
  struct ustat	ust;
#else
  struct mntent *mnt;
  FILE *fp;
#endif
  
  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "ejecting?\n");

  if (fstat(d->fd, &stbuf) != 0)
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "that weird fstat() thingy\n");
      return (-2);
    }
  
  /* Is this a mounted filesystem? */
#if !defined(BSD_MOUNTTEST)
  if (ustat(stbuf.st_rdev, &ust) == 0)
    return (-3);
#else
  /*
   * This is the same test as in the WorkBone interface.
   * I should eliminate it there, because there is no need
   * for it in the UI
   */ 
  /* check if drive is mounted (from Mark Buckaway's cdplayer code) */
  /* Changed it again (look at XPLAYCD from ????                    */
  /* It's better to check the device name rather than one device is */
  /* mounted as iso9660. That prevents "no playing" if you have more*/
  /* than one CD-ROM, and one of them is mounted, but it's not the  */
  /* audio CD                                              -dirk    */
  if ((fp = setmntent (MOUNTED, "r")) == NULL)
    {
      wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS, "Could not open %s: %s\n", MOUNTED, strerror (errno));
      return(-3);
    }
  while ((mnt = getmntent (fp)) != NULL)
    {
      if (strcmp (mnt->mnt_fsname, cd_device) == 0)
	{
	  wm_lib_message(WM_MSG_LEVEL_ERROR|WM_MSG_CLASS, "CDROM already mounted (according to mtab). Operation aborted.\n");
	  endmntent (fp);
	  return(-3);
	}
    }
  endmntent (fp);
#endif /* BSD_MOUNTTEST */
 
  ioctl( d->fd, CDROM_LOCKDOOR, 0 );

  if (ioctl(d->fd, CDROMEJECT))
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "eject failed (%s).\n", strerror(errno));
      return (-1);
    }
    
  /*------------------
   * Things in "foobar_one" are left over from 1.4b3
   * I put them here for further observation. In 1.4b3, however,
   * that workaround didn't help at least for /dev/sbpcd
   * (The tray closed just after ejecting because re-opening the
   * device causes the tray to close)
   *------------------*/
#ifdef foobar_one 
  extern int intermittent_dev
    /*
     * Some drives (drivers?) won't recognize a new CD if we leave the
     * device open.
     */
    if (intermittent_dev)
      {
	close(d->fd);
	d->fd = -1;
      }			  
#endif
  
  return (0);
} /* gen_eject() */

/*----------------------------------------*
 * Close the CD tray
 *----------------------------------------*/

int 
gen_closetray(struct wm_drive *d)
{
#ifdef CAN_CLOSE
#ifdef CDROMCLOSETRAY
  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "CDROMCLOSETRAY closing tray...\n");
  if (ioctl(d->fd, CDROMCLOSETRAY))
    return (-1);
#else
  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_reopen() closing tray...\n");
  if(!close(d->fd))
    {
      d->fd=-1;
      return(wmcd_reopen(d));
    } else {
      return(-1);
    }
#endif /* CDROMCLOSETRAY */
#else /* CAN_CLOSE */
  /* Always succeed if the drive can't close. */
  return(0);
#endif /* CAN_CLOSE */
} /* gen_closetray() */


/*------------------------------------------------------------------------*
 * scale_volume(vol, max)
 *
 * Return a volume value suitable for passing to the CD-ROM drive.  "vol"
 * is a volume slider setting; "max" is the slider's maximum value.
 * This is not used if sound card support is enabled.
 *
 *------------------------------------------------------------------------*/
static int
scale_volume( int vol, int max )
{
#ifdef CURVED_VOLUME
  return ((max * max - (max - vol) * (max - vol)) *
	  (max_volume - min_volume) / (max * max) + min_volume);                        
#else
  return ((vol * (max_volume - min_volume)) / max + min_volume);
#endif        
} /* scale_volume() */

/*---------------------------------------------------------------------*
 * Set the volume level for the left and right channels.  Their values
 * range from 0 to 100.
 *---------------------------------------------------------------------*/
int
gen_set_volume( struct wm_drive *d, int left, int right )
{
  struct	cdrom_volctrl v;
  
  /* Adjust the volume to make up for the CD-ROM drive's weirdness. */
  left = scale_volume(left, 100);
  right = scale_volume(right, 100);
  
  v.channel0 = v.channel2 = left < 0 ? 0 : left > 255 ? 255 : left;
  v.channel1 = v.channel3 = right < 0 ? 0 : right > 255 ? 255 : right;
  
  return (ioctl(d->fd, CDROMVOLCTRL, &v));
} /* gen_set_volume() */

/*------------------------------------------------------------------------*
 * gen_get_cdtext(drive, buffer, lenght)
 *
 * Return a buffer with cdtext-stream. buffer will be allocated and filled
 *
 * needs send packet interface -> for IDE, linux at 2.2.16
 * depends on scsi.c which depends on wm_scsi defined in here
 * (which also takes care of IDE drives)
 *------------------------------------------------------------------------*/

int
gen_get_cdtext(struct wm_drive *d, unsigned char **pp_buffer, int *p_buffer_lenght)
{
  return wm_scsi_get_cdtext(d, pp_buffer, p_buffer_lenght);
} /* gen_get_cdtext() */

/*---------------------------------------------------------------------*
 * Read the initial volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 *---------------------------------------------------------------------*/
int
gen_get_volume( struct wm_drive *d, int *left, int *right )
{
#if defined(BUILD_CDDA) && defined(WMCDDA_DONE) /* { */
  struct cdda_block	blk;
  
  if (cdda_slave > -1)
    {
      write(cdda_slave, "G", 1);
      get_ack(cdda_slave);
      read(cdda_slave, &blk, sizeof(blk));
      
      *left = *right = (blk.volume * 100 + 254) / 255;
      
      if (blk.balance < 110)
	{
	  *right = (((blk.volume * blk.balance + 127) / 128) *
		    100 + 254) / 255;
	} else if (blk.balance > 146) {
	  *left = (((blk.volume * (255 - blk.balance) +
		     127) / 128) * 100 + 254) / 255;
	}
      return (0);
    }
#else /* } */
  /* Suns, HPs, Linux, NEWS can't read the volume; oh well */
  *left = *right = -1;
#endif
  return (0);
} /* gen_get_volume() */

/*-------------------------------------------------------*
 *
 *
 *                    CDDA functions.
 *
 *
 *-------------------------------------------------------*/

#ifdef BUILD_CDDA /* { */

/*
 * Try to initialize the CDDA slave.  Returns 0 on error.
 */
int
cdda_init( struct wm_drive *d )
{
#if defined(WMCDDA_DONE) /* { */
  int	slavefds[2];
  
  if (cdda_slave > -1)
    return (1);
  
  fprintf( stderr, "slave okay\n" );
  
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, slavefds))
    {
      perror("socketpair");
      return (0);
    }
  
  fprintf( stderr, "going to fork\n" );
  switch (fork()) {
  case 0:
    close(slavefds[0]);
    dup2(slavefds[1], 1);
    dup2(slavefds[1], 0);
    close(slavefds[1]);
    close(d->fd);
    /* Try the default path first. */
    execl(cddaslave_path, cddaslave_path, cd_device, NULL);
    /* Search $PATH if that didn't work. */
    execlp("cddaslave", "cddaslave", cd_device, NULL);
    perror(cddaslave_path);
    exit(1);
    
  case -1:
    close(slavefds[0]);
    close(slavefds[1]);
    perror("fork");
    return (0);
  }
  
  close(slavefds[1]);
  cdda_slave = slavefds[0];
  
  if (!get_ack(cdda_slave))
    {
      fprintf( stderr, "get_ack failed\n" );
      cdda_slave = -1;
      /*		codec_start(); */
      return (0);
    }
  return (1);

#else /* BUILD_CDDA only } { */
  /*
   * If we're not building CDDA support, don't even bother trying.
   */
  return (0);
#endif
} /* cdda_init()  */

/*
 * Wait for an acknowledgement from the CDDA slave.
 */
static int
get_ack(int fd)
{
#if defined(WMCDDA_DONE) /* { */
  struct cdda_block	blk;
  
  do {
    if (read(fd, &blk, sizeof(blk)) <= 0)
      {
	return (0);
      }
  } while (blk.status != WMCDDA_ACK);
#endif /* } */
	return (1);
} /* get_ack() */

/*
 * Turn off the CDDA slave.
 */
void
cdda_kill( struct wm_drive *d )
{
  if (cdda_slave > -1)
    {
      write(cdda_slave, "Q", 1);
      get_ack(cdda_slave);
      wait(NULL);
      cdda_slave = -1;
      /*		codec_start(); */
    }
} /* cdda_kill() */


/*
 * Tell the CDDA slave to set the play direction.
 */
void
gen_set_direction( int newdir )
{
  unsigned char	buf[2];
  
  if (cdda_slave > -1)
    {
      buf[0] = 'd';
      buf[1] = newdir;
      write(cdda_slave, buf, 2);
      get_ack(cdda_slave);
    }
}

/*
 * Tell the CDDA slave to set the play speed.
 */
void
gen_set_speed( int speed )
{
  unsigned char	buf[2];
  
  if (cdda_slave > -1)
    {
      buf[0] = 's';
      buf[1] = speed;
      write(cdda_slave, buf, 2);
      get_ack(cdda_slave);
    }
} /* gen_set_speed() */

/*
 * Tell the CDDA slave to set the loudness level.
 */
void
gen_set_loudness( int loud )
{
  unsigned char	buf[2];
  
  if (cdda_slave > -1)
    {
      buf[0] = 'L';
      buf[1] = loud;
      write(cdda_slave, buf, 2);
      get_ack(cdda_slave);
    }
} /* gen_set_loudness() */

/*
 * Tell the CDDA slave to start (or stop) saving to a file.
 */
void
gen_save( char *filename )
{
  int	len;
  
  if (filename == NULL || filename[0] == '\0')
    len = 0;
  else
    len = strlen(filename);
  write(cdda_slave, "F", 1);
  write(cdda_slave, &len, sizeof(len));
  if (len)
    write(cdda_slave, filename, len);
  get_ack(cdda_slave);
} /* gen_save() */

#endif /* BUILD_CDDA } */

#endif /* __linux__ */

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
 * Sun-specific drive control routines.
 */

static char plat_sun_id[] = "$Id$";

#if defined(sun) || defined(__sun)

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "include/wm_config.h"
#include "include/wm_helpers.h"
#include "include/wm_cdrom.h"
#include "include/wm_cdtext.h"

#include <ustat.h>
#include <unistd.h>
#include <signal.h>
#ifdef solbourne
# include <mfg/dklabel.h>
# include <mfg/dkio.h>
# include <sys/unistd.h>
# include <dev/srvar.h>
#else /* A real Sun */
# ifdef SYSV
#  include <poll.h>
#  include <stdlib.h>
#  include <sys/cdio.h>
#  include <sys/socket.h>
#  include <sys/scsi/impl/uscsi.h>
#  include "include/wm_cdda.h"
# else
#  include <sys/buf.h>
#  include <sun/dkio.h>
#  include <scsi/targets/srdef.h>
#  include <scsi/impl/uscsi.h>
#  include <scsi/generic/commands.h>
# endif
#endif

#include "include/wm_struct.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

int	min_volume = 0;
int	max_volume = 255;

extern char	*cd_device, *cddaslave_path; 
extern int	intermittent_dev;

int	cdda_slave = -1;

int	current_end;

#if defined(SYSV) && defined(SIGTHAW)
#ifdef __GNUC__
void sigthawinit(void) __attribute__ ((constructor));
#else
#pragma init(sigthawinit)
#endif /* GNUC */

static int last_left, last_right;
static struct wm_drive *thecd;

/*
 * Handling for Sun's Suspend functionality
 */
static void 
thawme(int sig)
{
// Just leave this line in as a reminder for a missing
// functionality in the GUI.
// change_mode(NULL, WM_CDM_STOPPED, NULL);
  codec_init();
  if( thecd )
    gen_set_volume(thecd, last_left, last_right);
} /* thawme() */

void 
sigthawinit( void )
{
  struct sigaction sa;
  
  sa.sa_handler = thawme;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  
  sigaction(SIGTHAW, &sa, NULL);
} /* sigthawinit() */

#endif /* SYSV && SIGTHAW */

/*
 * find_cdrom
 *
 * Determine the name of the CD-ROM device.
 *
 * Use the first of /vol/dev/aliases/cdrom0, /dev/rdsk/c0t6d0s2, and /dev/rsr0
 * that exists.  (Check for /vol/dev/aliases, not cdrom0, since it won't be
 * there if there's no CD in the drive.)  This is done so a single SunOS 4.x
 * binary can be used on any 4.x or higher Sun system.
 */
int
find_cdrom( void )
{
  if (access("/vol/dev/aliases", X_OK) == 0)
    {
      /* Volume manager.  Device might not be there. */
      intermittent_dev = 1;
      
      /* If vold is running us, it'll tell us the device name. */
      cd_device = getenv("VOLUME_DEVICE");
      /*
      ** the path of the device has to include /dev
      ** otherwise we are vulnerable to race conditions
      ** Thomas Biege <thomas@suse.de>
      */
      if (cd_device == NULL || 
	  strncmp("/vol/dev/", cd_device, 9) || 
	  strstr(cd_device, "/../") )
	cd_device = "/vol/dev/aliases/cdrom0";
    }
  else if (access("/dev/rdsk/c0t6d0s2", F_OK) == 0)
    {
      /* Solaris 2.x w/o volume manager. */
      cd_device = "/dev/rdsk/c0t6d0s2";
    }
  else if (access("/dev/rcd0", F_OK) == 0)
    {
      cd_device = "/dev/rcd0";
    }
  else if (access("/dev/rsr0", F_OK) == 0)
    cd_device = "/dev/rsr0";
  else
    {
      fprintf(stderr, "Couldn't find a CD device!\n");
      return 0;
    }
  return 1;
} /* find_cdrom() */

/*
 * Initialize the drive.  A no-op for the generic driver.
 */
int
gen_init( struct wm_drive *d )
{
  codec_init();
  return (0);
} /* gen_init() */


/*
 * Open the CD device and figure out what kind of drive is attached.
 */
int
wmcd_open( struct wm_drive *d )
{
  int		fd;
  static int	warned = 0;
  char	vendor[32] = WM_STR_GENVENDOR;
  char	 model[32] = WM_STR_GENMODEL;
  char	   rev[32] = WM_STR_GENREV;
  
  if (cd_device == NULL)
    find_cdrom();
  
  if (d->fd >= 0)		/* Device already open? */
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): [device is open (fd=%d)]\n", d->fd);
      return (0);
    }

  d->fd = open(cd_device, 0);
  if (d->fd < 0)
    {
      /* Solaris 2.2 volume manager moves links around */
      if (errno == ENOENT && intermittent_dev)
	return (1);
      
      if (errno == EACCES)
	{
	  if (!warned)
	    {
              /*
	      char	realname[MAXPATHLEN];
	      
	      if (realpath(cd_device, realname) == NULL)
		{
		  perror("realpath");
		  return (1);
		}
                */
              return -EACCES;
            }
	}
      else if (errno != ENXIO)
	{
          return( -6 );
	}
      
      /* No CD in drive. */
      return (1);
    }
  
  /* Now fill in the relevant parts of the wm_drive structure. */
  fd = d->fd;
  
  /*
   * See if we can do digital audio.
   */
#if defined(BUILD_CDDA) && defined(WMCDDA_DONE)
  if (cdda_init(d))
    /* WARNING: Old GUI call. How could this survive? */
    enable_cdda_controls(1);
#endif
  
  /* Can we figure out the drive type? */
  if (wm_scsi_get_drive_type(d, vendor, model, rev)) 
    {
      if (errno == EPERM) 
	{
	  /*
	   * Solaris 2.4 seems to refuse to do USCSICMD ioctls
	   * when not running as root.  SunOS 4.x allows it
	   * as an unprivileged user, though.
	   */
	  fprintf(stderr, "Warning: WorkMan can't adapt itself to your drive unless it runs as root.\n");
	} else {
	  fprintf(stderr, "Warning: WorkMan couldn't determine drive type\n");
	}
      *d = *(find_drive_struct("Generic", "drive type", ""));
    } else {
      *d = *(find_drive_struct(vendor, model, rev));
    }
  
  wm_drive_settype(vendor, model, rev);
  d->fd = fd;
  
  (d->init)(d);
  
  return (0);
} /* wmcd_open() */

/*
 * Re-Open the device if it is open.
 */
int
wmcd_reopen( struct wm_drive *d )
{
  int status;
  
  do {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_reopen\n");
    if (d->fd >= 0)		/* Device really open? */
      {
	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "closing the device\n");
	status = close( d->fd );   /* close it! */
	/* we know, that the file is closed, do we? */
	d->fd = -1;
      }
    wm_susleep( 1000 );
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "calling wmcd_open()\n");
    status = wmcd_open( d ); /* open it as usual */
    wm_susleep( 1000 );
  } while ( status != 0 );
  return status;
} /* wmcd_reopen() */


#ifndef solbourne
/*
 * Send an arbitrary SCSI command out the bus and optionally wait for
 * a reply if "retbuf" isn't NULL.
 */
int
wm_scsi( struct wm_drive *d,
	 unsigned char *cdb,
	 int cdblen, void *retbuf,
	 int retbuflen, int getreply )
{
  char			x;
  struct uscsi_cmd	cmd;
  
  memset(&cmd, 0, sizeof(cmd));
  cmd.uscsi_cdb = (void *) cdb;
  cmd.uscsi_cdblen = cdblen;
  cmd.uscsi_bufaddr = retbuf ? retbuf : (void *)&x;
  cmd.uscsi_buflen = retbuf ? retbuflen : 0;
  cmd.uscsi_flags = USCSI_ISOLATE | USCSI_SILENT;
  if (getreply)
    cmd.uscsi_flags |= USCSI_READ;
  
  if (ioctl(d->fd, USCSICMD, &cmd))
    return (-1);
  
  if (cmd.uscsi_status)
    return (-1);
  
  return (0);
}
#else
int wm_scsi() { return (-1); }
#endif

/* Alarm signal handler. */
static void do_nothing( int x ) { x++; }

/*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 */
int
gen_get_drive_status( struct wm_drive *d,
		      enum wm_cd_modes oldmode,
		      enum wm_cd_modes *mode,
		      int *pos, int *track, int *index )
{
  struct cdrom_subchnl		sc;
  struct itimerval		old_timer, new_timer;
  struct sigaction		old_sig, new_sig;
  
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
  
#if defined(BUILD_CDDA) && defined(WMCDDA_DONE) /* { */
  if ((oldmode == WM_CDM_PAUSED || oldmode == WM_CDM_PLAYING 
       || oldmode == WM_CDM_STOPPED) && cdda_slave > -1)
    {
      struct cdda_block	blk;
      struct pollfd		fds;
      int			gotone = 0;
      
      fds.fd = cdda_slave;
      fds.events = POLLRDNORM;
      
      *mode = oldmode;
      
      while (poll(&fds, 1, 0) > 0)
	{
	  read(cdda_slave, &blk, sizeof(blk));
	  gotone = 1;
	}
      
      /* We only want to use the latest status report. */
      if (gotone)
	{
	  if (blk.status == WMCDDA_PLAYED)
	    {
	      *track = blk.track;
	      *index = blk.index;
	      *pos = blk.minute * 60 * 75 +
		blk.second * 75 +
		blk.frame;
	      
	      *mode = WM_CDM_PLAYING;
	    }
	  else if (blk.status == WMCDDA_DONE)
	    *mode = WM_CDM_TRACK_DONE;
	  else if (blk.status == WMCDDA_STOPPED)
	    {
	      if (oldmode == WM_CDM_PLAYING || oldmode == WM_CDM_PAUSED)
		*mode = WM_CDM_PAUSED;
	      else
		*mode = WM_CDM_STOPPED;
	    }
	  else if (blk.status == WMCDDA_ERROR)
	    {
	      /*
	       * An error near the end of the CD probably
	       * just means we hit the end.
	       */
	      *mode = WM_CDM_TRACK_DONE;
	    }
	  else if (blk.status == WMCDDA_EJECTED)
	    {
	      *mode = WM_CDM_EJECTED;
	    }
	}
      
      return (0);
    }
#endif /* } */
  
  /*
   * Solaris 2.2 hangs on this ioctl if someone else ejects the CD.
   * So we schedule a signal to break out of the hang if the call
   * takes an unreasonable amount of time.  The signal handler and
   * timer are restored immediately to avoid interfering with XView.
   */
  if (intermittent_dev)
    {
      /*
       * First clear out the timer so XView's signal doesn't happen
       * while we're diddling with the signal handler.
       */
      timerclear(&new_timer.it_interval);
      timerclear(&new_timer.it_value);
      setitimer(ITIMER_REAL, &new_timer, &old_timer);
      
      /*
       * Now install the no-op signal handler.
       */
      new_sig.sa_handler = do_nothing;
      memset(&new_sig.sa_mask, 0, sizeof(new_sig.sa_mask));
      new_sig.sa_flags = 0;
      if (sigaction(SIGALRM, &new_sig, &old_sig))
	perror("sigaction");
      
      /*
       * And finally, set the timer.
       */
      new_timer.it_value.tv_sec = 2;
      setitimer(ITIMER_REAL, &new_timer, NULL);
    }
  
  sc.cdsc_format = CDROM_MSF;
  
  if (ioctl(d->fd, CDROMSUBCHNL, &sc))
    {
      if (intermittent_dev)
	{
	  sigaction(SIGALRM, &old_sig, NULL);
	  setitimer(ITIMER_REAL, &old_timer, NULL);
	  
	  /* If the device can disappear, let it do so. */
	  close(d->fd);
	  d->fd = -1;
	}
      
      return (0);
    }
  
  if (intermittent_dev)
    {
      sigaction(SIGALRM, &old_sig, NULL);
      setitimer(ITIMER_REAL, &old_timer, NULL);
    }
  
  switch (sc.cdsc_audiostatus) {
  case CDROM_AUDIO_PLAY:
    *mode = WM_CDM_PLAYING;
    *track = sc.cdsc_trk;
    *index = sc.cdsc_ind;
    *pos = sc.cdsc_absaddr.msf.minute * 60 * 75 +
      sc.cdsc_absaddr.msf.second * 75 +
      sc.cdsc_absaddr.msf.frame;
    break;
    
  case CDROM_AUDIO_PAUSED:
  case CDROM_AUDIO_INVALID:
  case CDROM_AUDIO_NO_STATUS:
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
    
    /* CD ejected manually during play. */
  case CDROM_AUDIO_ERROR:
    break;
    
  case CDROM_AUDIO_COMPLETED:
    *mode = WM_CDM_TRACK_DONE; /* waiting for next track. */
    break;
    
  default: 
    *mode = WM_CDM_UNKNOWN;
    break;
  }
  
  return (0);
} /* gen_get_drive_status() */

/*
 * Get the number of tracks on the CD.
 */
int
gen_get_trackcount( struct wm_drive *d, int *tracks )
{
  struct cdrom_tochdr	hdr;
  
  if (ioctl(d->fd, CDROMREADTOCHDR, &hdr))
    return (-1);
  
  *tracks = hdr.cdth_trk1;
  return (0);
} /* gen_get_trackcount() */

/*
 * Get the start time and mode (data or audio) of a track.
 */
int
gen_get_trackinfo( struct wm_drive *d, int track, int *data, int *startframe)
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
} /* gen_get_trackinfo() */

/*
 * Get the number of frames on the CD.
 */
int
gen_get_cdlen(struct wm_drive *d, int *frames )
{
  int		tmp;
  
  return (gen_get_trackinfo(d, CDROM_LEADOUT, &tmp, frames));
} /* gen_get_cdlen() */

/*
 * Play the CD from one position to another.
 *
 *	d		Drive structure.
 *	start		Frame to start playing at.
 *	end		End of this chunk.
 *	realstart	Beginning of this chunk (<= start)
 */
int
gen_play( struct wm_drive *d, int start, int end, int realstart)
{
  struct cdrom_msf		msf;
  unsigned char			cmdbuf[10];
  
  current_end = end;
  

  if (cdda_slave > -1)
    {
      cmdbuf[0] = 'P';
      cmdbuf[1] = start / (60 * 75);
      cmdbuf[2] = (start % (60*75)) / 75;
      cmdbuf[3] = start % 75;
      cmdbuf[4] = end / (60*75);
      cmdbuf[5] = (end % (60*75)) / 75;
      cmdbuf[6] = end % 75;
      cmdbuf[7] = realstart / (60 * 75);
      cmdbuf[8] = (realstart % (60*75)) / 75;
      cmdbuf[9] = realstart % 75;
      
      /* Write the play command and make sure the slave has it. */
      write(cdda_slave, cmdbuf, 10);
      get_ack(cdda_slave);
      
      return (0);
    }
  
  msf.cdmsf_min0 = start / (60*75);
  msf.cdmsf_sec0 = (start % (60*75)) / 75;
  msf.cdmsf_frame0 = start % 75;
  msf.cdmsf_min1 = end / (60*75);
  msf.cdmsf_sec1 = (end % (60*75)) / 75;
  msf.cdmsf_frame1 = end % 75;
  
  codec_start();
  if (ioctl(d->fd, CDROMSTART))
    return (-1);
  if (ioctl(d->fd, CDROMPLAYMSF, &msf))
    return (-2);
  
  return (0);
} /* gen_play() */

/*
 * Pause the CD.
 */
int
gen_pause( struct wm_drive *d )
{
  if (cdda_slave > -1)
    {
      int	dummy, mode = WM_CDM_PLAYING;
      
      write(cdda_slave, "S", 1);
      get_ack(cdda_slave);
      /*
      while (mode != WM_CDM_PAUSED)
	gen_get_drive_status(d, WM_CDM_PAUSED, &mode, &dummy, &dummy,
			     &dummy);
      */
      return (0);
    }
  
  codec_stop();
  return (ioctl(d->fd, CDROMPAUSE));
} /* gen_pause() */

/*
 * Resume playing the CD (assuming it was paused.)
 */
int
gen_resume( struct wm_drive *d )
{
  if (cdda_slave > -1)
    return (1);
  
  codec_start();
  return (ioctl(d->fd, CDROMRESUME));
} /* gen_resume() */

/*
 * Stop the CD.
 */
int
gen_stop( struct wm_drive *d )
{
  if (cdda_slave > -1)
    {
      write(cdda_slave, "S", 1);
      get_ack(cdda_slave);
      
      /*
       * The WMCDDA_STOPPED status message will be caught by
       * gen_get_drive_status.
       */
      
      return (0);
    }
  codec_stop();
  return (ioctl(d->fd, CDROMSTOP));
} /* gen_stop() */

/*
 * Eject the current CD, if there is one.
 */
int
gen_eject( struct wm_drive *d )
{
  struct stat	stbuf;
  struct ustat	ust;
  
  if (fstat(d->fd, &stbuf) != 0)
    return (-2);
  
  /* Is this a mounted filesystem? */
  if (ustat(stbuf.st_rdev, &ust) == 0)
    return (-3);
  
  if (cdda_slave > -1)
    {
      write(cdda_slave, "S", 1);
      get_ack(cdda_slave);
    }
  
  if (ioctl(d->fd, CDROMEJECT))
    return (-1);
  
  /* Close the device if it needs to vanish. */
  if (intermittent_dev)
    {
      close(d->fd);
      d->fd = -1;
      /* Also remember to tell the cddaslave since volume
	 manager switches links around on us */
      if (cdda_slave > -1)
	{
	  write(cdda_slave, "E", 1);
	  get_ack(cdda_slave);
	}
    }
  
  return (0);
} /* gen_eject() */

/*----------------------------------------*
 * Close the CD tray
 *
 * Please edit and send changes to
 * milliByte@DeathsDoor.com
 *----------------------------------------*/

int 
gen_closetray(struct wm_drive *d)
{
#ifdef CAN_CLOSE
  if(!close(d->fd))
    {
      d->fd=-1;
      return(wmcd_reopen(d));
    } else {
      return(-1);
    }
#else
  /* Always succeed if the drive can't close */
  return(0);
#endif /* CAN_CLOSE */
} /* gen_closetray() */

/*
 * Set the volume level for the left and right channels.  Their values
 * range from 0 to 100.
 */
int
gen_set_volume( struct wm_drive *d, int left, int right )
{
  struct cdrom_volctrl v;
  
#if defined(SIGTHAW) && defined(SYSV)
  last_left = left;
  last_right = right;
  thecd = d;
#endif
  
  if (cdda_slave > -1)
    {
      int		bal, vol;
      unsigned char	cmd[2];
      
      bal = (right - left) + 100;
      bal *= 255;
      bal /= 200;
      if (right > left)
	vol = right;
      else
	vol = left;
      vol *= 255;
      vol /= 100;
      
      cmd[0] = 'B';
      cmd[1] = bal;
      write(cdda_slave, cmd, 2);
      cmd[0] = 'V';
      cmd[1] = vol;
      write(cdda_slave, cmd, 2);
      /*
       * Don't wait for the ack, or the user won't be able to drag
       * the volume slider smoothly.
       */
      
      return (0);
    }
  
  left = (left * (max_volume - min_volume)) / 100 + min_volume;
  right = (right * (max_volume - min_volume)) / 100 + min_volume;
  
  v.channel0 = left < 0 ? 0 : left > 255 ? 255 : left;
  v.channel1 = right < 0 ? 0 : right > 255 ? 255 : right;
  
  return (ioctl(d->fd, CDROMVOLCTRL, &v));
} /* gen_set_volume() */

/*
 * Read the initial volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 */
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
	*right = (((blk.volume * blk.balance + 127) / 128) *
		  100 + 254) / 255;
      else if (blk.balance > 146)
	*left = (((blk.volume * (255 - blk.balance) +
		   127) / 128) * 100 + 254) / 255;
      
      return (0);
    }
#endif /* } */
  
  *left = *right = -1;
  
  return (wm_scsi2_get_volume(d, left, right));
} /* gen_get_volume() */

/* Trying to patch things up. Yuck. */
/* #ifdef BUILD_CDDA */

/*
 * Try to initialize the CDDA slave.  Returns 0 on error.
 */
int
cdda_init( struct wm_drive *d )
{
#if defined(BUILD_CDDA) && defined(WMCDDA_DONE) /* { */
  int	slavefds[2];
  
  if (cdda_slave > -1)
    return (1);
  
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, slavefds))
    {
      perror("socketpair");
      return (0);
    }
  
  switch (fork()) 
    {
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
      cdda_slave = -1;
      codec_start();
      return (0);
    }
  
  return (1);
  
#else /* BUILD_CDDA only (WMCDDA_DONE } { */
  /*
   * If we're not building CDDA support, don't even bother trying.
   */
  return (0);
#endif
} /* cdda_init() */

/*
 * Wait for an acknowledgement from the CDDA slave.
 */
static int
get_ack(int fd)
{
#if defined(WMCDDA_DONE) /* { */
  struct cdda_block	blk;
  
  do
    if (read(fd, &blk, sizeof(blk)) <= 0)
      return (0);
  while (blk.status != WMCDDA_ACK);
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
      codec_start();
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
} /* gen_set_direction() */

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

/*
#endif
*/

/*
 * The following code activates the internal CD audio passthrough on
 * SPARCstation 5 systems (and possibly others.)
 *
 * Thanks to <stevep@ctc.ih.att.com>, Roger Oscarsson <roger@cs.umu.se>
 * and Steve McKinty <>
 *
 * Most CD drives have a headphone socket on the front, but it
 * is often more convenient to route the audio though the
 * built-in audio device. That way the user can leave their
 * headphones plugged-in to the base system, for use with
 * other audio stuff like ShowMeTV
 */

#ifdef CODEC /* { */
#ifdef SYSV /* { */

# include <sys/ioctl.h>
# include <sys/audioio.h>
# include <stdlib.h>

#else /* } { */

# include <sun/audioio.h>
# define AUDIO_DEV_SS5STYLE 5
typedef int audio_device_t;

#endif /* } */
#endif /* } */

/*
 * Don't do anything with /dev/audio if we can't set it to high quality.
 * Also, don't do anything real if it's not Solaris.
 */
#if !defined(AUDIO_ENCODING_LINEAR) || !defined(CODEC) || !defined(SYSV) /* { */
codec_init() { return 0; }
codec_start() { return 0; }
codec_stop() { return 0; }
#else

#ifndef AUDIO_INTERNAL_CD_IN
#define AUDIO_INTERNAL_CD_IN	0x4
#endif

static char* devname = 0;
static char* ctlname = 0;
static int ctl_fd = -1;
static int port = AUDIO_LINE_IN;
int internal_audio = 1;

codec_init( void )
{
  register int i;
  char* ctlname;
  audio_info_t foo;
  audio_device_t aud_dev;
  
  if (internal_audio == 0)
    {
      ctl_fd = -1;
      return(0);
    }
  
  if (!(devname = getenv("AUDIODEV"))) devname = "/dev/audio";
  ctlname = strcat(strcpy(malloc(strlen(devname) + 4), devname), "ctl");
  if ((ctl_fd = open(ctlname, O_WRONLY, 0)) < 0) 
    {
      perror(ctlname);
      return -1;
    }
  if (ioctl(ctl_fd, AUDIO_GETDEV, &aud_dev) < 0) 
    {
      close(ctl_fd);
      ctl_fd = -1;
      return -1;
    }
  /*
   * Instead of filtering the "OLD_SUN_AUDIO", try to find the new ones.
   * Not sure if this is all correct.
   */
#ifdef SYSV
  if (strcmp(aud_dev.name, "SUNW,CS4231") &&
      strcmp(aud_dev.name, "SUNW,sb16") &&
      strcmp(aud_dev.name, "SUNW,sbpro"))
#else
    if (aud_dev != AUDIO_DEV_SS5STYLE)
#endif
      {
	close(ctl_fd);
	ctl_fd = -1;
	return 0;					/* but it's okay */
      }
  
  /*
   * Does the chosen device have an internal CD port?
   * If so, use it. If not then try and use the
   * Line In port.
   */
  if (ioctl(ctl_fd, AUDIO_GETINFO, &foo) < 0)
    {
      perror("AUDIO_GETINFO");
      close(ctl_fd);
      ctl_fd = -1;
      return(-1);
    }
  if (foo.record.avail_ports & AUDIO_INTERNAL_CD_IN)
    port = AUDIO_INTERNAL_CD_IN;
  else
    port = AUDIO_LINE_IN;
  
  /*
   * now set it up to use it. See audio(7I)
   */
  
  AUDIO_INITINFO(&foo);
  foo.record.port = port;
  foo.record.balance = foo.play.balance = AUDIO_MID_BALANCE;
#ifdef BUILD_CDDA
  if (cdda_slave > -1)
    foo.monitor_gain = 0;
  else
#endif
    foo.monitor_gain = AUDIO_MAX_GAIN;
  /*
   * These next ones are tricky. The voulme will depend on the CD drive
   * volume (set by the knob on the drive and/or by workman's volume
   * control), the audio device record gain and the audio device
   * play gain.  For simplicity we set the latter two to something
   * reasonable, but we don't force them to be reset if the user
   * wants to change them.
   */
  foo.record.gain = (AUDIO_MAX_GAIN * 80) / 100;
  foo.play.gain = (AUDIO_MAX_GAIN * 40) / 100;
  
  ioctl(ctl_fd, AUDIO_SETINFO, &foo);
  return 0;
}

static int
kick_codec( void ) 
{
  audio_info_t foo;
  int dev_fd;
  int retval = 0;
  
  /*
   * Open the audio device, not the control device. This
   * will fail if someone else has taken it.
   */
  
  if ((dev_fd = open(devname, O_WRONLY|O_NDELAY, 0)) < 0) 
    {
      perror(devname);
      return -1;
    }
  
  AUDIO_INITINFO(&foo);
  foo.record.port = port;
  foo.monitor_gain = AUDIO_MAX_GAIN;
  
  /* These can only be set on the real device */
  foo.play.sample_rate = 44100;
  foo.play.channels = 2;
  foo.play.precision = 16;
  foo.play.encoding = AUDIO_ENCODING_LINEAR;
  
  if ((retval = ioctl(dev_fd, AUDIO_SETINFO, &foo)) < 0)
    perror(devname);
  
  close(dev_fd);
  return retval;
} /* kick_codec() */

codec_start( void )
{
  audio_info_t foo;
  
  if (ctl_fd < 0)
    return 0;
  
  if (ioctl(ctl_fd, AUDIO_GETINFO, &foo) < 0)
    return -1;
  
  if (foo.play.channels != 2) return kick_codec();
  if (foo.play.encoding != AUDIO_ENCODING_LINEAR) return kick_codec();
  if (foo.play.precision != 16) return kick_codec();
  if (foo.play.sample_rate != 44100) return kick_codec();
  
  if (foo.record.channels != 2) return kick_codec();
  if (foo.record.encoding != AUDIO_ENCODING_LINEAR) return kick_codec();
  if (foo.record.precision != 16) return kick_codec();
  if (foo.record.sample_rate != 44100) return kick_codec();
  
  if (foo.monitor_gain != AUDIO_MAX_GAIN) return kick_codec();
  if (foo.record.port != port) return kick_codec();
  
  return 0;
} /* codec_start() */

codec_stop( void ) { return 0; }

#endif /* CODEC } */

/*------------------------------------------------------------------------*
 * gen_get_cdtext(drive, buffer, lenght)
 *------------------------------------------------------------------------*/

int
gen_get_cdtext(struct wm_drive *d, unsigned char **pp_buffer, int *p_buffer_lenght)
{
  /* This needs to be tested */
  return wm_scsi_get_cdtext(d, pp_buffer, p_buffer_lenght);
} /* gen_get_cdtext() */

#endif /* sun } */

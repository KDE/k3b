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
 * Sony NEWS-specific drive control routines.
 */

static char plat_news_id[] = "$Id$";

#if defined( __sony_news) || defined(sony_news)

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <ustat.h>
#include <CD.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdtext.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

void *malloc();
char *strchr();

extern char	*cd_device;
extern int	intermittent_dev;

int	min_volume = 128;
int	max_volume = 255;

/*
 * Initialize the drive.  A no-op for the generic driver.
 */
int
gen_init( struct wm_drive *d )
{
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
  
  if (d->fd >= 0)		/* Device already open? */
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): [device is open (fd=%d)]\n", d->fd);
      return (0);
    }
  
  intermittent_dev = 1;
  if (cd_device == NULL)
    cd_device = DEFAULT_CD_DEVICE;
  
  if ((d->fd = CD_Open(cd_device, 0)) < 0)
    {
      /* Solaris 2.2 volume manager moves links around */
      if (errno == ENOENT && intermittent_dev)
	return (0);
      
      if (errno == EACCES)
	{
	  if (!warned)
	    {
              /*
	      char	realname[MAXPATHLEN];
	      
	      if (realpath(cd_device, realname) == NULL)
		{
		  perror("realpath");
		  return 1;
		}
                */
              return -EACCES;
	    }
	}
      else if (errno != EIO)	/* defined at top */
	{
          return (-6);
	}
      
      /* No CD in drive. */
      return (1);
    }
  
  /* Now fill in the relevant parts of the wm_drive structure. */
  fd = d->fd;
  
  /* Figure out the drive type, if possible */
  wm_scsi_get_drive_type(d, vendor, model, rev);
  *d = *(find_drive_struct(vendor, model, rev));
  wm_drive_settype(vendor, model, rev);
  
  d->fd = fd;
  
  (d->init)(d);
  
  return (0);
} /* wmcd_open() */

/*
 * Close the CD device.
 */
int
wmcd_close(int fd)
{
  int	ret;
  
  ret = CD_Close(fd);
  wm_susleep(3000000);
  return (ret);
} /* wmcd_close */

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
	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "closing the device.\n");
	status = wmcd_close( d->fd );   /* close it! */
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

/*
 * Pass SCSI commands to the device.
 */
int
wm_scsi(struct wm_drive *d, unsigned char *cdb, int cdblen, 
	unsigned char *buf, int buflen, int getreply)
{
	/* NEWS can't do SCSI passthrough... or can it? */
	return (-1);
} /* wm_scsi() */


/*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 */
int
gen_get_drive_status( struct wm_drive *d, enum wm_cd_modes oldmode, 
		      enum wm_cd_modes *mode, int *pos, int *track, int *index)
{
  struct CD_Status		sc;

  /* If we can't get status, the CD is ejected, so default to that. */
  *mode = WM_CDM_EJECTED;
  
  /* Is the device open? */
  if (d->fd < 0)
    {
      switch (wmcd_open(d)) {
      case -1:	/* error */
	return (-1);
	
      case 1:		/* retry */
	return (0);
      }
    }
  
  /* Disc is ejected.  Close the device. */
  if (CD_GetStatus(d->fd, &sc))
    {
      wmcd_close(d->fd);
      d->fd = -1;
      return (0);
    }
  
  switch (sc.status) {
  case CDSTAT_PLAY:
    *mode = PLAYING;
    *track = sc.tno;
    *index = sc.index;
    *pos = sc.baddr;
    break;
    
  case CDSTAT_PAUSE:
    if (oldmode == WM_CDM_PLAYING || oldmode == WM_CDM_PAUSED)
      {
	*mode = WM_CDM_PAUSED;
	*track = sc.tno;
	*index = sc.index;
	*pos = sc.baddr;
      }
    else
      *mode = WM_CDM_STOPPED;
    break;
    
  case CDSTAT_STOP:
    if (oldmode == WM_CDM_PLAYING)
      {
	*mode = WM_CDM_TRACK_DONE;	/* waiting for next track. */
	break;
      }
    /* fall through */
    
  default:
    *mode = WM_CDM_STOPPED;
    break;
  }
  
  return (0);
} /* gen_get_drive_status() */

/*
 * Get the number of tracks on the CD.
 */
int
gen_get_trackcount(struct wm_drive *d, int *tracks)
{
  struct CD_Capacity	cc;
  
  if (CD_GetCapacity(d->fd, &cc))
    return (-1);
  
  *tracks = cc.etrack - 1;
  return (0);
} /* gen_get_trackcount() */

/*
 * Get the start time and mode (data or audio) of a track.
 */
int
gen_get_trackinfo( struct wm_drive *d, int track, int *data, int *startframe )
{
  struct CD_TOCinfo	hdr;
  struct CD_TOCdata	ent;
  
  hdr.strack = track;
  hdr.ntrack = 1;
  hdr.data = &ent;
  if (CD_ReadTOC(d->fd, &hdr))
    return (-1);
  
  *data = (ent.control & 4) ? 1 : 0;
  *startframe = ent.baddr;
  
  return (0);
} /* gen_get_trackinfo */

/*
 * Get the number of frames on the CD.
 */
int
gen_get_cdlen( struct wm_drive *d, int *frames )
{
  int		tmp;
  
  if ((d->get_trackcount)(d, &tmp))
    return (-1);
  
  return (gen_get_trackinfo(d, tmp + 1, &tmp, frames));
} /* gen_get_cdlen() */


/*
 * Play the CD from one position to another (both in frames.)
 */
int
gen_play( struct wm_drive *d, int start, int end )
{
  struct CD_PlayAddr		msf;
  
  msf.addrmode			= CD_MSF;
  msf.addr.msf.startmsf.min	= start / (60*75);
  msf.addr.msf.startmsf.sec	= (start % (60*75)) / 75;
  msf.addr.msf.startmsf.frame	= start % 75;
  msf.addr.msf.endmsf.min		= end / (60*75);
  msf.addr.msf.endmsf.sec		= (end % (60*75)) / 75;
  msf.addr.msf.endmsf.frame	= end % 75;
  
  if (CD_Play(d->fd, &msf))
    {
      printf("wm_cd_play_chunk(%d,%d)\n",start,end);
      printf("msf = %d:%d:%d %d:%d:%d\n",
	     msf.addr.msf.startmsf.min,
	     msf.addr.msf.startmsf.sec,
	     msf.addr.msf.startmsf.frame,
	     msf.addr.msf.endmsf.min,
	     msf.addr.msf.endmsf.sec,
	     msf.addr.msf.endmsf.frame);
      perror("CD_Play");
      return (-1);
    }
  
  return (0);
} /* gen_play() */

/*
 * Pause the CD.
 */
int
gen_pause( struct wm_drive *d )
{
  CD_Pause(d->fd);
  return (0);
} /* gen_pause() */

/*
 * Resume playing the CD (assuming it was paused.)
 */
int
gen_resume( struct wm_drive *d )
{
	CD_Restart(d->fd);
	return (0);
} /* gen_resume() */

/*
 * Stop the CD.
 */
int
gen_stop( struct wm_drive *d )
{
	CD_Stop(d->fd);
	return (0);
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
  
  if (CD_AutoEject(d->fd))
    return (-1);
  
  /* Close the device if it needs to vanish. */
  if (intermittent_dev)
    {
      wmcd_close(d->fd);
      d->fd = -1;
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
  if(!wmcd_close(d->fd))
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
gen_set_volume( struct wm_drive *d, int left, int right)
{
  /* NEWS can't adjust volume! */
  return (0);
}

/*
 * Read the initial volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 */
int
gen_get_volume( struct wm_drive *d, omt *left, int *right)
{
  /* Suns, HPs, Linux, NEWS can't read the volume; oh well */
  *left = *right = -1;
  return (0);
} /* gen_get_volume() */

/*------------------------------------------------------------------------*
 * gen_get_cdtext(drive, buffer, lenght)
 *------------------------------------------------------------------------*/

int
gen_get_cdtext(struct wm_drive *d, unsigned char **pp_buffer, int *p_buffer_lenght)
{
  return -1; /* no SCSI, no CDTEXT */
} /* gen_get_cdtext() */

#endif

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
 * plat_aix - AIX 4.x IDE and SCSI support  16 Dec 1998
 *
 * AIX 4.x Port: Erik O'Shaughnessy 
 * Original AIX IDE Code: Cloyce Spradling (xmcd libdi_d/aixioc.c )
 *
 * Taken from the ascd distribution.
 *
 */


static char plat_aix_id[] = "$Id$";

#if defined(AIXV3) || defined(__AIXV3)

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/cdrom.h>
#include <sys/devinfo.h>
#include <sys/scsi.h>
#include <sys/scdisk.h>

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdtext.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

#define LEADOUT 0xaa

extern char *cd_device;

int min_volume = 128;
int max_volume = 255;



/* NAME: gen_init
 *
 * FUNCTION:
 *
 * RETURNS: 
 */
int 
gen_init(struct wm_drive *d)
{
  return 0;
} /* gen_init() */


/* NAME: wmcd_open
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
wmcd_open(struct wm_drive *d)
{
  char vendor[32] = WM_STR_GENVENDOR;
  char  model[32] = WM_STR_GENMODEL;
  char    rev[32] = WM_STR_GENREV;
  
  int fd;
  
  if( ! d )
    {
      errno = EFAULT;
      return -1;
    }

  if(d->fd > -1)			/* device already open? */
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): [device is open (fd=%d)]\n", d->fd);
      return 0;
    }

  if( cd_device == (char *)NULL )
    cd_device = DEFAULT_CD_DEVICE;

  if( (fd = openx(cd_device,O_RDONLY,NULL,SC_SINGLE)) < 0 )
    {
      perror("openx");
      return -6;
      /* return 1 */
    }
  
  *d = *(find_drive_struct(vendor, model, rev));
  wm_drive_settype(vendor, model, rev);
  
  d->fd = fd;
  d->init(d);
  return 0;
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

/* NAME: wm_scsi
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
wm_scsi(struct wm_drive *d,
	uchar_t *cdb, int cdblen,
	void *retbuf, int retbuflen,
	int getreply)
{
  return 0;
} /* wm_scsi() */


/* NAME: gen_get_drive_status
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_get_drive_status(struct wm_drive *d,
		     enum wm_cd_modes oldmode,
		     enum wm_cd_modes *mode,
		     int *pos,
		     int *track,
		     int *index)
{ 
  struct cd_audio_cmd cmd;
  
  *mode = WM_CDM_EJECTED;
  
  if(d->fd < 0)
    switch( wmcd_open(d) )
      {
      case -1:
	return -1;
      case 1:
	return 0;
      }
  
  cmd.audio_cmds = CD_INFO_AUDIO;

  if( ioctl(d->fd,DKAUDIO,&cmd) < 0)
    return -1;
  
  switch(cmd.status)
    {
    case CD_PLAY_AUDIO:
      *mode = WM_CDM_PLAYING;
      *track = cmd.indexing.info_audio.current_track;
      *index = cmd.indexing.info_audio.current_index;
      *pos = cmd.indexing.info_audio.current_mins * 60 * 75 +
	cmd.indexing.info_audio.current_secs * 75 +
	cmd.indexing.info_audio.current_frames;
      break;
      
    case CD_PAUSE_AUDIO:
      *mode = WM_CDM_PAUSED;
      *track = cmd.indexing.info_audio.current_track;
      *index = cmd.indexing.info_audio.current_index;
      *pos = cmd.indexing.info_audio.current_mins * 60 * 75 +
	cmd.indexing.info_audio.current_secs * 75 +
	cmd.indexing.info_audio.current_frames;
      
      break;
    case CD_NO_AUDIO:		/* no play audio in progress */
    case CD_COMPLETED:		/* play operation completed successfully */
    case CD_STATUS_ERROR:	/* invalid status or play stopped due to err */
    case CD_NOT_VALID:		/* audio status is invalid or not supported */
      *mode = WM_CDM_STOPPED;
      break;
    default:
      *mode = WM_CDM_UNKNOWN;
      break;
    }
  
  return 0;
} /* gen_get_drive_status() */



/* NAME: gen_get_trackcount
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_get_trackcount(struct wm_drive *d,int *tracks)
{
  struct cd_audio_cmd cmd;
  
  cmd.audio_cmds = CD_TRK_INFO_AUDIO;
  cmd.msf_flag = 0;
  
  if( ioctl(d->fd,DKAUDIO,&cmd) < 0)
    {
      perror("DKAUDIO");
      return -1;
    }

  *tracks = cmd.indexing.track_index.last_track;
  
  return 0;
} /* gen_get_trackcount() */

/* NAME: gen_get_trackinfo
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_get_trackinfo(struct wm_drive *d,int track,int *data,int *startframe)
{
  struct cd_audio_cmd cmd;
  
  cmd.audio_cmds = CD_GET_TRK_MSF;
  cmd.msf_flag = 1;
  
  cmd.indexing.track_msf.track = track;
  
  if( ioctl(d->fd,DKAUDIO,&cmd) < 0)
    return -1;
  
  *startframe = cmd.indexing.track_msf.mins * 60 * 75 +
    cmd.indexing.track_msf.secs * 75 +
    cmd.indexing.track_msf.frames;
  
  *data = 0;
  
  return 0;
} /* gen_get_trackinfo() */

/* NAME: gen_get_cdlen
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_get_cdlen(struct wm_drive *d,int *frames)
{
  int tmp;
  
  return gen_get_trackinfo(d,LEADOUT,&tmp,frames);
} /* gen_get_cdlen() */


/* NAME: gen_play
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_play(struct wm_drive *d,int start,int end)
{
  struct cd_audio_cmd cmd;
  
  cmd.audio_cmds = CD_PLAY_AUDIO;
  cmd.msf_flag = 1;
  
  cmd.indexing.msf.first_mins = start / (60*75);
  cmd.indexing.msf.first_secs = (start % (60*75)) / 75;
  cmd.indexing.msf.first_frames = start % 75;
  
  cmd.indexing.msf.last_mins = end / (60*75);
  cmd.indexing.msf.last_secs = (end % (60*75)) / 75;
  cmd.indexing.msf.last_frames = end % 75;
  
  if( ioctl(d->fd,DKAUDIO,&cmd) < 0)
    {
      perror("DKAUDIO:CD_PLAY_AUDIO");
      return -1;
    }
  return 0;
} /* gen_play() */

/* NAME: gen_pause
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_pause(struct wm_drive *d)
{
  struct cd_audio_cmd cmd;
  
  cmd.audio_cmds = CD_PAUSE_AUDIO;
  
  return ioctl(d->fd,DKAUDIO,&cmd);
} /* gen_pause() */


/* NAME: gen_resume
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_resume(struct wm_drive *d)
{
  struct cd_audio_cmd cmd;
  
  cmd.audio_cmds = CD_RESUME_AUDIO;
  return ioctl(d->fd,DKAUDIO,&cmd);
} /* gen_resume() */

/* NAME: gen_stop
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_stop(struct wm_drive *d)
{
  struct cd_audio_cmd cmd;
  
  cmd.audio_cmds = CD_STOP_AUDIO;
  return ioctl(d->fd,DKAUDIO,&cmd);
} /* gen_stop() */

/* NAME: gen_eject
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_eject(struct wm_drive *d)
{
  return ioctl(d->fd,DKEJECT,NULL);
}

/*----------------------------------------*
 * Close the CD tray
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


int 
scale_volume(int vol,int max)
{
  return ((vol * (max_volume - min_volume)) / max + min_volume);
}

int 
unscale_volume(int vol,int max)
{
  int n;
  n = ( vol - min_volume ) * max_volume / (max - min_volume);
  return (n <0)?0:n;
}

/* NAME: gen_set_volume
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_set_volume(struct wm_drive *d,int left,int right)
{
  struct cd_audio_cmd cmd;
  
  cmd.audio_cmds = CD_SET_VOLUME;
  cmd.volume_type = CD_VOLUME_CHNLS;
  
  cmd.out_port_0_vol = scale_volume(left,100);
  cmd.out_port_1_vol = scale_volume(right,100);
  
  if( ioctl(d->fd,DKAUDIO,&cmd) < 0)
    {
      perror("CD_SET_VOLUME");
      return -1;
    }
  
  return 0;
} /* gen_set_volume() */

/* NAME: gen_get_volume
 *
 * FUNCTION:
 *
 * RETURNS:
 */
int 
gen_get_volume(struct wm_drive *d,int *left,int *right)
{ 
  struct cd_audio_cmd cmd;
  int l,r;
  
  fprintf(stderr,"gen_get_volume\n");
  
  cmd.audio_cmds = CD_INFO_AUDIO;
  if( ioctl(d->fd,DKAUDIO,&cmd) < 0)
    return -1;
  
  *left = unscale_volume(cmd.out_port_0_vol,100);
  *right = unscale_volume(cmd.out_port_1_vol,100);
  
  return 0;
} /* gen_get_volume() */

/*------------------------------------------------------------------------*
 * gen_get_cdtext(drive, buffer, lenght)
 *------------------------------------------------------------------------*/

int
gen_get_cdtext(struct wm_drive *d, unsigned char **pp_buffer, int *p_buffer_lenght)
{
  return -1; /* no SCSI, no CDTEXT */
} /* gen_get_cdtext() */



#endif /* _AIX */






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
 * BSD/386-specific drive control routines.
 */

static char plat_bsd386_id[] = "$Id$";

#if defined(__bsdi__) || defined(__bsdi)

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "include/wm_config.h"

/*
 * The following is included from the Linux module. However, I didn't
 * see a check here if the CD to be ejected is mounted...
 */
#if defined(BSD_MOUNTTEST)
  #include <mntent.h>
#endif


#include <sys/time.h>
#include <string.h>
#include <cdrom.h>
#ifdef SOUNDBLASTER
# include <sys/soundcard.h>
#endif

#include "include/wm_struct.h"
#include "include/wm_helpers.h"
#include "include/wm_cdtext.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM


/*
 * Since we can't sense the drive type with libcdrom anyway, and since the
 * library doesn't provide "pause" or "resume" functions, use the daux field
 * to point to the frame number at which we paused.
 */
struct pause_info 
{
  int	frame;
  int	endframe;
};

#define	PAUSE_FRAME	(((struct pause_info *) d->daux)->frame)
#define	END_FRAME	(((struct pause_info *) d->daux)->endframe)
#define CUR_CD		((struct cdinfo *) d->aux)

void *malloc();

extern char	*cd_device;

#ifdef SOUNDBLASTER
       int     min_volume = 0;
       int     max_volume = 100;
       int     min_volume_drive = 10;  /* Toshiba drive does low values. */
       int     max_volume_drive = 255;
#else
       int     min_volume = 10;
       int     max_volume = 255;
#endif

/*--------------------------------------------------------*
 * Initialize the drive.  A no-op for the generic driver.
 *--------------------------------------------------------*/
int
gen_init(struct wm_drive *d)
{
  return (0);
} /* gen_init() */

/*-----------------------------------------------------------------------*
 * Open the CD device.  We can't determine the drive type under BSD/386.
 *-----------------------------------------------------------------------*/
int
wmcd_open(struct wm_drvie *d)
{
  void	*aux = NULL, *daux = NULL;
  int fd = -1;
  
  if (d->aux)	/* Device already open? */
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): [device is open (aux=%d)]\n", d->aux);
      return (0);
    }
  
  if ((aux = cdopen(cd_device)) == NULL)
    {
      fprintf(stderr, "No cdrom found by libcdrom\n");
      return (-6);
    }
  
  if ((daux = malloc(sizeof(struct pause_info))) == NULL)
    return (-1);
  
#ifdef SOUNDBLASTER
  fd = open("/dev/mixer", O_RDWR, 0);
#endif

  /* Now fill in the relevant parts of the wm_drive structure. */
  *d = *(find_drive_struct("", "", ""));
  d->aux = aux;
  d->daux = daux;
  d->fd = fd;
  PAUSE_FRAME = 0;
  END_FRAME = 0;
  
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
  int tries = 0;
  
  do {
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_reopen\n");
    if (d->aux == NULL)		/* Device really open? */
      {
	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "closing the device\n");
	status = close( d->fd );   /* close it! */
	 if (d->fd >= 0)
             close(d->fd); /* close mixer if open */
	 d->fd = -1;
	 status = cdclose( d->aux );   /* close the cdrom drive! */
	d->aux = NULL;
	free(d->daux);
	d->daux = NULL;
	/* we know, that the file is closed, do we? */
      }
    wm_susleep( 1000 );
    wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "calling wmcd_open()\n");
    status = wmcd_open( d ); /* open it as usual */
    wm_susleep( 1000 );
    tries++;
  } while ( (status != 0) && (tries < 10) );
  return status;
} /* wmcd_reopen() */


/*---------------------------------------------*
 * Send an arbitrary SCSI command to a device.
 *---------------------------------------------*/
int
wm_scsi(struct wm_drive *d, unsigned char *cdb, int cdblen,
        void *retbuf, int retbuflen, int getreply)
{
	/* Don't know how to do SCSI passthrough... */
	return (-1);
} /* wm_scsi() */

/*--------------------------------------------------------------------------*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 *--------------------------------------------------------------------------*/
#define DOPOS \
  *pos = status.abs_frame; \
*track = status.track_num; \
*index = status.index_num

int
gen_get_drive_status(struct wm_drive *d, enum wm_cd_modes oldmode,
                     enum wm_cd_modes *mode, int *pos, int *track, int *index)
{
  struct cdstatus	status;
  extern enum wm_cd_modes cur_cdmode;
  
  /* If we can't get status, the CD is ejected, so default to that. */
  *mode = WM_CDM_EJECTED;
  
  /* Is the device open? */
  if (d->aux == NULL)
    {
      switch (wmcd_open(d)) 
	{
	case -1:	/* error */
	  return (-1);
	  
	case 1:		/* retry */
	  return (0);
	}
    }
  
  if (cdstatus (CUR_CD, &status) < 0)
    {
      *mode = WM_CDM_TRACK_DONE;	/* waiting for next track. */
      return (0);
    }
  
  switch (status.state) 
    {
    case cdstate_playing:
      *mode = WM_CDM_PLAYING;
      DOPOS;
      break;
      
    case cdstate_stopped:
      /* the MITSUMI drive doesn't have a "paused" state,
	 so it always comes here and not to the paused section.
	 The PAUSE_FRAME stuff below (in gen_pause())
	 fakes out the paused state. */
      if (oldmode == WM_CDM_PLAYING) 
	{
	  *mode = WM_CDM_TRACK_DONE;
	  break;
	} else if (cur_cdmode != WM_CDM_PAUSED) {
	  *mode = WM_CDM_STOPPED;
	  DOPOS;
	  break;
	}
      /* fall through if paused */
      
    case cdstate_paused:
      /* the SCSI2 code in the cdrom library only pauses with
	 cdstop(); it never truly stops a disc (until an in-progress
	 play reaches the end).  So it always comes here. */
      if (cur_cdmode == WM_CDM_STOPPED) 
	{
	  *mode = WM_CDM_STOPPED;
	  DOPOS;
	  break;
	}
      if (oldmode == WM_CDM_PLAYING || oldmode == WM_CDM_PAUSED) 
	{
	  *mode = WM_CDM_PAUSED;
	  DOPOS;
	} else {
	  *mode = WM_CDM_STOPPED;
	  DOPOS;
	}
      break;
      
    default:
      *mode = WM_CDM_STOPPED;
    }
  
  return (0);
} /* gen_get_drive_status() */


/*-------------------------------------*
 * Get the number of tracks on the CD.
 *-------------------------------------*/
int
gen_get_trackcount(struct wm_drive *d, int *tracks)
{
  *tracks = CUR_CD->ntracks;
  
  return (0);
} /* gen_get_trackcount() */

/*---------------------------------------------------------*
 * Get the start time and mode (data or audio) of a track.
 *---------------------------------------------------------*/
int
gen_get_trackinfo(struct wm_drive *d, int track, int *data, int *startframe)
{
  *data = (CUR_CD->tracks[track - 1].control & 4) ? 1 : 0;
  *startframe = CUR_CD->tracks[track - 1].start_frame;
  
  return (0);
} /* gen_get_trackinfo() */

/*-------------------------------------*
 * Get the number of frames on the CD.
 *-------------------------------------*/
int
gen_get_cdlen(struct wm_drive *d, int *frames)
{
  *frames = CUR_CD->total_frames;
  
  return (0);
} /* gen_get_cdlen() */

/*------------------------------------------------------------*
 * Play the CD from one position to another (both in frames.)
 *------------------------------------------------------------*/
int
gen_play(struct wm_drive *d, int start, int end)
{
  END_FRAME = end;
  if (cdplay(d->aux, start, end) < 0)
    return (-1);
  else
    return (0);
} /* gen_play() */

/*--------------------------------------------------------------------*
 * Pause the CD.  This is a bit of a trick since there's no cdpause()
 * function in the library.  We fake it by saving the frame number
 * and stopping.
 *--------------------------------------------------------------------*/
int
gen_pause(struct wm_drive *d)
{
  struct cdstatus	status;
  
  if (cdstatus(d->aux, &status) < 0)
    return (-1);
  if (status.state != cdstate_playing)
    PAUSE_FRAME = CUR_CD->tracks[0].start_frame;
  else
    PAUSE_FRAME = status.abs_frame;
  if (cdstop(d->aux) < 0)
    return (-1);
  
  return (0);
} /* gen_pause() */

/*-------------------------------------------------*
 * Resume playing the CD (assuming it was paused.)
 *-------------------------------------------------*/
int
gen_resume(struct wm_drive *d)
{
  int	status;
  
  status = (d->play)(d, PAUSE_FRAME, END_FRAME);
  PAUSE_FRAME = 0;
  return (status);
} /* gen_resume() */

/*--------------*
 * Stop the CD.
 *--------------*/
int
gen_stop(struct wm_drive *d)
{
  return cdstop(d->aux);
} /* gen_stop() */

/*----------------------------------------*
 * Eject the current CD, if there is one.
 *----------------------------------------*/
int
gen_eject(struct wm_drive *d)
{
  cdeject(d->aux);
  cdclose(d->aux);
  d->aux = NULL;
  free(d->daux);
  d->daux = NULL;
  
  if (d->fd >= 0)
     close(d->fd);     /* close mixer */
  d->fd = -1;
  return (0);
} /* gen_eject() */

/*----------------------------------------*
 * Close the CD tray
 *----------------------------------------*/
int 
gen_closetray(struct wm_drive *d)
{
#ifdef CAN_CLOSE
  if (!cdload(d->aux))
      return(0);
   return(-1);
#else
  /* Always succeed if the drive can't close */
  return(0);
#endif /* CAN_CLOSE */
} /* gen_closetray() */


/*------------------------------------------------------------------------*
 * Return a volume value suitable for passing to the CD-ROM drive.  "vol"
 * is a volume slider setting; "max" is the slider's maximum value.
 *------------------------------------------------------------------------*/
static int
scale_volume(int vol, int max)
{
  /* on Toshiba XM-3401B drive, and on soundblaster, this works fine. */
  return ((vol * (max_volume - min_volume)) / max + min_volume);
} /* scale_volume() */

/*---------------------------------------------------------------------------*
 * unscale_volume(cd_vol, max)
 *
 * Given a value between min_volume and max_volume, return the volume slider
 * value needed to achieve that value.
 *
 * Rather than perform floating-point calculations to reverse the above
 * formula, we simply do a binary search of scale_volume()'s return values.
 *--------------------------------------------------------------------------*/
static int
unscale_volume(int cd_vol, int max)
{
  int	vol = 0, top = max, bot = 0, scaled;
  
  while (bot <= top)
    {
      vol = (top + bot) / 2;
      scaled = scale_volume(vol, max);
      if (cd_vol == scaled)
	break;
      if (cd_vol < scaled)
	top = vol - 1;
      else
	bot = vol + 1;
    }
  
  if (vol < 0)
    vol = 0;
  else if (vol > max)
    vol = max;
  
  return (vol);
} /* unscale_volume() */

/*---------------------------------------------------------------------*
 * Set the volume level for the left and right channels.  Their values
 * range from 0 to 100.
 *---------------------------------------------------------------------*/
int
gen_set_volume(struct wm_drive *d, int left, int right)
{
  int level;

  left = scale_volume(left, 100);
  right = scale_volume(right, 100);
  level = right << 8 | left; 
  
  /* Send a Mixer IOCTL */
  if (d->fd >= 0)
       (void) ioctl(d->fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &level);
#ifdef notnow
  /* NOTE: the cdvolume2() call is an addition to the cdrom library.
     Pick it up from the archives on bsdi.com */
  cdvolume2 (CUR_CD, left < 0 ? 0 : left > 255 ? 255 : left,
	     right < 0 ? 0 : right > 255 ? 255 : right);
  
#endif
  return (0);
}

/*---------------------------------------------------------------------*
 * Read the initial volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 *---------------------------------------------------------------------*/
int
gen_get_volume(struct wm_drive *d, int *left, int *right)
{
  int level;

  /* Most systems can't seem to do this... */
  *left = *right = -1;

  /* Send a Mixer IOCTL */
  if (d->fd >= 0) {
      if (ioctl(d->fd, MIXER_READ(SOUND_MIXER_VOLUME), &level) == 0) {
         *left = unscale_volume((level & 0xff) & 0xff, 100);
         *right = unscale_volume((level >> 8) & 0xff, 100);
       }
    }
  return (0);
}

/*------------------------------------------------------------------------*
 * gen_get_cdtext(drive, buffer, lenght)
 *------------------------------------------------------------------------*/

int
gen_get_cdtext(struct wm_drive *d, unsigned char **pp_buffer, int *p_buffer_lenght)
{
  return -1; /* no SCSI, no CDTEXT */
} /* gen_get_cdtext() */


#endif /* __bsdi__ */

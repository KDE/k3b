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
 * IRIX specific.
 *
 * Taken from the kscd distribution
 *
 * Paul Kendall
 * paul@orion.co.nz, or
 * paul@kcbbs.gen.nz
 */

static char plat_irix_id[] = "$Id$";

#if defined(sgi) || defined(__sgi)
 
#include "include/wm_config.h"

/*
 * Yes, it was designed for WorkMan 1.4b3
 * Because I did start over from 1.3a, I disable it here.
 * There is no guarantee of getting working code by defining
 * CDDA yourself.
 *
 */
#undef CDDA
/*#define CDDA*/

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sigfpe.h>
#include <dmedia/cdaudio.h>
#include <dmedia/audio.h>
#include <errno.h>

#include "include/wm_struct.h"
#include "include/wm_cdtext.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

void *malloc();
char *strchr();

int	min_volume = 0;
int	max_volume = 255;

extern char	*cd_device;

#ifdef CDDA
static int playing = STOPPED;
static CDPLAYER *icd;
static CDPARSER *icdp;
static CDFRAME cdbuf[12];
static ALport audioport;
static ALconfig aconfig;
static struct itimerval audiotimer = { {0,0}, {0,25000} };
static int cdtrack=0;
static int cdframe=0;
static int cdstopframe=0;

/*
 * Platform specific internal functions for CDDA
 */
void
cbprognum(void *arg, CDDATATYPES type, CDPROGNUM* prognum)
{
  cdtrack = prognum->value;
} /* cbprognum() */

void
cbabstime(void *arg, CDDATATYPES type, struct cdtimecode* atime)
{
  cdframe = CDtctoframe(atime);
  if( cdframe == cdstopframe )
    playing = STOPPED;
} /* cbabstime() */

void
cbplayaudio(void *arg, CDDATATYPES type, short* audio)
{
	if(playing != PLAYING) return;
	ALwritesamps(audioport, audio, CDDA_NUMSAMPLES);
} /* cbplayaudio() */

static void 
alarmsignal()
{
  int n, i;
  if(playing != PLAYING) return;
  if( ALgetfilled(audioport) < CDDA_NUMSAMPLES*8 ) 
    {
      /* Only get more samples and play them if we're getting low
       * this ensures that the CD stays close to the sound
       */
      n = CDreadda(icd, cdbuf, 12);
      if( n == 0 ) return;
      for( i=0 ; i<12 ; i++ )
	CDparseframe(icdp, &cdbuf[i]);
    }
  signal(SIGALRM, alarmsignal);
  setitimer(ITIMER_REAL, &audiotimer, NULL);
} /* alarmsignal() */
#endif

/*--------------------------------------------------------*
 * Initialize the drive.  A no-op for the generic driver.
 *--------------------------------------------------------*/
int
gen_init( struct wm_drive *d )
{
#ifdef CDDA
  long Param[4];
  /* Set the audio rate to 44100Hz 16bit 2s-comp stereo */
  aconfig = ALnewconfig();
  ALsetwidth(aconfig, AL_SAMPLE_16);
  ALsetsampfmt(aconfig, AL_SAMPFMT_TWOSCOMP);
  ALsetchannels(aconfig, 2);
  Param[0] = AL_OUTPUT_RATE;      Param[1] = AL_RATE_44100;
  Param[2] = AL_CHANNEL_MODE;     Param[3] = AL_STEREO;
  ALsetparams(AL_DEFAULT_DEVICE, Param, 4);
  audioport = ALopenport("KDE KSCD Audio", "w", aconfig);
  
  /* setup cdparser */
  icdp = CDcreateparser();
  CDaddcallback(icdp, cd_audio, (CDCALLBACKFUNC)cbplayaudio, 0);
  CDaddcallback(icdp, cd_pnum, (CDCALLBACKFUNC)cbprognum, 0);
  CDaddcallback(icdp, cd_atime, (CDCALLBACKFUNC)cbabstime, 0);
  
  /* Lets handle those floating point exceptions expeditiously. */
  sigfpe_[_UNDERFL].repls = _ZERO;
  handle_sigfpes(_ON, _EN_UNDERFL, NULL, _ABORT_ON_ERROR, NULL);
#endif
  return 0;
} /* gen_init() */

/*-------------------------------------------------------------*
 * Open the CD and figure out which kind of drive is attached.
 *-------------------------------------------------------------*/
int
wmcd_open( struct wm_drive *d )
{
  int	fd;
  CDSTATUS s;
  
  if (d->fd < 0)		/* Device already open? */
    {
      if (cd_device == NULL)
	cd_device = DEFAULT_CD_DEVICE;
      
      d->fd = 1;
      
      /* Now fill in the relevant parts of the wm_drive structure. */
      fd = d->fd;
      *d = *(find_drive_struct("", "", ""));
      d->fd = fd;
      (d->init)(d);
      
      d->daux = CDopen(cd_device,"r");
      if (d->daux == 0)
	{
          return (-6);
	}
#ifdef CDDA
      icd = d->daux;
#endif
    } else {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): [device is open (fd=%d)]\n", d->fd);
    }
  
  CDgetstatus(d->daux, &s);
  if( s.state==CD_NODISC || s.state==CD_ERROR )
    return 1;
  
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


/*----------------------------------*
 * Send a SCSI command out the bus.
 *----------------------------------*/
int 
wm_scsi( struct wm_drive *d, unsigned char *xcdb, int cdblen, 
	 char *retbuf, int retbuflen, int getreply)
{
  return -1;
} /* wm_scsi() */

/*--------------------------------------------------------------------------*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 *--------------------------------------------------------------------------*/
int
gen_get_drive_status( struct wm_drive *d, enum wm_cd_modes oldmode,
                      enum wm_cd_modes *mode, int *pos, int *track,
                      int *index )
{
#ifdef CDDA
  *mode = playing;
  *track = cdtrack;
  *pos = cdframe;
  *index = 0;
#else
  CDSTATUS s;
  if( CDgetstatus(d->daux, &s)==0 )
    return -1;
  *pos = CDmsftoframe(s.min,s.sec,s.frame);
  *track = s.track;
  *index = 0;
  switch( s.state )
    {
    case CD_READY:	*mode = WM_CDM_STOPPED;
      break;
    case CD_STILL:
    case CD_PAUSED: *mode = WM_CDM_PAUSED;
      break;
    case CD_PLAYING: *mode = WM_CDM_PLAYING;
      break;
    default:		*mode = WM_CDM_UNKNOWN;
    }
#endif
  return 0;
} /* gen_get_drive_status() */

/*-------------------------------------*
 * Get the number of tracks on the CD.
 *-------------------------------------*/
int
gen_get_trackcount( struct wm_drive *d, int *tracks )
{
  CDSTATUS s;
  if( CDgetstatus(d->daux, &s)==0 )
    return -1;
  *tracks = s.last;
  return 0;
} /* gen_get_trackcount() */

/*---------------------------------------------------------*
 * Get the start time and mode (data or audio) of a track.
 *---------------------------------------------------------*/
int
gen_get_trackinfo( struct wm_drive *d, int track, int *data, int *startframe)
{
  CDTRACKINFO i;
  int ret = CDgettrackinfo(d->daux, track, &i);
  if( ret == 0 )
    return -1;
  *data = 0;
  *startframe = CDmsftoframe(i.start_min,i.start_sec,i.start_frame);
  return 0;
} /* gen_get_trackinfo() */

/*-------------------------------------*
 * Get the number of frames on the CD.
 *-------------------------------------*/
int
gen_get_cdlen( struct wm_drive *d, int *frames )
{
  CDSTATUS s;
  if( CDgetstatus(d->daux, &s)==0 )
    return -1;
  *frames = CDmsftoframe(s.total_min,s.total_sec,s.total_frame);
  return 0;
} /* gen_get_cdlen() */

/*------------------------------------------------------------*
 * Play the CD from one position to another (both in frames.)
 *------------------------------------------------------------*/
int
gen_play( struct wm_drive *d, int start, int end )
{
#ifdef CDDA
  int m, s, f;
  CDframetomsf(start, &m, &s, &f);
  CDseek(icd, m, s, f);
  cdstopframe = end;
  playing = PLAYING;
  signal(SIGALRM, alarmsignal);
  setitimer(ITIMER_REAL, &audiotimer, NULL);
#else
  int m, s, f;
  CDframetomsf(start, &m, &s, &f);
  CDplayabs(d->daux, m, s, f, 1);
#endif
  return 0;
} /* gen_play() */

/*---------------*
 * Pause the CD.
 *---------------*/
int
gen_pause( struct wm_drive *d )
{
#ifdef CDDA
  playing = WM_CDM_PAUSED;
#else
  CDSTATUS s;
  if( CDgetstatus(d->daux, &s)==0 )
    return -1;
  if(s.state == CD_PLAYING)
    CDtogglepause(d->daux);
#endif
  return 0;
} /* gen_pause() */

/*-------------------------------------------------*
 * Resume playing the CD (assuming it was paused.)
 *-------------------------------------------------*/
int
gen_resume( struct wm_drive *d )
{
#ifdef CDDA
  playing = WM_CDM_PLAYING;
  signal(SIGALRM, alarmsignal);
  setitimer(ITIMER_REAL, &audiotimer, NULL);
#else
  CDSTATUS s;
  if( CDgetstatus(d->daux, &s)==0 )
    return -1;
  if(s.state == CD_PAUSED)
    CDtogglepause(d->daux);
#endif
  return 0;
} /* gen_resume() */

/*--------------*
 * Stop the CD.
 *--------------*/
int
gen_stop( struct wm_drive *d )
{
#ifdef CDDA
  playing = WM_CDM_STOPPED;
#else
  CDstop(d->daux);
#endif
  return 0;
} /* gen_stop() */

/*----------------------------------------*
 * Eject the current CD, if there is one.
 *----------------------------------------*/
int
gen_eject( struct wm_drive *d )
{
#ifdef CDDA
  playing = WM_CDM_STOPPED;
#endif
  CDeject(d->daux);
  return 0;
} /* gen_eject() */

/*----------------------------------------*
 * Close the CD tray
 *
 * Please edit and send changes to
 * milliByte@Deathsdoor.com
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

/*---------------------------------------------------------------------*
 * Set the volume level for the left and right channels.  Their values
 * range from 0 to 100.
 *---------------------------------------------------------------------*/
int
gen_set_volume( struct wm_drive *d, int left, int right )
{
  long Param[4];
  Param[0] = AL_LEFT_SPEAKER_GAIN;      Param[1] = left*255/100;
  Param[2] = AL_RIGHT_SPEAKER_GAIN;     Param[3] = right*255/100;
  ALsetparams(AL_DEFAULT_DEVICE, Param, 4);
  return 0;
} /* gen_set_volume() */

/*---------------------------------------------------------------------*
 * Read the initial volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 *---------------------------------------------------------------------*/
int
gen_get_volume( struct wm_drive *d, int *left, int *right )
{
  long Param[4];
  Param[0] = AL_LEFT_SPEAKER_GAIN;      Param[1] = 0;
  Param[2] = AL_RIGHT_SPEAKER_GAIN;     Param[3] = 0;
  ALgetparams(AL_DEFAULT_DEVICE, Param, 4);
  *left = Param[1] * 100 / 255;
  *right = Param[3] * 100 / 255;
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


#endif


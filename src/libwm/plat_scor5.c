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
 * SCO Openserver R5 specific. Derived from the KSCD plat_scor5.c
 *
 */


static char plat_linux_id[] = "$Id$";

#if defined(M_UNIX) || defined(__M_UNIX)


#include "include/wm_config.h"

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/scsi.h>
#include <sys/scsicmd.h>
#include <errno.h>
#include <macros.h>

#include "include/wm_struct.h"
#include "include/wm_scsi.h"
#include "include/wm_cdtext.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

#define SENSE_SZ EXTD_SENSE_LEN

void *malloc();
char *strchr();

int	min_volume = 0;
int	max_volume = 255;

extern char	*cd_device;

/*
 * platformspecific internal function
 */
static int
create_cdrom_node(char *dev_name)
{
  char pass_through[100];
  int file_des;
  struct stat sbuf;
  int err;
  int ccode;
  
  
  strncpy(pass_through, dev_name, sizeof(pass_through)-2);
  strcat(pass_through, "p" );
  
  if (setreuid(-1,0) < 0)
    {
      perror("setregid/setreuid/access");
      return -1;
    }
  
  ccode = access(pass_through, F_OK);
  
  if (ccode < 0)
    {
      
      if (stat(dev_name, &sbuf) < 0)
	{
	  perror("Call to get pass-through device number failed");
	  return -1;
	}
      
      if (mknod(pass_through, (S_IFCHR | S_IREAD | S_IWRITE),
		sbuf.st_rdev) < 0)
	{
	  perror("Unable to make pass-through node");
	  return -1;
	}
      
      if (chown(pass_through, 0 , 0) < 0)
	{
	  perror("chown");
	  return -1;
	}
      
      if (chmod(pass_through, 0660 ) < 0)
	{
	  perror("chmod");
	  return -1;
	}
    }
  
  file_des = open( pass_through, O_RDONLY);
  err = errno;
  
  /*
  if ( (setreuid(-1,getuid()) < 0) || (setregid(-1,getgid()) < 0) )
    {
      perror("setreuid/setregid");
      exit(1);
    }
  
  */

  errno = err;
  return file_des;
} /* create_cdrom_node() */


/*
 * Initialize the drive.  A no-op for the generic driver.
 */
int
gen_init( struct wm_drive *d )
{
  return (0);
} /* gen_init() */

/*
 * Open the CD and figure out which kind of drive is attached.
 */
int
wmcd_open( struct wm_drive *d)
{
  int		fd;
  static int	warned = 0;
  char		vendor[9], model[17], rev[5];
  
  if (d->fd >= 0)		/* Device already open? */
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): [device is open (fd=%d)]\n", d->fd);
      return (0);
    }
  
  if (cd_device == NULL)
    {
      fprintf(stderr,"cd_device string empty\n");
      return (-1);
    }
  
  
  d->fd = create_cdrom_node(cd_device); /* this will do open */
  
  if (d->fd < 0)
    {
      if (errno == EACCES)
	{
	  if (! warned)
	    {
	      fprintf(stderr,"Cannot access %s\n",cd_device);
	      warned++;
	    }
	}
      else if (errno != EINTR)
	{
	  perror(cd_device);
          return( -6 );
	}
      
      /* can not acces CDROM device */
      return (-1);
    }
  
  if (warned)
    {
      warned = 0;
      fprintf(stderr, "Thank you.\n");
    }
  
  /* Now fill in the relevant parts of the wm_drive structure. */
  
  fd = d->fd;
  
  if (wm_scsi_get_drive_type(d, vendor, model, rev) < 0)
    {
      perror("Cannot inquiry drive for it's type");
      return (-1);
    }
  *d = *(find_drive_struct(vendor, model, rev));
  /*	about_set_drivetype(d->vendor, d->model, rev);*/
  
  d->fd = fd;
  
  return (0);
} /* wmcd_open */

/*
 * Re-Open the device
 */
wmcd_reopen( struct wm_drive *d )
{
  int status;
  int tries = 0;
  do {
    if (d->fd >= 0) /* Device really open? */
      {
	close(d->fd);  /* ..then close it */
	d->fd = -1;      /* closed */
      }
    susleep( 1000 );
    status = wmcd_open( d );
    susleep( 1000 );
    tries++;
  } while ( (status != 0) && (tries < 10) );
  return status;
} /* wmcd_reopen() */

/*
 * Send a SCSI command out the bus.
 */
int 
wm_scsi(struct wm_drive *d, unsigned char *xcdb, int cdblen, 
	char *retbuf, int retbuflen, int getreply)
{
  int ccode;
  int file_des = d->fd;
  unsigned char sense_buffer[ SENSE_SZ ];
  
  /* getreply == 1 is read, == 0 is write */
  
  struct scsicmd2 sb;	/* Use command with automatic sense */
  
  if (cdblen > SCSICMDLEN)
    {
      fprintf(stderr,"Cannot handle longer commands than %d bytes.\n", SCSICMDLEN);
      exit(-1);
    }
  
  /* Get the command */
  memcpy(sb.cmd.cdb, xcdb, cdblen);
  sb.cmd.cdb_len = cdblen;
  
  /* Point to data buffer */
  sb.cmd.data_ptr = retbuf;
  sb.cmd.data_len = retbuflen;
  
  /* Is this write or read ? */
  sb.cmd.is_write = (getreply==1) ? 0 : 1;
  
  /* Zero out return status fields */
  sb.cmd.host_sts = 0;
  sb.cmd.target_sts = 0;
  
  /* Set up for possible sense info */
  
  sb.sense_ptr = sense_buffer;
  sb.sense_len = sizeof(sense_buffer);
  
  ccode =  ioctl(file_des, SCSIUSERCMD2,  &sb);
  
  if ( sb.cmd.target_sts != 02 )
    return ccode;
  
  return 0;
} /* wm_scsi() */

void
keep_cd_open() { }

/*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 */
int
gen_get_drive_status(wm_drive *d, enum cd_modes oldmode, enum cd_modes *mode, 
		     int *pos, int *track, int *index)
{
  return (wm_scsi2_get_drive_status(d, oldmode, mode, pos, track, index));
} /* gen_get_drive_status() */

/*
 * Get the number of tracks on the CD.
 */
int
gen_get_trackcount(struct wm_drive *d, int *tracks)
{
  return (wm_scsi2_get_trackcount(d, tracks));
} /* gen_get_trackcount() */


/*
 * Get the start time and mode (data or audio) of a track.
 */
int
gen_get_trackinfo(struct wm_drive *d, int track, int *data, int *startframe)
{
  return (wm_scsi2_get_trackinfo(d, track, data, startframe));
} /* gen_get_trackinfo() */

/*
 * Get the number of frames on the CD.
 */
int
gen_get_cdlen(struct wm_drive *d, int *frames)
{
  return (wm_scsi2_get_cdlen(d, frames));
} /* gen_get_cdlen() */


/*
 * Play the CD from one position to another (both in frames.)
 */
int
gen_play(struct wm_drive *d, int start, int end)
{
  return (wm_scsi2_play(d, start, end));
} /* gen_play() */

/*
 * Pause the CD.
 */
int
gen_pause( struct wm_drive *d )
{
  return (wm_scsi2_pause(d));
} /* gen_pause() */

/*
 * Resume playing the CD (assuming it was paused.)
 */
int
gen_resume( struct wm_drive *d )
{
  return (wm_scsi2_resume(d));
} /* gen_resume() */

/*
 * Stop the CD.
 */
int
gen_stop(struct wm_drive *d)
{
  return (wm_scsi2_stop(d));
} /* gen_stop() */

/*
 * Eject the current CD, if there is one.
 */
int
gen_eject(struct wm_drive *d)
{
  int stat;
  
  stat = wm_scsi2_eject(d);
  sleep(2);
  return (stat);
} /* gen_eject() */


int
gen_closetray(struct wm_drive *d)
{
  return(wm_scsi2_closetray(d));
} /* gen_closetray() */


/*
 * Set the volume level for the left and right channels.  Their values
 * range from 0 to 100.
 */
int
gen_set_volume(struct wm_drive *d, int left, int right)
{
  return (wm_scsi2_set_volume(d, left, right));
} /* gen_set_volume() */

/*
 * Read the initial volume from the drive, if available.  Each channel
 * ranges from 0 to 100, with -1 indicating data not available.
 */
int
gen_get_volume(struct wm_drive *d, int *left, int *right)
{
	return (wm_scsi2_get_volume(d, left, right));
} /* gen_get_volume() */

/*------------------------------------------------------------------------*
 * gen_get_cdtext(drive, buffer, lenght)
 *------------------------------------------------------------------------*/

int
gen_get_cdtext(struct wm_drive *d, unsigned char **pp_buffer, int *p_buffer_lenght)
{
  /* This needs to be tested! */
  return wm_scsi_get_cdtext(d, pp_buffer, p_buffer_lenght);
} /* gen_get_cdtext() */



#endif /* M_UNIX */





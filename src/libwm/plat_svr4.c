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
 * SVR4 specific.  Much of this is similar to plat_hpux.c.
 */

static char plat_svr4_id[] = "$Id$";

#if (defined(SVR4) || defined(__SVR4)) && !defined(sun) && !defined(__sun) && !defined(sony_news) && !defined(__sony_news)


#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <sys/sdi.h>
#include <sys/sdi_edt.h>
#include <sys/scsi.h>
#include <errno.h>

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cdtext.h"

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM


void *malloc();
char *strchr();

int	min_volume = 0;
int	max_volume = 255;

extern char	*cd_device;

static int
create_cdrom_node(char *dev_name)
{
  char pass_through[100];
  int file_des;
  dev_t pass_thru_device;
  int err;
  int ccode;
  
  
  strncpy(pass_through, dev_name, sizeof(pass_through) - 2);
  strcat(pass_through, "p" );
  
  if (setreuid(-1,0) < 0)
    {
      perror("setregid/setreuid/access");
      return -1;
    }
  
  ccode = access(pass_through, F_OK);
  
  if (ccode < 0)
    {
      if ((file_des = open(dev_name, O_RDONLY)) < 0)
	{
	  perror("open cdrom devices failed");
	  return -1;
	}
      
      if (ioctl(file_des, B_GETDEV, &pass_thru_device) < 0)
	{
	  perror("Call to get pass-through device number failed");
	  return -1;
	}
      
      (void)close(file_des);
      
      if (mknod(pass_through, (S_IFCHR | S_IREAD | S_IWRITE),
		pass_thru_device) < 0)
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
  
  file_des = open( pass_through, O_RDWR);
  err = errno;
  
  if ( (setreuid(-1,getuid()) < 0) || (setregid(-1,getgid()) < 0) )
    {
      perror("setreuid/setregid");
      return -1;
    }
  errno = err;
  return file_des;
} /* create_cdrom_node() */

find_cdrom()
{
  /*
  ** the path of the device has to start w/ /dev
  ** otherwise we are vulnerable to race conditions
  ** Thomas Biege <thomas@suse.de>
  */
  
  cd_device = getenv("CDROM");
  if ( (cd_device != NULL) && 
       !(strncmp("/dev/", cd_device, 5) || 
	 strstr(cd_device, "/../") ))
    return 1;
  
  if (access("/dev/cdrom/cdrom1", F_OK) == 0)
    {
      cd_device = "/dev/cdrom/cdrom1";
      return 1;
    }
  else if (access("/dev/cdrom/cdrom2", F_OK) == 0)
    {
      cd_device = "/dev/cdrom/cdrom2";
      return 1;
    } else {
      fprintf(stderr, "Couldn't find a CD device!\n");
      return 0;
    }
} /* find_cdrom() */

/*
 * Initialize the drive.  A no-op for the generic driver.
 */
int
gen_init(struct wm_drive *d)
{
  return (0);
} /* gen_init() */

/*
 * Open the CD and figure out which kind of drive is attached.
 */
int
wmcd_open(struct wm_drive *d)
{
  int		fd, flag = 1;
  static int	warned = 0;
  char	vendor[32] = WM_STR_GENVENDOR;
  char	 model[32] = WM_STR_GENMODEL;
  char	   rev[32] = WM_STR_GENREV;
  
  if (d->fd >= 0)		/* Device already open? */
    {
      wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wmcd_open(): [device is open (fd=%d)]\n", d->fd);
      return (0);
    }
  
  if (cd_device == NULL)
    cd_device = DEFAULT_CD_DEVICE;
  
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
          return ( -6 );
	}
      
      /* No CD in drive. (Is this true also for svr4 ? XXX ) */
      return (1);
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
      exit(1);
    }
  *d = *(find_drive_struct(vendor, model, rev));
  wm_drive_settype(vendor, model, rev);
  
  d->fd = fd;
  
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


/*
 * Send a SCSI command out the bus.
 */
int 
wm_scsi( struct wm_drive *d, unsigned char *xcdb, int cdblen, 
	 char *retbuf, int retbuflen, int getreply)
{
  int ccode;
  int file_des = d->fd;
  int i,j;
  unsigned char sense_buffer[ SENSE_SZ ];
  int errno_save;
  
  /* getreply == 1 is read, == 0 is write */
  
  struct sb sb;
  struct scs scs;
  
  sb.sb_type = ISCB_TYPE;
  
  sb.SCB.sc_comp_code = SDI_PROGRES;
  sb.SCB.sc_int = NULL;
  sb.SCB.sc_wd = 0;
  sb.SCB.sc_dev.sa_major = 0;
  sb.SCB.sc_dev.sa_minor = 0;
  sb.SCB.sc_dev.sa_lun = 0;
  sb.SCB.sc_dev.sa_exlun = 0;
  sb.SCB.sc_status = 0;
  sb.SCB.sc_link = (struct sb *) NULL;
  sb.SCB.sc_resid = 0;
  
  sb.SCB.sc_cmdpt = (void *)xcdb;
  sb.SCB.sc_cmdsz = cdblen;
  
  sb.SCB.sc_datapt = retbuf ;
  sb.SCB.sc_datasz = retbuflen ;
  
  if (getreply == 1)
    sb.SCB.sc_mode = SCB_READ;
  else
    sb.SCB.sc_mode = SCB_WRITE;
  
  sb.SCB.sc_time = 500;
  
  ccode =  ioctl(file_des, SDI_SEND,  &sb);
  
  if ( (sb.SCB.sc_comp_code != 0xd000000e ) ||
       ( sb.SCB.sc_status != 02) )
    return ccode;
  
  errno_save = errno;
  
  sb.SCB.sc_comp_code = SDI_PROGRES;
  sb.SCB.sc_int = NULL;
  sb.SCB.sc_wd = 0;
  sb.SCB.sc_dev.sa_major = 0;
  sb.SCB.sc_dev.sa_minor = 0;
  sb.SCB.sc_dev.sa_lun = 0;
  sb.SCB.sc_dev.sa_exlun = 0;
  sb.SCB.sc_status = 0;
  sb.SCB.sc_link = (struct sb *) NULL;
  sb.SCB.sc_resid = 0;
  
  scs.ss_op	=	SS_REQSEN;
  scs.ss_lun	=	0;
  scs.ss_addr1	=	0;
  scs.ss_addr	=	0;
  scs.ss_len	=	SENSE_SZ;
  scs.ss_cont	=	0;
  
  sb.SCB.sc_cmdpt = SCS_AD(&scs);
  sb.SCB.sc_cmdsz = SCS_SZ;
  sb.SCB.sc_datapt = sense_buffer;
  sb.SCB.sc_datasz = 18;
  sb.SCB.sc_mode = SCB_READ;
  sb.SCB.sc_time = 5000;
  
  if (ioctl(file_des, SDI_SEND,  &sb) < 0)
    {
      fprintf(stderr,"Cannot read sense.\n");
      exit(-1);
    }
  
  errno=errno_save;
  return -1;
} /* wm_scsi() */

/*
 * Get the current status of the drive: the current play mode, the absolute
 * position from start of disc (in frames), and the current track and index
 * numbers if the CD is playing or paused.
 */
int
gen_get_drive_status(struct wm_drive *d, enum wm_cd_modes oldmode,
		     enum wm_cd_modes *mode, int *pos, int *track, int *index)
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
  int		tmp;
 
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
gen_pause(struct wm_drive *d)
{
  return (wm_scsi2_pause(d));
} /* gen_pause() */

/*
 * Resume playing the CD (assuming it was paused.)
 */
int
gen_resume(struct wm_drive *d)
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
  return (wm_scsi2_eject(d));
} /* gen_eject() */

/*
 * Close the tray.
 * please review scsi.c / wm_scsi2_closetray()
 * and send changes to milliByte@DeathsDoor.com
 */
int
gen_closetray( struct wm_drive *d )
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
  /* This needs to be tested */
  return wm_scsi_get_cdtext(d, pp_buffer, p_buffer_lenght);
} /* gen_get_cdtext() */

#endif

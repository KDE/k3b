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
 * Frontend functions for sending raw SCSI commands to the CD-ROM drive.
 * These depend on wm_scsi(), which should be defined in each platform
 * module.
 */

static char scsi_id[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_scsi.h"
#include "include/wm_platform.h"
#include "include/wm_helpers.h"
#include "include/wm_cdrom.h"
#include "include/wm_cdtext.h"

#define SCMD_INQUIRY		0x12
#define SCMD_MODE_SELECT	0x15
#define SCMD_MODE_SENSE		0x1a
#define SCMD_START_STOP		0x1b
#define SCMD_PREVENT		0x1e
#define SCMD_READ_SUBCHANNEL	0x42
#define SCMD_READ_TOC		0x43
#define SCMD_PLAY_AUDIO_MSF	0x47
#define SCMD_PAUSE_RESUME	0x4b

#define SUBQ_STATUS_INVALID	0x00
#define SUBQ_STATUS_PLAY	0x11
#define SUBQ_STATUS_PAUSE	0x12
#define SUBQ_STATUS_DONE	0x13
#define SUBQ_STATUS_ERROR	0x14
#define SUBQ_STATUS_NONE	0x15
#define SUBQ_STATUS_NO_DISC	0x17	/* Illegal, but Toshiba returns it. */
#define SUBQ_ILLEGAL		0xff

#define	PAGE_AUDIO		0x0e
#define LEADOUT			0xaa

#define WM_MSG_CLASS WM_MSG_CLASS_SCSI

/*
 * Send a SCSI command over the bus, with all the CDB bytes specified
 * as unsigned char parameters.  This doesn't use varargs because some
 * systems have stdargs instead and the number of bytes in a CDB is
 * limited to 12 anyway.
 *
 * d	Drive structure
 * buf	Buffer for data, both sending and receiving
 * len	Size of buffer
 * dir	TRUE if the command expects data to be returned in the buffer.
 * a0-	CDB bytes.  Either 6, 10, or 12 of them, depending on the command.
 */
/*VARARGS4*/
int
sendscsi( struct wm_drive *d, void *buf,
          unsigned int len, int dir,
	  unsigned char a0, unsigned char a1,
          unsigned char a2, unsigned char a3,
          unsigned char a4, unsigned char a5,
          unsigned char a6, unsigned char a7,
          unsigned char a8, unsigned char a9,
          unsigned char a10, unsigned char a11 )

{
	int		cdblen = 0;
	unsigned char	cdb[12];

	cdb[0] = a0;
	cdb[1] = a1;
	cdb[2] = a2;
	cdb[3] = a3;
	cdb[4] = a4;
	cdb[5] = a5;

	switch ((a0 >> 5) & 7) {
	case 0:
		cdblen = 6;
		break;
	
	case 5:
		cdb[10] = a10;
		cdb[11] = a11;
		cdblen = 12;

	case 1:
	case 2:
	case 6:		/* assume 10-byte vendor-specific codes for now */
		cdb[6] = a6;
		cdb[7] = a7;
		cdb[8] = a8;
		cdb[9] = a9;
		if (! cdblen)
			cdblen = 10;
		break;
	}

	return (wm_scsi(d, cdb, cdblen, buf, len, dir));
}

/*
 * Send a MODE SENSE command and return the results (minus header cruft)
 * in a user buffer.
 *
 * d	Drive structure
 * page	Number of page to query (plus page control bits, if any)
 * buf	Result buffer
 */
int
wm_scsi_mode_sense( struct wm_drive *d, unsigned char page, unsigned char *buf )
{
	unsigned char	pagebuf[255];
	int		status, i, len, offset;

	status = sendscsi(d, pagebuf, sizeof(pagebuf), 1, SCMD_MODE_SENSE, 0,
			page, 0, sizeof(pagebuf), 0,0,0,0,0,0,0);
	if (status < 0)
		return (status);
	
	/*
	 * The first byte of the returned data is the transfer length.  Then
	 * two more bytes and the length of whatever header blocks are in
	 * front of the page we want.
	 */
	len = pagebuf[0] - pagebuf[3] - 3;
	offset = pagebuf[3] + 4;
	for (i = 0; i < len; i++)
		buf[i] = pagebuf[offset + i];

	return (0);
}

/*
 * Send a MODE SELECT command.
 *
 * d	Drive structure
 * buf	Page buffer (no need to put on block descriptors)
 * len	Size of page
 */
int
wm_scsi_mode_select(d, buf, len)
	struct wm_drive	*d;
	unsigned char	*buf;
	unsigned char	len;
{
	unsigned char	pagebuf[255];
	int		i;

	pagebuf[0] = pagebuf[1] = pagebuf[2] = pagebuf[3] = 0;
	for (i = 0; i < (int) len; i++)
		pagebuf[i + 4] = buf[i];

	return (sendscsi(d, pagebuf, len + 4, 0, SCMD_MODE_SELECT, 0x10, 0,
			0, len + 4, 0,0,0,0,0,0,0));
}

/*
 * Send an INQUIRY command to get the drive type.
 *
 * d		Drive structure
 * vendor	Buffer for vendor name (8 bytes + null)
 * model	Buffer for model name (16 bytes + null)
 * rev		Buffer for revision level (4 bytes + null)
 *
 * The above string lengths apply to the SCSI INQUIRY command. The
 * actual WorkMan drive structure reserves 31 bytes + NULL per entry.
 *
 * If the model name begins with "CD-ROM" and zero or more spaces, that will
 * all be stripped off since it's just extra junk to WorkMan.
 */
int
wm_scsi_get_drive_type( struct wm_drive *d, char *vendor,
			 char *model, char *rev )
{
/* removed   unsigned*/
	char		*s, *t, buf[36];

	wm_lib_message(WM_MSG_CLASS_SCSI | WM_MSG_LEVEL_INFO, "Sending SCSI inquiry command...\n");
	if (sendscsi(d, buf, sizeof(buf), 1, SCMD_INQUIRY, 0, 0, 0, sizeof(buf), 0,0,0,0,0,0,0))
	  {
		sprintf( vendor, WM_STR_GENVENDOR);
		sprintf( model, WM_STR_GENMODEL);
		sprintf( rev, WM_STR_GENREV);
		wm_lib_message(WM_MSG_CLASS_SCSI | WM_MSG_LEVEL_ERROR, "SCSI Inquiry command not supported in this context\n");
		return (WM_ERR_SCSI_INQUIRY_FAILED);
	  }
	wm_lib_message(WM_MSG_CLASS_SCSI | WM_MSG_LEVEL_DEBUG, "sent.\n");

	memcpy(vendor, buf + 8, 8);
	vendor[8] = '\0';
	memcpy(model, buf + 16, 16);
	model[16] = '\0';
	memcpy(rev, buf + 32, 4);
	rev[4] = '\0';
	wm_lib_message(WM_MSG_CLASS_SCSI | WM_MSG_LEVEL_VERB, "SCSI Inquiry result: [%s|%s|%s]\n", vendor, model, rev);

	
	/* Remove "CD-ROM " from the model. */
	if (! strncmp(model, "CD-ROM", 6))
	{
		s = model + 6;
		t = model;
		while (*s == ' ' || *s == '\t')
			s++;
		while( (*t++ = *s++) )
			;
	}
	wm_lib_message(WM_MSG_CLASS_SCSI | WM_MSG_LEVEL_INFO, "scsi: Cooked data: %s %s rev. %s\n", vendor, model,rev);
	return (0);
} /* wm_scsi_get_drive_type() */

/*
 * Send a SCSI-2 PAUSE/RESUME command.  "resume" is 1 to resume, 0 to pause.
 */
int
wm_scsi2_pause_resume(d, resume)
	struct wm_drive	*d;
	int		resume;
{
	return (sendscsi(d, NULL, 0, 0, SCMD_PAUSE_RESUME, 0, 0, 0, 0, 0, 0,
			0, resume ? 1 : 0, 0,0,0));
}

/*
 * Send a SCSI-2 "prevent media removal" command.  "prevent" is 1 to lock
 * caddy in.
 */
int
wm_scsi2_prevent(d, prevent)
	struct wm_drive	*d;
	int		prevent;
{
	return (sendscsi(d, NULL, 0, 0, SCMD_PREVENT, 0, 0, 0, 0, 0, 0,
			0, prevent ? 1 : 0, 0,0,0));
}

/*
 * Send a SCSI-2 PLAY AUDIO MSF command.  Pass the starting and ending
 * frame numbers.
 */
int
wm_scsi2_play(d, sframe, eframe)
	struct wm_drive	*d;
	int		sframe, eframe;
{
	return (sendscsi(d, NULL, 0, 0, SCMD_PLAY_AUDIO_MSF, 0, 0,
			sframe / (60 * 75), (sframe / 75) % 60, sframe % 75,
			eframe / (60 * 75), (eframe / 75) % 60, eframe % 75,
			0,0,0));
}

/*
 * Send a SCSI-2 READ TOC command to get the data for a particular track.
 * Fill in track information from the returned data.
 */
int
wm_scsi2_get_trackinfo(d, track, data, startframe)
	struct wm_drive	*d;
	int		track, *data, *startframe;
{
	unsigned char	buf[12];	/* one track's worth of info */

	if (sendscsi(d, buf, sizeof(buf), 1, SCMD_READ_TOC, 2,
			0, 0, 0, 0, track, sizeof(buf) / 256,
			sizeof(buf) % 256, 0,0,0))
		return (-1);
	
	*data = buf[5] & 4 ? 1 : 0;
	*startframe = buf[9] * 60 * 75 + buf[10] * 75 + buf[11];

	return (0);
}

/*
 * Get the starting frame for the leadout area (which should be the same as
 * the length of the disc as far as WorkMan is concerned).
 */
int
wm_scsi2_get_cdlen(d, frames)
	struct wm_drive	*d;
	int		*frames;
{
	int		tmp;

	return (wm_scsi2_get_trackinfo(d, LEADOUT, &tmp, frames));
}

/*
 * Get the current status of the drive by sending the appropriate SCSI-2
 * READ SUB-CHANNEL command.
 */
int
wm_scsi2_get_drive_status(d, oldmode, mode, pos, track, index)
	struct wm_drive	*d;
	enum wm_cd_modes oldmode, *mode;
	int		*pos, *track, *index;
{
	unsigned char	buf[48];

	/* If we can't get status, the CD is ejected, so default to that. */
	*mode = WM_CDM_EJECTED;

	/* Is the device open? */
	if (d->fd < 0)
	{
/*
 * stupid somehow, but necessary this time
 */		
		switch( wmcd_open( d ) ) {

		case -1:	/* error */
			return (-1);
		
		case 1:		/* retry */
			return (0);
		}
	}

	/* If we can't read status, the CD has been ejected. */
	buf[1] = SUBQ_ILLEGAL;
	if (sendscsi(d, buf, sizeof(buf), 1, SCMD_READ_SUBCHANNEL, 2, 64, 1,
			0, 0, 0, sizeof(buf) / 256, sizeof(buf) % 256, 0,0,0))
		return (0);
	
        switch (buf[1]) {
        case SUBQ_STATUS_PLAY:
		*mode = WM_CDM_PLAYING;
		*track = buf[6];
		*index = buf[7];
		*pos = buf[9] * 60 * 75 + buf[10] * 75 + buf[11];
		break;

	case SUBQ_STATUS_PAUSE:
		if (oldmode == WM_CDM_PLAYING || oldmode == WM_CDM_PAUSED)
		{
			*mode = WM_CDM_PAUSED;
			*track = buf[6];
			*index = buf[7];
			*pos = buf[9] * 60 * 75 +
				buf[10] * 75 +
				buf[11];
		}
		else
			*mode = WM_CDM_STOPPED;
		break;

	/*
	 * SUBQ_STATUS_DONE is sometimes returned when the CD is idle,
	 * even though the spec says it should only be returned when an
	 * audio play operation finishes.
	 */
	case SUBQ_STATUS_DONE:
	case SUBQ_STATUS_NONE:
	case SUBQ_STATUS_INVALID:
		if (oldmode == WM_CDM_PLAYING)
			*mode = WM_CDM_TRACK_DONE;
		else
			*mode = WM_CDM_STOPPED;
		break;

	/*
	 * This usually means there's no disc in the drive.
	 */
	case SUBQ_STATUS_NO_DISC:
		break;

	/*
	 * This usually means the user ejected the CD manually.
	 */
	case SUBQ_STATUS_ERROR:
		break;

	case SUBQ_ILLEGAL:	/* call didn't really succeed */
		break;

        default:
		*mode = WM_CDM_UNKNOWN;
#ifdef DEBUG		
		if( getenv( "WORKMAN_DEBUG" ) != NULL )
			printf("wm_scsi2_get_drive_status: status is 0x%x\n",
				buf[1]);
#endif				
		break;
        }

	return (0);
}

/*
 * Get the number of tracks on the CD using the SCSI-2 READ TOC command.
 */
int
wm_scsi2_get_trackcount(d, tracks)
	struct wm_drive	*d;
	int		*tracks;
{
	unsigned char	buf[4];

	if (sendscsi(d, buf, sizeof(buf), 1, SCMD_READ_TOC, 0,
			0, 0, 0, 0, 0, sizeof(buf) / 256,
			sizeof(buf) % 256, 0,0,0))
		return (-1);

	*tracks = buf[3] - buf[2] + 1;
	return (0);
}

/*
 * Pause the CD.
 */
int
wm_scsi2_pause(d)
	struct wm_drive	*d;
{
	return (wm_scsi2_pause_resume(d, 0));
}

/*
 * Resume playing after a pause.
 */
int
wm_scsi2_resume(d)
	struct wm_drive	*d;
{
	return (wm_scsi2_pause_resume(d, 1));
}

/*
 * Stop playing the CD by sending a START STOP UNIT command.
 */
int
wm_scsi2_stop(d)
	struct wm_drive	*d;
{
	return (sendscsi(d, NULL, 0, 0, SCMD_START_STOP, 0, 0,0,0,0,0,0,0,0,0,0));
}

/*
 * Eject the CD by sending a START STOP UNIT command.
 */
int
wm_scsi2_eject(struct wm_drive *d)
{
	/* Unlock the disc (possibly unnecessary). */
	if (wm_scsi2_prevent(d, 0))
		return (-1);
	wm_lib_message(WM_MSG_CLASS_SCSI | WM_MSG_LEVEL_VERB, "Issuing START_STOP for ejecting...\n");
	return (sendscsi(d, NULL, 0, 0, SCMD_START_STOP, 2, 0,0,0,0,0,0,0,0,0,0));
}

/*
 * Something like a dummy. The SCSI-2 specs are too hard for me to
 * understand here...
 *
 * If you have the correct command handy, please send the code to
 * milliByte@DeathsDoor.com
 */
int
wm_scsi2_closetray(struct wm_drive *d)
{
	wm_lib_message(WM_MSG_CLASS_SCSI | WM_MSG_LEVEL_VERB, "Issuing START_STOP for closing...\n");
	return (sendscsi(d, NULL, 0,0, SCMD_START_STOP, 2, 0,0,0,0,0,0,0,0,0,0));
}

/*
 * Get the volume by doing a MODE SENSE command.
 */
int
wm_scsi2_get_volume(d, left, right)
	struct wm_drive	*d;
	int		*left, *right;
{
	unsigned char	mode[16];

	*left = *right = -1;

	/* Get the current audio parameters first. */
	if (wm_scsi_mode_sense(d, PAGE_AUDIO, mode))
		return (-1);

	*left = ((int) mode[9] * 100) / 255;
	*right = ((int) mode[11] * 100) / 255;

	return (0);
}

/*
 * Set the volume by doing a MODE SELECT command.
 */
int
wm_scsi2_set_volume(d, left, right)
	struct wm_drive	*d;
	int		left, right;
{
	unsigned char	mode[16];

	/* Get the current audio parameters first. */
	if (wm_scsi_mode_sense(d, PAGE_AUDIO, mode))
		return (-1);
	
	/* Tweak the volume part of the parameters. */
	mode[9] = (left * 255) / 100;
	mode[11] = (right * 255) / 100;

	/* And send them back to the drive. */
	return (wm_scsi_mode_select(d, mode, sizeof(mode)));
}

/*------------------------------------------------------------------------*
 * wm_scsi_get_cdtext(drive, buffer, lenght)
 *
 * Return a buffer with cdtext-stream. buffer mus be allocated and filled
 *
 *
 *------------------------------------------------------------------------*/
#ifndef CGC_DATA_READ
  #define CGC_DATA_READ 2
#endif
  
int
wm_scsi_get_cdtext(struct wm_drive *d, unsigned char **pp_buffer, int *p_buffer_length)
{
  int ret, capability;
  unsigned char temp[8];
  unsigned char *dynamic_temp;
  int cdtext_possible;
  int pi_buffer_length = *p_buffer_length;
  unsigned short cdtext_data_length;
  unsigned long feature_list_length;
#define IGNORE_FEATURE_LIST
#ifndef IGNORE_FEATURE_LIST
  struct feature_list_header *pHeader;
  struct feature_descriptor_cdread *pDescriptor;
#endif /* IGNORE_FEATURE_LIST */

  dynamic_temp = NULL;
  cdtext_possible = 0;
  wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS, "wm_scsi_get_cdtext entered\n");

  printf("CDTEXT INFO: use GET_FEATURY_LIST(0x46)...\n");
  ret = sendscsi(d, temp, 8, CGC_DATA_READ,
    0x46, 0x02, 0x00, 0x1E, 0,
    0, 0, 0, 8, 0, 0, 0);

  if(ret)
  {
    printf("CDTEXT ERROR: GET_FEATURY_LIST(0x46) not implemented or broken. ret = %i!\n", ret);
#ifndef IGNORE_FEATURE_LIST
    printf("CDTEXT ERROR: Try #define IGNORE_FEATURE_LIST in libwm/scsi.c\n");
#else
    cdtext_possible = 1;
    printf("CDTEXT INFO: GET_FEATURY_LIST(0x46) ignored. It's OK, becose many CDROMS don't implement this featury\n");
#endif /* IGNORE_FEATURE_LIST */
  }
  else
  {
    feature_list_length = temp[0]*0xFFFFFF + temp[1]*0xFFFF + temp[2]*0xFF + temp[3] + 4;

    dynamic_temp = malloc(feature_list_length);

    if(!dynamic_temp)
      return -1;

    memset(dynamic_temp, 0, feature_list_length);
    ret = sendscsi(d, dynamic_temp, feature_list_length, CGC_DATA_READ,
      0x46, 0x02, 0x00, 0x1E, 0, 0,
      0, (feature_list_length>>8) & 0xFF, feature_list_length & 0xFF, 0, 0, 0);


#ifndef IGNORE_FEATURE_LIST
    if(!ret)
    {
      pHeader = (struct feature_list_header*)dynamic_temp;
/*     printf("length = %i, profile = 0x%02X%02X\n", pHeader->lenght_lsb, pHeader->profile_msb, pHeader->profile_lsb);*/
      pDescriptor = (struct feature_descriptor_cdread*)(dynamic_temp + sizeof(struct feature_list_header));
/*     printf("code = 0x%02X%02X, settings = 0x%02X, add_length = %i, add_settings = 0x%02X \n",
         pDescriptor->feature_code_msb, pDescriptor->feature_code_lsb, pDescriptor->settings,
         pDescriptor->add_lenght, pDescriptor->add_settings);*/

      cdtext_possible = pDescriptor->add_settings & 0x01;
    }
    else
    {
      cdtext_possible = 0;
    }

#else
    cdtext_possible = 1;
#endif /* IGNORE_FEATURE_LIST */	

    free (dynamic_temp);
    dynamic_temp = 0;
  }

  if(!cdtext_possible)
  {
    printf("CDTEXT INFO: GET_FEATURY_LIST(0x46) says, CDTEXT is not present!\n");
    return EXIT_SUCCESS;
  }

  printf("CDTEXT INFO: try to read, how long CDTEXT is?\n");
  ret = sendscsi(d, temp, 4, CGC_DATA_READ,
    SCMD_READ_TOC, 0x00, 0x05, 0, 0, 0,
    0, 0, 4, 0, 0, 0);

  if(ret)
  {
    printf("CDTEXT ERROR: READ_TOC(0x43) with format code 0x05 not implemented or broken. ret = %i!\n", ret);
  }
  else
  {
    cdtext_data_length = temp[0]*0xFF + temp[1] + 4 + 1; /* divide by 18 + 4 ? */
    /* cdtext_data_length%18 == 0;? */
    printf("CDTEXT INFO: CDTEXT is a %i byte(s) long\n", cdtext_data_length);
    /* cdc_buffer[2];  cdc_buffer[3]; reserwed */
    dynamic_temp = malloc(cdtext_data_length);
    if(!dynamic_temp)
      return -1;

    memset(dynamic_temp, 0, cdtext_data_length);
    printf("CDTEXT INFO: try to read CDTEXT\n");
    ret = sendscsi(d, dynamic_temp, cdtext_data_length, CGC_DATA_READ,
      SCMD_READ_TOC, 0x00, 0x05, 0, 0, 0,
      0, (cdtext_data_length>>8) & 0xFF, cdtext_data_length & 0xFF, 0, 0, 0);
   
    if(ret)	
    {
      printf("CDTEXT ERROR: READ_TOC(0x43) with format code 0x05 not implemented or broken. ret = %i!\n", ret);
    }
    else
    {
      cdtext_data_length = temp[0]*0xFF + temp[1] + 4 + 1; /* divide by 18 + 4 ? */
      printf("CDTEXT INFO: read %i byte(s) of CDTEXT\n", cdtext_data_length);

      /* send cdtext only 18 bytes packs * ? */
      *(p_buffer_length) = cdtext_data_length - 4;
      *pp_buffer = malloc(*p_buffer_length);
      if(!(*pp_buffer))
      {
        return -1;
      }
      memcpy(*pp_buffer, dynamic_temp + 4, *p_buffer_length);
    }
    free(dynamic_temp);
    dynamic_temp = 0;
  }

  return ret;
} /* wm_scsi_get_cdtext() */

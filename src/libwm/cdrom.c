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
 * Interface between most of WorkMan and the low-level CD-ROM library
 * routines defined in plat_*.c and drv_*.c.  The goal is to have no
 * platform- or drive-dependent code here.
 */

static char cdrom_id[] = "$Id$";

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
/* #include <sys/time.h> */

#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_cddb.h"
#include "include/wm_cdrom.h"
#include "include/wm_database.h"
#include "include/wm_platform.h"
#include "include/wm_helpers.h"
#include "include/wm_cdinfo.h"
#include "include/wm_cdtext.h"

#ifdef CAN_CLOSE
#include <fcntl.h>
#endif

#define WM_MSG_CLASS WM_MSG_CLASS_CDROM

/* extern struct wm_drive generic_proto, toshiba_proto, sony_proto; */
/*	toshiba33_proto; <=== Somehow, this got lost */

/*
 * The supported drive types are listed here.  NULL means match anything.
 * The first match in the list is used, and substring matches are done (so
 * put long names before their shorter prefixes.)
 */
struct drivelist {
	char		*ven;
	char		*mod;
	char		*rev;
	struct wm_drive	*proto;
} drives[] = {
{	"TOSHIBA",		"XM-3501",		NULL,		&toshiba_proto	},
{	"TOSHIBA",		"XM-3401",		NULL,		&toshiba_proto	},
{	"TOSHIBA",		"XM-3301",		NULL,		&toshiba_proto	},
{	"SONY",			"CDU-8012",		NULL,		&sony_proto	},
{	"SONY",			"CDU 561",		NULL,		&sony_proto	},
{       "SONY",         	"CDU-76S",      	NULL,   	&sony_proto	},
{	WM_STR_GENVENDOR,	WM_STR_GENMODEL,	WM_STR_GENREV,	&generic_proto  },
{	NULL,			NULL,			NULL,		&generic_proto	}
};

/*
 * Solaris 2.2 will remove the device out from under us.  Getting an ENOENT
 * is therefore sometimes not a problem.
 */
int	intermittent_dev = 0;

/*
 * Do we want to keep the CD device open after quitting by default?
 *
   int	keep_open = 0;
 */

#if defined DEFAULT_CD_DEVICE
char	*cd_device = DEFAULT_CD_DEVICE;
#else
char	*cd_device = NULL;
#endif


int wm_cd_cur_balance = 10;

struct wm_drive	drive = { -1, "", "", "", NULL, NULL };

char _wm_drive_vendor[32] = 	"Generic";
char _wm_drive_model[32] = 	"drive type";
char _wm_drive_revision[32] =	"";

/*
 * Give information about the drive we found during wmcd_open()
 */

char *wm_drive_vendor( void )
{
	char *s = NULL;

	wm_strmcpy( &s, _wm_drive_vendor );
	return s;
}

char *wm_drive_model( void )
{
	char *s = NULL;

	wm_strmcpy( &s, _wm_drive_model );
	return s;
}

char *wm_drive_revision( void )
{
	char *s = NULL;

	wm_strmcpy( &s, _wm_drive_revision );
	return s;
}

void wm_drive_settype( char *vendor, char *model, char *revision )
{
	sprintf( _wm_drive_vendor, "%s", vendor );
	sprintf( _wm_drive_model, "%s", model );
	sprintf( _wm_drive_revision, "%s", revision );
}

/*
 * Figure out which prototype drive structure we should be using based
 * on the vendor, model, and revision of the current drive.
 */
struct wm_drive *
find_drive_struct(char *vendor, char *model, char *rev)
{
	struct drivelist	*d;

	for (d = drives; d; d++)
	{
		if( ( (d->ven != NULL) && strncmp(d->ven, vendor, strlen(d->ven)) ) ||
		    ( (d->mod != NULL) && strncmp(d->mod, model, strlen(d->mod)) ) ||
		    ( (d->rev != NULL) && strncmp(d->rev, rev, strlen(d->rev)) ) )
			continue;
		
		if (d->proto->vendor[0] == '\0')
			strcpy(d->proto->vendor, vendor);
		if (d->proto->model[0] == '\0')
			strcpy(d->proto->model, model);

		return (d->proto);
	}

	return (NULL);	/* this means the list is badly terminated. */
} /* find_drive_struct() */

/*
 * read_toc()
 *
 * Read the table of contents from the CD.  Return a pointer to a wm_cdinfo
 * struct containing the relevant information (minus artist/cdname/etc.)
 * This is a static struct.  Returns NULL if there was an error.
 *
 * XXX allocates one trackinfo too many.
 */
struct wm_cdinfo *
read_toc()
{

        struct wm_playlist *l;
        int    i;
        int    pos;
	if ((drive.get_trackcount)(&drive, &thiscd.ntracks) < 0)
	{
		perror("trackcount");
		return (NULL);
	}

	thiscd.artist[0] = thiscd.cdname[0] = '\0';
	thiscd.whichdb = thiscd.otherrc = thiscd.otherdb = thiscd.user = NULL;
	thiscd.length = 0;
	thiscd.autoplay = thiscd.playmode = thiscd.volume = 0;

	/* Free up any left-over playlists. */
	if (thiscd.lists != NULL)
	{
		for (l = thiscd.lists; l->name != NULL; l++)
		{
			free(l->name);
			free(l->list);
		}
		free(thiscd.lists);
		thiscd.lists = NULL;
	}

	if (thiscd.trk != NULL)
		free(thiscd.trk);

	thiscd.trk = malloc((thiscd.ntracks + 1) * sizeof(struct wm_trackinfo));
	if (thiscd.trk == NULL)
	{
		perror("malloc");
		return (NULL);
	}

	for (i = 0; i < thiscd.ntracks; i++)
	{
		if ((drive.get_trackinfo)(&drive, i + 1, &thiscd.trk[i].data,
					&thiscd.trk[i].start) < 0)
		{
			perror("CD track info read");
			return (NULL);
		}

		thiscd.trk[i].avoid = thiscd.trk[i].data;
		thiscd.trk[i].length = thiscd.trk[i].start / 75;

		thiscd.trk[i].songname = thiscd.trk[i].otherrc =
		thiscd.trk[i].otherdb = NULL;
		thiscd.trk[i].contd = 0;
		thiscd.trk[i].volume = 0;
		thiscd.trk[i].track = i + 1;
		thiscd.trk[i].section = 0;
	}

	if ((drive.get_cdlen)(&drive, &thiscd.trk[i].start) < 0)
	{
		perror("CD length read");
		return (NULL);
	}
	thiscd.trk[i].length = thiscd.trk[i].start / 75;

/* Now compute actual track lengths. */
	pos = thiscd.trk[0].length;

	for (i = 0; i < thiscd.ntracks; i++)
	{
		thiscd.trk[i].length = thiscd.trk[i+1].length - pos;
		pos = thiscd.trk[i+1].length;
		if (thiscd.trk[i].data)
			thiscd.trk[i].length = (thiscd.trk[i + 1].start -
				thiscd.trk[i].start) * 2;
		if (thiscd.trk[i].avoid)
			wm_strmcpy(&thiscd.trk[i].songname, "DATA TRACK");
	}

	thiscd.length = thiscd.trk[thiscd.ntracks].length;
        thiscd.cddbid = cddb_discid(drive);
        
	wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS_MISC, "read_toc() returning &thiscd\n");
	return (&thiscd);
} /* read_toc() */

/*
 * wm_cd_status()
 *
 * Return values:
 *
 *	0	No CD in drive.
 *	1	CD in drive.
 *	2	CD has just been inserted (TOC has been read)
 *
 * Updates cur_track, cur_pos_rel, cur_pos_abs and other variables.
 */
int
wm_cd_status( void )
{
	static enum wm_cd_modes	oldmode = WM_CDM_UNKNOWN;
	enum wm_cd_modes		mode;
	int			status, trackno = cur_track;
	int			ret = WM_CDS_DISC_READY;


	if( cur_cdmode == WM_CDM_DEVICECHANGED )
	  {
	    /* Don't open the device now, just change the mode to ejected */
	    cur_cdmode = WM_CDM_EJECTED;
	    status = 0;
	  } else {
	    /* Open the drive.  This returns 1 if the device isn't ready. */
	    status = wmcd_open( &drive );
	    if (status < 0)
	      return (status);
	    if (status > 0)
	      return (WM_CDS_NO_DISC);
	  }

	/* If the user hit the stop button, don't pass PLAYING as oldmode.
         * Likewise, if we've just started playing, don't remember that
         * we were stopped before (or the state machine in get_drive_status
         * can get confused.)
         */
	if( (cur_cdmode == WM_CDM_STOPPED) || (cur_cdmode == WM_CDM_PLAYING) )
		oldmode = cur_cdmode;

	if( (drive.get_drive_status)(&drive, oldmode, &mode, &cur_frame,
					&trackno, &cur_index) < 0)
	{
		perror("CD get drive status");
		return (-1);
	}
	oldmode = mode;

	if (mode == WM_CDM_EJECTED || mode == WM_CDM_UNKNOWN)
	{
		cur_cdmode = WM_CDM_EJECTED;
		cur_track = -1;
		cur_cdlen = cur_tracklen = 1;
		cur_pos_abs = cur_pos_rel = cur_frame = 0;

		if (exit_on_eject)
			exit(0);

		return (WM_CDS_NO_DISC);
	}

	/* If there wasn't a CD before and there is now, learn about it. 
	 * If the device has changed, this will close the old fd and     
         * re-open the device before gathering information */
	if (cur_cdmode == WM_CDM_EJECTED)
	{
		cur_pos_rel = cur_pos_abs = 0;

		status = wmcd_reopen( &drive );

		if ((cd = read_toc()) == NULL)
                {
		        wm_lib_message(WM_MSG_LEVEL_DEBUG|WM_MSG_CLASS_MISC,"status: returned toc was NULL\n");
			if (exit_on_eject)
			{
				exit(-1);
			} else {
				return (-1);
			}
		}

		cur_nsections = 0;
		cur_ntracks = cd->ntracks;
		cur_cdlen = cd->length;
		cur_artist = cd->artist;
		cur_cdname = cd->cdname;
		cur_cdmode = WM_CDM_STOPPED;
		ret = WM_CDS_JUST_INSERTED;
	}

	switch (mode) {
	case WM_CDM_PLAYING:
	case WM_CDM_PAUSED:
		cur_pos_abs = cur_frame / 75;

		/* Only look up the current track number if necessary. */
		if (cur_track < 1 || cur_frame < cd->trk[cur_track-1].start ||
				cur_frame >= (cur_track >= cur_ntracks ?
				(cur_cdlen + 1) * 75 :
				cd->trk[cur_track].start))
		{
			cur_track = 0;
			while (cur_track < cur_ntracks && cur_frame >=
					cd->trk[cur_track].start)
				cur_track++;
		}
		if (cur_track >= 1 && trackno > cd->trk[cur_track-1].track)
			cur_track++;
		/* Fall through */

	case WM_CDM_UNKNOWN:
		if (mode == WM_CDM_UNKNOWN)
		{
			mode = WM_CDM_STOPPED;
			cur_lasttrack = cur_firsttrack = -1;
		}
		/* Fall through */

	case WM_CDM_STOPPED:
		if (cur_track >= 1 && cur_track <= cur_ntracks)
		{
			cur_trackname = cd->trk[cur_track-1].songname;
			cur_avoid = cd->trk[cur_track-1].avoid;
			cur_contd = cd->trk[cur_track-1].contd;
			cur_pos_rel = (cur_frame -
				cd->trk[cur_track-1].start) / 75;
			if (cur_pos_rel < 0)
				cur_pos_rel = -cur_pos_rel;
		}

		if( (playlist != NULL) && playlist[0].start & (cur_listno > 0))
		{
			cur_pos_abs -= cd->trk[playlist[cur_listno-1].
				start - 1].start / 75;
			cur_pos_abs += playlist[cur_listno-1].starttime;
		}
		if (cur_pos_abs < 0)
			cur_pos_abs = cur_frame = 0;

		if (cur_track < 1)
			cur_tracklen = cd->length;
		else
			cur_tracklen = cd->trk[cur_track-1].length;
		/* Fall through */

	case WM_CDM_TRACK_DONE:
		cur_cdmode = mode;
		break;
	case WM_CDM_FORWARD:
	case WM_CDM_EJECTED:
		break;	
	}

	return (ret);
}

#undef CLIF_VOL 
#ifdef CLIF_VOL
/*
 * cd_volume(vol, bal, max)
 *
 * Set the volume levels.  "vol" and "bal" are the volume and balance knob
 * settings, respectively.  "max" is the maximum value of the volume knob
 * (the balance knob is assumed to always go from 0 to 20.)
 */
void
cd_volume(vol, bal, max)
	int	vol, bal, max;
{
	int	left, right, scale;

/*
 * Set "left" and "right" to volume-slider values accounting for the
 * balance setting.
 */
/*	printf("Vol = %d, Bal = %d, Max = %d\n", vol, bal, max);
*/

        vol  = (vol * 100 + max - 16) / max;  
	scale = (vol + 5) / 10;

        if (bal < 9)
	{
                right = vol - scale * (10 - bal);
#ifdef SYMETRIC_BALANCE
		left  = vol + scale * (10 - bal);
#else
		left = vol;
#endif
        }
	else if (bal > 11)
	{
#ifdef SYMETRIC_BALANCE		
                right = vol + scale * (bal - 10);
#else
		right = vol;
#endif
		left  = vol - scale * (bal - 10);
        }
	else
                left = right = vol;

/*
 * some plat_*.c is missing the limitation
 */
	left = left < 0 ? 0 : left > 100 ? 100 : left;
	right = right < 0 ? 0 : right > 100 ? 100 : right;
/*	printf("Left = %d, Right = %d\n", left, right);
*/
	(void) (drive.set_volume)(&drive, left, right);
} /* cd_volume() */

#else

/*
 * cd_volume(vol, bal, max)
 *
 * Set the volume levels.  "vol" and "bal" are the volume and balance knob
 * settings, respectively.  "max" is the maximum value of the volume knob
 * (the balance knob is assumed to always go from 0 to 20.)
 */
void
cd_volume( int vol, int bal, int max )
{
	int	left, right;

/*
 * Set "left" and "right" to volume-slider values accounting for the
 * balance setting.
 *
 * XXX - the maximum volume setting is assumed to be in the 20-30 range.
 */
	if (bal < 9)
		right = vol - (9 - bal) * 2;
	else
		right = vol;
	if (bal > 11)
		left = vol - (bal - 11) * 2;
	else
		left = vol;

	left = (left * 100 + max - 1) / max;
	right = (right * 100 + max - 1) / max;
	if (left > 100)
		left = 100;
	if (right > 100)
		right = 100;

	(void) (drive.set_volume)(&drive, left, right);
} /* cd_volume() */

#endif /* CLIF_VOL */


/*
 * wm_cd_pause()
 *
 * Pause the CD, if it's in play mode.  If it's already paused, go back to
 * play mode.
 */
void
wm_cd_pause( void )
{
	static int paused_pos;

	if (cur_cdmode == WM_CDM_EJECTED)	/* do nothing if there's no CD! */
		return;

	switch (cur_cdmode) {
	case WM_CDM_PLAYING:		/* playing */
		cur_cdmode = WM_CDM_PAUSED;
		(drive.pause)(&drive);
                paused_pos = cur_pos_rel;
		break;

	case WM_CDM_PAUSED:		/* paused */
		cur_cdmode = WM_CDM_PLAYING;
/*		(drive.resume)(&drive); */
		if ((drive.resume)(&drive) > 0 )
                  {
			wm_cd_play(cur_track, paused_pos,
				playlist[cur_listno-1].end);
                  }
	default: /* */
		break;	
	}
} /* wm_cd_pause() */

/*
 * wm_cd_stop()
 *
 * Stop the CD if it's not already stopped.
 */
void
wm_cd_stop( void )
{
	if (cur_cdmode == WM_CDM_EJECTED)
		return;

	if (cur_cdmode != WM_CDM_STOPPED)
	{
		cur_lasttrack = cur_firsttrack = -1;
		cur_cdmode = WM_CDM_STOPPED;
		(drive.stop)(&drive);
		cur_track = 1;
	}
} /* wm_cd_stop() */


void
wm_cd_get_cdtext( void )
{
	wm_get_cdtext(&drive);
}

/*
 * wm_cd_play_chunk(start, end)
 *
 * Play the CD from one position to another (both in frames.)
 */
void
wm_cd_play_chunk( int start, int end, int realstart )
{
	if (cur_cdmode == WM_CDM_EJECTED || cd == NULL)
		return;

	end--;
	if (start >= end)
		start = end-1;

	(drive.play)(&drive, start, end, realstart);
}

/*
 * wm_cd_play(starttrack, pos, endtrack)
 *
 * Start playing the CD or jump to a new position.  "pos" is in seconds,
 * relative to start of track.
 */
void
wm_cd_play( int start, int pos, int end )
{
      printf("insert\n");

	if (cur_cdmode == WM_CDM_EJECTED || cd == NULL){
            printf("1\n");
		return;
	}

        /*
         * Try to avoid mixed mode and CD-EXTRA data tracks
         */
        if( (start == 1) && (cd->trk[start-1].data == 1))
          {
            start++;
          }

        /* -2: -1 (array-index) + -1 (leadout->last track)*/
        if( (cd->trk[end-2].data == 1 ) )
          {
            end--;
          }

        /* CD-EXTRA: The data track was requested to play.
         * Play the last audio track instead.
         */
        if( start >= end )
          {
            start = end-1;
          }

	cur_firsttrack = start;
	start--;
	end--;
	cur_lasttrack = end;

        /* Don't play if it's just one data track or still a data track */
        if( start < 0 ) start = 0;
        if( cd->trk[start].data == 1 )
          {
            printf("status start track\n");
            wm_cd_status();
            cur_cdmode = WM_CDM_STOPPED;
            printf("2");
            return;
          }
        printf("play chunk\n");
        wm_cd_play_chunk(cd->trk[start].start + pos * 75, end >= cur_ntracks ?
                         cur_cdlen * 75 : cd->trk[end].start - 1,
                         cd->trk[start].start);
	/* So we don't update the display with the old frame number */
	printf("status\n");
	wm_cd_status();
	cur_frame = cd->trk[start].start + pos * 75;
	cur_track = cur_firsttrack;
	cur_cdmode = WM_CDM_PLAYING;
}

/*
 * Set the offset into the current track and play.  -1 means end of track
 * (i.e., go to next track.)
 */
void
wm_cd_play_from_pos( int pos )
{
	if (pos == -1)
		if (cd)
			pos = cd->trk[cur_track - 1].length - 1;
	if (cur_cdmode == WM_CDM_PLAYING)
		wm_cd_play(cur_track, pos, playlist[cur_listno-1].end);
} /* wm_cd_play_from_pos() */

/*
 * Eject the current CD, if there is one, and set the mode to 5.
 *
 * Returns 0 on success, 1 if the CD couldn't be ejected, or 2 if the
 * CD contains a mounted filesystem.
 */
int
wm_cd_eject( void )
{
	int	status;

	status = (drive.eject)(&drive);
	if (status < 0)
	{
		if (status == -3)
		{
			return (2);
		} else {
			return (1);
		}
	}
	
	if (exit_on_eject)
		exit(0);

	cur_track = -1;
	cur_cdlen = cur_tracklen = 1;
	cur_pos_abs = cur_pos_rel = cur_frame = 0;
	cur_cdmode = WM_CDM_EJECTED;

	return (0);
}

int wm_cd_closetray(void)
{
	return((drive.closetray)(&drive) ? 0 : wm_cd_status()==2 ? 1 : 0);
} /* wm_cd_closetray() */

/*
 * find_trkind(track, index, start)
 *
 * Start playing at a particular track and index, optionally using a particular
 * frame as a starting position.  Returns a frame number near the start of the
 * index mark if successful, 0 if the track/index didn't exist.
 *
 * This is made significantly more tedious (though probably easier to port)
 * by the fact that CDROMPLAYTRKIND doesn't work as advertised.  The routine
 * does a binary search of the track, terminating when the interval gets to
 * around 10 frames or when the next track is encountered, at which point
 * it's a fair bet the index in question doesn't exist.
 */
int
find_trkind( int track, int index, int start )
{
	int	top = 0, bottom, current, interval, ret = 0, i;

	if( cur_cdmode == WM_CDM_EJECTED || cd == NULL )
		return ( 0 ); /* WARNING: was nothing */

	for (i = 0; i < cur_ntracks; i++)
		if (cd->trk[i].track == track)
			break;
	bottom = cd->trk[i].start;

	for (; i < cur_ntracks; i++)
		if (cd->trk[i].track > track)
			break;

	top = i == cur_ntracks ? (cd->length - 1) * 75 : cd->trk[i].start;

	if (start > bottom && start < top)
		bottom = start;

	current = (top + bottom) / 2;
	interval = (top - bottom) / 4;

	do {
		wm_cd_play_chunk(current, current + 75, current);

		if (wm_cd_status() != 1)
			return (0);
		while (cur_frame < current)
			if (wm_cd_status() != 1 || cur_cdmode != WM_CDM_PLAYING)
				return (0);
			else
				wm_susleep(1);

		if (cd->trk[cur_track - 1].track > track)
			break;

		if (cur_index >= index)
		{
			ret = current;
			current -= interval;
		}
		else
			current += interval;
		interval /= 2;
	} while (interval > 2);

	return (ret);
} /* find_trkind() */

/*
 * Read the initial volume from the drive, if available.  Set cur_balance to
 * the balance level (0-20, 10=centered) and return the proper setting for
 * the volume knob.
 *
 * "max" is the maximum value of the volume knob.
 */
int
wm_cd_read_initial_volume( int max )
{
	int	left, right;

	if ((drive.get_volume)(&drive, &left, &right) < 0 || left == -1)
		return (max);

	left = (left * max + 99) / 100;
	right = (right * max + 99) / 100;

	if (left < right)
	{
		wm_cd_cur_balance = (right - left) / 2 + 11;
		if (wm_cd_cur_balance > 20)
			wm_cd_cur_balance = 20;

		return (right);
	}
	else if (left == right)
	{
		wm_cd_cur_balance = 10;
		return (left);
	}
	else
	{
		wm_cd_cur_balance = (right - left) / 2 + 9;
		if (wm_cd_cur_balance < 0)
			wm_cd_cur_balance = 0;

		return (left);
	}
} /* wm_cd_read_initial_volume() */

/*
 * Prototype wm_drive structure, with generic functions.  The generic functions
 * will be replaced with drive-specific functions as appropriate once the drive
 * type has been sensed.
 */
struct wm_drive generic_proto = {
	-1,			/* fd */
	"Generic\0",		/* vendor */
	"drive type\0     ",	/* model */
	"\0",			/* revision */
	NULL,			/* aux */
	NULL,			/* daux */

	gen_init,		/* functions... */
	gen_get_trackcount,
	gen_get_cdlen,
	gen_get_trackinfo,
	gen_get_drive_status,
	gen_get_volume,
	gen_set_volume,
	gen_pause,
	gen_resume,
	gen_stop,
	gen_play,
	gen_eject,
	gen_closetray,
	gen_get_cdtext
};

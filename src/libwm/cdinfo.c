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
 * Get information about a CD.
 */

static char cdinfo_id[] = "$Id$";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "include/wm_config.h"

#include "include/wm_struct.h"
#include "include/wm_cdrom.h"
#include "include/wm_cdinfo.h"
#include "include/wm_database.h"
#include "include/wm_helpers.h"

struct wm_play *playlist = NULL;
struct wm_cdinfo thiscd, *cd = &thiscd;

int	cur_track = -1;	/* Current track number, starting at 1 */
int	cur_index = 0;	/* Current index mark */
int	cur_lasttrack = 999;	/* Last track to play in current chunk */
int	cur_firsttrack = 0;	/* First track of current chunk */
int	cur_pos_abs;	/* Current absolute position in seconds */
int	cur_frame;	/* Current frame number */
int	cur_pos_rel;	/* Current track-relative position in seconds */
int	cur_tracklen;	/* Length in seconds of current track */
int	cur_cdlen;	/* Length in seconds of entire CD */
int	cur_ntracks;	/* Number of tracks on CD (= tracks + sections) */
int	cur_nsections;	/* Number of sections currently defined */
enum wm_cd_modes	cur_cdmode = WM_CDM_EJECTED;
int	cur_listno;	/* Current index into the play list, if playing */
char *	cur_artist;	/* Name of current CD's artist */
char *	cur_cdname;	/* Album name */
char *	cur_trackname;	/* Take a guess */
char	cur_contd;	/* Continued flag */
char	cur_avoid;	/* Avoid flag */

int	exit_on_eject = 0;

int cur_stopmode = -1;
int info_modified;

/*
 * insert_trackinfo()
 *
 * Add a new track to the CD info structure.  Pass the position of the new
 * entry in the track list -- 0 will make this the first track, 1 the second,
 * etc.  The new entry will be zeroed out.
 */
void
insert_trackinfo(num)
	int	num;
{
	struct wm_trackinfo *newtrk;

	/* Easy case: the list is empty */
	if (cd->trk == NULL) {
		if ((cd->trk = (struct wm_trackinfo *) calloc(1,
						sizeof(*newtrk))) == NULL)
		{
nomem:
			perror("insert_trackinfo");
			exit(1);
		} else {
			return;
                } /* if() else */
        } /* if() */
	/* Stick the new entry in cd->trk[]. */
	if ((newtrk = (struct wm_trackinfo *) malloc(sizeof(*newtrk) *
						(cur_ntracks + 1))) == NULL)
		goto nomem;

	if (num)
		memcpy(newtrk, cd->trk, sizeof(*newtrk) * num);
	memset(&newtrk[num], 0, sizeof(*newtrk));
	if (num < cur_ntracks)
		memcpy(&newtrk[num + 1], &cd->trk[num], sizeof(*newtrk) *
			(cur_ntracks - num));

	free(cd->trk);
	cd->trk = newtrk;
}

/*
 * split_trackinfo()
 *
 * Split a track in two at a particular position (absolute, in frames).  All
 * internal data structures and variables will be adjusted to the new
 * numbering scheme.  Pass in the track number (>=1) to split, which is also
 * the index into cd->trk[] of the new entry.
 *
 * If pos is within 1 second of the start of another track, the split fails.
 *
 * Returns 1 on success, 0 if the track couldn't be inserted.
 *
 * Note: updating user interface elements is up to the caller.
 */
int
split_trackinfo( int pos )
{
	int	i, l, num;

	if (pos < cd->trk[0].start)
		return (0);

	/* First find the appropriate track. */
	for (num = 0; num < cur_ntracks; num++)
		if (cd->trk[num].start - 75 < pos &&
						cd->trk[num].start + 75 > pos)
			return (0);
		else if (cd->trk[num].start > pos)
			break;
	if (num == 0)
		return (0);

	/* Insert the new entry into the track array. */
	insert_trackinfo(num);

	/* Update the easy variables. */
	if (cur_track > num)
		cur_track++;
	if (cur_firsttrack > num)
		cur_firsttrack++;
	if (cur_lasttrack > num)
		cur_lasttrack++;

	/* Update the user-defined playlists. */
	if (cd->lists != NULL)
		for (l = 0; cd->lists[l].name != NULL; l++)
			if (cd->lists[l].list != NULL)
				for (i = 0; cd->lists[l].list[i]; i++)
					if (cd->lists[l].list[i] > num)
						cd->lists[l].list[i]++;

	/* Update the internal playlist. */
	if (playlist != NULL)
		for (i = 0; playlist[i].start; i++)
		{
			if (playlist[i].start > num)
				playlist[i].start++;
			if (playlist[i].end > num)
				playlist[i].end++;
		}
	
	/* Now adjust the information in cd->trk[]. */
	cd->trk[num].start = pos;
	if (num == cur_ntracks)
		cd->trk[num].length = cur_cdlen - pos / 75;
	else
		cd->trk[num].length = (cd->trk[num + 1].start - pos) / 75;
	cd->trk[num - 1].length -= cd->trk[num].length;
	if (cur_track == num)
		cur_tracklen -= cd->trk[num].length;
	cd->trk[num].track = cd->trk[num - 1].track;
	cd->trk[num].data = cd->trk[num - 1].data;
	cd->trk[num].contd = 1;
	cd->trk[num].volume = cd->trk[num - 1].volume;

	if (cd->trk[num - 1].section == 0)
		cd->trk[num - 1].section = 1;
	cd->trk[num].section = cd->trk[num - 1].section + 1;

	cur_ntracks++;
	cur_nsections++;

	for (i = num + 1; i < cur_ntracks; i++)
		if (cd->trk[i].track == cd->trk[num].track)
			cd->trk[i].section++;
	
	return (1);
}

/*
 * remove_trackinfo()
 *
 * Remove a track's internal data.  This is similar to split_trackinfo()
 * above, but simpler.  A track's initial section can't be removed.  Track
 * numbers start at 0.
 *
 * Returns 1 on success, 0 on failure.
 */
int
remove_trackinfo( int num )
{
	int	i, l;

	if (num < 1 || num >= cur_ntracks || cd->trk[num].section < 2)
		return (0);
	
	cd->trk[num - 1].length += cd->trk[num].length;

	for (i = num; i < cur_ntracks - 1; i++)
		memcpy(&cd->trk[i], &cd->trk[i + 1], sizeof(cd->trk[0]));

	if (cur_track > num)
		cur_track--;
	if (cur_firsttrack > num)
		cur_firsttrack--;
	if (cur_lasttrack > num)
		cur_lasttrack--;
	
	/* Update the user-defined playlists. */
	if (cd->lists != NULL)
		for (l = 0; cd->lists[l].name != NULL; l++)
			if (cd->lists[l].list != NULL)
				for (i = 0; cd->lists[l].list[i]; i++)
					if (cd->lists[l].list[i] > num)
						cd->lists[l].list[i]--;
	
	/* Update the internal playlist. */
	if (playlist != NULL)
		for (i = 0; playlist[i].start; i++)
		{
			if (playlist[i].start > num)
				playlist[i].start--;
			if (playlist[i].end > num)
				playlist[i].end--;
		}
	
	cur_ntracks--;
	cur_nsections--;

	/*
	 * Update the section numbers for this track.  If this is the only
	 * user-created section in a track, get rid of the section number
	 * in the track's entry.
	 */
	if (num == cur_ntracks || cd->trk[num - 1].track != cd->trk[num].track)
	{
		if (cd->trk[num - 1].section == 1)
			cd->trk[num - 1].section = 0;
	}
	else
		for (i = num; i < cur_ntracks; i++)
			if (cd->trk[i].track == cd->trk[num - 1].track)
				cd->trk[i].section--;

	return (1);
}

/*
 * listentry()
 *
 * Return a scrolling list entry.
 */
char *
listentry( int num )
{
	static char	buf[600];
	char		*name, tracknum[20];
	int		digits;
	int		sdigits;

	if (num >= 0 && num < cur_ntracks)
	{

/*
		if (big_spaces)
		{
			digits = 2;
			sdigits = cur_nsections < 9 ? -1 : -2;
		}
		else
		{
			digits = cd->trk[num].track < 10 ? 3 : 2;
			sdigits = cur_nsections < 9 ? -1 : -3;
		}
*/

		digits = 2;
		sdigits = cur_nsections < 9 ? -1 : -2;
		
		name = cd->trk[num].songname ? cd->trk[num].songname : "";

		if (cur_nsections)
	        {
    			if (cd->trk[num].section > 9) 
			{
				sprintf(tracknum, "%*d.%d", digits,
					cd->trk[num].track,
					cd->trk[num].section);
			} else {
				if (cd->trk[num].section)
				{
					sprintf(tracknum, "%*d.%*d", digits,
						cd->trk[num].track, sdigits,
						cd->trk[num].section);
				} else {
					sprintf(tracknum, "%*d%*s", digits,
						cd->trk[num].track,
						2 - sdigits, " ");
/*						2 - sdigits - big_spaces, " ");*/
				}
			}
		} else {
			sprintf(tracknum, "%*d", digits, cd->trk[num].track);
		}

		if (cd->trk[num].data)
		{
			sprintf(buf, "%s) %3dMB %s", tracknum,
				cd->trk[num].length / 1024, name);
		} else {
			sprintf(buf, "%s) %02d:%02d %s", tracknum,
				cd->trk[num].length / 60,
				cd->trk[num].length % 60, name);
                }

		return (buf);
	} else {
		return (NULL);
        }
} /* listentry() */

/*
 * trackname()
 *
 * Return a track's name.
 */
char *
trackname( int num )
{
	if (num >= 0 && num < cur_ntracks) 
	{
		if (cd->trk[num].songname)
		{
			return (cd->trk[num].songname);
		} else {
			return ("");
		}
	} else {
		return (NULL);
	}
} /* trackname() */

/*
 * tracklen()
 *
 * Return a track's length in seconds.
 */
int
tracklen( int num )
{
	if (cd != NULL && num >= 0 && num < cur_ntracks)
		return (cd->trk[num].length);
	else
		return (0);
}

/*
 * get_default_volume()
 *
 * Return the default volume (0-32, 0=none) for the CD or a track.
 */
int
get_default_volume( int track )
{
	if (! track)
		return (cd->volume);
	else if (track <= cur_ntracks)
		return (cd->trk[track - 1].volume);
	else
		return (0);
}

/*
 * get_contd()
 *
 * Return the contd value for a track.
 */
int
get_contd( int num )
{
	if (num >= 0 && num < cur_ntracks)
		return (cd->trk[num].contd);
	else
		return (0);
}

/*
 * get_avoid()
 *
 * Return the avoid value for a track.
 */
int
get_avoid( int num )
{
	if (num >= 0 && num < cur_ntracks)
		return (cd->trk[num].avoid);
	else
		return (0);
}

/*
 * get_autoplay()
 *
 * Is autoplay set on this disc?
 */
int
get_autoplay( void )
{
	return ( cd->autoplay );
}

/*
 * get_playmode()
 *
 * Return the default playmode for the CD.
 */
int
get_playmode( void )
{
	return ( cd->playmode );
}

/*
 * get_runtime()
 *
 * Return the total running time for the current playlist in seconds.
 */
int
get_runtime( void )
{
	int	i;

	if (playlist == NULL || playlist[0].start == 0 || cur_firsttrack == -1)
		return (cd == NULL ? 0 : cd->length);

	for (i = 0; playlist[i].start; i++)
		;

	return (playlist[i].starttime);
}

/*
 * default_volume()
 *
 * Set the default volume for the CD or a track.
 */
void
default_volume( int track, int vol )
{
	if (track == 0)
		cd->volume = vol;
	else if (track <= cur_ntracks)
		cd->trk[track - 1].volume = vol;
}

/*
 * Play the next thing on the playlist, if any.
 */
void
play_next_entry( int forward )
{
	if (cd == NULL)
		return;
	if (playlist != NULL && playlist[cur_listno].start)
	{
		wm_cd_play(playlist[cur_listno].start, 0,
			playlist[cur_listno].end);
		cur_listno++;
	}
	else
		wm_cd_stop();
}

/*
 * Play the next track, following playlists as necessary.
 */
void
play_next_track( int forward )
{
	if (cd == NULL)
		return;

	/* Is the current playlist entry done?  Move on, if so. */
	if (playlist == NULL || cur_track + 1 == playlist[cur_listno - 1].end)
		play_next_entry( forward );
	else
		wm_cd_play(cur_track + 1, 0, playlist[cur_listno - 1].end);
}

/*
 * Play the previous track, hopping around the playlist as necessary.
 */
void
play_prev_track( int forward )
{
	if (cd == NULL)
		return;

	if (playlist == NULL)
		return;

	/* If we're in the middle of this playlist entry, go back one track */
	if (cur_track > playlist[cur_listno - 1].start)
		wm_cd_play(cur_track - 1, 0, playlist[cur_listno - 1].end);
	else
		if (cur_listno > 1)
		{
			cur_listno--;
			wm_cd_play(playlist[cur_listno - 1].end - 1, 0,
				playlist[cur_listno - 1].end);
		}
		else
			wm_cd_play(playlist[0].start, 0, playlist[0].end);
}

/*
 * stash_cdinfo(artist, cdname)
 */
void
stash_cdinfo(char *artist, char *cdname, int autoplay, int playmode )
{
	if (cd != NULL)
	{
		if (strcmp(cd->artist, artist))
			info_modified = 1;
		strcpy(cd->artist, artist);

		if (strcmp(cd->cdname, cdname))
			info_modified = 1;
		strcpy(cd->cdname, cdname);

		if (!!cd->autoplay != !!autoplay)
			info_modified = 1;
		cd->autoplay = autoplay;

		if (!!cd->playmode != !!playmode)
			info_modified = 1;
		cd->playmode = playmode;
	}
} /* stash_cdinfo() */

/*
 * wipe_cdinfo()
 *
 * Clear out all a CD's soft information (presumably in preparation for
 * reloading from the database.)
 */
void
wipe_cdinfo( void )
{
	struct wm_playlist	*l;
	int		i;

	if (cd != NULL)
	{
		cd->artist[0] = cd->cdname[0] = '\0';
		cd->autoplay = cd->playmode = cd->volume = 0;
		cd->whichdb = NULL;
		freeup(&cd->otherrc);
		freeup(&cd->otherdb);

		if (thiscd.lists != NULL)
		{
			for (l = thiscd.lists; l->name != NULL; l++)
			{
				free(l->name);
				free(l->list);
			}
			freeup( (char **)&thiscd.lists );
		}

		for (i = 0; i < cur_ntracks; i++)
		{
			freeup(&cd->trk[i].songname);
			freeup(&cd->trk[i].otherrc);
			freeup(&cd->trk[i].otherdb);
			cd->trk[i].avoid = cd->trk[i].contd = 0;
			cd->trk[i].volume = 0;
			if (cd->trk[i].section > 1)
				remove_trackinfo(i--);
		}
	}
}

/*
 * stash_trkinfo(track, songname, contd, avoid)
 *
 * Update information about a track on the current CD.
 */
void
stash_trkinfo( int track, char *songname, int contd, int avoid )
{
	if (cd != NULL)
	{
		track--;
		if (!!cd->trk[track].contd != !!contd)
			info_modified = 1;
		cd->trk[track].contd = track ? contd : 0;

		if (!!cd->trk[track].avoid != !!avoid)
			info_modified = 1;
		cd->trk[track].avoid = avoid;

		if ((cd->trk[track].songname == NULL && songname[0]) ||
				(cd->trk[track].songname != NULL &&
				strcmp(cd->trk[track].songname, songname)))
		{
			info_modified = 1;
			wm_strmcpy(&cd->trk[track].songname, songname);
		}
	}
}

/*
 * new_list()
 *
 * Add a playlist to a CD.
 */
struct wm_playlist *
new_list(cd, listname)
	struct wm_cdinfo	*cd;
	char		*listname;
{
	int	nlists = 0;
	struct wm_playlist *l;

	if (cd->lists != NULL)
	{
		for (nlists = 0; cd->lists[nlists].name != NULL; nlists++)
			;
		l = (struct wm_playlist *)realloc(cd->lists, (nlists + 2) *
			sizeof (struct wm_playlist));
	}
	else
		l = (struct wm_playlist *)malloc(2 * sizeof (struct wm_playlist));

	if (l == NULL)
		return (NULL);

	l[nlists + 1].name = NULL;
	l[nlists].name = NULL;		/* so wm_strmcpy doesn't free() it */
	wm_strmcpy(&l[nlists].name, listname);
	l[nlists].list = NULL;
	cd->lists = l;

	return (&l[nlists]);
}

/*
 * make_playlist()
 *
 * Construct a playlist for the current CD.  If we're in shuffle mode, play
 * each non-avoided track once, keeping continued tracks in the right order.
 *
 * If playmode is 2, use playlist number (playmode-2).  XXX should do
 * bounds checking on this, probably.
 *
 * If consecutive tracks are being played, only make one playlist entry for
 * them, so the CD player won't pause between tracks while we wake up.
 */
void
make_playlist( int playmode, int starttrack )
{
	int	i, avoiding = 1, entry = 0, count, track,
		*thislist;

	cur_listno = 0;
	if (playlist != NULL)
		free(playlist);
	playlist = malloc(sizeof (*playlist) * (cur_ntracks + 1));
	if (playlist == NULL)
	{
		perror("playlist");
		exit(1);
	}

	/* If this is a data-only CD, we can't play it. */
	if ((starttrack && cd->trk[starttrack - 1].data) ||
		(cur_ntracks == 1 && cd->trk[0].data))
	{
		playlist[0].start = 0;
		playlist[0].end = 0;
		playlist[1].start = 0;
		return;
	}

	if (playmode == 1)
	{
		char *done = malloc(cur_ntracks);

		if (done == NULL)
		{
			perror("randomizer");
			exit(1);
		}

		count = cur_ntracks;
		if (starttrack && cd->trk[starttrack - 1].avoid)
			count++;
		for (i = 0; i < cur_ntracks; i++)
			if (cd->trk[i].contd || cd->trk[i].avoid ||
				cd->trk[i].data)
			{
				done[i] = 1;
				count--;
			}
			else
				done[i] = 0;

		for (i = 0; i < count; i++)
		{
			int end;	/* for readability */
			if (starttrack)
			{
				track = starttrack - 1;
				starttrack = 0;
			}
			else
				while (done[track = rand() % cur_ntracks])
					;

			playlist[i].start = track + 1;

			/* play all subsequent continuation tracks too */
			for (end = track + 1; end < cur_ntracks + 1; end++)
				if (! cd->trk[end].contd ||
						cd->trk[end].avoid ||
						cd->trk[end].data)
					break;
			playlist[i].end = end + 1;

			done[track]++;
		}
		playlist[i].start = 0;

		free(done);
	}
	else if (playmode >= 2 && cd->lists && cd->lists[playmode - 2].name)
	{
		count = 2;	/* one terminating entry, and one for start */
		thislist = cd->lists[playmode - 2].list;

		for (i = 0; thislist[i]; i++)
			if (thislist[i + 1] != thislist[i] + 1)
				count++;

		if (playlist != NULL)
			free(playlist);
		playlist = malloc(sizeof (*playlist) * count);
		if (playlist == NULL)
		{
			perror("playlist");
			exit(1);
		}

		count = 0;
		if (starttrack)
		{
			playlist[0].start = starttrack;
			for (track = 0; thislist[track]; track++)
				if (starttrack == thislist[track])
					break;
			if (! thislist[track])
			{
				playlist[0].end = starttrack + 1;
				playlist[1].start = thislist[0];
				count = 1;
				track = 0;
			}
		}
		else
		{
			playlist[0].start = thislist[0];
			track = 0;
		}

		for (i = track; thislist[i]; i++)
			if (thislist[i + 1] != thislist[i] + 1)
			{
				playlist[count].end = thislist[i] + 1;
				count++;
				playlist[count].start = thislist[i + 1];
			}
	}
	else
	{
		for (i = starttrack ? starttrack - 1 : 0; i < cur_ntracks; i++)
			if (avoiding && ! (cd->trk[i].avoid || cd->trk[i].data))
			{
				playlist[entry].start = i + 1;
				avoiding = 0;
			}
			else if (! avoiding && (cd->trk[i].avoid ||
						cd->trk[i].data))
			{
				playlist[entry].end = i + 1;
				avoiding = 1;
				entry++;
			}
		if (! avoiding)
			playlist[entry].end = i + 1;
		playlist[entry + !avoiding].start = 0;
	}

	/*
	 * Now go through the list, whatever its source, and figure out
	 * cumulative starting times for each entry.
	 */
	entry = count = 0;
	do {
		playlist[entry].starttime = count;

		if (playlist[entry].start)
			for (i = playlist[entry].start; i <
						playlist[entry].end; i++)
				count += cd->trk[i - 1].length;
	} while (playlist[entry++].start);
}

/*
 * Find a particular track's location in the current playlist.  Sets the
 * appropriate variables (cur_listno, cur_firsttrack, cur_lasttrack).
 */
void
pl_find_track( int track )
{
	int	i;

	if (playlist == NULL)
	{
		fprintf(stderr, "Null playlist!  Huh?\n");
		return;
	}

	for (i = 0; playlist[i].start; i++)
		if (track >= playlist[i].start && track < playlist[i].end)
		{
			cur_listno = i + 1;
			cur_firsttrack = playlist[i].start;
			cur_lasttrack = playlist[i].end - 1;
			return;
		}
	
	/*
	 * Couldn't find the track in question.  Make a special entry with
	 * just that track.
	 */
	if (! playlist[i].start)
	{
		playlist = realloc(playlist, (i + 2) * sizeof(*playlist));
		if (playlist == NULL)
		{
			perror("playlist realloc");
			exit(1);
		}

		playlist[i + 1].start = playlist[i + 1].end = 0;
		playlist[i + 1].starttime = playlist[i].starttime +
			cd->trk[track - 1].length;
		playlist[i].start = track;
		playlist[i].end = track + 1;
		cur_listno = i + 1;
		cur_firsttrack = track;
		cur_lasttrack = track;
	}
}

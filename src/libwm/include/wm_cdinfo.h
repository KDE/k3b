#ifndef WM_CDINFO_H
#define WM_CDINFO_H
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
 * Prototypes from cdinfo.c
 * 
 * This is just one more step to a more modular and understandable code.
 */

#include "wm_struct.h"
extern char *cur_trackname;		/* Take a guess */
extern int cur_index;			/* Current index mark */
extern int cur_frame;			/* Current frame number */
extern struct wm_play *playlist;		/* = NULL */
extern struct wm_cdinfo thiscd;
extern struct wm_cdinfo *cd;
extern int cur_track;			/* Current track number, starting at 1 */
extern char *cur_artist;		/* Name of current CD's artist */
extern char cur_avoid;			/* Avoid flag */
extern char cur_contd;			/* Continued flag */
extern char *cur_cdname;		/* Album name */
extern int cur_nsections;		/* Number of sections currently defined */
extern int exit_on_eject;
extern int cur_track;
extern int cur_pos_abs;
extern int cur_pos_rel;
extern int cur_tracklen;
extern int cur_cdlen;
extern enum wm_cd_modes cur_cdmode;
extern int cur_ntracks;
extern int cur_lasttrack;
extern int cur_firsttrack;
extern int cur_listno;
extern int cur_stopmode;

void	wipe_cdinfo( void );
void	play_next_entry( int forward );
void	make_playlist( int playmode, int starttrack );
int	get_autoplay( void );
int	get_playmode( void );
void	pl_find_track( int track );
void	play_prev_track( int forward );
void	play_next_track( int forward );
int	tracklen( int num );
int	get_default_volume( int track );
int	split_trackinfo( int pos );
int	remove_trackinfo( int num );
void	freeup( char **x );
int	get_runtime( void );
char   *trackname( int num );
void	stash_cdinfo( char *artist, char *cdname, int autoplay, int playmode );
void	stash_trkinfo( int track, char *songname, int contd, int avoid );
int	get_avoid( int num );
int	get_contd( int num );
void	default_volume( int track, int vol );
char   *listentry( int num );

#endif /* WM_CDINFO_H */

#ifndef WM_CDROM_H
#define WM_CDROM_H
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
 * Prototypes from cdrom.c
 * 
 * This is just one more step to a more modular and understandable code.
 */

#define WM_CDS_NO_DISC		0
#define WM_CDS_DISC_READY	1
#define WM_CDS_JUST_INSERTED	2

#define WM_STR_GENVENDOR	"Generic"
#define WM_STR_GENMODEL		"drive"
#define WM_STR_GENREV		"type"

extern int wm_cd_cur_balance;

extern char *cd_device;

char *	wm_drive_vendor( void );
char *	wm_drive_model( void );
char *	wm_drive_revision( void );
void 	wm_drive_settype( char *vendor, char *model, char *revision );

int	wm_cd_status( void );

void	wm_cd_play( int start, int pos, int end );
void	wm_cd_play_chunk( int start, int end, int realstart );
void	wm_cd_play_from_pos( int pos );
void	wm_cd_pause( void );
void	wm_cd_stop( void );
int	wm_cd_eject( void );
int     wm_cd_closetray( void );
int	wm_cd_read_initial_volume( int max );
void	wm_cd_get_cdtext( void );

/*
 * Following are the missing to rename.
 */
int	find_trkind( int track, int index, int start );
void	cd_volume( int vol, int bal, int max );

#endif /* WM_CDROM_H */

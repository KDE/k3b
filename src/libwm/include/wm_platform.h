#ifndef WM_PLATFORM_H
#define WM_PLATFORM_H
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
 * The platform interface
 * 
 * This is just one more step to a more modular and understandable code.
 */

#include "wm_struct.h"

int wmcd_open( struct wm_drive *d );
int wmcd_reopen( struct wm_drive *d );

/*
 * void	keep_cd_open( void );
 */
int	wm_scsi( struct wm_drive *d, unsigned char *cdb, int cdblen,
         	 void *retbuf, int retbuflen, int getreply );

/****************************************
 *
 * The drive prototypes.
 *
 */
extern struct wm_drive generic_proto;
extern struct wm_drive sony_proto;
extern struct wm_drive toshiba_proto;

#endif /* WM_PLATFORM_H */

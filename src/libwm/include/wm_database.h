#ifndef WM_DATABASE_H
#define WM_DATABASE_H
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
 * Prototypes for WorkMan database
 * 
 * This is just one more step to a more modular and understandable code.
 */


#define WM_DB_SAVE_ERROR	1
#define WM_DB_SAVE_DISABLED	2
 
int wm_db_get_playnew( void );
void	split_workmandb( void );
int	save( void );
void	load( void );
void	load_settings( void );

extern int wm_db_save_disabled;
extern int cur_playnew;

#endif /* WM_DATABASE_H */

#ifndef WM_CDDB_H
#define WM_CDDB_H
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
 */


extern struct wm_cddb cddb;
extern int cur_cddb_protocol;
extern char *cur_cddb_server, *cur_cddb_mail_adress,
	*cur_cddb_path_to_cgi, *cur_cddb_proxy_server;

unsigned long cddb_discid();
void cddb_struct2cur();
void cddb_cur2struct();
void cddb_select();
void connect_cddb();
void update_cddbserver();
void cddb_request();

#endif /* WM_CDDB_H */

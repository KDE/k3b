/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2001  Andreas Mueller <andreas@daneb.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>

class Sample;

char *strdupCC ( const char *s );
char *strdup3CC ( const char *s1, const char *s2, const char *s3 );
char *strdupvCC ( const char *s1, ... );

long fullRead ( int fd, void *buf, long count );
long fullWrite ( int fd, const void *buf, long count );
long readLong ( FILE * fp );
short readShort ( FILE * fp );

void swapSamples ( Sample * buf, unsigned long len );

unsigned char int2bcd ( int );
int bcd2int ( unsigned char );

const char *stripCwd ( const char *fname );

#endif

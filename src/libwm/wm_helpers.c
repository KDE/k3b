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
 * Some helpful functions...
 *
 */

static char wm_helpers_id[] = "$Id$";

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>
#include "include/workman_defs.h"
#include "include/wm_config.h"
#include "include/wm_helpers.h"
#include "include/wm_struct.h"

#define WM_MSG_CLASS WM_MSG_CLASS_MISC

int wm_lib_verbosity = WM_MSG_LEVEL_NONE;

/* 
 * Some seleced functions of version reporting follow...
 */

int wm_libver_major( void ){return WM_LIBVER_MAJOR;}
int wm_libver_minor( void ){return WM_LIBVER_MINOR;}
int wm_libver_pl( void ){return WM_LIBVER_PL;}

char *wm_libver_name( void )
{
	char *s = NULL;
	
	wm_strmcat(&s, WM_LIBVER_NAME);
	return s;
} /* wm_libver_name() */

char *wm_libver_number( void )
{
	char *s = NULL;

	s = malloc(10);
	/* this is not used very often, so don't care about speed...*/
	sprintf(s, "%d.%d.%d", wm_libver_major(), wm_libver_minor(), wm_libver_pl());
	return s;
} /* wm_libver_number() */

char *wm_libver_date( void )
{
	char *s = NULL;
	wm_strmcat(&s, __DATE__);
	return s;
} /* wm_libver_date() */

char *wm_libver_string( void )
{
	char *s = NULL;

	wm_strmcat( &s, wm_libver_name() ); 
	wm_strmcat( &s, " " ); 
	wm_strmcat( &s, wm_libver_number() ); 
	return s;
} /* wm_libver_string() */


/*
 *
 * Now for some memory management...
 *
 */

/* Free some memory and set a pointer to null. */
void freeup( char **x )
{
	if (*x != NULL)
	{
		free(*x);
		*x = NULL;
	}
} /* freeup() */

/* Copy into a malloced string. */
void
wm_strmcpy( char **t, char *s )
{
	wm_lib_message(WM_MSG_CLASS_MISC | WM_MSG_LEVEL_DEBUG, "wm_strmcpy(%s, '%s')\n", *t, s);
	if (*t != NULL)
	  {
	    wm_lib_message(WM_MSG_CLASS_MISC | WM_MSG_LEVEL_DEBUG, "wm_strmcpy freeing pointer 0x%08X\n", *t);
	    free(*t);
	  }

	*t = malloc(strlen(s) + 1);
	if (*t == NULL)
	{
		perror("wm_strmcpy");
		exit(1);
	}

	wm_lib_message(WM_MSG_CLASS_MISC | WM_MSG_LEVEL_DEBUG, "wm_strmcpy finally copying (0x%08X, '%s')\n", *t, s);
	strncpy(*t, s, strlen(s));
} /* wm_strmcpy() */

/* Add to a malloced string. */
void
wm_strmcat( char **t, char *s)
{
	int	len = strlen(s) + 1;

	wm_lib_message(WM_MSG_CLASS_MISC | WM_MSG_LEVEL_DEBUG, "wm_strmcat(%s, %s)\n", *t, s);

	if (*s == '\0')
		return;

	if (*t != NULL)
	{
		len += strlen(*t);
		*t = realloc(*t, len);
		if (*t == NULL)
		{
			perror("wm_strmcat");
			exit(1);
		}
		strcat(*t, s);
	}
	else
		wm_strmcpy(t, s);
} /* wm_strmcat() */

/* Duplicate a string.  Some systems have this in libc, but not all. */
char *
wm_strdup( char *s )
{
	char	*new;

	new = malloc(strlen(s) + 1);
	if (new)
		strcpy(new, s);
	return (new);
} /* wm_strdup() */


/*
 * set and get verbosity level.
 */
void wm_lib_set_verbosity( int level )
{
	if( WM_MSG_LEVEL_NONE <= level <= WM_MSG_LEVEL_DEBUG )
	{
	 	wm_lib_verbosity = level;
		wm_lib_message(WM_MSG_CLASS_MISC | WM_MSG_LEVEL_DEBUG, "Verbosity set to %d|%d\n", WM_MSG_LEVEL_DEBUG, level & WM_MSG_CLASS_ALL);
	}
} /* wm_lib_set_verbosity */

int wm_lib_get_verbosity( void )
{
	return wm_lib_verbosity;
}

/*
 * wm_lib_message().
 *
 * any message that falls into allowed classes and has at least
 * verbosity level wm_lib_verbosity & 0xf will be printed.
 *
 * Usage:
 *
 * wm_lib_message( WM_MSG_LEVEL | WM_MSG_CLASS, "format", contents);
 *
 * To simplify the usage, you may simply use WM_MSG_CLASS. It should be
 * defined in each module to reflect the correct message class.
 *
 */
void wm_lib_message( unsigned int level, char *fmt, ... )
{
	va_list ap;
	/* verbosity level */
	unsigned int vlevel = wm_lib_verbosity & 0xf;
	/* allowed classes */
	unsigned int vclass = (level & WM_MSG_CLASS_ALL) & (wm_lib_verbosity & WM_MSG_CLASS_ALL);

	/*
         * just give me the level
         */
	level &= 0xf;
	if(level <= WM_MSG_LEVEL_NONE)
	{
		fprintf(stderr, "LibWorkMan warning: A LibWorkMan programmer specified an invalid message level.\n");
	}
        /*
         * print it only if level and class are allowed.
         */	
	if( (level <= vlevel) && (vclass != 0) )
	{
		fprintf(stderr, "libWorkMan: ");
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
} /* wm_lib_message() */

/*
 * Simulate usleep() using select().
 */
int
wm_susleep( int usec )
{
	struct timeval	tv;

	timerclear(&tv);
	tv.tv_sec = usec / 1000000;
	tv.tv_usec = usec % 1000000;
	return (select(0, NULL, NULL, NULL, &tv));
} /* wm_susleep() */



#ifndef WM_HELPERS_H
#define WM_HELPERS_H
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
 * Here to be found: Prototypes. Including variable names to be easier
 * to read.
 * This is just one more step to a more modular and understandable code.
 *
 */

/*
 * LibWorkMan message levels. I'm not sure how to call them all and which
 * use they should fulfill. This is not very urgent now, because there
 * aren't many messages in LibWorkMan now. 
 */
#define WM_MSG_LEVEL_NONE	0	/**/
#define WM_MSG_LEVEL_ERROR	1	/**/
#define WM_MSG_LEVEL_TWO	2
#define WM_MSG_LEVEL_THREE	3
#define WM_MSG_LEVEL_FOUR	4
#define WM_MSG_LEVEL_INFO	5	/**/
#define WM_MSG_LEVEL_SIX	6
#define WM_MSG_LEVEL_VERB	7	/**/
#define WM_MSG_LEVEL_EIGHT	8
#define WM_MSG_LEVEL_DEBUG	9	/**/

/*
 * Message classes. This is somehow a definition of
 * the message's source.
 */
 
#define WM_MSG_CLASS_PLATFORM	0x010
#define WM_MSG_CLASS_SCSI	0x020
#define WM_MSG_CLASS_CDROM      0x040
#define WM_MSG_CLASS_DB		0x080
#define WM_MSG_CLASS_MISC	0x100

#define WM_MSG_CLASS_ALL	0xff0

extern int wm_lib_verbosity;

/*
 * I did not know any better place...
 */
#ifdef DEBUG
#define CHECKPOINT(t) fprintf(stderr, "%s (%d): %s\n", __FILE__, __LINE__, t );
#else
#define CHECKPOINT(t) 
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifdef __linux__
#include <signal.h>
/* Linux doesn't have a SIGEMT */
#if !defined( SIGEMT )
#  define SIGEMT SIGUNUSED
#endif
#endif /* linux */

void		freeup( char **x );
void	 	wm_strmcat( char **t, char *s);
void	 	wm_strmcpy( char **t, char *s );
char *		wm_strdup( char *s );
/* Somebody's version query unsatisfied? */
int		wm_libver_major( void );	/* return major internal version number */
int		wm_libver_minor( void );	/* return minor internal version number */
int		wm_libver_pl( void );		/* return internal patchlevel number */
char *		wm_libver_name( void );		/* return internal name (LibWorkMan) */
char *		wm_libver_number( void );	/* returns string: "<major>.<minor>.<pl>" */
char *		wm_libver_string( void ); 	/* returns string: "<name> <number>" */
char *		wm_libver_date( void );		/* returns string: date of compilation */
void 		wm_lib_set_verbosity( int level ); /* set verbosity level */
int 		wm_lib_get_verbosity( void );      /* get verbosity level */
void 		wm_lib_message( unsigned int level, char *format, ... ); /* put out a message on stderr */
int		wm_susleep( int usec );

#endif /* WM_HELPERS_H */

#ifndef WM_CDDA_H
#define WM_CDDA_H
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

/*
 * Information about a particular block of CDDA data.
 */
struct cdda_block {
	unsigned char	status;		/* see below */
	unsigned char	track;
	unsigned char	index;
	unsigned char	minute;
	unsigned char	second;
	unsigned char	frame;

	/* Average volume levels, for level meters */
	unsigned char	lev_chan0;
	unsigned char	lev_chan1;

	/* Current volume setting (0-255) */
	unsigned char	volume;

	/* Current balance setting (0-255, 128 = balanced) */
	unsigned char	balance;
};

/*
 * cdda_block status codes.
 */
#define WMCDDA_ERROR	0	/* Couldn't read CDDA from disc */
#define WMCDDA_OK	1	/* Read this block successfully (raw data) */
#define WMCDDA_PLAYED	2	/* Just played the block in question */
#define WMCDDA_STOPPED	3	/* Block data invalid; we've just stopped */
#define WMCDDA_ACK	4	/* Acknowledgement of command from parent */
#define WMCDDA_DONE	5	/* Chunk of data is done playing */
#define WMCDDA_EJECTED	6	/* Disc ejected or offline */

/*
 * Enable or disable CDDA building depending on platform capabilities, and
 * determine endianness based on architecture.  (Gross!)
 *
 * For header-comfort, the macros LITTLE_ENDIAN and BIG_ENDIAN had to be
 * renamed. At least Linux does have bytesex.h and endian.h for easy
 * byte-order examination.
 */

#ifdef HAVE_MACHINE_ENDIAN_H
	#include <machine/endian.h>
	#if BYTE_ORDER == LITTLE_ENDIAN
		#define WM_LITTLE_ENDIAN 1
		#define WM_BIG_ENDIAN 0
	#else
		#define WM_LITTLE_ENDIAN 0
		#define WM_BIG_ENDIAN 1
	#endif
#elif defined(__sun) || defined(sun) 
# ifdef SYSV
#  include <sys/types.h>
#  include <sys/cdio.h>
#  ifndef CDROMCDDA
#   undef BUILD_CDDA
#  endif
#  ifdef i386
#   define WM_LITTLE_ENDIAN 1
#   define WM_BIG_ENDIAN 0
#  else
#   define WM_BIG_ENDIAN 1
#   define WM_LITTLE_ENDIAN 0
#  endif
# else
#  undef BUILD_CDDA
# endif

/* Linux only allows definition of endianness, because there's no
 * standard interface for CDROM CDDA functions that aren't available
 * if there is no support.
 */
#elif defined(__linux__)
/*# include <bytesex.h>*/
# include <endian.h>
/*
 * XXX could this be a problem? The results are only 0 and 1 because
 * of the ! operator. How about other linux compilers than gcc ?
 */
# define WM_LITTLE_ENDIAN !(__BYTE_ORDER - __LITTLE_ENDIAN)
# define WM_BIG_ENDIAN !(__BYTE_ORDER - __BIG_ENDIAN)
#elif defined WORDS_BIGENDIAN
	#define WM_LITTLE_ENDIAN 0
	#define WM_BIG_ENDIAN 1
#else
	#define WM_LITTLE_ENDIAN 1
	#define WM_BIG_ENDIAN 0
#endif

/*
 * The following code shouldn't take effect now. 
 * In 1998, the WorkMan platforms don't support __PDP_ENDIAN
 * architectures.
 *
 */

#if !defined(WM_LITTLE_ENDIAN)
#  if !defined(WM_BIG_ENDIAN)
#    error yet unsupported architecture
	foo bar this is to stop the compiler. 
#  endif
#endif
#endif /* WM_CDDA_H */

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
 * Sun (really Solaris) CDDA functions.
 */

#include "include/wm_config.h"

static char plat_sun_cdda_id[] = "$Id$";
#if defined(sun) || defined(__sun__) && defined(SYSV) && defined(BUILD_CDDA) && defined(WMCDDA_DONE) /* { */


#include "include/wm_cdda.h"
/* types.h and cdio.h are included by wmcdda.h */

#include <stdio.h>
#include <math.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <errno.h>

#define WM_MSG_CLASS WM_MSG_CLASS_PLATFORM

#define CDDABLKSIZE 2368
#define SAMPLES_PER_BLK 588

/* Address of next block to read. */
int	current_position = 0;

/* Address of first and last blocks to read. */
int	starting_position = 0;
int	ending_position = 0;

/* Playback direction. */
int	direction = 1;

/* Number of blocks to read at once; initialize to the maximum. */
/* (was 30. Set to 15 for INTeL. Maybe config option? */
int	numblocks = 15;

/*
 * This is the fastest way to convert from BCD to 8-bit.
 */
unsigned char unbcd[256] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,0,0,0,0,0,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,  0,0,0,0,0,0,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,  0,0,0,0,0,0,
 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,  0,0,0,0,0,0,
 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,  0,0,0,0,0,0,
 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,  0,0,0,0,0,0,
 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,  0,0,0,0,0,0,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,  0,0,0,0,0,0,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,  0,0,0,0,0,0,
 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,  0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/*
 * Initialize the CDDA data buffer and open the appropriate device.
 *
 * NOTE: We allocate twice as much space as we need to actually read a block;
 * this lets us do audio manipulations without bothering to malloc a second
 * buffer.
 *
 * Also, test to see if we can actually *do* CDDA on this drive; if not, we
 * need to exit right away so the UI doesn't show the user any CDDA controls.
 */
int
wmcdda_init(char **bufadr, long *buflenadr, int init_fd, char *devname)
{
	struct cdrom_cdda	cdda;

	*bufadr = malloc(numblocks * CDDABLKSIZE * 2);
	if (*bufadr == NULL)
		return (-1);

	*buflenadr = numblocks * CDDABLKSIZE;

	/*init_fd = open(devname, 0);
	if (init_fd == -1)
		init_fd = open("/dev/rdsk/c0t6d0s2", 0);
	*/
	init_fd = wmcdda_open(devname);

	if (init_fd > -1)
	{
		cdda.cdda_addr = 200;
		cdda.cdda_length = 1;
		cdda.cdda_data = *bufadr;
		cdda.cdda_subcode = CDROM_DA_SUBQ;

		if (ioctl(init_fd, CDROMCDDA, &cdda) < 0)
		{
			close(init_fd);
			init_fd = -1;
		}
	}

	return (init_fd);
}

/*
 * Try to open the CD device
 */
int
wmcdda_open(char *devname)
{
    int fd;

    fd = open(devname, 0); 
    if (fd == -1)
	fd = open("/dev/rdsk/c0t6d0s2", 0);

    return(fd);
}


/*
 * Close the CD-ROM device in preparation for exiting.
 */
void
wmcdda_close(int fd)
{
	close(fd);
}

/*
 * Set up for playing the CD.  Actually this doesn't play a thing, just sets a
 * couple variables so we'll know what to do when we're called.
 */
void
wmcdda_setup(int start, int end, int realstart)
{
	current_position = start - 150;
	ending_position = end - 150;
	starting_position = realstart - 150;

	/*
	 * Special case: don't start at the "end" of a track if we're
	 * playing backwards!
	 */
	if (direction == -1 && start == realstart)
		current_position = ending_position - numblocks;
}

/*
 * Read some blocks from the CD.  Stop if we hit the end of the current region.
 *
 * Returns number of bytes read, -1 on error, 0 if stopped for a benign reason.
 */
long
wmcdda_read(int fd, unsigned char *rawbuf, long buflen,
	struct cdda_block *block)
{
	struct cdrom_cdda	cdda;
	int			blk;
	unsigned char		*q;
	extern int		speed;

	if ((direction > 0 && current_position >= ending_position) ||
	    (direction < 0 && current_position < starting_position))
	{
		block->status = WMCDDA_DONE;
		return (0);
	}

	cdda.cdda_addr = current_position;
	if (ending_position && current_position + numblocks > ending_position)
		cdda.cdda_length = ending_position - current_position;
	else
		cdda.cdda_length = numblocks;
	cdda.cdda_data = rawbuf;
	cdda.cdda_subcode = CDROM_DA_SUBQ;

	if (ioctl(fd, CDROMCDDA, &cdda) < 0)
	{
		if (errno == ENXIO)	/* CD ejected! */
		{
			block->status = WMCDDA_EJECTED;
			return (-1);
		}

		if (current_position + numblocks > ending_position)
		{
			/*
			 * Hit the end of the CD, probably.
			 */
			block->status = WMCDDA_DONE;
			return (0);
		}

		/* Sometimes it fails once, dunno why */
		if (ioctl(fd, CDROMCDDA, &cdda) < 0)
		{
			if (ioctl(fd, CDROMCDDA, &cdda) < 0)
			{
				if (ioctl(fd, CDROMCDDA, &cdda) < 0)
				{
					perror("CDROMCDDA");
					block->status = WMCDDA_ERROR;
					return (-1);
				}
			}
		}
	}

	if (speed > 148)
	{
		/*
		 * We want speed=148 to advance by cdda_length, but
		 * speed=256 to advance cdda_length * 4.
		 */
		current_position = current_position +
			(cdda.cdda_length * direction * (speed - 112)) / 36;
	}
	else
		current_position = current_position +
			cdda.cdda_length * direction;

	for (blk = 0; blk < numblocks; blk++)
	{
		/*
		 * New valid Q-subchannel information?  Update the block
		 * status.
		 */
		q = &rawbuf[blk * CDDABLKSIZE + SAMPLES_PER_BLK * 4];
		if (*q == 1)
		{
			block->status = WMCDDA_OK;
			block->track =  unbcd[q[1]];
			block->index =  unbcd[q[2]];
			block->minute = unbcd[q[7]];
			block->second = unbcd[q[8]];
			block->frame =  unbcd[q[9]];
		}
	}

	return (cdda.cdda_length * CDDABLKSIZE);
}

/*
 * Normalize a bunch of CDDA data.  Basically this means ripping out the
 * Q subchannel data and doing byte-swapping, since the CD audio is in
 * littleendian format.
 *
 * Scanning is handled here too.
 *
 * XXX - do byte swapping on Intel boxes?
 */
long
wmcdda_normalize(unsigned char *rawbuf, long buflen, struct cdda_block *block)
{
	int		i, nextq;
	int		blocks = buflen / CDDABLKSIZE;
	unsigned char	*dest = rawbuf;
	unsigned char	tmp;
	long		*buf32 = (long *)rawbuf, tmp32;

/*
 * this was #ifndef LITTLEENDIAN
 * in wmcdda it was called LITTLE_ENDIAN. Was this a flaw?
 */
#if WM_BIG_ENDIAN
	if (blocks--)
		for (i = 0; i < SAMPLES_PER_BLK * 2; i++)
		{
			/* Only need to use temp buffer on first block. */
			tmp = *rawbuf++;
			*dest++ = *rawbuf++;
			*dest++ = tmp;
		}
#endif

	while (blocks--)
	{
		/* Skip over Q data. */
		rawbuf += 16;

		for (i = 0; i < SAMPLES_PER_BLK * 2; i++)
		{
#if WM_LITTLE_ENDIAN
			*dest++ = *rawbuf++;
			*dest++ = *rawbuf++;
#else
			*dest++ = rawbuf[1];
			*dest++ = rawbuf[0];
			rawbuf += 2;
#endif
		}
	}

	buflen -= ((buflen / CDDABLKSIZE) * 16);

	/*
	 * Reverse the data here if we're playing backwards.
	 * XXX - ideally this should be done above.
	 */
	if (direction < 0)
	{
		buflen /= 4;	/* we can move 32 bits at a time. */

		for (i = 0; i < buflen / 2; i++)
		{
			tmp32 = buf32[i];
			buf32[i] = buf32[buflen - i - 1];
			buf32[buflen - i - 1] = tmp32;
		}

		buflen *= 4;
	}

	return (buflen);
}

/*
 * Set the playback direction.
 */
void
wmcdda_direction(int newdir)
{
	if (newdir == 0)
	{
		numblocks = 20;
		direction = 1;
	}
	else
	{
		numblocks = 30;
		direction = -1;
	}
}

/*
 * Do system-specific stuff to get ready to play at a particular speed.
 */
void
wmcdda_speed(int speed)
{
	if (speed > 128)
		numblocks = 12;
	else
		numblocks = direction > 0 ? 20 : 30;
}

#endif /* } */

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
 * Vendor-specific drive control routines for Toshiba XM-3401 series.
 */

static char drv_toshiba_id[] = "$Id$";

#include <stdio.h>
#include <errno.h>
#include "include/wm_config.h"
#include "include/wm_struct.h"
#include "include/wm_scsi.h"

#define	SCMD_TOSH_EJECT		0xc4

/* local prototypes */
static int	tosh_init( struct wm_drive *d );
static int	tosh_eject( struct wm_drive *d );
static int	tosh_set_volume( struct wm_drive *d, int left, int right );
static int	tosh_get_volume( struct wm_drive *d, int *left, int *right );

struct wm_drive toshiba_proto = {
	-1,			/* fd */
	"Toshiba",		/* vendor */
	"",			/* model */
	"",			/* revision */
	NULL,			/* aux */
	NULL,			/* daux */

	tosh_init,		/* functions... */
	gen_get_trackcount,
	gen_get_cdlen,
	gen_get_trackinfo,
	gen_get_drive_status,
	tosh_get_volume,
	tosh_set_volume,
	gen_pause,
	gen_resume,
	gen_stop,
	gen_play,
	tosh_eject,
	gen_closetray
};

/*
 * Initialize the driver.
 */
static int
tosh_init( struct wm_drive *d )
{
	extern int	min_volume;

/* Sun use Toshiba drives as well */
#if defined(SYSV) && defined(CODEC)
        codec_init();
#endif
	min_volume = 0;
	return ( 0 );
}

/*
 * Send the Toshiba code to eject the CD.
 */
static int
tosh_eject( struct wm_drive *d )
{
	return (sendscsi(d, NULL, 0, 0, SCMD_TOSH_EJECT, 1, 0,0,0,0,0,0,0,0,0,0));
}

/*
 * Set the volume.  The low end of the scale is more sensitive than the high
 * end, so make up for that by transforming the volume parameters to a square
 * curve.
 */
static int
tosh_set_volume( struct wm_drive *d, int left, int right )
{
	left = (left * left * left) / 10000;
	right = (right * right * right) / 10000;
	return (gen_set_volume(d, left, right));
}

/*
 * Undo the transformation above using a binary search (so no floating-point
 * math is required.)
 */
static int
unscale_volume(cd_vol, max)
	int	cd_vol, max;
{
	int	vol = 0, top = max, bot = 0, scaled = 0;

	/*cd_vol = (cd_vol * 100 + (max_volume - 1)) / max_volume;*/

	while (bot <= top)
	{
		vol = (top + bot) / 2;
		scaled = (vol * vol) / max;
		if (cd_vol <= scaled)
			top = vol - 1;
		else
			bot = vol + 1;
	}
	
	/* Might have looked down too far for repeated scaled values */
	if (cd_vol < scaled)
		vol++;

	if (vol < 0)
		vol = 0;
	else if (vol > max)
		vol = max;

	return (vol);
}

/*
 * Get the volume.
 */
static int
tosh_get_volume( struct wm_drive *d, int *left, int *right )
{
	int		status;

	status = gen_get_volume(d, left, right);
	if (status < 0)
		return (status);
	*left = unscale_volume(*left, 100);
	*right = unscale_volume(*right, 100);

	return (0);
}

/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2000  Andreas Mueller <mueller@daneb.ping.de>
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
/*
 * Revision 1.5  2001/10/01 18:08:41  andreasm
 * Enabled remote progress messages for blanking.
 *
 * Revision 1.4  2000/11/05 19:20:59  andreasm
 * Unified progress messages sent from cdrdao to gcdmaster.
 *
 * Revision 1.3  2000/10/08 16:39:41  andreasm
 * Remote progress message now always contain the track relative and total
 * progress and the total number of processed tracks.
 *
 * Revision 1.2  2000/04/23 16:29:50  andreasm
 * Updated to state of my private development environment.
 *
 * Revision 1.2  1999/12/15 20:31:46  mueller
 * Added remote messages for 'read-cd' progress used by a GUI.
 *
 * Revision 1.1  1999/11/07 09:17:08  mueller
 * Initial revision
 *
 */

#ifndef __REMOTE_H__
#define __REMOTE_H__

#define PGSMSG_MIN PGSMSG_RCD_ANALYZING
#define PGSMSG_RCD_ANALYZING   1
#define PGSMSG_RCD_EXTRACTING  2 
#define PGSMSG_WCD_LEADIN      3
#define PGSMSG_WCD_DATA        4
#define PGSMSG_WCD_LEADOUT     5
#define PGSMSG_BLK             6
#define PGSMSG_MAX PGSMSG_BLK

struct ProgressMsg {
  int status;         // see PGSMSG_* constants
  int totalTracks;    // total number of tracks
  int track;          // actually written track
  int trackProgress;  // progress for current track 0..1000
  int totalProgress;  // total writing progress 0..1000
  int bufferFillRate; // buffer fill rate 0..100
};

#endif

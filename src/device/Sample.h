/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998  Andreas Mueller <mueller@daneb.ping.de>
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
 * $Log$
 * Revision 1.1  2001/10/15 21:38:15  trueg
 * new added
 *
 * Revision 1.1.1.1  2000/02/05 01:32:25  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 * Revision 1.2  1998/09/22 19:15:49  mueller
 * Added setting of sample data.
 *
 */

#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#define SAMPLES_PER_BLOCK 588

// represents one audio sample
struct Sample {
    unsigned char msbLeft;
    unsigned char lsbLeft;
    unsigned char msbRight;
    unsigned char lsbRight;

    short left (  ) const {
        return ( msbLeft << 8 ) | lsbLeft;
    } short right (  ) const {
        return ( msbRight << 8 ) | lsbRight;
    } void left ( short d ) {
        msbLeft = d >> 8;
        lsbLeft = d;
    } void right ( short d ) {
        msbRight = d >> 8;
        lsbRight = d;
    }

    void swap (  );
};

inline void Sample::swap (  )
{
    char tmp;

    tmp = msbLeft;
    msbLeft = lsbLeft;
    lsbLeft = tmp;

    tmp = msbRight;
    msbRight = lsbRight;
    lsbRight = tmp;
}

#endif

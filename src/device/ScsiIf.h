/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998, 1999  Andreas Mueller <mueller@daneb.ping.de>
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
 * Revision 1.1.1.1  2000/02/05 01:35:11  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 * Revision 1.4  1999/04/02 16:44:30  mueller
 * Removed 'revisionDate' because it is not available in general.
 *
 * Revision 1.3  1999/03/27 20:53:25  mueller
 * Added argument 'showMessage' to 'sendCmd()'.
 * Introduced function 'printError()' to print error message for last
 * failed command.
 *
 * Revision 1.2  1998/08/13 19:13:28  mueller
 * Added member function 'timout()' to set timeout of SCSI commands.
 *
 */

#ifndef __SCSIIF_H__
#define __SCSIIF_H__

class ScsiIfImpl;

class ScsiIf {
  public:

    ScsiIf ( const char *dev );
     ~ScsiIf (  );

    const char *vendor (  ) const {
        return vendor_;
    } const char *product (  ) const {
        return product_;
    } const char *revision (  ) const {
        return revision_;
    } int init (  );

    int maxDataLen (  ) const {
        return maxDataLen_;
    } int sendCmd ( const unsigned char *cmd, int cmdLen,
                    const unsigned char *dataOut, int dataOutLen,
                    unsigned char *dataIn, int dataInLen, int showMessage = 1 );

    const unsigned char *getSense ( int &len ) const;

    void printError (  );

    // sets new timeout (seconds) and returns old timeout
    int timeout ( int );

    // checks if unit is ready
    // return: 0: OK, ready
    //         1: not ready (busy)
    //         2: not ready, no disk in drive
    //         3: scsi command failed
    //int testUnitReady();

    struct ScanData {
        int bus;
        int id;
        int lun;

        char vendor[9];
        char product[17];
        char revision[5];
    };

    // scans for all SCSI devices and returns a newly allocated 'ScanData' array.
    static ScanData *scan ( int *len );

  private:
    char vendor_[9];
    char product_[17];
    char revision_[5];

    int maxDataLen_;

    int inquiry (  );

    ScsiIfImpl *impl_;
};

#endif

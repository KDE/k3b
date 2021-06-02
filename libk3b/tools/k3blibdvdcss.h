/*

    SPDX-FileCopyrightText: 2004-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_LIBDVDCSS_H_
#define _K3B_LIBDVDCSS_H_

#include "k3b_export.h"

namespace K3b {
    namespace Device {
        class Device;
    }


    /**
     * Wrapper class for libdvdcss. dynamically opens the library if it
     * is available on the system.
     */
    class LIBK3B_EXPORT LibDvdCss
    {
    public:
        ~LibDvdCss();

        static const int DVDCSS_BLOCK_SIZE = 2048;
        static const int DVDCSS_NOFLAGS = 0;
        static const int DVDCSS_READ_DECRYPT = (1 << 0);
        static const int DVDCSS_SEEK_MPEG = (1 << 0);
        static const int DVDCSS_SEEK_KEY = (1 << 1);

        /**
         * Try to open a Video DVD and authenticate it.
         * @return true if the Video DVD could be authenticated successfully, false otherwise.
         */
        bool open( Device::Device* dev );
        void close();

        int seek( int sector, int flags );
        int read( void* buffer, int sectors, int flags );

        /**
         * This method optimized the seek calls to maximize reading performance.
         * It also makes sure we never read unscrambled and scrambled data at the same time.
         *
         * You have to call crackAllKeys() before using this. Do never call this in combination
         * with seek or read!
         */
        int readWrapped( void* buffer, int firstSector, int sectors );

        /**
         * Cache all CSS keys to guarantee smooth reading further on.
         * This method also creates a title offset list which is needed by readWrapped.
         */
        bool crackAllKeys();

        /**
         * returns 0 if the libdvdcss could not
         * be found on the system.
         * Otherwise you have to take care of
         * deleting.
         */
        static LibDvdCss* create();

    private:
        class Private;
        Private* d;

        LibDvdCss();
    };
}

#endif

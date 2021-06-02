/* 

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef _K3B_CRC_H_
#define _K3B_CRC_H_

#include <qglobal.h>

namespace K3b {
    namespace Device
    {
        //  static Crc* x25();

        // bool check( unsigned char* message, unsigned int len, unsigned char* crc, unsigned int crcLen );

        quint16 calcX25( unsigned char* message, unsigned int len, quint16 start = 0x0000 );

        /**
         * subdata is 12 bytes in long.
         */
        bool checkQCrc( unsigned char* subdata );
    }
}

#endif

/* 
 *
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_ENCODING_CONVERTER_H_
#define _K3B_ENCODING_CONVERTER_H_

#include <QByteArray>

namespace K3b {
    
class EncodingConverter
{
public:
    EncodingConverter();
    ~EncodingConverter();

    /**
    * Check if a string is encoded using the local codeset
    *
    * \return True if the string is encoded in the local encoding.
    */
    bool encodedLocally( const QByteArray& s );

private:
    class Private;
    Private* d;
};

}

#endif

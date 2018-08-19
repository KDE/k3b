/* 
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_MUSICBRAINZ_H_
#define _K3B_MUSICBRAINZ_H_

#include <config-k3b.h>

#include <QByteArray>
#include <QString>


/**
 * A wrapper class around libmusicbrainz. Use in combination with TRM.
 *
 * Tries to determine the artist and title related to a trm.
 */
namespace K3b {
class MusicBrainz
{
public:
    MusicBrainz();
    ~MusicBrainz();

    /**
     * \return number of found results.
     */
    int query( const QByteArray& trm );

    QString title( unsigned int i = 0 ) const;
    QString artist( unsigned int i = 0 ) const;

private:
    class Private;
    Private* d;
};
}


#endif

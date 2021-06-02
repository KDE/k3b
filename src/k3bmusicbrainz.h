/* 

    SPDX-FileCopyrightText: 2005-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
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

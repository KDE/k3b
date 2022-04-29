/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3BPATTERNPARSER_H
#define K3BPATTERNPARSER_H

#include <QString>

namespace KCDDB {
    class CDInfo;
}


/**
 *@author Sebastian Trueg
 */
namespace K3b {
    class PatternParser
    {
    public:
        static QString parsePattern( const KCDDB::CDInfo& entry,
                                     int trackNumber,
                                     const QString& fileExtension,
                                     const QString& pattern,
                                     bool replace = false,
                                     const QString& replaceString = "_" );

    private:
        enum {
            TITLE   = 't',
            ARTIST  = 'a',
            NUMBER  = 'n',
            COMMENT = 'c',
            YEAR    = 'y',
            GENRE   = 'g',
            ALBUMTITLE   = 'T',
            ALBUMARTIST  = 'A',
            ALBUMCOMMENT = 'C',
            DATE = 'd',
            EXTENSION = 'e'
        };
    };
}

#endif

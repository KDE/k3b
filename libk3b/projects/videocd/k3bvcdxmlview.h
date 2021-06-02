/*

    SPDX-FileCopyrightText: 2003-2004 Christian Kvasny <chris@k3b.org>
    THX to Manfred Odenstein <odix@chello.at>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_VCD_XMLVIEW_H
#define K3B_VCD_XMLVIEW_H

#include "k3bvcddoc.h"

#include <QString>

class QFile;

namespace K3b {
    class VcdTrack;

    class VcdXmlView
    {

    public:
        explicit VcdXmlView( VcdDoc* doc );
        ~VcdXmlView();

        void write( QFile& file );
        const QString& xmlString() const;

    private:
        class Private;
        Private* d;
    };
}

#endif

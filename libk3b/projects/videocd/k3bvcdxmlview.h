/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*             THX to Manfred Odenstein <odix@chello.at>
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
        VcdXmlView( VcdDoc* doc );
        ~VcdXmlView();

        void write( QFile& file );
        const QString& xmlString() const;

    private:
        class Private;
        Private* d;
    };
}

#endif

/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
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

#ifndef K3BVCDLISTVIEWITEM_H
#define K3BVCDLISTVIEWITEM_H

#include <k3blistview.h>

namespace K3b {
    class VcdTrack;
}

namespace K3b {
class VcdListViewItem : public ListViewItem
{

    public:
        VcdListViewItem( VcdTrack* track, ListView* parent );
        VcdListViewItem( VcdTrack* track, ListView* parent, Q3ListViewItem* after );
        ~VcdListViewItem();

        /** reimplemented from QListViewItem */
        QString text( int i ) const;

        /** reimplemented from QListViewItem */
        void setText( int col, const QString& text );

        /** reimplemented from QListViewItem */
        QString key( int column, bool a ) const;
        bool animate();

        VcdTrack* vcdTrack()
        {
            return m_track;
        }

    private:
        VcdTrack* m_track;
};
}

#endif

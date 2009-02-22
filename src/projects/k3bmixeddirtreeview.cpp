/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bmixeddirtreeview.h"

#include "k3bmixeddoc.h"
#include "k3baudiotrackaddingdialog.h"
#include <k3blistview.h>
#include <k3baudiodoc.h>
#include <k3bdataviewitem.h>

#include <qevent.h>
//Added by qt3to4:
#include <QDropEvent>

#include <kdebug.h>
#include <kiconloader.h>
#include <k3urldrag.h>
#include <klocale.h>


class K3b::MixedDirTreeView::PrivateAudioRootViewItem : public K3b::ListViewItem
{
public:
    PrivateAudioRootViewItem( K3b::MixedDoc* doc, Q3ListView* parent, Q3ListViewItem* after )
        : K3b::ListViewItem( parent, after ),
          m_doc(doc)
    {
        setPixmap( 0, SmallIcon("audio-x-generic") );
    }

    QString text( int col ) const {
        if( col == 0 )
            return i18n("Audio Tracks") + QString(" (%1)" ).arg(m_doc->audioDoc()->numOfTracks());
        else
            return QString();
    }

private:
    K3b::MixedDoc* m_doc;
};


K3b::MixedDirTreeView::MixedDirTreeView( K3b::View* view, K3b::MixedDoc* doc, QWidget* parent )
    : K3b::DataDirTreeView( view, doc->dataDoc(), parent ), m_doc(doc)
{
//     m_audioRootItem = new PrivateAudioRootViewItem( doc, this, root() );

//     connect( this, SIGNAL(selectionChanged(Q3ListViewItem*)),
//              this, SLOT(slotSelectionChanged(Q3ListViewItem*)) );
//     connect( m_doc->audioDoc(), SIGNAL(changed()), this, SLOT(slotNewAudioTracks()) );
}


K3b::MixedDirTreeView::~MixedDirTreeView()
{
}

#warning Port the mixed dragndrop to model/view
#if 0
void K3b::MixedDirTreeView::slotDropped( QDropEvent* e, Q3ListViewItem* parent, Q3ListViewItem* after )
{
    if( !e->isAccepted() )
        return;

    Q3ListViewItem* droppedItem = itemAt(e->pos());
    if( droppedItem == m_audioRootItem ) {
        KUrl::List urls;
        if( K3URLDrag::decode( e, urls ) ) {
            K3b::AudioTrackAddingDialog::addUrls( urls, m_doc->audioDoc(), 0, 0, 0, this );
        }
    }
    else
        K3b::DataDirTreeView::slotDropped( e, parent, after );
}


void K3b::MixedDirTreeView::slotSelectionChanged( Q3ListViewItem* i )
{
    if( i == m_audioRootItem )
        emit audioTreeSelected();
    else
        emit dataTreeSelected();
}


void K3b::MixedDirTreeView::slotNewAudioTracks()
{
    // update the tracknumber
    m_audioRootItem->repaint();
}
#endif
#include "k3bmixeddirtreeview.moc"

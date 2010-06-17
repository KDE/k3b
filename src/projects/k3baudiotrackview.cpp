/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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


#include "k3baudiotrackview.h"
#include "k3baudiodoc.h"
#include "k3baudioprojectmodel.h"

#include <QHeaderView>

namespace K3b {


AudioTrackView::AudioTrackView( AudioDoc* doc, QWidget* parent )
:
    QTreeView( parent ),
    m_doc( doc )
{
    setAcceptDrops( true );
    setDragEnabled( true );
    setDragDropMode( QTreeView::DragDrop );
    setItemsExpandable( false );
    setRootIsDecorated( false );
    setSelectionMode( QTreeView::ExtendedSelection );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    // FIXME: make QHeaderView::Interactive the default but connect to model changes and call header()->resizeSections( QHeaderView::ResizeToContents );
    //header()->setResizeMode( QHeaderView::ResizeToContents );
    setEditTriggers( QAbstractItemView::NoEditTriggers );

    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
}


AudioTrackView::~AudioTrackView()
{
}

} // namespace K3b

#include "k3baudiotrackview.moc"

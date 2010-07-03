/*
 *
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudioprojectdelegate.h"
#include "k3baudioprojectmodel.h"

#include <KIcon>
#include <QAbstractItemView>

namespace K3b {


AudioProjectDelegate::AudioProjectDelegate( QAbstractItemView& view, QObject* parent )
:
    QStyledItemDelegate( parent ),
    m_view( view )
{
}


AudioProjectDelegate::~AudioProjectDelegate()
{
}


void AudioProjectDelegate::setPlayingTrack( const QModelIndex& index )
{
    if( m_playingTrack.isValid() ) {
        m_view.viewport()->update( m_view.visualRect( m_playingTrack ) );
    }

    if( index.isValid() ) {
        m_playingTrack = index.model()->index( index.row(), AudioProjectModel::TitleColumn );
        m_view.viewport()->update( m_view.visualRect( m_playingTrack ) );
    }
    else {
        m_playingTrack = QPersistentModelIndex();
    }
}


void AudioProjectDelegate::initStyleOption( QStyleOptionViewItem* option, const QModelIndex& index ) const
{
    QStyledItemDelegate::initStyleOption( option, index );
    if( option->version == 4 && index.isValid() && index == m_playingTrack ) {
        QStyleOptionViewItemV4* optionV4 = static_cast<QStyleOptionViewItemV4*>( option );
        optionV4->icon = KIcon( "media-playback-start" );
        optionV4->features |= QStyleOptionViewItemV2::HasDecoration;
    }
}

} // namespace K3b

#include "k3baudioprojectdelegate.moc"

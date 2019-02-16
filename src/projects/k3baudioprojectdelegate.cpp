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

#include <QIcon>
#include <QKeyEvent>
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


QWidget* AudioProjectDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    m_current = index;
    QWidget* widget = QStyledItemDelegate::createEditor( parent, option, index );
    widget->installEventFilter( const_cast<AudioProjectDelegate*>( this ) );
    return widget;
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
    if( index.isValid() && index == m_playingTrack ) {
        option->icon = QIcon::fromTheme( "media-playback-start" );
        option->features |= QStyleOptionViewItem::HasDecoration;
    }
}


bool AudioProjectDelegate::eventFilter( QObject* object, QEvent* event )
{
    if( event->type() == QEvent::KeyPress ) {
        QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>( event );
        if( keyEvent->key() == Qt::Key_Return ) {
            QWidget* editor = dynamic_cast<QWidget*>( object );
            Q_EMIT commitData( editor );
            if( m_current.row() < m_current.model()->rowCount( m_current.parent() ) - 1 )
                Q_EMIT closeEditor( editor, EditNextItem );
            else
                Q_EMIT closeEditor( editor, NoHint );
            event->accept();
            return true;
        }
        else if( keyEvent->key() == Qt::Key_Up ) {
            Q_EMIT closeEditor( dynamic_cast<QWidget*>( object ), EditPreviousItem );
            event->accept();
            return true;
        }
        else if( keyEvent->key() == Qt::Key_Down ) {
            Q_EMIT closeEditor( dynamic_cast<QWidget*>( object ), EditNextItem );
            event->accept();
            return true;
        }
    }
    return QStyledItemDelegate::eventFilter( object, event );
}

} // namespace K3b



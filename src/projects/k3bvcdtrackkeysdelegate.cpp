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

#include "k3bvcdtrackkeysdelegate.h"
#include "k3bvcdtrackkeysmodel.h"
#include "k3bvcdtrack.h"

#include <QComboBox>

Q_DECLARE_METATYPE( K3b::VcdTrack* )

namespace K3b {


class VcdTrackKeysDelegate::Private
{
public:
    Private( QList<VcdTrack*>& t ) : tracks( t ) {}
    
    QList<VcdTrack*>& tracks;
};


VcdTrackKeysDelegate::VcdTrackKeysDelegate( QList<VcdTrack*>& tracks, QObject* parent )
    : QStyledItemDelegate( parent ),
      d( new Private( tracks ) )
{
}


VcdTrackKeysDelegate::~VcdTrackKeysDelegate()
{
    delete d;
}


QWidget* VcdTrackKeysDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& index ) const
{
    if( index.isValid() && index.column() == VcdTrackKeysModel::PlayingColumn ) {
        QComboBox* combobox = new QComboBox( parent );
        combobox->addItem( QString(), QVariant() );
        Q_FOREACH( VcdTrack* track, d->tracks ) {
            combobox->addItem(
                VcdTrackKeysModel::trackIcon( track ),
                VcdTrackKeysModel::trackName( track ),
                QVariant::fromValue( track ) );
        }
        combobox->addItem(
            VcdTrackKeysModel::trackIcon( 0 ),
            VcdTrackKeysModel::trackName( 0 ),
            QVariant::fromValue<VcdTrack*>( 0 ) );
        return combobox;
    }
    else {
        return 0;
    }
}


void VcdTrackKeysDelegate::setEditorData( QWidget* editor, const QModelIndex& index ) const
{
    if( QComboBox* combobox = qobject_cast<QComboBox*>( editor ) ) {
        QVariant data = index.data( VcdTrackKeysModel::TrackRole );
        combobox->setCurrentIndex( combobox->findData( data ) );
    }
}


void VcdTrackKeysDelegate::setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const
{
    if( QComboBox* combobox = qobject_cast<QComboBox*>( editor ) ) {
        model->setData( index, combobox->itemData( combobox->currentIndex() ) );
    }
}


void VcdTrackKeysDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/ ) const
{
    editor->setGeometry( option.rect );
}

} // namespace K3b

/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_PROJECT_MODEL_H_
#define _K3B_AUDIO_PROJECT_MODEL_H_

#include <QAbstractItemModel>

namespace K3b {

    class AudioDataSource;
    class AudioDoc;
    class AudioTrack;

    class AudioProjectModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        AudioProjectModel( AudioDoc* doc, QObject* parent );
        ~AudioProjectModel() override;

        enum Columns {
            TrackNumberColumn = 0,
            TitleColumn,
            ArtistColumn,
            TypeColumn,
            LengthColumn,
            FilenameColumn,
            NumColumns
        };

        AudioDoc* project() const;

        AudioTrack* trackForIndex( const QModelIndex& index ) const;
        AudioDataSource* sourceForIndex( const QModelIndex& index ) const;

        QModelIndex indexForTrack( const AudioTrack* track ) const;
        QModelIndex indexForSource( AudioDataSource* source ) const;

        int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
        QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
        QVariant headerData ( int section, Qt::Orientation orientation, int role ) const override;
        Qt::ItemFlags flags( const QModelIndex& index ) const override;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;
        QModelIndex parent( const QModelIndex& index ) const override;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
        bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole ) override;
        QMimeData* mimeData( const QModelIndexList& indexes ) const override;
        Qt::DropActions supportedDragActions() const override;
        Qt::DropActions supportedDropActions() const override;
        QStringList mimeTypes() const override;
        bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent ) override;

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_trackAboutToBeAdded( int position ) )
        Q_PRIVATE_SLOT( d, void _k_trackAdded() )
        Q_PRIVATE_SLOT( d, void _k_trackAboutToBeRemoved( int position ) )
        Q_PRIVATE_SLOT( d, void _k_trackRemoved() )
        Q_PRIVATE_SLOT( d, void _k_sourceAboutToBeAdded( K3b::AudioTrack* parent, int position ) )
        Q_PRIVATE_SLOT( d, void _k_sourceAdded() )
        Q_PRIVATE_SLOT( d, void _k_sourceAboutToBeRemoved( K3b::AudioTrack* parent, int position ) )
        Q_PRIVATE_SLOT( d, void _k_sourceRemoved() )
    };
}

#endif

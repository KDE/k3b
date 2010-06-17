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

#include <QtCore/QAbstractItemModel>

namespace K3b {
    class AudioDoc;
}
namespace K3b {
    class AudioTrack;
}
namespace K3b {
    class AudioDataSource;
}

namespace K3b {
    class AudioProjectModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        AudioProjectModel( AudioDoc* doc, QObject* parent );
        ~AudioProjectModel();

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

        QModelIndex indexForTrack( AudioTrack* track ) const;
        QModelIndex indexForSource( AudioDataSource* source ) const;

        int columnCount( const QModelIndex& parent = QModelIndex() ) const;
        QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
        QVariant headerData ( int section, Qt::Orientation orientation, int role ) const;
        Qt::ItemFlags flags( const QModelIndex& index ) const;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
        QModelIndex parent( const QModelIndex& index ) const;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const;
        bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
        QMimeData* mimeData( const QModelIndexList& indexes ) const;
        Qt::DropActions supportedDropActions() const;
        QStringList mimeTypes() const;
        bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_docChanged() )
        Q_PRIVATE_SLOT( d, void _k_aboutToAddTrack( int position ) )
        Q_PRIVATE_SLOT( d, void _k_trackAdded( K3b::AudioTrack* ) )
        Q_PRIVATE_SLOT( d, void _k_aboutToRemoveTrack( K3b::AudioTrack* ) )
        Q_PRIVATE_SLOT( d, void _k_trackRemoved() )
        Q_PRIVATE_SLOT( d, void _k_aboutToAddSource( K3b::AudioTrack*, int ) )
        Q_PRIVATE_SLOT( d, void _k_sourceAdded( K3b::AudioTrack*, int ) )
        Q_PRIVATE_SLOT( d, void _k_aboutToRemoveSource( K3b::AudioTrack*, int ) )
        Q_PRIVATE_SLOT( d, void _k_sourceRemoved( K3b::AudioTrack* ) )
    };
}

#endif

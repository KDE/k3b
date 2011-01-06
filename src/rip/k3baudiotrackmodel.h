/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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

#ifndef _K3B_AUDIO_TRACK_MODEL_H_
#define _K3B_AUDIO_TRACK_MODEL_H_

#include <QtCore/QAbstractItemModel>

namespace K3b {
    class Medium;
}

namespace KCDDB {
    class CDInfo;
}

namespace K3b {
    class AudioTrackModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        AudioTrackModel( QObject* parent = 0 );
        ~AudioTrackModel();

        enum Columns {
            TrackNumberColumn = 0,
            ArtistColumn,
            TitleColumn,
            LengthColumn,
            NumColumns
        };

        enum Roles {
            MediumRole = 6000,
            TrackNumberRole,
            ArtistRole,
            TitleRole,
            CommentRole,
            LengthRole
        };

        void setMedium( const Medium& medium );
        Medium medium() const;

        void setCddbInfo( const KCDDB::CDInfo& data );
        KCDDB::CDInfo cddbInfo() const;

        void setTrackChecked( int track, bool checked );
        bool trackChecked( int trackIndex ) const;
        QList<int> checkedTrackIndices() const;

        virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
        virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
        virtual QVariant headerData ( int section, Qt::Orientation orientation, int role ) const;
        virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
        virtual QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
        virtual QModelIndex parent( const QModelIndex& index ) const;
        virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
        virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
        virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
        virtual QStringList mimeTypes() const;

    public Q_SLOTS:
        void checkAll();
        void uncheckAll();

    private:
        class Private;
        Private* const d;
    };
}

#endif

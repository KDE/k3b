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

#ifndef K3BVCDTRACKKEYSMODEL_H
#define K3BVCDTRACKKEYSMODEL_H

#include <QAbstractTableModel>
#include <QMap>


namespace K3b {
    
class VcdTrack;

class VcdTrackKeysModel : public QAbstractTableModel
{
public:
    enum Columns {
        KeyColumn,
        PlayingColumn,
        ColumnCount
    };
    
    enum Roles {
        TrackRole = Qt::UserRole
    };
    
    typedef QMap<int, VcdTrack*> Key2Track;
    
public:
    VcdTrackKeysModel( int keyCount, QObject* parent = 0 );
    ~VcdTrackKeysModel();
    
    int keyCount() const;
    void setKeys( const Key2Track& keys );
    const Key2Track& keys() const;
    
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual QModelIndex buddy( const QModelIndex& index ) const;
    
    static QString trackName( VcdTrack* track );
    static QIcon trackIcon( VcdTrack* track );
    
private:
    class Private;
    Private* const d;
};

}

#endif // K3BVCDTRACKKEYSMODEL_H

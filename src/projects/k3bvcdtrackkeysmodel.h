/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
    explicit VcdTrackKeysModel( int keyCount, QObject* parent = nullptr );
    ~VcdTrackKeysModel() override;
    
    int keyCount() const;
    void setKeys( const Key2Track& keys );
    const Key2Track& keys() const;
    
    Qt::ItemFlags flags( const QModelIndex& index ) const override;
    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
    bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole ) override;
    int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
    int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
    QModelIndex buddy( const QModelIndex& index ) const override;
    
    static QString trackName( VcdTrack* track );
    static QIcon trackIcon( VcdTrack* track );
    
private:
    class Private;
    Private* const d;
};

}

#endif // K3BVCDTRACKKEYSMODEL_H

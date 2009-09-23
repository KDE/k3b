/*
 *
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
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

#ifndef _K3B_VIDEODVDTITLEMODEL_H_
#define _K3B_VIDEODVDTITLEMODEL_H_

#include <QAbstractTableModel>


namespace K3b {
    
namespace VideoDVD { class VideoDVD; }

class VideoDVDTitleModel : public QAbstractTableModel
{
    Q_OBJECT
    
public:
    VideoDVDTitleModel( QObject* parent = 0 );
    ~VideoDVDTitleModel();

    enum Columns {
        TitleColumn = 0,
        PreviewColumn,
        VideoColumn,
        AudioColumn,
        SubpictureColumn,
        NumColumns
    };

    enum Roles {
        ChaptersRole = Qt::UserRole,    // returns QString
        PreviewRole,                    // returns QPixmap
        AspectRatioRole,                // returns QString
        AudioStreamsRole,               // returns QStringList
        SubpictureStreamsRole,          // returns QStringList
        LengthRole
    };
    
    void setVideoDVD( const VideoDVD::VideoDVD& dvd );
    QList<int> selectedTitles() const;
    
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
    
public Q_SLOTS:
    void checkAll();
    void uncheckAll();
    
private Q_SLOTS:
    void slotPreviewDone( bool success );

private:
    class Private;
    Private* d;
};

} // namespace K3b

#endif // _K3B_VIDEODVDTITLEMODEL_H_

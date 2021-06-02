/*

    SPDX-FileCopyrightText: 2009 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
    explicit VideoDVDTitleModel( QObject* parent = 0 );
    ~VideoDVDTitleModel() override;

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

    Qt::ItemFlags flags( const QModelIndex& index ) const override;
    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
    bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole ) override;
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
    int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
    int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
    QModelIndex buddy( const QModelIndex& index ) const override;

public Q_SLOTS:
    void checkAll();
    void uncheckAll();
    void stopPreviewGen();

private Q_SLOTS:
    void slotPreviewDone( bool success );

private:
    class Private;
    Private* d;
};

} // namespace K3b

#endif // _K3B_VIDEODVDTITLEMODEL_H_

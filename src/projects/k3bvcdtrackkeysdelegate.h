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

#ifndef K3B_K3BVCDTRACKKEYSDELEGATE_H
#define K3B_K3BVCDTRACKKEYSDELEGATE_H

#include <QList>
#include <QStyledItemDelegate>


namespace K3b {

class VcdTrack;

class VcdTrackKeysDelegate : public QStyledItemDelegate
{
public:
    explicit VcdTrackKeysDelegate( QList<VcdTrack*>& tracks, QObject* parent = 0 );
    ~VcdTrackKeysDelegate() override;
    
    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
    void setEditorData( QWidget* editor, const QModelIndex& index ) const override;
    void setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const override;
    void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
    
private:
    class Private;
    Private* const d;
};

}

#endif // K3B_K3BVCDTRACKKEYSDELEGATE_H

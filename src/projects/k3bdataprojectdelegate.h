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

#ifndef K3BDATAPROJECTDELEGATE_H
#define K3BDATAPROJECTDELEGATE_H

#include <QMimeDatabase>
#include <QStyledItemDelegate>

namespace K3b {

    class DataProjectDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        explicit DataProjectDelegate( QObject* parent = 0 );
        QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;

    protected:
        bool eventFilter( QObject* obj, QEvent* event ) override;

    private:
        mutable QPersistentModelIndex m_current;
        QMimeDatabase m_mimeDatabase;
    };

}

#endif // K3BDATAPROJECTDELEGATE_H

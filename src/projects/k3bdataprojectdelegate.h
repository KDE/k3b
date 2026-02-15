/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
        explicit DataProjectDelegate( QObject* parent = nullptr );
        QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;

    protected:
        bool eventFilter( QObject* obj, QEvent* event ) override;

    private:
        mutable QPersistentModelIndex m_current;
        QMimeDatabase m_mimeDatabase;
    };

}

#endif // K3BDATAPROJECTDELEGATE_H

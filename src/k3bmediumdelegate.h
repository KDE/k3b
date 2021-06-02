/*

    SPDX-FileCopyrightText: 2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_MEDIUM_DELEGATE_H_
#define _K3B_MEDIUM_DELEGATE_H_

#include <QStyledItemDelegate>

namespace K3b {
    class MediumDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        explicit MediumDelegate( QObject* parent );
        ~MediumDelegate() override;

        // FIXME: move this elsewhere
        enum CustomRoles {
            MediumRole = 7777
        };

        QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
        void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
    };
}

#endif

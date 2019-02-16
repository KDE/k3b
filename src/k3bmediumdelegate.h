/*
 *
 * Copyright (C) 2009 Sebastian Trueg <trueg@k3b.org>
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

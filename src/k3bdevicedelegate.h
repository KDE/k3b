/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DEVICE_DELEGATE_H_
#define _K3B_DEVICE_DELEGATE_H_

#include <KIOWidgets/KFileItemDelegate>

namespace K3b {
    class DeviceDelegate : public KFileItemDelegate
    {
        Q_OBJECT

    public:
        explicit DeviceDelegate( QObject* parent );
        ~DeviceDelegate();

        QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
        void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    };
}

#endif

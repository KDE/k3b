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

#ifndef _K3B_VIDEODVDTITLEDELEGATE_H_
#define _K3B_VIDEODVDTITLEDELEGATE_H_

#include <QStyledItemDelegate>

namespace K3b {

    class VideoDVDTitleDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        explicit VideoDVDTitleDelegate( QObject* parent = 0 );
        ~VideoDVDTitleDelegate() override;
        void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
        QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
        
    protected:
        void initStyleOption( QStyleOptionViewItem* option, const QModelIndex& index ) const override;
    };

} // namespace K3b

#endif // _K3B_VIDEODVDTITLEDELEGATE_H_

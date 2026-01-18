/*
    SPDX-FileCopyrightText: 2009 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_VIDEODVDTITLEDELEGATE_H_
#define _K3B_VIDEODVDTITLEDELEGATE_H_

#include <QStyledItemDelegate>

namespace K3b {

    class VideoDVDTitleDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        explicit VideoDVDTitleDelegate( QObject* parent = nullptr );
        ~VideoDVDTitleDelegate() override;
        void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
        QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
        
    protected:
        void initStyleOption( QStyleOptionViewItem* option, const QModelIndex& index ) const override;
    };

} // namespace K3b

#endif // _K3B_VIDEODVDTITLEDELEGATE_H_

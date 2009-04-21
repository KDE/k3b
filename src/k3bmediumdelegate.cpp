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

#include "k3bmediumdelegate.h"
#include "k3bmedium.h"f

#include <QtGui/QPainter>
#include <QtCore/QModelIndex>
#include <QtGui/QTextDocument>

Q_DECLARE_METATYPE(K3b::Medium)

static const int s_margin = 4;

K3b::MediumDelegate::MediumDelegate( QObject* parent )
    : QStyledItemDelegate( parent )
{
}


K3b::MediumDelegate::~MediumDelegate()
{
}


QSize K3b::MediumDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Medium medium = index.data( MediumRole ).value<Medium>();
    if( medium.isValid() ) {
        QTextDocument doc;
        doc.setHtml( medium.longString() );
        return doc.size().toSize() + QSize( s_margin, s_margin );
    }
    else {
        return QStyledItemDelegate::sizeHint( option, index );
    }
}


void K3b::MediumDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Medium medium = index.data( MediumRole ).value<Medium>();
    if( medium.isValid() ) {
        painter->save();

        if ( option.state & QStyle::State_Selected ) {
            painter->fillRect( option.rect, option.palette.highlight() );
        }

        QRect rect = option.rect.adjusted( s_margin, s_margin, -s_margin, -s_margin );

        QTextDocument doc;

        // QTextDocument ignores the QPainter color settings
        doc.setHtml( QString("<html><body><div style=\"color:%1\">%2</div></body></html>")
                     .arg( option.state & QStyle::State_Selected
                           ? option.palette.color( QPalette::HighlightedText ).name()
                           : option.palette.color( QPalette::Text ).name() )
                     .arg( medium.longString() ) );

        int iconSize = doc.size().height();
        QPixmap icon = medium.icon().pixmap( iconSize, iconSize );
        int iconPos = option.rect.top() + (option.rect.height() - icon.height())/2;

        painter->save();
        painter->setOpacity( 0.3 );
        painter->drawPixmap( rect.right() - icon.width(), iconPos, icon );
        painter->restore();

        painter->translate( rect.topLeft() );
        doc.drawContents( painter );

        painter->restore();
    }
    else {
        QStyledItemDelegate::paint( painter, option, index );
    }
}

#include "k3bmediumdelegate.moc"

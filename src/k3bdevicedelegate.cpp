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

#include "k3bdevicedelegate.h"
#include "k3bdevicemodel.h"

#include <QtGui/QPainter>

#include <KIconEffect>
#include <KIconLoader>


// A lot of code is from KFileItemDelegate. Sadly this is all hidden in the private stuff
namespace {
    QPixmap decoration( const QStyleOptionViewItem& option, const QModelIndex& index, const QSize& size )
    {
        QIcon icon = qvariant_cast<QIcon>( index.data( Qt::DecorationRole ) );
        QPixmap pixmap = icon.pixmap( size );
        if (!pixmap.isNull())
        {
            // If the item is selected, and the selection rectangle only covers the
            // text label, blend the pixmap with the highlight color.
            if (!option.showDecorationSelected && option.state & QStyle::State_Selected)
            {
                QPainter p(&pixmap);
                QColor color = option.palette.color(QPalette::Highlight);
                color.setAlphaF(0.5);
                p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
                p.fillRect(pixmap.rect(), color);
            }

            if ( option.state & QStyle::State_MouseOver ) {
                KIconEffect *effect = KIconLoader::global()->iconEffect();

                // Note that in KIconLoader terminology, active = hover.
                // ### We're assuming that the icon group is desktop/filemanager, since this
                //     is KFileItemDelegate.
                if (effect->hasEffect(KIconLoader::Desktop, KIconLoader::ActiveState))
                    return effect->apply(pixmap, KIconLoader::Desktop, KIconLoader::ActiveState);
            }
        }

        return pixmap;
    }
}


K3bDeviceDelegate::K3bDeviceDelegate( QObject* parent )
    : KFileItemDelegate( parent )
{
}


K3bDeviceDelegate::~K3bDeviceDelegate()
{
}


QSize K3bDeviceDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( index.data( K3bDeviceModel::IsDevice ).toBool() ) {
        const int margin = 4;

        QFont font( option.font );
        QFont blockDeviceFont( font );
        blockDeviceFont.setPointSize( blockDeviceFont.pointSize()-2 );
        blockDeviceFont.setBold( false );
        blockDeviceFont.setItalic( true );
        QFont mediumFont( font );
        mediumFont.setBold( true );
        mediumFont.setPointSize( mediumFont.pointSize()+2 );

        int iconHeight = QFontMetrics( font ).height() + margin + QFontMetrics( blockDeviceFont ).height();

        QString text1 = index.data( K3bDeviceModel::Vendor ).toString() + " - " + index.data( K3bDeviceModel::Description ).toString();

        QString text2 = index.data( K3bDeviceModel::BlockDevice ).toString();

        QString text3 = index.data( Qt::DisplayRole ).toString();

        QFontMetrics fontM( font );
        QFontMetrics blockDeviceFontM( blockDeviceFont );
        QFontMetrics mediumFontM( mediumFont );

        return QSize( qMax( iconHeight + margin + qMax( fontM.width( text1 ), blockDeviceFontM.width( text2 ) ),
                            mediumFontM.width( text3 ) ),
                      iconHeight + margin + mediumFontM.height() + margin );
    }
    else {
        return KFileItemDelegate::sizeHint( option, index );
    }
}


void K3bDeviceDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( index.data( K3bDeviceModel::IsDevice ).toBool() ) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        if ( option.state & QStyle::State_Selected ) {
            painter->fillRect( option.rect, option.palette.highlight() );
        }

        const int margin = 4;

        QFont font( option.font );
        QFont blockDeviceFont( font );
        blockDeviceFont.setPointSize( blockDeviceFont.pointSize()-2 );
        blockDeviceFont.setBold( false );
        blockDeviceFont.setItalic( true );
        QFont mediumFont( font );
        mediumFont.setBold( true );
        mediumFont.setPointSize( mediumFont.pointSize()+2 );

        QFontMetrics fontM( font );
        QFontMetrics blockDeviceFontM( blockDeviceFont );
        QFontMetrics mediumFontM( mediumFont );

        int iconHeight = fontM.height() + margin + blockDeviceFontM.height();

        // draw decoration
        QPixmap pix = decoration( option, index, QSize( iconHeight, iconHeight ) );
        painter->drawPixmap( option.rect.topLeft(), pix );

        // draw fixed device text
        painter->setFont( font );
        QString text = index.data( K3bDeviceModel::Vendor ).toString() + " - " + index.data( K3bDeviceModel::Description ).toString();
        painter->drawText( option.rect.left() + pix.width() + margin, option.rect.top() + fontM.height(), text );
        painter->setFont( blockDeviceFont );
        text = index.data( K3bDeviceModel::BlockDevice ).toString();
        painter->drawText( option.rect.left() + pix.width() + margin, option.rect.top() + fontM.height() + margin + blockDeviceFontM.height(), text );

        // draw medium text
        painter->setFont( mediumFont );
        text = index.data( Qt::DisplayRole ).toString();
        painter->drawText( option.rect.left(), option.rect.top() + pix.height() + margin + mediumFontM.height(), text );

        painter->restore();
    }
    else {
        KFileItemDelegate::paint( painter, option, index );
    }
}

#include "k3bdevicedelegate.moc"

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


// FIXME: Get the whole animated hovering code from KFileItemDelegate and put it into a generic class which we can then reuse here
//        To keep KFileItemDelegate BC it could simply forward calls to a subclass of the new generic delegate.


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


K3b::DeviceDelegate::DeviceDelegate( QObject* parent )
    : KFileItemDelegate( parent )
{
}


K3b::DeviceDelegate::~DeviceDelegate()
{
}


QSize K3b::DeviceDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( index.data( K3b::DeviceModel::IsDevice ).toBool() ) {
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

        int iconHeight = blockDeviceFontM.height() + margin + mediumFontM.height();

        QString text1 = index.data( K3b::DeviceModel::Vendor ).toString() + " - " + index.data( K3b::DeviceModel::Description ).toString();

        QString text2 = index.data( K3b::DeviceModel::BlockDevice ).toString();

        QString text3 = index.data( Qt::DisplayRole ).toString();


        return QSize( iconHeight + margin + qMax( blockDeviceFontM.width( text1 + " " + text2 ), mediumFontM.width( text3 ) ),
                      iconHeight + margin );
    }
    else {
        return KFileItemDelegate::sizeHint( option, index );
    }
}


void K3b::DeviceDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( index.data( K3b::DeviceModel::IsDevice ).toBool() ) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // HACK: we erase the branch
        QRect rect( option.rect );
        rect.setLeft( 0 );

        if ( option.state & QStyle::State_Selected ) {
            painter->fillRect( rect, option.palette.highlight() );
            painter->setPen( option.palette.color( QPalette::HighlightedText ) );
        }
        else {
            painter->fillRect( rect, option.palette.base() );
            painter->setPen( option.palette.color( QPalette::WindowText ) );
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
        painter->drawPixmap( rect.topLeft(), pix );

        // draw medium text
        painter->setFont( mediumFont );
        QString text = index.data( Qt::DisplayRole ).toString();
        painter->drawText( rect.left() + pix.width() + margin, rect.top() + mediumFontM.height(), text );

        // draw fixed device text
        painter->setFont( blockDeviceFont );
        text = index.data( K3b::DeviceModel::Vendor ).toString() + " - " + index.data( K3b::DeviceModel::Description ).toString();
        painter->drawText( rect.left() + pix.width() + margin, rect.top() + mediumFontM.height() + margin + blockDeviceFontM.height(), text );
//         painter->setFont( blockDeviceFont );
//         text = index.data( K3b::DeviceModel::BlockDevice ).toString();
//         painter->drawText( rect.left() + pix.width() + margin, rect.top() + fontM.height() + margin + blockDeviceFontM.height(), text );


        painter->restore();
    }
    else {
        KFileItemDelegate::paint( painter, option, index );
    }
}

#include "k3bdevicedelegate.moc"

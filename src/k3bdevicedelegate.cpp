/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdevicedelegate.h"
#include "k3bdevicemodel.h"

#include <QApplication>
#include <QPainter>
#include <QStyle>

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
    
    QFont cloneFont( const QFont& font, int pointSize, bool bold, bool italic )
    {
        QFont cloned( font );
        cloned.setPointSize( pointSize );
        cloned.setBold( bold );
        cloned.setItalic( italic );
        return cloned;
    }
    
    struct FontsAndMetrics
    {
        FontsAndMetrics( const QStyleOptionViewItem& option );
        
        QFont mediumFont;
        QFont blockDeviceFont;
        QFontMetrics fontM;
        QFontMetrics mediumFontM;
        QFontMetrics blockDeviceFontM;
        int margin;
        int iconHeight;
        QRect mediumRect;
        QRect deviceRect;
    };
    
    FontsAndMetrics::FontsAndMetrics( const QStyleOptionViewItem& option )
    :
        mediumFont( cloneFont( option.font, option.font.pointSize()+2, true, false ) ),
        blockDeviceFont( cloneFont( option.font, option.font.pointSize()-2, false, true ) ),
        fontM( option.font ),
        mediumFontM( mediumFont ),
        blockDeviceFontM( blockDeviceFont ),
        margin( 4 ),
        iconHeight( blockDeviceFontM.height() + margin + mediumFontM.height() ),
        mediumRect( option.rect ),
        deviceRect( option.rect )
    {
        mediumRect.setLeft( option.rect.left() + iconHeight + margin );
        mediumRect.setBottom( option.rect.top() + mediumFontM.height() + margin/2 );
        deviceRect.setLeft( option.rect.left() + iconHeight + margin );
        deviceRect.setTop( option.rect.top() + mediumFontM.height() + margin/2 );
    }
    
} // namespace


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
        FontsAndMetrics fam( option );

        QString text1 = index.data( K3b::DeviceModel::Vendor ).toString() + " - " + index.data( K3b::DeviceModel::Description ).toString();
        QString text2 = index.data( K3b::DeviceModel::BlockDevice ).toString();
        QString text3 = index.data( Qt::DisplayRole ).toString();

        return QSize( fam.iconHeight + fam.margin + qMax( fam.blockDeviceFontM.width( text1 + " " + text2 ), fam.mediumFontM.width( text3 ) ),
                      fam.iconHeight + fam.margin );
    }
    else {
        return KFileItemDelegate::sizeHint( option, index );
    }
}


void K3b::DeviceDelegate::paint( QPainter* painter, const QStyleOptionViewItem& optionOrig, const QModelIndex& index ) const
{
    if ( index.data( K3b::DeviceModel::IsDevice ).toBool() ) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // HACK: we erase the branch
        QStyleOptionViewItemV4 option( optionOrig );
        option.rect.setLeft( 0 );

        QStyle* style = QApplication::style();
        FontsAndMetrics fam( option );
        QPalette::ColorRole textRole = (option.state & QStyle::State_Selected) ?
                                        QPalette::HighlightedText : QPalette::WindowText;
        
        // draw background
        style->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

        // draw decoration
        QPixmap pix = decoration( option, index, QSize( fam.iconHeight, fam.iconHeight ) );
        style->drawItemPixmap( painter, option.rect, Qt::AlignLeft | Qt::AlignVCenter, pix );

        // draw medium text
        painter->setFont( fam.mediumFont );
        QString text = index.data( Qt::DisplayRole ).toString();
        style->drawItemText( painter, fam.mediumRect, option.displayAlignment, option.palette,
                             option.state & QStyle::State_Enabled,
                             fam.mediumFontM.elidedText( text, option.textElideMode, fam.mediumRect.width() ),
                             textRole );

        // draw fixed device text
        painter->setFont( fam.blockDeviceFont );
        text = index.data( K3b::DeviceModel::Vendor ).toString() + " - " + index.data( K3b::DeviceModel::Description ).toString();
        style->drawItemText( painter, fam.deviceRect, option.displayAlignment, option.palette,
                             option.state & QStyle::State_Enabled,
                             fam.blockDeviceFontM.elidedText( text, option.textElideMode, fam.deviceRect.width() ),
                             textRole );

        painter->restore();
    }
    else {
        KFileItemDelegate::paint( painter, optionOrig, index );
    }
}

#include "k3bdevicedelegate.moc"

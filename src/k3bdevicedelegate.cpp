/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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

#include <KIconEffect>
#include <KIconLoader>

#include <QPainter>
#include <QApplication>
#include <QStyle>


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
        FontsAndMetrics( const QFont& font );
        
        QFont mediumFont;
        QFont deviceFont;
        QFontMetrics fontM;
        QFontMetrics mediumFontM;
        QFontMetrics deviceFontM;
        int margin;
        int spacing;
    };
    
    FontsAndMetrics::FontsAndMetrics( const QFont& font )
    :
        mediumFont( cloneFont( font, font.pointSize()+2, true, false ) ),
        deviceFont( cloneFont( font, font.pointSize()-2, false, true ) ),
        fontM( font ),
        mediumFontM( mediumFont ),
        deviceFontM( deviceFont ),
        margin( fontM.descent() ),
        spacing( 0 )
    {
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
        const FontsAndMetrics fam( option.font );
        // It seems that width-part of size hint is not used anwyay so
        // we're ommiting here a computation of text's width
        return QSize( 0, fam.margin + fam.mediumFontM.height() + fam.spacing + fam.deviceFontM.height() + fam.margin );
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
        QStyleOptionViewItem option( optionOrig );
        option.rect.setLeft( 0 );
        painter->fillRect( option.rect, option.palette.base() );

        QStyle* style = QApplication::style();
        const FontsAndMetrics fam( option.font );
        const QPalette::ColorRole textRole = (option.state & QStyle::State_Selected) ?
                                              QPalette::HighlightedText : QPalette::Text;
        
        const QRect itemRect( option.rect.left() + fam.margin, option.rect.top() + fam.margin,
                              option.rect.width() - 2*fam.margin, option.rect.height() - 2*fam.margin );
        const QSize iconSize( itemRect.height(), itemRect.height() );
        const QSize mediumSize( itemRect.width() - iconSize.width() - fam.margin,
                                itemRect.height() - fam.spacing - fam.deviceFontM.height() );
        const QSize devicemSize( itemRect.width() - iconSize.width() - fam.margin,
                                 itemRect.height() - fam.spacing - fam.mediumFontM.height() );
        const QRect iconRect = style->alignedRect( option.direction, Qt::AlignLeft | Qt::AlignVCenter, iconSize, itemRect );
        const QRect mediumRect = style->alignedRect( option.direction, Qt::AlignRight | Qt::AlignTop, mediumSize, itemRect );
        const QRect deviceRect = style->alignedRect( option.direction, Qt::AlignRight | Qt::AlignBottom, devicemSize, itemRect );
        
        // draw background
        style->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

        // draw decoration
        QPixmap pixmap = decoration( option, index, iconSize );
        painter->drawPixmap( iconRect, pixmap );

        // draw medium text
        painter->setFont( fam.mediumFont );
        QString text = index.data( Qt::DisplayRole ).toString();
        style->drawItemText( painter, mediumRect, option.displayAlignment, option.palette,
                             option.state & QStyle::State_Enabled,
                             fam.mediumFontM.elidedText( text, option.textElideMode, mediumRect.width() ),
                             textRole );

        // draw fixed device text
        painter->setFont( fam.deviceFont );
        text = index.data( K3b::DeviceModel::Vendor ).toString() + " - " + index.data( K3b::DeviceModel::Description ).toString();
        style->drawItemText( painter, deviceRect, option.displayAlignment, option.palette,
                             option.state & QStyle::State_Enabled,
                             fam.deviceFontM.elidedText( text, option.textElideMode, deviceRect.width() ),
                             textRole );

        painter->restore();
    }
    else {
        KFileItemDelegate::paint( painter, optionOrig, index );
    }
}



/*
 *
 * Copyright (C) 2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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
#include "k3bmedium.h"

#include <KLocalizedString>

#include <QModelIndex>
#include <QPainter>
#include <QApplication>
#include <QStyle>

Q_DECLARE_METATYPE(K3b::Medium)

namespace
{

    QFont cloneFont( const QFont& font, int pointSize, bool bold )
    {
        QFont cloned( font );
        cloned.setPointSize( pointSize );
        cloned.setBold( bold );
        return cloned;
    }
    
    struct FontsAndMetrics
    {
        FontsAndMetrics( const QFont& font );
        
        QFont titleFont;
        QFontMetrics fontM;
        QFontMetrics titleFontM;
        int margin;
    };
    
    FontsAndMetrics::FontsAndMetrics( const QFont& font )
    :
        titleFont( cloneFont( font, font.pointSize()+2, true ) ),
        fontM( font ),
        titleFontM( font ),
        margin( fontM.descent() )
    {
    }
    
} // namespace

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
        const FontsAndMetrics fam( option.font );
        
        int height = fam.margin + fam.titleFontM.height() + fam.fontM.height() + fam.margin;
        if( medium.diskInfo().diskState() == K3b::Device::STATE_COMPLETE ||
            medium.diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE  )
            height += fam.fontM.height();
        if( medium.diskInfo().diskState() == K3b::Device::STATE_EMPTY ||
            medium.diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE  )
            height += fam.fontM.height();
        if( !medium.diskInfo().empty() && medium.diskInfo().rewritable() )
            height += fam.fontM.height();
        
        return QSize( 0, height );
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

        const FontsAndMetrics fam( option.font );
        const QPalette::ColorRole textRole = (option.state & QStyle::State_Selected) ?
                                              QPalette::HighlightedText : QPalette::Text;
        
        QStyle* style = QApplication::style();

        style->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );

        const QRect rect = option.rect.adjusted( fam.margin, fam.margin, -fam.margin, -fam.margin );
        const QRect titleRect( rect.left(), rect.top(),
                               rect.width(), fam.titleFontM.height() );
        const QRect contentRect( rect.left(), rect.top() + fam.titleFontM.height(),
                                 rect.width(), fam.fontM.height() );
        const QPixmap icon = medium.icon().pixmap( rect.height() );

        painter->save();
        painter->setOpacity( 0.3 );
        style->drawItemPixmap( painter, rect, Qt::AlignRight | Qt::AlignVCenter, icon );
        painter->restore();
        
        painter->setFont( fam.titleFont );
        style->drawItemText( painter, titleRect, option.displayAlignment, option.palette,
                             option.state & QStyle::State_Enabled,
                             fam.titleFontM.elidedText( medium.shortString(), option.textElideMode, titleRect.width() ),
                             textRole );
        
        painter->setFont( option.font );
        
        QString contentText = QString("(%1)").arg( medium.contentTypeString() );
        style->drawItemText( painter, contentRect, option.displayAlignment, option.palette,
                             option.state & QStyle::State_Enabled,
                             fam.titleFontM.elidedText( contentText, option.textElideMode, contentRect.width() ),
                             textRole );
        
        int previousHeights = fam.titleFontM.height() + fam.fontM.height();
        if( medium.diskInfo().diskState() == K3b::Device::STATE_COMPLETE ||
            medium.diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE  ) {
            const QRect tracksRect( rect.left(), rect.top() + previousHeights,
                                    rect.width(), fam.fontM.height() );
            previousHeights += tracksRect.height();
            QString tracksText =  i18np("%2 in %1 track", "%2 in %1 tracks",
                                        medium.toc().count(),
                                        KIO::convertSize(medium.diskInfo().size().mode1Bytes()) );
            if( medium.diskInfo().numSessions() > 1 )
                tracksText += i18np(" and %1 session", " and %1 sessions", medium.diskInfo().numSessions() );
            
            style->drawItemText( painter, tracksRect, option.displayAlignment, option.palette,
                                option.state & QStyle::State_Enabled,
                                fam.titleFontM.elidedText( tracksText, option.textElideMode, tracksRect.width() ),
                                textRole );
        }

        if( medium.diskInfo().diskState() == K3b::Device::STATE_EMPTY ||
            medium.diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE  ) {
            const QRect freeSpaceRect( rect.left(), rect.top() + previousHeights,
                                    rect.width(), fam.fontM.height() );
            previousHeights += freeSpaceRect.height();
            QString freeSpaceText =  i18n("Free space: %1",
                                          KIO::convertSize( medium.diskInfo().remainingSize().mode1Bytes() ) );
            style->drawItemText( painter, freeSpaceRect, option.displayAlignment, option.palette,
                                option.state & QStyle::State_Enabled,
                                fam.titleFontM.elidedText( freeSpaceText, option.textElideMode, freeSpaceRect.width() ),
                                textRole );
        }

        if( !medium.diskInfo().empty() && medium.diskInfo().rewritable() ) {
            const QRect capacityRect( rect.left(), rect.top() + previousHeights,
                                    rect.width(), fam.fontM.height() );
            previousHeights += capacityRect.height();
            QString capacityText =  i18n("Capacity: %1",
                                         KIO::convertSize( medium.diskInfo().capacity().mode1Bytes() ) );
            style->drawItemText( painter, capacityRect, option.displayAlignment, option.palette,
                                option.state & QStyle::State_Enabled,
                                fam.titleFontM.elidedText( capacityText, option.textElideMode, capacityRect.width() ),
                                textRole );
        }
        
        painter->restore();
    }
    else {
        QStyledItemDelegate::paint( painter, option, index );
    }
}



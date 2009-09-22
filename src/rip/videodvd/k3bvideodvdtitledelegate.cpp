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

#include "k3bvideodvdtitledelegate.h"
#include "k3bvideodvdtitlemodel.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>

namespace K3b {

VideoDVDTitleDelegate::VideoDVDTitleDelegate( QObject* parent )
    : QStyledItemDelegate( parent )
{
}

VideoDVDTitleDelegate::~VideoDVDTitleDelegate()
{
}

void VideoDVDTitleDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    painter->save();
    
    QStyle& style = *QApplication::style();
    style.drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );
    QPalette::ColorRole textRole = (option.state & QStyle::State_Selected) ?
                                    QPalette::HighlightedText : QPalette::WindowText;
    if( index.column() == VideoDVDTitleModel::TitleColumn ) {
        QStyleOptionButton checkOption;
        checkOption.direction = option.direction;
        checkOption.fontMetrics = option.fontMetrics;
        checkOption.palette = option.palette;
        checkOption.state |= (option.state & QStyle::State_Enabled) ? QStyle::State_Enabled : QStyle::State_None;
        checkOption.state |= (index.data( Qt::CheckStateRole ).toInt() == Qt::Checked) ? QStyle::State_On : QStyle::State_Off;
        QSize checkSize( style.pixelMetric( QStyle::PM_IndicatorWidth, &checkOption ),
                            style.pixelMetric( QStyle::PM_IndicatorHeight, &checkOption ) );
        checkOption.rect.setRect( option.rect.left(), option.rect.top() + option.rect.height()/2-checkSize.height()/2,
                                    checkSize.width(), checkSize.height() );
        QRect titleRect( option.rect.left() + checkSize.width(), option.rect.top(),
                        option.rect.width()-checkSize.width(), option.rect.height() );
        QRect chaptersRect( option.rect.left() + checkSize.width(), option.rect.top()+option.fontMetrics.height(),
                            option.rect.width()-checkSize.width(), option.rect.height() );
        
        style.drawPrimitive( QStyle::PE_IndicatorCheckBox, &checkOption, painter );
        style.drawItemText( painter, titleRect, Qt::AlignTop | Qt::AlignLeft, option.palette,
                            option.state & QStyle::State_Enabled,
                            option.fontMetrics.elidedText( index.data().toString(),
                                                           option.textElideMode, titleRect.width() ),
                            textRole );
        style.drawItemText( painter, chaptersRect, Qt::AlignTop | Qt::AlignLeft, option.palette,
                            option.state & QStyle::State_Enabled,
                            option.fontMetrics.elidedText( index.data(VideoDVDTitleModel::ChaptersRole).toString(),
                                                           option.textElideMode, chaptersRect.width() ),
                            textRole );
    }
    else if( index.column() == VideoDVDTitleModel::PreviewColumn ) {
        QVariant pixmap = index.data( VideoDVDTitleModel::PreviewRole );
        if( !pixmap.isNull() )
            style.drawItemPixmap( painter, option.rect, Qt::AlignCenter, pixmap.value<QPixmap>() );
    }
    else if( index.column() == VideoDVDTitleModel::VideoColumn ) {
        QRect ratiosRect( option.rect.left(), option.rect.top()+option.fontMetrics.height(),
                          option.rect.width(), option.rect.height() );
        
        style.drawItemText( painter, option.rect, Qt::AlignTop | Qt::AlignLeft, option.palette,
                            option.state & QStyle::State_Enabled,
                            option.fontMetrics.elidedText( index.data().toString(),
                                                           option.textElideMode, option.rect.width() ),
                            textRole );
        style.drawItemText( painter, ratiosRect, Qt::AlignTop | Qt::AlignLeft, option.palette,
                            option.state & QStyle::State_Enabled,
                            option.fontMetrics.elidedText( index.data(VideoDVDTitleModel::AspectRatioRole).toString(),
                                                           option.textElideMode, ratiosRect.width() ),
                            textRole );
    }
    
    painter->restore();
}

QSize VideoDVDTitleDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if( index.column() == VideoDVDTitleModel::PreviewColumn )
    {
        return QSize();
    }
    else
    {
        if( index.column() == VideoDVDTitleModel::TitleColumn )
        {
            QSize titleSize = option.fontMetrics.size( 0, index.data().toString() );
            QSize chaptersSize = option.fontMetrics.size( 0, index.data( VideoDVDTitleModel::ChaptersRole ).toString() );
            return QSize( qMax( titleSize.width(), chaptersSize.width() ),
                          titleSize.height() + titleSize.height() );
        }
        else if( index.column() == VideoDVDTitleModel::VideoColumn ) {
            QSize videoSize = option.fontMetrics.size( 0, index.data().toString() );
            QSize ratioSize = option.fontMetrics.size( 0, index.data( VideoDVDTitleModel::AspectRatioRole ).toString() );
            return QSize( qMax( videoSize.width(), ratioSize.width() ),
                          videoSize.height() + videoSize.height() );
        }
        else
        {
            return QSize();
        }
    }
}

void VideoDVDTitleDelegate::initStyleOption( QStyleOptionViewItem* option, const QModelIndex& index ) const
{
    QStyledItemDelegate::initStyleOption( option, index );
//     if( index.isValid() && index.column() == VideoDVDTitleModel::TitleColumn )
//     {
//         option->state = (index.data( Qt::CheckStateRole ).toInt() == Qt::Checked) ? QStyle::State_On : QStyle::State_Off;
//     }
}

} // namespace K3b

#include "k3bvideodvdtitledelegate.moc"

/*
 *
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

#include "k3bvideodvdtitledelegate.h"
#include "k3bvideodvdtitlemodel.h"

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QApplication>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionViewItem>

namespace K3b {
    
namespace {
    const int indicatorSpacing = 4;
    const int margin = 2;
} // namespace

VideoDVDTitleDelegate::VideoDVDTitleDelegate( QObject* parent )
    : QStyledItemDelegate( parent )
{
}


VideoDVDTitleDelegate::~VideoDVDTitleDelegate()
{
}


void VideoDVDTitleDelegate::paint( QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index ) const
{
    painter->save();
    
    QStyle& style = *QApplication::style();
    QStyleOptionViewItem option = opt;
    initStyleOption( &option, index );
    style.drawControl( QStyle::CE_ItemViewItem, &option, painter );
    
    QPalette::ColorRole textRole = (option.state & QStyle::State_Selected) ?
                                    QPalette::HighlightedText : QPalette::WindowText;
                                    
    if( index.column() == VideoDVDTitleModel::TitleColumn ) {
        QFont bold = option.font;
        bold.setBold( true );
        QFontMetrics boldMetrics( bold );
        
        QStyleOptionButton checkOption;
        checkOption.direction = option.direction;
        checkOption.fontMetrics = option.fontMetrics;
        int checkWidth = style.pixelMetric( QStyle::PM_IndicatorWidth, &checkOption ) + 2*indicatorSpacing;
        QRect titleRect( option.rect.left()+checkWidth, option.rect.top()+margin,
                         option.rect.width()-checkWidth-margin, option.rect.height()-2*margin );
        QRect chaptersRect( option.rect.left()+checkWidth, option.rect.top()+boldMetrics.height()+margin,
                            option.rect.width()-checkWidth-margin, option.rect.height()-2*margin );
        

        painter->setFont( bold );
        style.drawItemText( painter, QStyle::visualRect( option.direction, option.rect, titleRect ),
                            Qt::AlignTop | Qt::AlignLeft, option.palette,
                            option.state & QStyle::State_Enabled,
                            boldMetrics.elidedText( index.data().toString(),
                                                    option.textElideMode, titleRect.width() ),
                            textRole );
        
        painter->setFont( option.font );
        style.drawItemText( painter, QStyle::visualRect( option.direction, option.rect, chaptersRect ),
                            Qt::AlignTop | Qt::AlignLeft, option.palette,
                            option.state & QStyle::State_Enabled,
                            option.fontMetrics.elidedText( index.data(VideoDVDTitleModel::ChaptersRole).toString(),
                                                           option.textElideMode, chaptersRect.width() ),
                            textRole );
    }
    else if( index.column() == VideoDVDTitleModel::PreviewColumn ) {
        QVariant data = index.data( VideoDVDTitleModel::PreviewRole );
        QPixmap preview;
        if( !data.isNull() )
        {
            preview = data.value<QPixmap>().scaled( option.rect.width()-margin,
                                                    option.rect.height()-margin, Qt::KeepAspectRatio );
        }
        else
        {
            preview = QIcon::fromTheme( "image-missing" ).pixmap( qMin( option.rect.width()-margin,
                                                             option.rect.height()-margin ) );
        }
        style.drawItemPixmap( painter, option.rect, Qt::AlignCenter, preview );
    }
    else if( index.column() == VideoDVDTitleModel::VideoColumn ) {
        QRect videoRect( option.rect.left()+margin, option.rect.top()+margin,
                         option.rect.width()-2*margin, option.rect.height()-2*margin );
        QRect ratiosRect( option.rect.left()+margin, option.rect.top()+option.fontMetrics.height()+margin,
                          option.rect.width()-2*margin, option.rect.height()-2*margin );
        
        style.drawItemText( painter, QStyle::visualRect( option.direction, option.rect, videoRect ),
                            Qt::AlignTop | Qt::AlignLeft, option.palette,
                            option.state & QStyle::State_Enabled,
                            option.fontMetrics.elidedText( index.data().toString(),
                                                           option.textElideMode, option.rect.width() ),
                            textRole );
        style.drawItemText( painter, QStyle::visualRect( option.direction, option.rect, ratiosRect ),
                            Qt::AlignTop | Qt::AlignLeft, option.palette,
                            option.state & QStyle::State_Enabled,
                            option.fontMetrics.elidedText( index.data(VideoDVDTitleModel::AspectRatioRole).toString(),
                                                           option.textElideMode, ratiosRect.width() ),
                            textRole );
    }
    else if( index.column() == VideoDVDTitleModel::AudioColumn ) {
        QRect rect( option.rect.left()+margin, option.rect.top()+margin,
                    option.rect.width()-2*margin, option.rect.height()-2*margin );
        int lineHeight = option.fontMetrics.height();
        Q_FOREACH( const QString& line, index.data( VideoDVDTitleModel::AudioStreamsRole ).toStringList() )
        {
            style.drawItemText( painter, QStyle::visualRect( option.direction, option.rect, rect ),
                                Qt::AlignTop | Qt::AlignLeft, option.palette,
                                option.state & QStyle::State_Enabled,
                                option.fontMetrics.elidedText( line, option.textElideMode, option.rect.width() ),
                                textRole );
            rect.setTop( rect.top() + lineHeight );
        }
    }
    else if( index.column() == VideoDVDTitleModel::SubpictureColumn ) {
        QRect rect( option.rect.left()+margin, option.rect.top()+margin,
                    option.rect.width()-2*margin, option.rect.height()-2*margin );
        int lineHeight = option.fontMetrics.height();
        Q_FOREACH( const QString& line, index.data( VideoDVDTitleModel::SubpictureStreamsRole ).toStringList() )
        {
            style.drawItemText( painter, QStyle::visualRect( option.direction, option.rect, rect ),
                                Qt::AlignTop | Qt::AlignLeft, option.palette,
                                option.state & QStyle::State_Enabled,
                                option.fontMetrics.elidedText( line, option.textElideMode, option.rect.width() ),
                                textRole );
            rect.setTop( rect.top() + lineHeight );
        }
    }
    
    painter->restore();
}


QSize VideoDVDTitleDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyle& style = *QApplication::style();
    if( index.column() == VideoDVDTitleModel::TitleColumn ) {
        QStyleOptionButton checkOption;
        checkOption.direction = option.direction;
        checkOption.fontMetrics = option.fontMetrics;
        int checkWidth = style.pixelMetric( QStyle::PM_IndicatorWidth, &checkOption ) + 2*indicatorSpacing;
        QFont bold = option.font;
        bold.setBold( true );
        QFontMetrics boldMetrics( bold );
        QSize titleSize = boldMetrics.size( 0, index.data().toString() );
        QSize chaptersSize = option.fontMetrics.size( 0, index.data( VideoDVDTitleModel::ChaptersRole ).toString() );
        return QSize( qMax( titleSize.width(), chaptersSize.width() ) + checkWidth + margin + option.fontMetrics.averageCharWidth(),
                        titleSize.height() + titleSize.height() + 2*margin );
    }
    else if( index.column() == VideoDVDTitleModel::VideoColumn ) {
        QSize videoSize = option.fontMetrics.size( 0, index.data().toString() );
        QSize ratioSize = option.fontMetrics.size( 0, index.data( VideoDVDTitleModel::AspectRatioRole ).toString() );
        return QSize( qMax( videoSize.width(), ratioSize.width() ) + 2*margin + option.fontMetrics.averageCharWidth(),
                        videoSize.height() + videoSize.height() + 2*margin );
    }
    else if( index.column() == VideoDVDTitleModel::AudioColumn ) {
        QSize overallSize;
        Q_FOREACH( const QString& line, index.data( VideoDVDTitleModel::AudioStreamsRole ).toStringList() )
        {
            QSize lineSize = option.fontMetrics.size( 0, line );
            overallSize.setWidth( qMax( overallSize.width(), lineSize.width() ) );
            overallSize.setHeight( overallSize.height() + lineSize.height() );
        }
        return overallSize + QSize( 2*margin + option.fontMetrics.averageCharWidth(), 2*margin );
    }
    else if( index.column() == VideoDVDTitleModel::SubpictureColumn ) {
        QSize overallSize;
        Q_FOREACH( const QString& line, index.data( VideoDVDTitleModel::SubpictureStreamsRole ).toStringList() )
        {
            QSize lineSize = option.fontMetrics.size( 0, line );
            overallSize.setWidth( qMax( overallSize.width(), lineSize.width() ) );
            overallSize.setHeight( overallSize.height() + lineSize.height() );
        }
        return overallSize + QSize( 2*margin + option.fontMetrics.averageCharWidth(), 2*margin );
    }
    else {
        return QSize();
    }
}


void VideoDVDTitleDelegate::initStyleOption( QStyleOptionViewItem* option, const QModelIndex& index ) const
{
    if( index.isValid() && index.column() == VideoDVDTitleModel::TitleColumn )
    {
        option->index = index;
        QVariant value = index.data( Qt::CheckStateRole );
        if( value.isValid() && !value.isNull() ) {
            option->features |= QStyleOptionViewItem::HasCheckIndicator;
            option->checkState = static_cast<Qt::CheckState>( value.toInt() );
        }
    }
}

} // namespace K3b



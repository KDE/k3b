/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3btitlelabel.h"

#include <k3bstringutils.h>

#include <qpainter.h>
#include <qevent.h>
#include <qfontmetrics.h>
#include <qfont.h>
#include <qtooltip.h>
//Added by qt3to4:
#include <QHelpEvent>
#include <QFrame>


class K3bTitleLabel::Private
{
public:
    Private() {
        titleLength = subTitleLength = 0;
        margin = 2;
        alignment = Qt::AlignLeft;
        cachedMinimumWidth = 0;
        titleBaseLine = 0;
    }

    QString title;
    QString subTitle;

    QString displayTitle;
    QString displaySubTitle;

    int alignment;

    int titleLength;
    int subTitleLength;
    int displayTitleLength;
    int displaySubTitleLength;
    int titleBaseLine;
    int subTitleBaseLine;
    int margin;

    int cachedMinimumWidth;
};



K3bTitleLabel::K3bTitleLabel( QWidget* parent )
    : QFrame( parent )
{
    d = new Private();
}


K3bTitleLabel::~K3bTitleLabel()
{
    delete d;
}


void K3bTitleLabel::setTitle( const QString& title, const QString& subTitle )
{
    d->title = title;
    d->subTitle = subTitle;
    updatePositioning();
    update();
}


void K3bTitleLabel::setSubTitle( const QString& subTitle )
{
    d->subTitle = subTitle;
    updatePositioning();
    update();
}


void K3bTitleLabel::setAlignment( int align )
{
    d->alignment = align;
    update();
}


QSize K3bTitleLabel::sizeHint() const
{
    return QSize( d->titleLength + d->subTitleLength + 2*d->margin, d->titleBaseLine );
}

QSize K3bTitleLabel::minimumSizeHint() const
{
    return QSize( d->cachedMinimumWidth, d->titleBaseLine );
}

void K3bTitleLabel::resizeEvent( QResizeEvent* e )
{
    QFrame::resizeEvent( e );
    updatePositioning();
    update();
}

void K3bTitleLabel::paintEvent( QPaintEvent* )
{

    QPainter p(this);
    QRect r = contentsRect();
    p.eraseRect( r );

    QFont f(font());
    f.setBold(true);
    f.setPointSize( f.pointSize() + 2 );

    p.setFont(f);

    int neededWidth = d->displayTitleLength;
    if( !d->displaySubTitle.isEmpty() )
        neededWidth += d->displaySubTitleLength + 5;

    int startPos = 0;
    if( d->alignment & Qt::AlignHCenter )
        startPos = r.left() + ( r.width() - 2*d->margin - neededWidth ) / 2;
    else if( d->alignment & Qt::AlignRight )
        startPos = r.right() - d->margin - neededWidth;
    else
        startPos = r.left() + d->margin;

    // paint title
    p.drawText( startPos, r.top() + d->titleBaseLine, d->displayTitle );

    if( !d->subTitle.isEmpty() ) {
        f.setBold(false);
        f.setPointSize( f.pointSize() - 4 );
        p.setFont(f);
        p.drawText( startPos + d->displayTitleLength + 5, r.top() + d->subTitleBaseLine, d->displaySubTitle );
    }
}


void K3bTitleLabel::setMargin( int m )
{
    d->margin = m;
    updatePositioning();
    update();
}


void K3bTitleLabel::updatePositioning()
{
    QFont f(font());
    f.setBold(true);
    f.setPointSize( f.pointSize() + 2 );
    QFontMetrics titleFm(f);

    f.setBold(false);
    f.setPointSize( f.pointSize() - 4 );
    QFontMetrics subTitleFm(f);

    d->titleBaseLine = contentsRect().height()/2 + titleFm.height()/2 - titleFm.descent();
    d->titleLength = titleFm.width( d->title );

    d->subTitleBaseLine = d->titleBaseLine - titleFm.underlinePos() + subTitleFm.underlinePos();

    d->subTitleLength = ( d->subTitle.isEmpty() ? 0 : subTitleFm.width( d->subTitle ) );

    // cut the text to window width
    d->displayTitle = d->title;
    d->displaySubTitle = d->subTitle;
    //FIXME add margin
    int widthAvail = contentsRect().width() /*- 2*margin()*/;

    // 5 pix spacing between title and subtitle
    if( !d->subTitle.isEmpty() )
        widthAvail -= 5;

    if( d->titleLength > widthAvail/2 ) {
        if( d->subTitleLength <= widthAvail/2 )
            d->displayTitle = K3b::cutToWidth( titleFm, d->title, widthAvail - d->subTitleLength );
        else
            d->displayTitle = K3b::cutToWidth( titleFm, d->title, widthAvail/2 );
    }
    if( d->subTitleLength > widthAvail/2 ) {
        if( d->titleLength <= widthAvail/2 )
            d->displaySubTitle = K3b::cutToWidth( subTitleFm, d->subTitle, widthAvail - d->titleLength );
        else
            d->displaySubTitle = K3b::cutToWidth( subTitleFm, d->subTitle, widthAvail/2 );
    }

    d->displayTitleLength = titleFm.width( d->displayTitle );
    d->displaySubTitleLength = subTitleFm.width( d->displaySubTitle );


    //
    // determine the minimum width for the minumum size hint
    //
    d->cachedMinimumWidth = 2*d->margin;

    QString cutTitle = d->title;
    if( cutTitle.length() > 2 ) {
        cutTitle.truncate( 2 );
        cutTitle += "...";
    }
    QString cutSubTitle = d->subTitle;
    if( cutSubTitle.length() > 2 ) {
        cutSubTitle.truncate( 2 );
        cutSubTitle += "...";
    }

    d->cachedMinimumWidth += titleFm.width( cutTitle ) + subTitleFm.width( cutSubTitle );
    // 5 pix spacing between title and subtitle
    if( !d->subTitle.isEmpty() )
        d->cachedMinimumWidth += 5;
}


bool K3bTitleLabel::event( QEvent* event )
{
    if ( event->type() == QEvent::ToolTip ) {
        QHelpEvent* he = ( QHelpEvent* )event;
        QPoint pos = he->pos();

        QRect r = contentsRect();

        int neededWidth = d->displayTitleLength;
        if( !d->displaySubTitle.isEmpty() )
            neededWidth += d->displaySubTitleLength + 5;

        int startPos = 0;
        if( d->alignment & Qt::AlignHCenter )
            startPos = r.left() + ( r.width() - 2*d->margin - neededWidth ) / 2;
        else if( d->alignment & Qt::AlignRight )
            startPos = r.right() - d->margin - neededWidth;
        else
            startPos = r.left() + d->margin;

        QRect titleTipRect( startPos, 0, d->displayTitleLength, height() );
        QRect subTitleTipRect( startPos + d->displayTitleLength, 0, d->displaySubTitleLength, height() );

        if( titleTipRect.contains( pos ) &&
            d->displayTitle != d->title ) {
            QToolTip::showText( he->globalPos(), d->title, this, titleTipRect );
        }
        else if( subTitleTipRect.contains( pos ) &&
                 d->displaySubTitle != d->subTitle ) {
            QToolTip::showText( he->globalPos(), d->subTitle, this, subTitleTipRect );
        }

        event->accept();

        return true;
    }

    return QFrame::event( event );
}

#include "k3btitlelabel.moc"

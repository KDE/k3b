/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3btitlelabel.h"

#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QPainter>
#include <QStyle>
#include <QToolTip>


class K3b::TitleLabel::Private
{
public:
    Private()
    :
        alignment( Qt::AlignLeft | Qt::AlignVCenter ),
        titleLength( 0 ),
        subTitleLength( 0 ),
        displayTitleLength( 0 ),
        displaySubTitleLength( 0 ),
        titleBaseLine( 0 ),
        subTitleBaseLine( 0 ),
        margin( 2 ),
        spacing( 5 ),
        cachedMinimumWidth( 0 )
    {
    }

    QString title;
    QString subTitle;

    QString displayTitle;
    QString displaySubTitle;

    Qt::Alignment alignment;

    int titleLength;
    int subTitleLength;
    int displayTitleLength;
    int displaySubTitleLength;
    int titleBaseLine;
    int subTitleBaseLine;
    int margin;
    int spacing;

    int cachedMinimumWidth;
    
    QRect titleRect( const QRect& boundingRect ) const;
    QRect subTitleRect( const QRect& subTitleRect, const QRect& titleRect ) const;
};


QRect K3b::TitleLabel::Private::titleRect( const QRect& boundingRect ) const
{
    int neededWidth = displayTitleLength;
    if( !displaySubTitle.isEmpty() )
        neededWidth += displaySubTitleLength + spacing;
    
    QRect titleRect;
    if( alignment & Qt::AlignHCenter )
        titleRect.setLeft( boundingRect.left() + ( boundingRect.width() - neededWidth ) / 2 );
    else if( alignment & Qt::AlignRight )
        titleRect.setLeft( boundingRect.right() - neededWidth );
    else
        titleRect.setLeft( boundingRect.left() );
    titleRect.setTop( boundingRect.top() );
    titleRect.setWidth( displayTitleLength );
    titleRect.setHeight( boundingRect.height() );
    return titleRect;
}


QRect K3b::TitleLabel::Private::subTitleRect( const QRect& boundingRect, const QRect& titleRect ) const
{
    return QRect( titleRect.left() + displayTitleLength + spacing, boundingRect.top(),
                  displaySubTitleLength, boundingRect.height() );
}


K3b::TitleLabel::TitleLabel( QWidget* parent )
    : QFrame( parent )
{
    d = new Private();
}


K3b::TitleLabel::~TitleLabel()
{
    delete d;
}


void K3b::TitleLabel::setTitle( const QString& title, const QString& subTitle )
{
    d->title = title;
    d->subTitle = subTitle;
    updatePositioning();
    update();
}


void K3b::TitleLabel::setSubTitle( const QString& subTitle )
{
    d->subTitle = subTitle;
    updatePositioning();
    update();
}


void K3b::TitleLabel::setAlignment( Qt::Alignment alignment )
{
    d->alignment = alignment;
    update();
}


QSize K3b::TitleLabel::sizeHint() const
{
    return QSize( d->titleLength + d->subTitleLength + 2*d->margin, d->titleBaseLine );
}


QSize K3b::TitleLabel::minimumSizeHint() const
{
    return QSize( d->cachedMinimumWidth, d->titleBaseLine );
}


void K3b::TitleLabel::resizeEvent( QResizeEvent* e )
{
    QFrame::resizeEvent( e );
    updatePositioning();
    update();
}


void K3b::TitleLabel::paintEvent( QPaintEvent* e )
{
    QPainter p( this );
    p.eraseRect( e->rect() );
    p.setLayoutDirection( layoutDirection() );

    const QRect rect = e->rect().adjusted( d->margin, d->margin, -d->margin, -d->margin );
    const QRect titleRect = d->titleRect( rect );

    QFont f( font() );
    f.setBold(true);
    f.setPointSize( f.pointSize() + 2 );

    // paint title
    p.setFont(f);
    p.drawText( QStyle::visualRect( layoutDirection(), rect, titleRect ),
                QStyle::visualAlignment( layoutDirection(), d->alignment ),
                d->displayTitle );

    if( !d->subTitle.isEmpty() ) {
        f.setBold(false);
        f.setPointSize( f.pointSize() - 4 );
        p.setFont(f);
        const QRect subTitleRect = d->subTitleRect( rect, titleRect );
        p.drawText( QStyle::visualRect( layoutDirection(), rect, subTitleRect ),
                    QStyle::visualAlignment( layoutDirection(), d->alignment ),
                    d->displaySubTitle );
    }
}


void K3b::TitleLabel::setMargin( int m )
{
    d->margin = m;
    updatePositioning();
    update();
}


void K3b::TitleLabel::updatePositioning()
{
    QFont f(font());
    f.setBold(true);
    f.setPointSize( f.pointSize() + 2 );
    QFontMetrics titleFm(f);

    f.setBold(false);
    f.setPointSize( f.pointSize() - 4 );
    QFontMetrics subTitleFm(f);

    d->titleBaseLine = contentsRect().height()/2 + titleFm.height()/2 - titleFm.descent();
    d->titleLength = titleFm.boundingRect( d->title ).width();

    d->subTitleBaseLine = d->titleBaseLine;

    d->subTitleLength = ( d->subTitle.isEmpty() ? 0 : subTitleFm.horizontalAdvance( d->subTitle ) );

    // cut the text to window width
    d->displayTitle = d->title;
    d->displaySubTitle = d->subTitle;
    //FIXME add margin
    int widthAvail = contentsRect().width() /*- 2*margin()*/;

    if( !d->subTitle.isEmpty() )
        widthAvail -= d->spacing;

    if( d->titleLength > widthAvail/2 ) {
        if( d->subTitleLength <= widthAvail/2 )
            d->displayTitle = titleFm.elidedText( d->title, Qt::ElideRight, widthAvail - d->subTitleLength );
        else
            d->displayTitle = titleFm.elidedText( d->title, Qt::ElideRight, widthAvail/2 );
    }
    if( d->subTitleLength > widthAvail/2 ) {
        if( d->titleLength <= widthAvail/2 )
            d->displaySubTitle = subTitleFm.elidedText( d->subTitle, Qt::ElideRight, widthAvail - d->titleLength );
        else
            d->displaySubTitle = subTitleFm.elidedText( d->subTitle, Qt::ElideRight, widthAvail/2 );
    }

    d->displayTitleLength = titleFm.horizontalAdvance( d->displayTitle );
    d->displaySubTitleLength = subTitleFm.horizontalAdvance( d->displaySubTitle );


    //
    // determine the minimum width for the minimum size hint
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

    d->cachedMinimumWidth += titleFm.horizontalAdvance( cutTitle ) + subTitleFm.horizontalAdvance( cutSubTitle );
    if( !d->subTitle.isEmpty() )
        d->cachedMinimumWidth += d->spacing;

    qDebug() << d->titleBaseLine << d->subTitleBaseLine;
}


bool K3b::TitleLabel::event( QEvent* event )
{
    if ( event->type() == QEvent::ToolTip ) {
        QHelpEvent* he = static_cast<QHelpEvent *>(event);
        QPoint pos = he->pos();

        const QRect rect = contentsRect().adjusted( d->margin, d->margin, -d->margin, -d->margin );
        const QRect titleRect = d->titleRect( rect );
        const QRect subTitleRect = d->subTitleRect( rect, titleRect );
        const QRect actualTitleRect = QStyle::visualRect( layoutDirection(), rect, titleRect );
        const QRect actualSubTitleRect = QStyle::visualRect( layoutDirection(), rect, subTitleRect );

        if( actualTitleRect.contains( pos ) && d->displayTitle != d->title ) {
            QToolTip::showText( he->globalPos(), d->title, this, actualTitleRect );
        }
        else if( actualSubTitleRect.contains( pos ) && d->displaySubTitle != d->subTitle ) {
            QToolTip::showText( he->globalPos(), d->subTitle, this, actualSubTitleRect );
        }

        event->accept();

        return true;
    }

    return QFrame::event( event );
}

#include "moc_k3btitlelabel.cpp"

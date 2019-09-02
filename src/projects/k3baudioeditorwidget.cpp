/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010-2011 Michal Malek <michalm@jabster.pl>
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

#include "k3baudioeditorwidget.h"

#include <QLinkedList>
#include <QCursor>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPolygon>
#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>
#include <QToolTip>



class K3b::AudioEditorWidget::Range
{
public:
    Range( int i,
           const K3b::Msf& s,
           const K3b::Msf& e,
           bool sf,
           bool ef,
           const QString& t,
           const QBrush& b )
        : id(i),
          start(s),
          end(e),
          startFixed(sf),
          endFixed(ef),
          brush(b),
          toolTip(t) {
    }

    int id;
    K3b::Msf start;
    K3b::Msf end;
    bool startFixed;
    bool endFixed;
    QBrush brush;
    QString toolTip;

    bool operator<( const K3b::AudioEditorWidget::Range& r ) const {
        return start < r.start;
    }
    bool operator>( const K3b::AudioEditorWidget::Range& r ) const {
        return start > r.start;
    }
    bool operator==( const K3b::AudioEditorWidget::Range& r ) const {
        return id == r.id;
    }

    typedef QLinkedList<Range> List;
};


class K3b::AudioEditorWidget::Marker
{
public:
    Marker( int i,
            const K3b::Msf& msf,
            bool f,
            const QColor& c,
            const QString& t )
        : id(i),
          pos(msf),
          fixed(f),
          color(c),
          toolTip(t) {
    }

    int id;
    K3b::Msf pos;
    bool fixed;
    QColor color;
    QString toolTip;

    operator K3b::Msf& () { return pos; }

    bool operator==( const Marker& r ) const {
        return id == r.id;
    }

    typedef QLinkedList<Marker> List;
};


struct K3b::AudioEditorWidget::SortByStart
{
    bool operator()( Range const* lhs, Range const* rhs )
    {
        return lhs->start < rhs->start;
    }
};


class K3b::AudioEditorWidget::Private
{
public:
    Private()
        : allowOverlappingRanges(true),
          rangeSelectionEnabled(false),
          selectedRangeId(0),
          draggedRangeId(0),
          movedRangeId(0),
          maxMarkers(1),
          idCnt(1),
          mouseAt(true),
          draggingRangeEnd(false),
          draggedMarker(0),
          margin(5) {
    }

    QBrush selectedRangeBrush;

    bool allowOverlappingRanges;
    bool rangeSelectionEnabled;

    int selectedRangeId;
    int draggedRangeId;
    int movedRangeId;
    K3b::Msf lastMovePosition;

    Range::List ranges;
    Marker::List markers;

    int maxMarkers;
    K3b::Msf length;
    int idCnt;
    bool mouseAt;

    bool draggingRangeEnd;
    Marker* draggedMarker;

    /**
     * Margin around the timethingy
     */
    int margin;
};


K3b::AudioEditorWidget::AudioEditorWidget( QWidget* parent )
    : QFrame( parent )
{
    d = new Private;
    d->selectedRangeBrush = palette().highlight();

    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
    setFrameStyle( StyledPanel|Sunken );
    setMouseTracking(true);
    setCursor( Qt::PointingHandCursor );
}


K3b::AudioEditorWidget::~AudioEditorWidget()
{
    delete d;
}


QSize K3b::AudioEditorWidget::minimumSizeHint() const
{
    // some fixed height minimum and enough space for a tickmark every minute
    // But never exceed 2/3 of the screen width, otherwise it just looks ugly
    // FIXME: this is still bad for long sources and there might be 60 minutes sources!

    int maxWidth = QApplication::desktop()->width()*2/3;
    int wantedWidth = 2*d->margin + 2*frameWidth() + (d->length.totalFrames()/75/60 + 1) * fontMetrics().width( "000" );
    return QSize( qMin( maxWidth, wantedWidth ),
                  2*d->margin + 12 + 6 /*12 for the tickmarks and 6 for the markers */ + fontMetrics().height() + 2*frameWidth() );
}


QSize K3b::AudioEditorWidget::sizeHint() const
{
    return minimumSizeHint();
}


void K3b::AudioEditorWidget::setLength( const K3b::Msf& length )
{
    d->length = length;
    // TODO: remove markers beyond length
    // TODO: shorten ranges if nesseccary
    update();
}


const K3b::Msf K3b::AudioEditorWidget::length() const
{
    return d->length;
}


void K3b::AudioEditorWidget::setSelectedRangeBrush( const QBrush& b )
{
    d->selectedRangeBrush = b;
}


const QBrush& K3b::AudioEditorWidget::selectedRangeBrush() const
{
    return d->selectedRangeBrush;
}


void K3b::AudioEditorWidget::setAllowOverlappingRanges( bool b )
{
    d->allowOverlappingRanges = b;
}


bool K3b::AudioEditorWidget::allowOverlappingRanges() const
{
    return d->allowOverlappingRanges;
}


void K3b::AudioEditorWidget::enableRangeSelection( bool b )
{
    d->rangeSelectionEnabled = b;
    update();
}


bool K3b::AudioEditorWidget::rangeSelectedEnabled() const
{
    return d->selectedRangeId != 0;
}


void K3b::AudioEditorWidget::setSelectedRange( int id )
{
    d->selectedRangeId = id;
    if( rangeSelectedEnabled() ) {
        update();
        emit selectedRangeChanged( d->selectedRangeId );
    }
}


int K3b::AudioEditorWidget::selectedRange() const
{
    return d->selectedRangeId;
}


int K3b::AudioEditorWidget::addRange( const K3b::Msf& start, const K3b::Msf& end,
                                    bool startFixed, bool endFixed,
                                    const QString& toolTip,
                                    const QBrush& brush )
{
    if( start > end || end > d->length-1 )
        return -1;

    Range r( d->idCnt++, start, end, startFixed, endFixed, toolTip,
             brush.style() != Qt::NoBrush ? brush : palette().window() );
    d->ranges.append( r );

    // only update the changed range
    QRect rect = contentsRect();
    rect.setLeft( msfToPos( start ) );
    rect.setRight( msfToPos( end ) );
    update( rect );

    return r.id;
}


int K3b::AudioEditorWidget::findRange( int pos ) const
{
    Range* r = findRange( QPoint( pos, 0 ) );
    if( r )
        return r->id;
    else
        return 0;
}


int K3b::AudioEditorWidget::findRangeEdge( int pos, bool* end ) const
{
    if( Range* r = findRangeEdge( QPoint( pos, 0 ), end ) )
        return r->id;
    else
        return 0;
}


bool K3b::AudioEditorWidget::modifyRange( int identifier, const K3b::Msf& start, const K3b::Msf& end )
{
    if( Range* range = getRange( identifier ) ) {
        if( start > end )
            return false;

        if( end > d->length )
            return false;

        range->start = start;
        range->end = end;

        if( !d->allowOverlappingRanges )
            fixupOverlappingRanges( range->id );

        repaint();

        return true;
    }
    else
        return false;
}


bool K3b::AudioEditorWidget::removeRange( int identifier )
{
    if( Range* range = getRange( identifier ) ) {
        emit rangeRemoved( identifier );

        // repaint only the part of the range
        QRect rect = contentsRect();
        rect.setLeft( msfToPos( range->start ) );
        rect.setRight( msfToPos( range->end ) );

        if( d->selectedRangeId == range->id )
            setSelectedRange( 0 );

        d->ranges.removeAll( *range );

        update( rect );

        return true;
    }
    else
        return false;
}


K3b::Msf K3b::AudioEditorWidget::rangeStart( int identifier ) const
{
    if( Range* range = getRange( identifier ) )
        return range->start;
    else
        return 0;
}


K3b::Msf K3b::AudioEditorWidget::rangeEnd( int identifier ) const
{
    if( Range* range = getRange( identifier ) )
        return range->end;
    else
        return 0;
}


QList<int> K3b::AudioEditorWidget::allRanges() const
{
    // First collect all ranges to one random-access container...
    QList<Range const*> ranges;
    Q_FOREACH( Range const& range, d->ranges ) {
        ranges.push_back( &range );
    }

    // ...then sort it...
    std::sort(ranges.begin(), ranges.end(), SortByStart());

    // ...finally collect its identifiers and return the collection.
    QList<int> identifiers;
    Q_FOREACH( Range const* range, ranges ) {
        identifiers.push_back( range->id );
    }
    return identifiers;
}


void K3b::AudioEditorWidget::setMaxNumberOfMarkers( int i )
{
    d->maxMarkers = i;

    // remove last markers
    while( d->markers.count() > qMax( 1, d->maxMarkers ) ) {
        removeMarker( d->markers.last().id );
    }
}


int K3b::AudioEditorWidget::addMarker( const K3b::Msf& pos, bool fixed, const QString& toolTip, const QColor& color )
{
    if( pos < d->length ) {
        Marker m( d->idCnt++, pos, fixed, color.isValid() ? color : palette().windowText().color(), toolTip );
        d->markers.append( m );
        return m.id;
    }
    else
        return -1;
}


bool K3b::AudioEditorWidget::removeMarker( int identifier )
{
    if( Marker* m = getMarker( identifier ) ) {
        emit markerRemoved( identifier );

        // TODO: in case a marker is bigger than one pixel this needs to be changed
        QRect rect = contentsRect();
        rect.setLeft( msfToPos( m->pos ) );
        rect.setRight( msfToPos( m->pos ) );

        d->markers.removeAll( *m );

        update( rect );

        return true;
    }
    else
        return false;
}


bool K3b::AudioEditorWidget::moveMarker( int identifier, const K3b::Msf& pos )
{
    if( pos < d->length )
        if( Marker* m = getMarker( identifier ) ) {
            QRect rect = contentsRect();
            rect.setLeft( qMin( msfToPos( pos ), msfToPos( m->pos ) ) );
            rect.setRight( qMax( msfToPos( pos ), msfToPos( m->pos ) ) );

            m->pos = pos;

            // TODO: in case a marker is bigger than one pixel this needs to be changed
            update( rect );

            return true;
        }

    return false;
}


void K3b::AudioEditorWidget::enableMouseAtSignal( bool b )
{
    d->mouseAt = b;
}


void K3b::AudioEditorWidget::paintEvent( QPaintEvent* e )
{
    Q_UNUSED( e );

    QPainter p( this );

    QRect drawRect( contentsRect() );
    drawRect.setLeft( drawRect.left() + d->margin );
    drawRect.setRight( drawRect.right() - d->margin );

    // from minimumSizeHint()
//   int neededHeight = fontMetrics().height() + 12 + 6;

//   drawRect.setTop( drawRect.top() + (drawRect.height() - neededHeight)/2 );
//   drawRect.setHeight( neededHeight );

    drawRect.setTop( drawRect.top() + d->margin );
    drawRect.setBottom( drawRect.bottom() - d->margin );

    drawAll( &p, drawRect );
}


void K3b::AudioEditorWidget::drawAll( QPainter* p, const QRect& drawRect )
{
    // we simply draw the ranges one after the other.
    for( Range::List::const_iterator it = d->ranges.constBegin(); it != d->ranges.constEnd(); ++it )
        drawRange( p, drawRect, *it );

    // Hack to make sure the currently selected range is always on top
    if( Range* selectedRange = getRange( d->selectedRangeId ) )
        drawRange( p, drawRect, *selectedRange );

    for( Marker::List::const_iterator it = d->markers.constBegin(); it != d->markers.constEnd(); ++it )
        drawMarker( p, drawRect, *it );


    // left vline
    p->drawLine( drawRect.left(), drawRect.top(),
                 drawRect.left(), drawRect.bottom() );

    // timeline
    p->drawLine( drawRect.left(), drawRect.bottom(),
                 drawRect.right(), drawRect.bottom() );

    // right vline
    p->drawLine( drawRect.right(), drawRect.top(),
                 drawRect.right(), drawRect.bottom() );

    // draw minute markers every minute
    int minute = 1;
    int minuteStep = 1;
    int markerVPos = drawRect.bottom();
    int maxMarkerWidth = fontMetrics().width( QString::number(d->length.minutes()) );
    int minNeededSpace = maxMarkerWidth + 1;
    int x = 0;
    while( minute*60*75 < d->length ) {
        int newX = msfToPos( minute*60*75 );

        // only draw the mark if we have anough space
        if( newX - x >= minNeededSpace ) {
            p->drawLine( newX, markerVPos, newX, markerVPos-5 );
            QRect txtRect( newX-(maxMarkerWidth/2),
                           markerVPos - 6 - fontMetrics().height(),
                           maxMarkerWidth,
                           fontMetrics().height() );
            p->drawText( txtRect, Qt::AlignCenter, QString::number(minute) );

            // FIXME: draw second markers if we have enough space

            x = newX;
        }
        else {
            minute -= minuteStep;
            if( minuteStep == 1 )
                minuteStep = 5;
            else
                minuteStep *= 2;
        }

        minute += minuteStep;
    }
}


void K3b::AudioEditorWidget::drawRange( QPainter* p, const QRect& drawRect, const K3b::AudioEditorWidget::Range& r )
{
    p->save();

    int start = msfToPos( r.start );
    int end = msfToPos( r.end );

    if( rangeSelectedEnabled() && r.id == d->selectedRangeId )
        p->setBrush( selectedRangeBrush() );
    else
        p->setBrush( r.brush );

    p->drawRect( start, drawRect.top()+6 , end-start+1-1, drawRect.height()-6-1 );

    p->restore();
}


void K3b::AudioEditorWidget::drawMarker( QPainter* p, const QRect& drawRect, const K3b::AudioEditorWidget::Marker& m )
{
    p->save();

    p->setPen( m.color );
    p->setBrush( m.color );

    int x = msfToPos( m.pos );
    p->drawLine( x, drawRect.bottom(), x, drawRect.top() );

    QPolygon points( 3 );
    points.setPoint( 0, x, drawRect.top() + 6 );
    points.setPoint( 1, x-3, drawRect.top() );
    points.setPoint( 2, x+3, drawRect.top() );
    p->drawPolygon( points );

    p->restore();
}


void K3b::AudioEditorWidget::fixupOverlappingRanges( int rangeId )
{
    Range* r = getRange( rangeId );
    Range::List::iterator range = d->ranges.begin();

    while( r != 0 && range != d->ranges.end() ) {
        if( range->id != rangeId ) {

            // remove the range if it is covered completely
            if( range->start >= r->start &&
                range->end <= r->end ) {
                if( d->selectedRangeId == range->id )
                    setSelectedRange( 0 );

                range = d->ranges.erase( range );
                emit rangeRemoved( rangeId );
                // "r" may be invalid at this point, let's find it once again
                r = getRange( rangeId );
            }
            else {
                // split the range if it contains r completely
                if( r->start >= range->start &&
                    r->end <= range->end ) {
                    // create a new range that spans the part after r
                    addRange( r->end+1, range->end,
                              range->startFixed, range->endFixed,
                              range->toolTip,
                              range->brush );

                    // modify the old range to only span the part before r
                    range->end = r->start-1;
                    emit rangeChanged( range->id, range->start, range->end );
                }
                else if( range->start >= r->start && range->start <= r->end ) {
                    range->start = r->end+1;
                    emit rangeChanged( range->id, range->start, range->end );
                }
                else if( range->end >= r->start && range->end <= r->end ) {
                    range->end = r->start-1;
                    emit rangeChanged( range->id, range->start, range->end );
                }
                ++range;
            }
        }
        else {
            ++range;
        }
    }
}


void K3b::AudioEditorWidget::mousePressEvent( QMouseEvent* e )
{
    d->draggedRangeId = 0;
    d->draggedMarker = 0;

    bool end;
    if( Range* r = findRangeEdge( e->pos(), &end ) ) {
        d->draggedRangeId = r->id;
        d->draggingRangeEnd = end;
        setSelectedRange( r->id );
    }
    else if( Range* r = findRange( e->pos() ) ) {
        d->movedRangeId = r->id;
        d->lastMovePosition = posToMsf( e->pos().x() );
        setSelectedRange( r->id );
        d->draggedMarker = findMarker( e->pos() );
    }

    QFrame::mousePressEvent(e);
}


void K3b::AudioEditorWidget::mouseReleaseEvent( QMouseEvent* e )
{
    if( !d->allowOverlappingRanges ) {
        //
        // modify and even delete ranges that we touched
        //
        if( d->draggedRangeId != 0 ) {
            fixupOverlappingRanges( d->draggedRangeId );
            repaint();
        }
        else if( d->movedRangeId != 0 ) {
            fixupOverlappingRanges( d->movedRangeId );
            repaint();
        }
    }

    d->draggedRangeId = 0;
    d->draggedMarker = 0;
    d->movedRangeId = 0;

    QFrame::mouseReleaseEvent(e);
}


void K3b::AudioEditorWidget::mouseDoubleClickEvent( QMouseEvent* e )
{
    QFrame::mouseDoubleClickEvent(e);
}


void K3b::AudioEditorWidget::mouseMoveEvent( QMouseEvent* e )
{
    if( d->mouseAt )
        emit mouseAt( posToMsf( e->pos().x() ) );

    if( e->buttons() & Qt::LeftButton ) {
        if( Range* draggedRange = getRange( d->draggedRangeId ) ) {
            // determine the position the range's end was dragged to and its other end
            K3b::Msf msfPos = qMax( K3b::Msf(), qMin( posToMsf( e->pos().x() ), d->length-1 ) );
            K3b::Msf otherEnd = ( d->draggingRangeEnd ? draggedRange->start : draggedRange->end );

            // move it to the new pos
            if( d->draggingRangeEnd )
                draggedRange->end = msfPos;
            else
                draggedRange->start = msfPos;

            // if we pass the other end switch them
            if( draggedRange->start > draggedRange->end ) {
                K3b::Msf buf = draggedRange->start;
                draggedRange->start = draggedRange->end;
                draggedRange->end = buf;
                d->draggingRangeEnd = !d->draggingRangeEnd;
            }

            emit rangeChanged( draggedRange->id, draggedRange->start, draggedRange->end );

            repaint();
        }
        else if( d->draggedMarker ) {
            d->draggedMarker->pos = posToMsf( e->pos().x() );
            emit markerMoved( d->draggedMarker->id, d->draggedMarker->pos );

            repaint();
        }
        else if( Range* movedRange = getRange( d->movedRangeId ) ) {
            int diff = posToMsf( e->pos().x() ).lba() - d->lastMovePosition.lba();
            if( movedRange->end + diff >= d->length )
                diff = d->length.lba() - movedRange->end.lba() - 1;
            else if( movedRange->start - diff < 0 )
                diff = -1 * movedRange->start.lba();
            movedRange->start += diff;
            movedRange->end += diff;

//       if( !d->allowOverlappingRanges )
// 	fixupOverlappingRanges( d->movedRangeId );

            d->lastMovePosition = posToMsf( e->pos().x() );

            emit rangeChanged( movedRange->id, movedRange->start, movedRange->end );

            repaint();
        }
    }
    else if( findRangeEdge( e->pos() ) || findMarker( e->pos() ) )
        setCursor( Qt::SizeHorCursor );
    else
        setCursor( Qt::PointingHandCursor );

    QFrame::mouseMoveEvent(e);
}

bool K3b::AudioEditorWidget::event( QEvent* e )
{
    if( e->type() == QEvent::ToolTip ) {
        QHelpEvent* helpEvent = dynamic_cast<QHelpEvent*>( e );
        const QPoint pos = mapFromGlobal( helpEvent->globalPos() );

        if( Marker* m = findMarker( pos ) ) {
            QToolTip::showText( helpEvent->globalPos(),
                                m->toolTip.isEmpty() ? m->pos.toString() : QString("%1 (%2)").arg(m->toolTip).arg(m->pos.toString()),
                                this );
        }
        else if( Range* range = findRange( pos ) ) {
            QToolTip::showText( helpEvent->globalPos(),
                                range->toolTip.isEmpty()
                                ? QString("%1 - %2").arg(range->start.toString()).arg(range->end.toString())
                                : QString("%1 (%2 - %3)").arg(range->toolTip).arg(range->start.toString()).arg(range->end.toString()),
                                this );

        }
        else {
            QToolTip::hideText();
        }

        e->accept();
        return true;
    }
    else {
        return QWidget::event( e );
    }
}


K3b::AudioEditorWidget::Range* K3b::AudioEditorWidget::getRange( int i ) const
{
    for( Range::List::iterator it = d->ranges.begin(); it != d->ranges.end(); ++it )
        if( (*it).id == i )
            return &( *it );

    return 0;
}


K3b::AudioEditorWidget::Range* K3b::AudioEditorWidget::findRange( const QPoint& p ) const
{
    // TODO: binary search; maybe store start and end positions in sorted lists for quick searching
    // this might be a stupid approach but we do not have many ranges anyway
    for( Range::List::iterator it = d->ranges.begin(); it != d->ranges.end(); ++it ) {
        Range& range = *it;
        int start = msfToPos( range.start );
        int end = msfToPos( range.end );

        if( p.x() >= start && p.x() <= end ) {
            return &range;
        }
    }
    return 0;
}


K3b::AudioEditorWidget::Range* K3b::AudioEditorWidget::findRangeEdge( const QPoint& p, bool* isEnd ) const
{
    // TODO: binary search
    // this might be a stupid approach but we do not have many ranges anyway
    for( Range::List::iterator it = d->ranges.begin(); it != d->ranges.end(); ++it ) {
        Range& range = *it;
        int start = msfToPos( range.start );
        int end = msfToPos( range.end );

        //
        // In case two ranges meet at one point moving the mouse cursor deeper into one
        // range allows for grabbing that end
        //

        if( p.x() - 3 <= start && p.x() >= start && !range.startFixed ) {
            if( isEnd )
                *isEnd = false;
            return &range;
        }
        else if( p.x() <= end && p.x() + 3 >= end && !range.endFixed ) {
            if( isEnd )
                *isEnd = true;
            return &range;
        }
    }
    return 0;
}


K3b::AudioEditorWidget::Marker* K3b::AudioEditorWidget::getMarker( int i ) const
{
    for( Marker::List::iterator it = d->markers.begin(); it != d->markers.end(); ++it )
        if( (*it).id == i )
            return &( *it );

    return 0;
}


K3b::AudioEditorWidget::Marker* K3b::AudioEditorWidget::findMarker( const QPoint& p ) const
{
    // TODO: binary search
    for( Marker::List::iterator it = d->markers.begin(); it != d->markers.end(); ++it ) {
        Marker& marker = *it;
        int start = msfToPos( marker.pos );

        if( p.x() - 1 <= start && p.x() + 1 >= start && !marker.fixed )
            return &marker;
    }

    return 0;
}


// p is in widget coordinates
K3b::Msf K3b::AudioEditorWidget::posToMsf( int p ) const
{
    int w = contentsRect().width() - 2*d->margin;
    int x = qMin( p-frameWidth()-d->margin, w );
    return ( (int)((double)(d->length.lba()-1) / (double)w * (double)x) );
}


// returns widget coordinates
int K3b::AudioEditorWidget::msfToPos( const K3b::Msf& msf ) const
{
    int w = contentsRect().width() - 2*d->margin;
    int pos = (int)((double)w / (double)(d->length.lba()-1) * (double)msf.lba());
    return frameWidth() + d->margin + qMin( pos, w-1 );
}




/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudioeditorwidget.h"

#include <qpainter.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <qcursor.h>


class K3bAudioEditorWidget::Range
{
public:
  Range( int i,
	 const K3b::Msf& s,
	 const K3b::Msf& e,
	 bool sf,
	 bool ef,
	 const QBrush& b )
    : id(i),
      start(s),
      end(e),
      startFixed(sf),
      endFixed(ef),
      brush(b) {
  }

  int id;
  K3b::Msf start;
  K3b::Msf end;
  bool startFixed;
  bool endFixed;
  QBrush brush;

  bool operator<( const K3bAudioEditorWidget::Range& r ) {
    return start < r.start;
  }
  bool operator>( const K3bAudioEditorWidget::Range& r ) {
    return start > r.start;
  }
  bool operator==( const K3bAudioEditorWidget::Range& r ) {
    return start == r.start;
  }
};


class K3bAudioEditorWidget::Marker
{
public:
  Marker( int i,
	  const K3b::Msf& msf,
	  bool f,
	  const QColor& c )
    : id(i),
      pos(msf),
      fixed(f),
      color(c) {
  }

  int id;
  K3b::Msf pos;
  bool fixed;
  QColor color;

  operator K3b::Msf& () { return pos; }
};


K3bAudioEditorWidget::K3bAudioEditorWidget( QWidget* parent, const char* name )
  : QFrame( parent, name ),
    m_maxMarkers(1),
    m_idCnt(1),
    m_mouseAt(true),
    m_draggedRange(0),
    m_draggedMarker(0)
{
  setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  setFrameStyle( StyledPanel|Sunken );
  setMouseTracking(true);
  setCursor( Qt::PointingHandCursor );

  m_margin = 5;
}


K3bAudioEditorWidget::~K3bAudioEditorWidget()
{
  m_ranges.setAutoDelete(true);
  m_markers.setAutoDelete(true);
  m_ranges.clear();
  m_markers.clear();
}


QSize K3bAudioEditorWidget::minimumSizeHint() const
{
  constPolish();
  // some fixed height
  return QSize( -1, 40 + 2*frameWidth() );
}


QSize K3bAudioEditorWidget::sizeHint() const
{
  return minimumSizeHint();
}


void K3bAudioEditorWidget::setLength( const K3b::Msf& length )
{
  m_length = length;
  // TODO: remove markers beyond length
  // TODO: shorten ranges if nesseccary
  update();
}


int K3bAudioEditorWidget::addRange( const K3b::Msf& start, const K3b::Msf& end, 
				    bool startFixed, bool endFixed,
				    const QBrush& brush )
{
  if( start > end || end > m_length-1 )
    return -1;

  Range* r = new Range( m_idCnt++, start, end, startFixed, endFixed,
			brush.style() != QBrush::NoBrush ? brush : QBrush(determineNewColor()) );
  m_ranges.inSort( r );

  // only update the changed range
  QRect rect = contentsRect();
  rect.setLeft( fromPosToX( start ) );
  rect.setRight( fromPosToX( end ) );
  update( rect );

  return r->id;
}


bool K3bAudioEditorWidget::modifyRange( int identifier, const K3b::Msf& start, const K3b::Msf& end )
{
  Range* range = getRange( identifier );
  if( range ) {
    if( start > end )
      return false;

    int x1 = QMIN( fromPosToX( range->start ), fromPosToX( start ) );
    int x2 = QMAX( fromPosToX( range->end ), fromPosToX( end ) );

    range->start = start;
    range->end = end;

    // this is faster then resorting the hole list
    m_ranges.removeRef( range );
    m_ranges.inSort( range );

    // repaint only the part of the range
    QRect rect = contentsRect();
    rect.setLeft( x1 );
    rect.setRight( x2 );
    update( rect );

    return true;
  }
  else
    return false;
}


bool K3bAudioEditorWidget::removeRange( int identifier )
{
  if( Range* range = getRange( identifier ) ) {
    m_ranges.removeRef( range );

    emit rangeRemoved( identifier );

    // repaint only the part of the range
    QRect rect = contentsRect();
    rect.setLeft( fromPosToX( range->start ) );
    rect.setRight( fromPosToX( range->end ) );
    delete range;

    update( rect );

    return true;
  }
  else
    return false;
}


K3b::Msf K3bAudioEditorWidget::rangeStart( int identifier ) const
{
  if( Range* range = getRange( identifier ) )
    return range->start;
  else
    return 0;
}


K3b::Msf K3bAudioEditorWidget::rangeEnd( int identifier ) const
{
  if( Range* range = getRange( identifier ) )
    return range->end;
  else
    return 0;
}


void K3bAudioEditorWidget::setMaxNumberOfMarkers( int i )
{
  m_maxMarkers = i;

  // remove last markers
  while( m_markers.count() > QMAX( 1, m_maxMarkers ) ) {
    removeMarker( m_markers.getLast()->id );
  }
}


int K3bAudioEditorWidget::addMarker( const K3b::Msf& pos, bool fixed )
{
  if( pos < m_length ) {
    Marker* m = new Marker( m_idCnt++, pos, fixed, determineNewColor() );
    m_markers.inSort( m );
    return m->id;
  }
  else
    return -1;
}


bool K3bAudioEditorWidget::removeMarker( int identifier )
{
  if( Marker* m = getMarker( identifier ) ) {
    m_markers.removeRef( m );

    emit markerRemoved( identifier );

    // TODO: in case a marker is bigger than one pixel this needs to be changed
    QRect rect = contentsRect();
    rect.setLeft( fromPosToX( m->pos ) );
    rect.setRight( fromPosToX( m->pos ) );
    delete m;

    update( rect );

    return true;
  }
  else
    return false;
}


bool K3bAudioEditorWidget::moveMarker( int identifier, const K3b::Msf& pos )
{
  if( pos < m_length )
    if( Marker* m = getMarker( identifier ) ) {
      QRect rect = contentsRect();
      rect.setLeft( QMIN( fromPosToX( pos ), fromPosToX( m->pos ) ) );
      rect.setRight( QMAX( fromPosToX( pos ), fromPosToX( m->pos ) ) );

      m->pos = pos;

      // TODO: in case a marker is bigger than one pixel this needs to be changed
      update( rect );

      return true;
    }

  return false;
}


void K3bAudioEditorWidget::drawContents( QPainter* p )
{
  // double buffering
  QPixmap pix( contentsRect().size() );
  pix.fill( colorGroup().base() );

  QPainter pixP;
  pixP.begin( &pix, this );

  // we simply draw the ranges one after the other.
  // since K3b doesn't use multiple overlapping ranges anyway this is no problem
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it )
    drawRange( &pixP, *it );
  
  for( QPtrListIterator<Marker> it( m_markers ); *it; ++it )
    drawMarker( &pixP, *it );


  // draw the timeline
  // FIXME: variable height
  pixP.drawLine( m_margin, m_margin, m_margin, 
		 contentsRect().height()-m_margin );
  pixP.drawLine( m_margin, 30, 
		 contentsRect().width()-m_margin, 30 );
  pixP.drawLine( contentsRect().width()-m_margin, m_margin, 
		 contentsRect().width()-m_margin, contentsRect().height()-m_margin );

  // draw the timemark things every second
  // FIXME: variable height
  // FIXME: seconds if enough space
  int pos = 1;
  while( pos*60*75 < m_length ) {
    int x = fromPosToX( pos*60*75 );
    pixP.drawLine( x, 30, x, 25 );
    pixP.drawText( x, 24, QString::number(pos) );
    ++pos;
  }

  pixP.end();

  QRect rect = p->clipRegion().boundingRect();
  QRect pixRect = rect;
  pixRect.moveBy( -1*frameWidth(), -1*frameWidth() );
  bitBlt( this, rect.topLeft(), &pix, pixRect );
}


void K3bAudioEditorWidget::drawRange( QPainter* p, K3bAudioEditorWidget::Range* r )
{
  p->save();

  int start = fromPosToX( r->start );
  int end = fromPosToX( r->end );

  // draw the range
  // FIXME: variable height
  p->fillRect( start, m_margin, end-start+1, contentsRect().height()-2*m_margin, r->brush );

  p->restore();
}


void K3bAudioEditorWidget::drawMarker( QPainter* p, K3bAudioEditorWidget::Marker* m )
{
  p->save();

  p->setPen( m->color );

  int x = fromPosToX( m->pos );
  p->drawLine( x, m_margin, x, contentsRect().height()-m_margin );

  p->restore();
}


void K3bAudioEditorWidget::mousePressEvent( QMouseEvent* e )
{
  m_draggedRange = 0;
  m_draggedMarker = 0;

  bool end;
  if( Range* r = findRange( e->pos(), &end ) ) {
    m_draggedRange = r;
    m_draggingRangeEnd = end;
  }
  else 
    m_draggedMarker = findMarker( e->pos() );

  QFrame::mousePressEvent(e);
}


void K3bAudioEditorWidget::mouseReleaseEvent( QMouseEvent* e )
{
  m_draggedRange = 0;
  m_draggedMarker = 0;

  QFrame::mouseReleaseEvent(e);
}


void K3bAudioEditorWidget::mouseDoubleClickEvent( QMouseEvent* e )
{
  QFrame::mouseDoubleClickEvent(e);
}


void K3bAudioEditorWidget::mouseMoveEvent( QMouseEvent* e )
{
  if( m_mouseAt )
    emit mouseAt( fromPointToPos( e->pos() ) );
  
  if( e->state() & Qt::LeftButton ) {
    if( m_draggedRange ) {
      int x1 = QMIN( e->pos().x(), fromPosToX( m_draggedRange->start ) );
      int x2 = QMAX( e->pos().x(), fromPosToX( m_draggedRange->end ) );

      // move it to the new pos
      if( m_draggingRangeEnd )
	m_draggedRange->end = fromPointToPos( e->pos() );
      else
	m_draggedRange->start = fromPointToPos( e->pos() );

      // if we pass the other end switch them
      if( m_draggedRange->start > m_draggedRange->end ) {
	K3b::Msf buf = m_draggedRange->start;
	m_draggedRange->start = m_draggedRange->end;
	m_draggedRange->end = buf;
	m_draggingRangeEnd = !m_draggingRangeEnd;
      }	

      emit rangeChanged( m_draggedRange->id, m_draggedRange->start, m_draggedRange->end );

      // only update the range stuff
      QRect rect = contentsRect();
      rect.setLeft( x1 );
      rect.setRight( x2 );
      update( rect );
    }
    else if( m_draggedMarker ) {
      int x1 = QMIN( e->pos().x(), fromPosToX( m_draggedMarker->pos ) );
      int x2 = QMAX( e->pos().x(), fromPosToX( m_draggedMarker->pos ) );

      m_draggedMarker->pos = fromPointToPos( e->pos() );
      emit markerMoved( m_draggedMarker->id, m_draggedMarker->pos );

      // only update the marker
      QRect rect = contentsRect();
      rect.setLeft( x1 );
      rect.setRight( x2 );
      update( rect );
    }
  }
  else if( findRange( e->pos(), 0 ) || findMarker( e->pos() ) )
    setCursor( Qt::SizeHorCursor );
  else
    setCursor( Qt::PointingHandCursor );

  QFrame::mouseMoveEvent(e);
}


QColor K3bAudioEditorWidget::determineNewColor() const
{
  // FIXME
  return Qt::green;
}


K3bAudioEditorWidget::Range* K3bAudioEditorWidget::getRange( int i ) const
{
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it )
    if( (*it)->id == i )
      return *it;

  return 0;
}


K3bAudioEditorWidget::Range* K3bAudioEditorWidget::findRange( const QPoint& p, bool* isEnd ) const
{
  // TODO: binary search
  // this might be a stupid approach but we have only one range anyway so speed is not an issue yet
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it ) {
    Range* range = *it;
    int start = fromPosToX( range->start );
    int end = fromPosToX( range->end );

    if( p.x() - 1 <= start && p.x() + 1 >= start && !range->startFixed ) {
      if( isEnd )
	*isEnd = false;
      return range;
    }
    else if( p.x() - 1 <= end && p.x() + 1 >= end && !range->endFixed ) {
      if( isEnd )
	*isEnd = true;
      return range;
    }
  }
  return 0;
}


K3bAudioEditorWidget::Marker* K3bAudioEditorWidget::getMarker( int i ) const
{
  for( QPtrListIterator<Marker> it( m_markers ); *it; ++it )
    if( (*it)->id == i )
      return *it;

  return 0;
}


K3bAudioEditorWidget::Marker* K3bAudioEditorWidget::findMarker( const QPoint& p ) const
{
  // TODO: binary search
  for( QPtrListIterator<Marker> it( m_markers ); *it; ++it ) {
    Marker* marker = *it;
    int start = fromPosToX( marker->pos );

    if( p.x() - 1 <= start && p.x() + 1 >= start && !marker->fixed )
      return marker;
  }

  return 0;
}


// p is in widget coordinates
K3b::Msf K3bAudioEditorWidget::fromPointToPos( const QPoint& p ) const
{

  int w = contentsRect().width() - 2*m_margin;
  int x = QMIN( p.x()-frameWidth()-m_margin, w );
  return ( (int)((double)(m_length.lba()-1) / (double)w * (double)x) );
}


// returns widget coordinates
int K3bAudioEditorWidget::fromPosToX( const K3b::Msf& msf ) const
{
  double w = (double)contentsRect().width() - 2*m_margin;
  return frameWidth() + m_margin + (int)(w / (double)(m_length.lba()-1) * (double)msf.lba());
}


#include "k3baudioeditorwidget.moc"

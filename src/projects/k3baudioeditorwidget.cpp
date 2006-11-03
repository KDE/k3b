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
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qtooltip.h>
#include <qpoint.h>
#include <qdatetime.h>
#include <qvaluelist.h>

#include <kaction.h>
#include <klocale.h>
#include <kactioncollection.h>
#include <kpopupmenu.h>

#include <stdlib.h>             /// for rand() function

class K3bAudioEditorWidget::Range
{


public:

  Range( int i,
	 const K3b::Msf& s,
	 const K3b::Msf& e,
	 bool sf,
	 bool ef,
	 const QString& t,
	 const QBrush& b,
	 bool active=false )
    : id(i),
      start(s),
      end(e),
      startFixed(sf),
      endFixed(ef),
      brush(b),
      toolTip(t),
      active(active) {
  }



  int id;
  K3b::Msf start;
  K3b::Msf end;
  bool startFixed;
  bool endFixed;
  QBrush brush;
  QString toolTip;
  bool active;


		

  bool operator<( const K3bAudioEditorWidget::Range& r ) {
    return start > r.start;   // modified   the operators are reversed so that ranges can be sorted in descending order
  }
  bool operator>( const K3bAudioEditorWidget::Range& r ) {
    return start < r.start;      // modified the operators are reversed so that ranges can be sorted in descending order
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
	  K3bAudioEditorWidget::Range* range,
	  bool end1,
	  bool f,
	  const QColor& c, 
	  const QString& t )
    : id(i),
      pos(msf),
      range(range),
      end(end1),
      fixed(f),
      color(c),
      toolTip(t) {
  }

  int id;
  K3b::Msf pos;
  K3bAudioEditorWidget::Range* range;
  bool end;
  bool fixed;

  QColor color;
  QString toolTip;

  operator K3b::Msf& () { return pos; }
};


class K3bAudioEditorWidget::ToolTip : public QToolTip
{
public:
  ToolTip( K3bAudioEditorWidget* w ) 
    : QToolTip( w ),
      m_editorWidget( w ) {
  }

protected:
  void maybeTip( const QPoint& p ) {
    QRect r = m_editorWidget->contentsRect();
    Marker* m = m_editorWidget->findMarker( p );
    if( m ) {
      r.setLeft( p.x() - 1 );
      r.setRight( p.x() + 1 );
      tip( r, m->toolTip.isEmpty() ? m->pos.toString() : QString("%1 (%2)").arg(m->toolTip).arg(m->pos.toString()) );
    }
    else {
      Range* range = m_editorWidget->findRange( p );
      if( range ) {
	r.setLeft( m_editorWidget->fromPosToX( range->start ) );
	r.setRight( m_editorWidget->fromPosToX( range->end ) );
	tip( r, 
	     range->toolTip.isEmpty()
	     ? QString("%1 - %2").arg(range->start.toString()).arg(range->end.toString())
	     : QString("%1 (%2 - %3)").arg(range->toolTip).arg(range->start.toString()).arg(range->end.toString()) );
      }
    }
  }

private:
  K3bAudioEditorWidget* m_editorWidget;
};





K3bAudioEditorWidget::K3bAudioEditorWidget( QWidget* parent, const char* name )
  : QFrame( parent, name, Qt::WNoAutoErase ),
    m_maxMarkers(1),
    m_idCnt(1),
    m_mouseAt(true),
    m_draggedRange(0),
    m_oppositeRange(0),
    m_draggedMarker(0)
{
  setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
  setFrameStyle( StyledPanel|Sunken );
  setMouseTracking(true);
  setCursor( Qt::PointingHandCursor );

  // setupSplitActions();   /// added here

  m_margin = 5;

  m_toolTip = new ToolTip( this );

  rangeLock=0; 

  //connect(this,SIGNAL(markerMoved( int, const K3b::Msf& )),this,SLOT( slotMarkerMoved(int, const K3b::Msf& ) ));
  //connect(this,SIGNAL(rangeChanged(int, const K3b::Msf&, const K3b::Msf& ,bool)),this,SLOT( slotRangeChanged(int,const K3b::Msf& ,const K3b::Msf&,bool) ));

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
  // some fixed height minimum and enough space for a tickmark every minute
  // But never exceed 2/3 of the the screen width, otherwise it just looks ugly
  // FIXME: this is still bad for long sources and there might be 60 minutes sources!

  int maxWidth = QApplication::desktop()->width()*2/3;
  int wantedWidth = 2*m_margin + 2*frameWidth() + (m_length.totalFrames()/75/60 + 1) * fontMetrics().width( "000" );
  return QSize( QMIN( maxWidth, wantedWidth ), 
		2*m_margin + 12 + 6 /*12 for the tickmarks and 6 for the markers */ + fontMetrics().height() + 2*frameWidth() );
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
				    const QString& toolTip,
				    const QBrush& brush )
{
  if( start > end || end > m_length-1 ) {
    
    return -1;
  }

  Range* r = new Range( m_idCnt++, start, end, startFixed, endFixed, toolTip,brush.style() != QBrush::NoBrush ? brush : QBrush(determineNewColor()) );

  m_ranges.inSort( r );

  
  // only update the changed range
  QRect rect = contentsRect();
  //rect.setLeft( fromPosToX( start ) );
  //rect.setRight( fromPosToX( end ) );
  //update( rect );

  update();
  reColor();

  return r->id;
}


bool K3bAudioEditorWidget::modifyRange( int identifier, const K3b::Msf& start, const K3b::Msf& end )
{
  Range* range = getRange( identifier );
  if( range ) {
    if( start > end )
      return false;
    // 
    if( end > m_length )
      return false;

    int x1 = QMIN( fromPosToX( range->start )-10, fromPosToX (start )-10);
    int x2 = QMAX( fromPosToX( range->end )+10, fromPosToX( end ) + 10 );

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
    //update();


    if(signalLockFlag==1)
      if(range==m_draggedRange)
	emit rangeChanged( identifier, range->start, range->end, m_draggingRangeEnd ); // changed   
      else
	emit rangeChanged( identifier, range->start, range->end, false ); // changed   false is default for everyone else


    return true;
  }
  else
    return false;
}


bool K3bAudioEditorWidget::removeRange( int identifier )
{
  if( Range* range = getRange( identifier ) ) {


    // removing the  corresponding marker
    Marker* m=0;
    for( QPtrListIterator<Marker> it( m_markers ); *it; ++it ) {
      
      if((*it)->range==range) { 
	m=*it; 
	break;
      }
    }

    if(m) {
      removeMarker(m->id);   
    }



    m_ranges.removeRef( range );

    emit rangeRemoved( identifier );


    reColor();

    // repaint only the part of the range
    QRect rect = contentsRect();
    rect.setLeft( fromPosToX( range->start ) );
    rect.setRight( fromPosToX( range->end ) );
    delete range;

    //update( rect );
    update();

    return true;
  }
  else
    return false;
}


bool K3bAudioEditorWidget::removeRange(QPoint m_rangePointClicked)
{
  Range* range=findRange(m_rangePointClicked);
  return removeRange(range->id);

}

bool K3bAudioEditorWidget::removeRangeAdjust(QPoint m_rangePointClicked)
{
  Range* range_delete=findRange(m_rangePointClicked),*temp_range1,*temp_range2,*temp_range3;
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it ) {
    temp_range1=*it;
    if(temp_range1==range_delete) {        /// is this the range to be deleted
      if(it.atFirst()) {
	++it;
	temp_range2=*it;
	
	modifyRange(temp_range2->id,temp_range2->start,temp_range1->end);
			
						
	
	temp_range2->endFixed=temp_range1->endFixed;
			
	m_draggedRange=temp_range2; //  modified so that msfEdit can pick a valid position
	m_draggingRangeEnd=false;
			
	emit edgeClicked(temp_range2->start); // signal faked to edit msfEdit in audioSplit 
      } 
      else if(it.atLast()) {
	--it;
	temp_range2=*it;
	modifyRange(temp_range2->id,temp_range1->start,temp_range2->end);
	temp_range2->startFixed=temp_range1->startFixed;
			
	m_draggedRange=temp_range2;       // modified so that msfEdit can pick a valid position
	m_draggingRangeEnd=true;         // 
	emit edgeClicked(temp_range2->end); // signal faked to edit msfEdit in audioSplit 
      }
      else {
	--it; // move one step back
	temp_range2=*it;
	++it;++it;  // move two steps forward ( which is actually one step forward from original position
	temp_range3=*it;
			
	modifyRange(temp_range2->id,fromPointToPos(m_rangePointClicked) + 1,temp_range2->end);
	modifyRange(temp_range3->id,temp_range3->start,fromPointToPos(m_rangePointClicked));
			
	m_draggedRange=temp_range2;       // modified so that msfEdit can pick a valid position
	m_draggingRangeEnd=false;         // 
	emit edgeClicked(temp_range2->start); // signal faked to edit msfEdit in audioSplit 
			
      }
      break;
    }
  } 
  m_oppositeRange=0;
  // THIS
  updateRangeMarkerColor();

  return removeRange(range_delete->id);
	
	
	
}


int K3bAudioEditorWidget::getRangeCount() const 
{
  return m_ranges.count();
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


int K3bAudioEditorWidget::addMarker( const K3b::Msf& pos,K3bAudioEditorWidget::Range* range, bool end,bool fixed, const QString& toolTip, const QColor& color )
{
  if( pos < m_length ) {
    Marker* m = new Marker( m_idCnt++, pos,range , end, fixed, color.isValid() ? color : determineNewColor(), toolTip );
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

  QRect drawRect( contentsRect() );
  drawRect.setLeft( drawRect.left() + m_margin );
  drawRect.setRight( drawRect.right() - m_margin );

  // from minimumSizeHint()
  //   int neededHeight = fontMetrics().height() + 12 + 6;

  //   drawRect.setTop( drawRect.top() + (drawRect.height() - neededHeight)/2 );
  //   drawRect.setHeight( neededHeight );

  drawRect.setTop( drawRect.top() + m_margin );
  drawRect.setBottom( drawRect.bottom() - m_margin );

  drawAll( &pixP, drawRect );

  pixP.end();

  QRect rect = p->clipRegion().boundingRect();
  QRect pixRect = rect;
  pixRect.moveBy( -1*frameWidth(), -1*frameWidth() );
  bitBlt( this, rect.topLeft(), &pix, pixRect );
}


void K3bAudioEditorWidget::drawAll( QPainter* p, const QRect& drawRect )
{
  // we simply draw the ranges one after the other.
  // since K3b doesn't use multiple overlapping ranges anyway this is no problem
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it )
    drawRange( p, drawRect, *it );

  for( QPtrListIterator<Marker> it( m_markers ); *it; ++it )
    drawMarker( p, drawRect, *it );


  // left vline
  p->drawLine( drawRect.left(), drawRect.top(), drawRect.left(), drawRect.bottom() );

  // timeline
  p->drawLine( drawRect.left(), drawRect.bottom(), drawRect.right(), drawRect.bottom() );

  // right vline
  p->drawLine( drawRect.right(), drawRect.top(), drawRect.right(), drawRect.bottom() );

  // draw minute markers every minute
  /*int minute = 1;
    int minuteStep = 1;
    int markerVPos = drawRect.bottom();
    int maxMarkerWidth = fontMetrics().width( QString::number(m_length.minutes()) );
    int minNeededSpace = maxMarkerWidth + 1;
    int x = 0;
    while( minute*60*75 < m_length ) {
    int newX = fromPosToX( minute*60*75 );

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
    }*/
}


void K3bAudioEditorWidget::drawRange( QPainter* p, const QRect& drawRect, K3bAudioEditorWidget::Range* r )
{
  p->save();
  int i=0;
  int start = fromPosToX( r->start );
  int end = fromPosToX( r->end );

  p->fillRect( start, drawRect.top() + 6 , end-start+1, drawRect.height() - 6, r->brush );

  if(r->active==false){
    
    p->setPen( determineNewColor() );
    p->setBrush( determineNewColor() );
  }
  else {
    
    p->setPen( Qt::red );
    p->setBrush( Qt::red );
  }

  

  if(r==m_draggedRange && m_draggingRangeEnd==true) {
    if(m_ranges.first()==r && !r->endFixed) {
      int x=end;
      int y=start;
      QPointArray points( 3 );
      points.setPoint( 0, x, drawRect.top() + 6 );
      points.setPoint( 1, x-3, drawRect.top() );
      points.setPoint( 2, x+3, drawRect.top() );
      p->drawPolygon( points );
      p->setPen( determineNewColor() );
      p->setBrush( determineNewColor() );
      points.setPoint( 0, y, drawRect.top() + 6 );
      points.setPoint( 1, y-3, drawRect.top() );
      points.setPoint( 2, y+3, drawRect.top() );
      p->drawPolygon( points );     
    }
  }
  else {
    if(!r->startFixed) {
      int x=start;
      QPointArray points( 3 );
      points.setPoint( 0, x, drawRect.top() + 6 );
      points.setPoint( 1, x-3, drawRect.top() );
      points.setPoint( 2, x+3, drawRect.top() );
      p->drawPolygon( points );
      if(m_ranges.first()==r && !r->endFixed){
	
        x=end;
        p->setPen( determineNewColor() );
	p->setBrush( determineNewColor() );
	points.setPoint( 0, x, drawRect.top() + 6 );
	points.setPoint( 1, x-3, drawRect.top() );
	points.setPoint( 2, x+3, drawRect.top() );
	p->drawPolygon( points );
	p->setPen( Qt::red );
	p->setBrush( Qt::red );
      }
    } 
    else if(m_ranges.first()==r && !r->endFixed && !m_draggingRangeEnd) {
      int x=end;
      QPointArray points( 3 );
      points.setPoint( 0, x, drawRect.top() + 6 );
      points.setPoint( 1, x-3, drawRect.top() );
      points.setPoint( 2, x+3, drawRect.top() );
      p->drawPolygon( points );
    }

  }

  p->restore();


}


void K3bAudioEditorWidget::drawMarker( QPainter* p, const QRect& drawRect, K3bAudioEditorWidget::Marker* m )
{
  p->save();

  p->setPen( m->color );
  p->setBrush( m->color );

  int x = fromPosToX( m->pos );
  //p->drawLine( x, drawRect.bottom(), x, drawRect.top() );

  QPointArray points( 3 );
  points.setPoint( 0, x, drawRect.top() + 6 );
  points.setPoint( 1, x-3, drawRect.top() );
  points.setPoint( 2, x+3, drawRect.top() );
  p->drawPolygon( points );

  p->restore();
}


void K3bAudioEditorWidget::mousePressEvent( QMouseEvent* e )
{
  m_draggedRange = 0;
  m_draggedMarker = 0; 

  bool end;
  if( Range* r = findRangeEdge( e->pos(), &end ) ) {
    
    m_draggedRange = r;
    m_draggingRangeEnd = end;
    m_oppositeRange = 0;

    // THIS
    //kdDebug()<<k_funcinfo<<r->start<<" "<<r->end<<endl;
    updateRangeMarkerColor();
    emit edgeClicked(fromPointToPos(e->pos()));
  }
  else 
    {
      m_draggedMarker = findMarker( e->pos() );   

    }

  QFrame::mousePressEvent(e);
}

void K3bAudioEditorWidget::contextMenuEvent( QContextMenuEvent * q)
{

  emit contextMenu(q->globalPos());

}

void K3bAudioEditorWidget::mouseReleaseEvent( QMouseEvent* e )
{
  //m_draggedRange = 0;
  m_draggedMarker = 0;


  QFrame::mouseReleaseEvent(e);
}


void K3bAudioEditorWidget::mouseDoubleClickEvent( QMouseEvent* e )
{
  QFrame::mouseDoubleClickEvent(e);
}


void K3bAudioEditorWidget::mouseMoveEvent( QMouseEvent* e )
{
  //kdDebug() << "(K3bAudioEditorWidget) function mouseMoveEvent called." << endl;
  if( m_mouseAt )
    emit mouseAt( fromPointToPos( e->pos() ) );

  if( e->state() & Qt::LeftButton ) {
    bool end;

    K3b::Msf msfPos=0,draggedEnd=0,otherEnd=0;
    if( m_draggedRange ) {

      signalLockFlag=1;
      adjustRange(e->pos());      

    }
    else if( m_draggedMarker ) {
      //      adjustMarker(m_draggedMarker->id,e->pos());

    }
  }
  else if( findRangeEdge( e->pos() ) || findMarker( e->pos() ) )
    setCursor( Qt::SizeHorCursor );
  else
    setCursor( Qt::PointingHandCursor );

  QFrame::mouseMoveEvent(e);
}


QColor K3bAudioEditorWidget::determineNewColor() 
{
  return QColorGroup::Highlight;
}


K3bAudioEditorWidget::Range* K3bAudioEditorWidget::getRange( int i ) const
{
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it )
    if( (*it)->id == i )
      return *it;

  return 0;
}


K3bAudioEditorWidget::Range* K3bAudioEditorWidget::findRange( const QPoint& p ) const
{
  // TODO: binary search; maybe store start and end positions in sorted lists for quick searching
  // this might be a stupid approach but we have only one range anyway so speed is not an issue yet

  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it ) {
    Range* range = *it;
    int start = fromPosToX( range->start );
    int end = fromPosToX( range->end );

    if( p.x() >= start && p.x() <= end ) {
      return range;
    }
  }
  return 0;
}


K3bAudioEditorWidget::Range* K3bAudioEditorWidget::findRangeEdge( const QPoint& p, bool* isEnd ) const
{
  // TODO: binary search
  // this might be a stupid approach but we have only one range anyway so speed is not an issue yet
  //kdDebug() << "(K3bAudioEditorWidget) function findRangeEdge called." << endl; 
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it ) {
    Range* range = *it;
    int start = fromPosToX( range->start );
    int end = fromPosToX( range->end );
    //kdDebug()<<k_funcinfo<<endl;
    //
    // In case two ranges meet at one point moving the mouse cursor deeper into one
    // range allows for grabbing that end
    //

    if( p.x() - 3 <= start && p.x() >= start && !range->startFixed ) {
      if( isEnd )
	*isEnd = false;
      return range;
    }
    else if( it.atFirst() && p.x() <= end && p.x() + 3 >= end && !range->endFixed ) {
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

  //kdDebug() << "(K3bAudioEditorWidget) function fromPointToPos called." << endl;  
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

QValueList<K3b::Msf> K3bAudioEditorWidget::getSplitPos()
{
  QValueList<K3b::Msf> pos;
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it ) {
    Range* range = *it;
    pos.append(range->start);
  } 
  pos.pop_back(); // we don't need the last item , beacuse it is the 'start' at the left most end
  return pos;

}

// sets essential parameters of range given the point , also sets the Msf of the point
void K3bAudioEditorWidget::getRangeParametersFromPoint(QPoint m_rangePointClicked,K3b::Msf& current,K3b::Msf& posStart,K3b::Msf& posEnd,
						       bool& startFixed,bool& endFixed)
{
  Range* range=findRange(m_rangePointClicked);
  posStart = range->start;
  posEnd = range->end; 
  startFixed=range->startFixed;
  endFixed=range->endFixed;

  // now set the Msf of the poin passed
  current=fromPointToPos(m_rangePointClicked);

}

void K3bAudioEditorWidget::reColor()
{
  QColor prevColor=Qt::white;
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it ) {
    Range* range = *it;
    (range->brush).setColor( prevColor= (prevColor==determineNewColor() ? Qt::white : determineNewColor()) );
	
  } 
}

void K3bAudioEditorWidget::getDraggedRangeParameters (int& rangeId,K3b::Msf& start,K3b::Msf& end , bool& draggedEnd)
{ 

  if(m_draggedRange)
    {
      
      rangeId=m_draggedRange->id ;
      start=m_draggedRange->start; 
      end=m_draggedRange->end;
      if(m_draggingRangeEnd)
	draggedEnd=1;
      else
	draggedEnd=0;
    }   
  else
    {
      
      rangeId=(m_ranges.first())->id; 
      start=(m_ranges.first())->start; 
      end=(m_ranges.first())->end;
      draggedEnd=false;
      m_draggedRange=m_ranges.first();
      // THIS
      updateRangeMarkerColor();
    } 

}

void K3bAudioEditorWidget::getOppositeRangeParameters (int& rangeId,K3b::Msf& start , K3b::Msf& end , bool& valid)
{  
	
  Range* r=0;
  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it ) 
    if((*it)==m_draggedRange) { 
      ++it;
      r=*it;
      break; 
    }

  if (r) {                      // that ,means we have a valid range
    rangeId=r->id;
    start=r->start;
    end=r->end;
    valid=true;
  }
  else
    valid=false;               // which means we do not have a valid range as the next range was found to be null


  if( m_draggedRange->endFixed && m_draggedRange->startFixed)
    valid=false;                // which means we do not have a valid range as the next range was found to be null


}
	
void K3bAudioEditorWidget::resetPointers(int rangeIdentifier1,int rangeIdentifier2)
{
  m_draggedRange=getRange(rangeIdentifier1);
  m_oppositeRange=getRange(rangeIdentifier2);
  m_draggingRangeEnd=false;
  // THIS
  updateRangeMarkerColor();

}
	
void K3bAudioEditorWidget::slotMarkerMoved( int draggedMarkerId, const K3b::Msf& draggedMarkerPos )
{

}

void K3bAudioEditorWidget::slotRangeChanged( int rangeId, const K3b::Msf& rangeStart ,
					     const K3b::Msf& rangeEnd , bool draggingRangeEnd )
{
  ;
}

bool K3bAudioEditorWidget::adjustRange(const QPoint& e)
{



  K3b::Msf msfPos=0,draggedEnd=0,otherEnd=0;


  if(!m_draggedRange) {
	
    m_draggedRange=m_ranges.first();
    m_draggingRangeEnd=false;
    updateRangeMarkerColor();
  }

  if( (m_draggedRange->startFixed==true && m_draggingRangeEnd==false)
      || (m_draggedRange->endFixed==true && m_draggingRangeEnd==true)
      ) 
    return false;


  
  int x1 = QMIN( e.x(), fromPosToX( m_draggedRange->start ) );
  int x2 = QMAX( e.x(), fromPosToX( m_draggedRange->end ) );

  msfPos = fromPointToPos( e );
  draggedEnd = ( m_draggingRangeEnd ? m_draggedRange->end : m_draggedRange->start );
  otherEnd = ( m_draggingRangeEnd ? m_draggedRange->start : m_draggedRange->end );

  int offSet=100;
  bool safetyValidate;
  
  
  if(msfPos > m_draggedRange->end ) { // safety
    
    Range* safety_range=findRange(e); 
     
    if(safety_range)
      m_draggedRange=safety_range;    
    else 
      return false;

    draggedEnd = ( m_draggingRangeEnd ? m_draggedRange->end : m_draggedRange->start );
    otherEnd = ( m_draggingRangeEnd ? m_draggedRange->start : m_draggedRange->end );

  }
  else {
 
    QPtrListIterator<Range> temp_it5( m_ranges ),temp_it6(m_ranges);
    for(; (*temp_it5)!=m_draggedRange; ++temp_it5 ) {
      ;}
    ++temp_it5;

    if(temp_it5) {
     
      if( msfPos < (*temp_it5)->start ) {

	Range* safety_range=findRange(e); 
     
	if(!safety_range)
	  return false;
	else {
	  for(; (*temp_it6)!=safety_range; ++temp_it6 ) {
	    ;}
	  --temp_it6; 
         
	  m_draggedRange=*temp_it6;
	  draggedEnd = ( m_draggingRangeEnd ? m_draggedRange->end : m_draggedRange->start );
	  otherEnd = ( m_draggingRangeEnd ? m_draggedRange->start : m_draggedRange->end );      
  

        }
      }

    }

  }
         

  for( QPtrListIterator<Range> it( m_ranges ); *it; ++it ) {
	
    Range* range = *it;

    //kdDebug()<<range->brush.color().name()<<range->start<<"  "<<range->end<<"   "<<msfPos<<" "<<m_draggedRange->start<<" "<<m_draggedRange->end<<" "<<endl;

    if( range != m_draggedRange ) {
	
      if ( msfPos > range->end ) {
	QPtrListIterator<Range> it5( m_ranges );
	for(; (*it5)!=m_draggedRange; ++it5 ) {
	  ;}
	--it5;
	Range* temp_i=0;
	if(it5) {
	  temp_i=*it5;
	
	  if ( msfPos > temp_i->start ) {
	    m_draggedRange=temp_i;
	    range=m_draggedRange;
	    draggedEnd = ( m_draggingRangeEnd ? m_draggedRange->end : m_draggedRange->start );
	    otherEnd = ( m_draggingRangeEnd ? m_draggedRange->start : m_draggedRange->end );
	    // THIS
	    updateRangeMarkerColor();
	  }
	}


	// avoiding collision
	if(otherEnd - msfPos == 0 ) { 

	  //kdDebug() << " checkpoint 1 "<<endl;   //   v1
	  //msfPos=otherEnd - offSet;              //   v1
	  // this means we have come two ranges are converging, so we have to eliminate the diminsihing one
	  // code to find range which is before draggedRange
	  QPtrListIterator<Range> it2( m_ranges );
	  Range* temp_range=0;
	  if((*it2)!=m_draggedRange)
	    for( ;*it2;++it2 ) {
	      if( (*it2)==m_draggedRange )
		break;
	      temp_range=(*it2);
	    } 
	
	  K3b::Msf temp=m_draggedRange->end;
	  bool temp_fix=m_draggedRange->endFixed;

	  removeRange(m_draggedRange->id); // this will also delete it's corresponding marker

	  m_draggedRange=temp_range; 
	  // THIS
	  updateRangeMarkerColor(); 

	  if(!m_draggedRange) { // which means this is the only range remaining

	    range->end=temp;
	    range->endFixed=temp_fix; // hence it has to be fixed at both ends

	    emit changeMsf(range->start);                                  

	    m_draggingRangeEnd=false;
	    setCursor( Qt::PointingHandCursor );
	  }


	  //search for new corresponding marker
	  Marker* m=0;
	  for( QPtrListIterator<Marker> it2( m_markers ); *it2; ++it2 ) {
	    if((*it2)->range==temp_range) {
	      m=*it2;
	      break;
	    }
	  }

	  m_draggedMarker=m; 
	  if(!m) {
	    m_draggedMarker=0;
	  }

	  // m_draggingRangeEnd remains the same 
	
	  reColor();

	}                                         //   v1
	/*if( m_draggedRange->start > m_draggedRange->end ) {

	K3b::Msf buf = m_draggedRange->start;
	m_draggedRange->start = m_draggedRange->end;
	m_draggedRange->end = buf;
	m_draggingRangeEnd = !m_draggingRangeEnd;
	}*/	



	modifyRange(range->id,range->start,msfPos);  /// **** otherEnd-msfPos should be > some value && 
	  // / ***    range->start -msfpos >5
	
	  if(m_draggedRange)  
	    m_oppositeRange=range;	// this is to compensate for  the msf editing
	  break;
      }  
      else 
	if ( msfPos >= range->start ) {
	  QPtrListIterator<Range> it5( m_ranges );   
	  for(; (*it5)!=m_draggedRange; ++it5 ) {
	    ;}
	  ++it5;
	  Range* temp_i=0;
	  if(it5) {
	    temp_i=*it5;
	    if ( msfPos < temp_i->start ) {
	      m_draggedRange=temp_i;
	      // THIS
	      updateRangeMarkerColor();
	    }

	  }
	
		

	  //kdDebug()<<"collision detector 2"<<msfPos-range->start<<endl;
	  // avoiding collision
	  if( msfPos - range->start == 0 ) {
	    //kdDebug() << " checkpoint 2 "<<endl;

	    //msfPos = range->start + offSet;
														

	    // as m_draggingRange is going to shrink, we need to assign that 'title' to the range right to it
		
	    Range* temp_range=0;
	    for(QPtrListIterator<Range> it2( m_ranges );*it2;++it2 ) {
	      if( (*it2)== range){    
		++it2;
		if(it2)
		  temp_range=(*it2);
		break;
	      }
	    } 
			
	    K3b::Msf temp=0;
	    bool temp_fix=range->startFixed;

	    removeRange(range->id); // this will also remove it's corresponding marker

	    range=temp_range; 
	    if(range==0) {// which means m_draggedRange is the only range remaining

	      m_draggedRange->start=temp;  
	      m_draggedRange->startFixed=temp_fix; // hence we have to fix the start 

	      //THIS                          
	      updateRangeMarkerColor();
	      setCursor( Qt::PointingHandCursor );
	    }
			

	    reColor(); // as ranges have changed, recolour the markers

	    for( QPtrListIterator<Range> it4( m_ranges ); *it4; ++it4 ) {
	      Range* range_n=*it4;;
	      
	
	    }

	  }

	  if(range) {
	    modifyRange(range->id,range->start,msfPos);  /// **** 
	    
	      }

	  m_oppositeRange=range;	// this is to compensate for  the msf editing	
	  break;
	

	}
	
	
    }
  }


  if(m_draggedRange) {

    
    // move it to the new pos
    if( m_draggingRangeEnd )
      m_draggedRange->end = msfPos-1;
	
    else
      m_draggedRange->start = msfPos+1;

    if (msfPos==0) {   // which means that this is the first range from the left ( starting from zer0 )
	
      m_draggedRange->start = 0;

    }

    // if we pass the other end switch them
    if( m_draggedRange->start > m_draggedRange->end ) {
	
      K3b::Msf buf = m_draggedRange->start;
      m_draggedRange->start = m_draggedRange->end;
      m_draggedRange->end = buf;
      m_draggingRangeEnd = !m_draggingRangeEnd;
    }	

    // THIS
    if(signalLockFlag==1)
      emit rangeChanged( m_draggedRange->id, m_draggedRange->start, m_draggedRange->end, m_draggingRangeEnd ); // changed


  } 

  signalLockFlag=0;

  for( QPtrListIterator<Range> it5( m_ranges ); *it5; ++it5 ) {
    Range* range_n=*it5;;
    
			
  }
  // only update the range stuff
  //       QRect rect = contentsRect();
  //       rect.setLeft( x1 );
  //       rect.setRight( x2 );
  //       update( rect );

  update();

  return true;   
}


bool K3bAudioEditorWidget::adjustRange(const K3b::Msf& pos)
{


  if( pos > m_length )
    return false;


  return adjustRange(QPoint(fromPosToX(pos),0));


}

void K3bAudioEditorWidget::updateRangeMarkerColor()
{

  if(m_draggedRange){
    Range* r=0;
    m_draggedRange->active=true;
    
    for( QPtrListIterator<Range> it( m_ranges ); *it; ++it )
      if((*it)!=m_draggedRange){
	if((*it)->active==true)
	  r=*it;
	(*it)->active=false;
      }
    int x1,x2;
    QRect rect = contentsRect();
    if(r) {
      x1 = QMIN( fromPosToX(r->start) -10 , fromPosToX(m_draggedRange->start) -10 );
      x2 = QMAX( fromPosToX(r->end) + 10 , fromPosToX(m_draggedRange->end) + 10 );
    }
    else {
      x1 = fromPosToX(m_draggedRange->start) - 10;
      x2= fromPosToX(m_draggedRange->end) + 10;
    }
    
    
    rect.setLeft( x1 );
    rect.setRight( x2 );
    update( rect );

    
 

  }                  

}


#include "k3baudioeditorwidget.moc"

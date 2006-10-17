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

#ifndef _K3B_AUDIO_EDITOR_WIDGET_H_
#define _K3B_AUDIO_EDITOR_WIDGET_H_

#include <qframe.h>
#include <qptrlist.h>
#include <qsortedlist.h>

#include <k3bmsf.h>

class QPoint;
class QPainter;


class K3bAudioEditorWidget : public QFrame
{
  Q_OBJECT

 public:
  K3bAudioEditorWidget( QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioEditorWidget();

  

  QSize sizeHint() const;
  QSize minimumSizeHint() const;

  /**
   * For now the Editor has only one parameter: the length data.
   */
  void setLength( const K3b::Msf& length );

  /**
   * Add a user editable range.
   * @param startFixed if true the range's start cannot be changed by the user, only with modifyRange
   * @param endFixed if true the range's end cannot be changed by the user, only with modifyRange
   * @param brush if not specified or it has the NoBrush style one is chosen automatically.
   *
   * @return -1 on error or an identifier on success (be aware that the highest value for end is length-1)
   */
  int addRange( const K3b::Msf& start, const K3b::Msf& end, 
		bool startFixed = false, bool endFixed = false,
		const QString& toolTip = QString::null,
		const QBrush& brush = QBrush() );

  /**
   * @returns false if the range does not exist or end was bigger than start.
   */
  bool modifyRange( int identifier, const K3b::Msf& start, const K3b::Msf& end );

  /**
   * @returns false if the range does not exist.
   */
  bool removeRange( int identifier );

  K3b::Msf rangeStart( int identifier ) const;
  K3b::Msf rangeEnd( int identifier ) const;

  void setMaxNumberOfMarkers( int );

  /**
   * @return false if the marker does not exist.
   */
  bool removeMarker( int identifier );

  /**
   * @return false if the range does does not exist
   */
  bool removeRange(QPoint m_rangePointClicked);

  /**
   * @return false if the range does does not exist
   */

  bool removeRangeAdjust(QPoint m_rangePointClicked);

  /**
   * @return false if the marker does not exist.
   */
  bool moveMarker( int identifier, const K3b::Msf& );

  void enableMouseAtSignal( bool b ) { m_mouseAt = b; }

  /**
    * prepare the menu to be popped up when user right clicks on a range
    */
  // void setupSplitActions();

  


  /**
   * @returns a list of positions at which the track is to be split
   */
  QValueList<K3b::Msf> K3bAudioEditorWidget::getSplitPos(); 

  /**
   * sets essential parameters of the range which contains the point (first parameter) and also sets the msf of that point 
   */
  void getRangeParametersFromPoint(QPoint m_rangePointClicked,K3b::Msf& current,K3b::Msf& posStart,K3b::Msf& posEnd,
                                  bool& startFixed,bool& endFixed);

  
  
  
   /** gets the parameters of the range which was being dragged 
    *
    */
  void getDraggedRangeParameters (int& rangeId,K3b::Msf& start,K3b::Msf& end , bool& draggedEnd ) ;
    
   /** gets the parameters of the range which was next to the one being dragged 
    *
    */
  void getOppositeRangeParameters (int& rangeId,K3b::Msf& start,K3b::Msf& end ,bool& draggedEnd) ;

  void resetPointers(int,int);
  
  /**
   * for remote adusting of range  
   */ 
  bool adjustRange(const K3b::Msf& pos);
  
  /**
   * @return s the no. of ranges in the track
   */
  int getRangeCount() const ;

 protected slots:
   //void slotSplitHere();

   //void slotRemoveRange();

     // to compensate for the msf edit movement
  
   /**
    * shows the popupMenu
    */
  //void showPopmenu(const QPoint&);

  void slotMarkerMoved( int draggedMarkerId, const K3b::Msf& draggedMarkerPos );
  void slotRangeChanged(int , const K3b::Msf& , const K3b::Msf& , bool );


 signals:
  /**
   * Emitted when enabled.
   */
  void mouseAt( const K3b::Msf& );

  /**
   * Emitted when the user changed a range.
   * This signal is not emitted when a range is changed via modifyRange.
   */
  void rangeChanged( int identifier, const K3b::Msf& start, const K3b::Msf& end , bool m_draggingRangeEnd);
  void rangeRemoved( int );

  /**
   * Emitted when the user moves a marker.
   * This signal is not emitted when a marker is changed via moveMarker.
   */
  void markerMoved( int identifier, const K3b::Msf& pos );
  void markerAdded( int identifier, const K3b::Msf& pos );
  void markerRemoved( int identifier );

  /**
    * emitted when user right clicks on range
    */ 
 
  void contextMenu(const QPoint&);  

  void edgeClicked(const K3b::Msf&);

  void changeMsf(const K3b::Msf&);


 private:
  class Range;
  class Marker;
  class ToolTip;

  void mousePressEvent( QMouseEvent* e );
  void mouseReleaseEvent( QMouseEvent* e );
  void mouseDoubleClickEvent( QMouseEvent* e );
  void mouseMoveEvent( QMouseEvent* e );
  void drawContents( QPainter* );
  void drawAll( QPainter*, const QRect& );
  void drawRange( QPainter* p, const QRect&, Range* r );
  void drawMarker( QPainter* p, const QRect&, Marker* m );

  void updateRangeMarkerColor();

  bool adjustRange(const QPoint& p);
  
  void adjustMarker(int identifier,const QPoint& p);
  void reColor();  

  Range* getRange( int i ) const;
  Marker* getMarker( int i ) const;
  Range* findRange( const QPoint& p ) const;
  Range* findRangeEdge( const QPoint& p, bool* end = 0 ) const;
  Marker* findMarker( const QPoint& p ) const;
  QColor determineNewColor() ;
  K3b::Msf fromPointToPos( const QPoint& p ) const;
  int fromPosToX( const K3b::Msf& msf ) const;

  void K3bAudioEditorWidget::contextMenuEvent( QContextMenuEvent * );

/**
   * @param fixed if true the marker cannot be changed by the user, only with moveMarker
   * @return -1 on error or an identifier on success.
   */
  int addMarker( const K3b::Msf&,K3bAudioEditorWidget::Range*, bool end=false ,bool fixed = false, 
		 const QString& toolTip = QString::null, const QColor& color = QColor() );

  

 int rangeLock;    // another one of my helpless hack  (not used currently)
 int signalLockFlag;

  int m_maxMarkers;
  QPoint m_rangePointClicked;
  K3b::Msf m_length;
  QSortedList<Range> m_ranges;
  QPtrList<Marker> m_markers;
  int m_idCnt;
  bool m_mouseAt;
  

  /**
   * Margin around the timethingy
   */
  int m_margin;

  Range* m_draggedRange;
  bool m_draggingRangeEnd;
  Range* m_oppositeRange;
  Marker* m_draggedMarker;

  

  ToolTip* m_toolTip;
};

#endif

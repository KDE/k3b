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

#include <k3bmsf.h>


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
   * @param fixed if true the marker cannot be changed by the user, only with moveMarker
   * @return -1 on error or an identifier on success.
   */
  int addMarker( const K3b::Msf& pos, bool fixed = false );

  /**
   * @return false if the marker does not exist.
   */
  bool removeMarker( int identifier );

  /**
   * @return false if the marker does not exist.
   */
  bool moveMarker( int identifier, const K3b::Msf& );

  void enableMouseAtSignal( bool b ) { m_mouseAt = b; }

 signals:
  /**
   * Emitted when enabled.
   */
  void mouseAt( const K3b::Msf& );

  /**
   * Emitted when the user changed a range.
   * This signal is not emitted when a range is changed via modifyRange.
   */
  void rangeChanged( int identifier, const K3b::Msf& start, const K3b::Msf& end );
  void rangeRemoved( int );

  /**
   * Emitted when the user moves a marker.
   * This signal is not emitted when a marker is changed via moveMarker.
   */
  void markerMoved( int identifier, const K3b::Msf& pos );
  void markerAdded( int identifier, const K3b::Msf& pos );
  void markerRemoved( int identifier );

 private:
  class Range;
  class Marker;

  void mousePressEvent( QMouseEvent* e );
  void mouseReleaseEvent( QMouseEvent* e );
  void mouseDoubleClickEvent( QMouseEvent* e );
  void mouseMoveEvent( QMouseEvent* e );
  void drawContents( QPainter* );
  void drawRange( QPainter* p, Range* r );
  void drawMarker( QPainter* p, Marker* m );

  Range* getRange( int i ) const;
  Marker* getMarker( int i ) const;
  Range* findRange( const QPoint& p, bool* end ) const;
  Marker* findMarker( const QPoint& p ) const;
  QColor determineNewColor() const;
  K3b::Msf fromPointToPos( const QPoint& p ) const;
  int fromPosToX( const K3b::Msf& msf ) const;

  int m_maxMarkers;
  K3b::Msf m_length;
  QPtrList<Range> m_ranges;
  QPtrList<Marker> m_markers;
  int m_idCnt;
  bool m_mouseAt;

  /**
   * Margin around the timethingy
   */
  int m_margin;

  Range* m_draggedRange;
  bool m_draggingRangeEnd;
  Marker* m_draggedMarker;
};

#endif

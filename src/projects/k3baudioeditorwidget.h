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

#ifndef _K3B_AUDIO_EDITOR_WIDGET_H_
#define _K3B_AUDIO_EDITOR_WIDGET_H_

#include "k3bmsf.h"

#include <QList>
#include <QMouseEvent>
#include <QFrame>


class QPainter;


namespace K3b {
class AudioEditorWidget : public QFrame
{
    Q_OBJECT

public:
    explicit AudioEditorWidget( QWidget* parent = 0 );
    ~AudioEditorWidget() override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /**
     * For now the Editor has only one parameter: the length data.
     */
    void setLength( const K3b::Msf& length );

    const K3b::Msf length() const;

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
                  const QString& toolTip = QString(),
                  const QBrush& brush = QBrush() );

    /**
     * \returns the identifier of the range which spans over x position \a pos or
     * 0 if no range is defined in this region.
     */
    int findRange( int pos ) const;

    /**
     * Searches for a range edge at x position \a pos.
     * \return The ranges identifier if an edge could be found or 0 if there is no
     * range edge at the position.
     */
    int findRangeEdge( int pos, bool* end = 0 ) const;

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

    /**
     * \return A list of all ranges' identifiers sorted ascending by start
     * offset
     */
    QList<int> allRanges() const;

    void setMaxNumberOfMarkers( int );

    /**
     * @param fixed if true the marker cannot be changed by the user, only with moveMarker
     * @return -1 on error or an identifier on success.
     */
    int addMarker( const K3b::Msf& pos, bool fixed = false,
                   const QString& toolTip = QString(), const QColor& color = QColor() );

    /**
     * @return false if the marker does not exist.
     */
    bool removeMarker( int identifier );

    /**
     * @return false if the marker does not exist.
     */
    bool moveMarker( int identifier, const K3b::Msf& );

    void enableMouseAtSignal( bool b );

    /**
     * By default ranges can overlap. If overlapping ranges are not allowed
     * the editor widget will modify and delete ranges accordingly.
     *
     * Caution: So far setting this will not check if ranges already overlap.
     */
    void setAllowOverlappingRanges( bool b );

    /**
     * Range selection is by default disabled. If it is enabled the editor visually
     * highlights the last clicked range.
     *
     * \sa setSelectedRange
     */
    void enableRangeSelection( bool b );

    bool allowOverlappingRanges() const;
    bool rangeSelectedEnabled() const;

    void setSelectedRange( int id );

    /**
     * \return The identifier of the currently selected range or 0
     * if none is selected.
     */
    int selectedRange() const;

    K3b::Msf posToMsf( int x ) const;
    int msfToPos( const K3b::Msf& msf ) const;

    /**
     * Set the brush to paint the selected range. Default is QColorGroup::Highlight
     */
    void setSelectedRangeBrush( const QBrush& );
    const QBrush& selectedRangeBrush() const;

Q_SIGNALS:
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

    void selectedRangeChanged( int );

    /**
     * Emitted when the user moves a marker.
     * This signal is not emitted when a marker is changed via moveMarker.
     */
    void markerMoved( int identifier, const K3b::Msf& pos );
    void markerAdded( int identifier, const K3b::Msf& pos );
    void markerRemoved( int identifier );

protected:
    void paintEvent( QPaintEvent* e ) override;
    void mousePressEvent( QMouseEvent* e ) override;
    void mouseReleaseEvent( QMouseEvent* e ) override;
    void mouseDoubleClickEvent( QMouseEvent* e ) override;
    void mouseMoveEvent( QMouseEvent* e ) override;
    bool event( QEvent* e ) override;

private:
    class Range;
    class Marker;
    struct SortByStart;

    class Private;
    Private* d;

    void drawAll( QPainter*, const QRect& );
    void drawRange( QPainter* p, const QRect&, const Range& r );
    void drawMarker( QPainter* p, const QRect&, const Marker& m );

    /**
     * Makes sure that \a r does not overlap any other range by modifying and
     * deleting other ranges.
     */
    void fixupOverlappingRanges( int rangeId );

    Range* getRange( int i ) const;
    Marker* getMarker( int i ) const;
    Range* findRange( const QPoint& p ) const;
    Range* findRangeEdge( const QPoint& p, bool* end = 0 ) const;
    Marker* findMarker( const QPoint& p ) const;
};
}

#endif

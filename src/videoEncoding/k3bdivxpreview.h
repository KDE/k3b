/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
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


#ifndef K3BDVDPREVIEW_H
#define K3BDVDPREVIEW_H

#include <qwidget.h>
#include <qcanvas.h>
#include <qstring.h>

class QCanvasLine;
class QCanvas;
class QCanvasPixmap;
class QPainter;
class QCanvasPixmap;
class QCanvasPixmapArray;


class K3bDivxPreview : public QCanvasView  {
   Q_OBJECT
public:
    K3bDivxPreview(QCanvas* c, QWidget *parent=0, const char *name=0);
    ~K3bDivxPreview();
    void setPreviewPicture( const QString& image );
    //void updatePreviewPicture( const QString &image );
    void setCroppingLines();
    void setTopLine( int offset );
    void setLeftLine( int offset );
    void setBottomLine( int offset );
    void setRightLine( int offset );
    void resetView();
    void updateLines();

protected:
   void drawContents( QPainter* p );

private:
    QCanvasLine *m_lineTop;
    QCanvasLine *m_lineBottom;
    QCanvasLine *m_lineLeft;
    QCanvasLine *m_lineRight;
    QCanvas *can;
    QCanvasSprite *m_sprite;  // image
    QCanvasPixmap *m_previewPixmap;
    QCanvasPixmapArray *m_previewPixmapArray;

    float m_imageScale;
    QString m_imageSource;
    // cropping line offset to sprite edges
    int m_offsetTop;
    int m_offsetLeft;
    int m_offsetBottom;
    int m_offsetRight;

    bool m_initialized;

    void updateLineOffsets(bool upScale);
};

#endif

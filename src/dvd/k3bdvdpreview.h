/***************************************************************************
                          k3bdvdpreview.h  -  description
                             -------------------
    begin                : Tue Apr 2 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BDVDPREVIEW_H
#define K3BDVDPREVIEW_H

#include <qwidget.h>
#include <qcanvas.h>

class QCanvasLine;
class QCanvas;
class QPainter;

/**
  *@author Sebastian Trueg
  */

class K3bDvdPreview : public QCanvasView  {
   Q_OBJECT
public: 
	K3bDvdPreview(QCanvas* c, QWidget *parent=0, const char *name=0);
	~K3bDvdPreview();
protected:
   void drawContents( QPainter* p );

private:
    QCanvasLine *lineTop;
    QCanvas *can;

};

#endif

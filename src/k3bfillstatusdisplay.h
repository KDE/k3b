/***************************************************************************
                          k3bfillstatusdisplay.h  -  description
                             -------------------
    begin                : Tue Apr 10 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3BFILLSTATUSDISPLAY_H
#define K3BFILLSTATUSDISPLAY_H

#include <qframe.h>

class QPainter;
class QMouseEvent;
class K3bDoc;
class KAction;
class KPopupMenu;


/**
  *@author Sebastian Trueg
  */

class K3bFillStatusDisplay : public QFrame  {

  Q_OBJECT

 public:
  K3bFillStatusDisplay(K3bDoc* doc, QWidget *parent=0, const char *name=0);
  ~K3bFillStatusDisplay();

 public slots:
  void showSize();
  void showTime();
	
 protected:
  void mousePressEvent( QMouseEvent* );
  void drawContents(QPainter*);
  void drawSize(QPainter*);
  void drawTime(QPainter*);
  void setupPopupMenu();
	
 private:
  bool m_showTime;
  K3bDoc* doc;

  KAction* m_showMinutes;
  KAction* m_showMegs;
  KPopupMenu* m_popup;
};

#endif

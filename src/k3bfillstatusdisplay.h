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
class KToggleAction;
class KPopupMenu;


/**
  *@author Sebastian Trueg
  */

class K3bFillStatusDisplay : public QFrame  {

  Q_OBJECT

 public:
  K3bFillStatusDisplay(K3bDoc* doc, QWidget *parent=0, const char *name=0);
  ~K3bFillStatusDisplay();

  QSize sizeHint() const;
  QSize minimumSizeHint() const;

 public slots:
  void showSize();
  void showTime();
	
 protected:
  void mousePressEvent( QMouseEvent* );
  void drawContents(QPainter*);
  void setupPopupMenu();

 private slots:
  void slot74Minutes();
  void slot80Minutes();
  void slot100Minutes();
  void slotCustomSize();

 private:
  long m_cdSize;
  bool m_showTime;
  K3bDoc* m_doc;

  KToggleAction* m_actionShowMinutes;
  KToggleAction* m_actionShowMegs;
  KToggleAction* m_action74Min;
  KToggleAction* m_action80Min;
  KToggleAction* m_action100Min;
  KToggleAction* m_actionCustomSize;

  KPopupMenu* m_popup;
};

#endif

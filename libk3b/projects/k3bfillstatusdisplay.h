
/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BFILLSTATUSDISPLAY_H
#define K3BFILLSTATUSDISPLAY_H

#include <qframe.h>


class QPaintEvent;
class QMouseEvent;
class K3bDoc;
class KToggleAction;
class KAction;
class KActionCollection;
class KPopupMenu;
class QToolButton;

namespace K3bCdDevice {
  class DeviceHandler;
}
namespace K3b {
  class Msf;
}


/**
  *@author Sebastian Trueg
  */
class K3bFillStatusDisplayWidget : public QWidget
{
  Q_OBJECT

 public:
  K3bFillStatusDisplayWidget( K3bDoc* doc, QWidget* parent );
  ~K3bFillStatusDisplayWidget();

  QSize sizeHint() const;
  QSize minimumSizeHint() const;

  const K3b::Msf& cdSize() const;

 public slots:
  void setShowTime( bool b );
  void setCdSize( const K3b::Msf& );

 signals:
  void contextMenu( const QPoint& );

 protected:
  void mousePressEvent( QMouseEvent* );
  void paintEvent(QPaintEvent*);

 private:
  class Private;
  Private* d;
};


class K3bFillStatusDisplay : public QFrame  {

  Q_OBJECT

 public:
  K3bFillStatusDisplay(K3bDoc* doc, QWidget *parent=0, const char *name=0);
  ~K3bFillStatusDisplay();

 public slots:
  void showSize();
  void showTime();
  void showDvdSizes( bool );

 protected:
  void setupPopupMenu();
  void paintEvent(QPaintEvent*);

 private slots:
  void slot74Minutes();
  void slot80Minutes();
  void slot100Minutes();
  void slotDvd4_7GB();
  void slotDvdDoubleLayer();
  void slotCustomSize();
  void slotMenuButtonClicked();
  void slotPopupMenu(const QPoint&);
  void slotDetermineSize();
  void slotRemainingSize( K3bCdDevice::DeviceHandler* );
  void slotDocSizeChanged();

  void slotLoadUserDefaults();
  void slotSaveUserDefaults();

 private:
  class Private;
  Private* d;
};

#endif

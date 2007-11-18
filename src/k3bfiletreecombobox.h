/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_FILETREE_COMBOBOX_H_
#define _K3B_FILETREE_COMBOBOX_H_

#include <kcombobox.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QPixmap>

class K3bFileTreeView;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

namespace K3bDevice {
  class Device;
}

class K3bFileTreeComboBox : public KComboBox
{
  Q_OBJECT

 public:
  K3bFileTreeComboBox( QWidget* parent = 0 );
  ~K3bFileTreeComboBox();

  void popup();
  void popdown();

  void setCurrentItem( int );
  void setCurrentText( const QString& );

 public slots:
  void setDevice( K3bDevice::Device* );
  void setUrl( const KUrl& url );
  void slotGoUrl();

 signals:
  void urlExecuted( const KUrl& url );
  void deviceExecuted( K3bDevice::Device* dev );

 private slots:
  void slotDeviceExecuted( K3bDevice::Device* );
  void slotUrlExecuted( const KUrl& url );

 protected:
  bool eventFilter( QObject*, QEvent* );
  void keyPressEvent( QKeyEvent* );
  void mousePressEvent( QMouseEvent* );
  void paintEvent( QPaintEvent* );

 private:
  void setEditText( const QPixmap& pix, const QString& t );

  class Private;
  Private* d;
  K3bFileTreeView* m_fileTreeView;
};

#endif

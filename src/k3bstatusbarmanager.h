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



#ifndef K3B_STATUSBAR_MANAGER_H
#define K3B_STATUSBAR_MANAGER_H

#include <qobject.h>
//Added by qt3to4:
#include <QEvent>
#include <QLabel>

class QLabel;
class K3bMainWindow;
class QEvent;
class K3bDoc;
class QTimer;

class K3bStatusBarManager : public QObject
{
  Q_OBJECT

 public:
  K3bStatusBarManager( K3bMainWindow* parent );
  ~K3bStatusBarManager();

 public Q_SLOTS:
  void update();

 private Q_SLOTS:
  void slotFreeTempSpace( const QString&, unsigned long, unsigned long, unsigned long );
  void showActionStatusText( const QString& text );
  void clearActionStatusText();
  void slotActiveProjectChanged( K3bDoc* doc );
  void slotUpdateProjectStats();

 private:
  bool eventFilter( QObject* o, QEvent* e );

  QLabel* m_labelInfoMessage;
  QLabel* m_pixFreeTemp;
  QLabel* m_labelFreeTemp;
  QLabel* m_versionBox;
  QLabel* m_labelProjectInfo;

  K3bMainWindow* m_mainWindow;

  QTimer* m_updateTimer;
};

#endif

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



#ifndef K3B_STATUSBAR_MANAGER_H
#define K3B_STATUSBAR_MANAGER_H

#include <qobject.h>

class QLabel;
class K3bBusyWidget;
class K3bMainWindow;


class K3bStatusBarManager : public QObject
{
  Q_OBJECT

 public:
  K3bStatusBarManager( K3bMainWindow* parent );
  ~K3bStatusBarManager();

 public slots:
  void update();
  void showBusyInfo( const QString& );
  void endBusy();

 private slots:
  void slotFreeTempSpace( const QString&, unsigned long, unsigned long, unsigned long );

 private:
  QLabel* m_labelInfoMessage;
  QLabel* m_pixFreeTemp;
  QLabel* m_labelFreeTemp;
  K3bBusyWidget* m_busyWidget;

  K3bMainWindow* m_mainWindow;
};

#endif

/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_WELCOME_WIDGET_H_
#define _K3B_WELCOME_WIDGET_H_

#include <qwidget.h>

class K3bMainWindow;


class K3bWelcomeWidget : public QWidget
{
  Q_OBJECT

 public:
  K3bWelcomeWidget( K3bMainWindow*, QWidget* parent = 0, const char* name = 0 );
  ~K3bWelcomeWidget();

 private slots:
  void slotUrlClick( const QString& );
  void slotMailClick( const QString& adress, const QString& );

 private:
  K3bMainWindow* m_mainWindow;
};

#endif

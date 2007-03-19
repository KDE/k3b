/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * K3bMiniButton is based on KDockButton_Private
 * Copyright (C) 2000 Max Judin <novaprint@mtu-net.ru>
 * Copyright (C) 2002,2003 Joseph Wenninger <jowenn@kde.org>
 * Copyright (C) 2005 Dominik Haumann <dhdev@gmx.de> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_MINI_BUTTON_H_
#define _K3B_MINI_BUTTON_H_

#include <qpushbutton.h>

class QPainter;
class QEvent;


/**
 * K3bMiniButton is a minimalistic button mainly used
 * to show a pixmap.
 */
class K3bMiniButton : public QPushButton
{
  Q_OBJECT

 public:
  K3bMiniButton( QWidget *parent=0, const char *name=0 );
  virtual ~K3bMiniButton();

 protected:
  virtual void drawButton( QPainter * );
  virtual void enterEvent( QEvent * );
  virtual void leaveEvent( QEvent * );
  
 private:
  bool m_mouseOver;
};

#endif

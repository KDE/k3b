/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_TOOL_BUTTON_H_
#define _K3B_TOOL_BUTTON_H_

#include "k3b_export.h"

#include <qtoolbutton.h>

class QPainter;
class QEvent;


/**
 * the K3bToolButton is an enhanced QToolButton which adds two functionalities:
 * <li>A delayed popup menu is shown immiadetely once the mouse is dragged downwards
 *     much like the KToolBarButton
 * <li>If a popup menu is set a little arrow indicates this.
 */
class LIBK3B_EXPORT K3bToolButton : public QToolButton
{
 public:
  K3bToolButton( QWidget* parent = 0 );
  ~K3bToolButton();

  void setInstantMenu( bool );

 protected:
  virtual void drawButton( QPainter* );
  virtual bool eventFilter( QObject*, QEvent* );

 private:
  class Private;
  Private* d;
};

#endif

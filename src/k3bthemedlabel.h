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

#ifndef _K3B_THEMED_LABEL_H_
#define _K3B_THEMED_LABEL_H_

#include <kcutlabel.h>
#include <k3bthememanager.h>


class K3bThemedLabel : public KCutLabel
{
  Q_OBJECT

 public:
  K3bThemedLabel( QWidget* parent = 0 );
  K3bThemedLabel( const QString& text, QWidget* parent = 0 );
  K3bThemedLabel( K3bTheme::PixmapType, QWidget* parent = 0 );

 public slots:
  void setThemePixmap( K3bTheme::PixmapType );

 private slots:
  void slotThemeChanged();

 private:
  int m_themePixmapCode;
};

#endif

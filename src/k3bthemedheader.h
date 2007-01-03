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

#ifndef _K3B_THEMED_HEADER_H_
#define _K3B_THEMED_HEADER_H_

#include <qframe.h>

#include "k3bthememanager.h"

class K3bTitleLabel;
class QLabel;

class K3bThemedHeader : public QFrame
{
  Q_OBJECT

 public:
  K3bThemedHeader( QWidget* parent = 0 );
  K3bThemedHeader( const QString& title, const QString& subtitle, QWidget* parent = 0 );
  ~K3bThemedHeader(); 

 public slots:
  void setTitle( const QString& title, const QString& subtitle = QString::null );
  void setSubTitle( const QString& subtitle );
  void setAlignment( int );
  void setLeftPixmap( K3bTheme::PixmapType );
  void setRightPixmap( K3bTheme::PixmapType );

 private slots:
  void slotThemeChanged();

 private:
  void init();

  K3bTitleLabel* m_titleLabel;
  QLabel* m_leftLabel;
  QLabel* m_rightLabel;
  K3bTheme::PixmapType m_leftPix;
  K3bTheme::PixmapType m_rightPix;
};

#endif

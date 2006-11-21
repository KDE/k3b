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


#ifndef K3BCD_CONTENTS_VIEW_H
#define K3BCD_CONTENTS_VIEW_H

#include <qwidget.h>
#include <k3bthememanager.h>

class K3bThemedHeader;


/**
 * Abstract class from which all cd views must be
 * derived.
 */
class K3bCdContentsView : public QWidget
{
  Q_OBJECT

 public:
  K3bCdContentsView( bool withHeader,
		     QWidget* parent = 0, const char* name = 0 );
  virtual ~K3bCdContentsView();

  virtual void reload();

 protected:
  QWidget* mainWidget();
  void setMainWidget( QWidget* );
  void setTitle( const QString& );
  void setLeftPixmap( K3bTheme::PixmapType );
  void setRightPixmap( K3bTheme::PixmapType );

 private:
  K3bThemedHeader* m_header;

  QWidget* m_centerWidget;
};

#endif

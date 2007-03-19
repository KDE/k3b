/* 
 *
 * $Id: k3bcdcontentsview.h 576315 2006-08-23 19:32:42Z trueg $
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


#ifndef _K3B_CONTENTS_VIEW_H_
#define _K3B_CONTENTS_VIEW_H_

#include <qwidget.h>
#include <k3bthememanager.h>

class K3bThemedHeader;


class K3bContentsView : public QWidget
{
  Q_OBJECT

 public:
  virtual ~K3bContentsView();

 protected:
  K3bContentsView( bool withHeader,
		   QWidget* parent = 0, 
		   const char* name = 0 );

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

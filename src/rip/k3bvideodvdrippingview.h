/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_VIDEODVD_RIPPING_VIEW_H_
#define _K3B_VIDEODVD_RIPPING_VIEW_H_

#include <k3bcdcontentsview.h>
#include <k3bmedium.h>

class K3bVideoDVDRippingTitleListView;

class K3bVideoDVDRippingView : public K3bCdContentsView
{
  Q_OBJECT

 public:
  K3bVideoDVDRippingView( QWidget* parent = 0, const char * name = 0 );
  ~K3bVideoDVDRippingView();

  void setMedium( const K3bMedium& medium );

 private:
  K3bVideoDVDRippingTitleListView* m_titleView;  
};

#endif

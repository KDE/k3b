/*
 *
 * $Id$
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



#ifndef K3BDISKINFOVIEW_H
#define K3BDISKINFOVIEW_H

#include "k3bmediacontentsview.h"
#include "k3bmedium.h"
//Added by qt3to4:
#include <QLabel>

class QLabel;
class K3ListView;
class K3bIso9660;

namespace K3bDevice {
  class DiskInfoDetector;
  class DiskInfo;
}

class K3bDiskInfoView : public K3bMediaContentsView
{
  Q_OBJECT

 public:
  K3bDiskInfoView( QWidget* parent = 0, const char* name = 0 );
  ~K3bDiskInfoView();

  void enableInteraction( bool enable );

 private:
  void reloadMedium();

  void createMediaInfoItems( const K3bMedium& );
  void createIso9660InfoItems( const K3bIso9660SimplePrimaryDescriptor& );

  K3ListView* m_infoView;

  class HeaderViewItem;
  class TwoColumnViewItem;
};


#endif

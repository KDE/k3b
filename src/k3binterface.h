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


#ifndef _K3B_INTERFACE_H_
#define _K3B_INTERFACE_H_

#include <dcopobject.h>
#include <dcopref.h>
#include <qvaluelist.h>

#include <kurl.h>


class K3bMainWindow;


class K3bInterface : public DCOPObject
{
  K_DCOP

 public:
  K3bInterface( K3bMainWindow* );

 k_dcop:
  /**
   * returns a DCOPRef to a K3bProjectInterface
   */
  DCOPRef createDataCDProject();
  DCOPRef createAudioCDProject();
  DCOPRef createMixedCDProject();
  DCOPRef createVideoCDProject();
  DCOPRef createMovixCDProject();
  DCOPRef createDataDVDProject();
  DCOPRef createVideoDVDProject();
  DCOPRef createMovixDVDProject();

  DCOPRef openDocument( const KURL& url );

  QValueList<DCOPRef> projects();

  /**
   * Add URLs to the current active project.
   * If no project is open a new Audio or Data CD
   * project will be created depending on the type
   * of the first file.
   */
  void addUrls( const KURL::List& urls );
  void addUrl( const KURL& url );

 private:
  K3bMainWindow* m_main;
};

#endif

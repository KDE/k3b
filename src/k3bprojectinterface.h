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


#ifndef _K3B_PROJECT_INTERFACE_H_
#define _K3B_PROJECT_INTERFACE_H_

#include <dcopobject.h>
#include <kurl.h>

class K3bDoc;

/**
 * Base class for all project interfaces
 */
class K3bProjectInterface : public DCOPObject
{
  K_DCOP

 public:
  K3bProjectInterface( K3bDoc*, const char* name = 0 );
  virtual ~K3bProjectInterface();

  // Generate a name for this interface. Automatically used if name=0 is
  // passed to the constructor
  static QCString newIfaceName();

 k_dcop:
  virtual void addUrls( const KURL::List& urls );
  virtual void addUrl( const KURL& url );

 private:
  K3bDoc* m_doc;
};

#endif

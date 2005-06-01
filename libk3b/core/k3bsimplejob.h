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


#ifndef _K3B_SIMPLE_JOB_H_
#define _K3B_SIMPLE_JOB_H_

#include <k3bjob.h>
#include "k3b_export.h"

/**
 * A simple job which does not require a JobHandler.
 *
 * @author Sebastian Trueg
 */
class LIBK3B_EXPORT K3bSimpleJob : public K3bJob
{
  Q_OBJECT
	
 public:
  K3bSimpleJob( QObject* parent = 0, const char* name = 0 )
    : K3bJob( 0, parent, name ) {
  }

  /**
   * reimplemented from K3bJobHandler
   */
  int waitForMedia( K3bDevice::Device*,
		    int, int, const QString& ) {
    return 0;
  }
  
  /**
   * reimplemented from K3bJobHandler
   */
  bool questionYesNo( const QString&,
		      const QString& ) { 
    return false;
  }
};
#endif

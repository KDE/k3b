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

#ifndef _K3B_DVDDOC_H_
#define _K3B_DVDDOC_H_

#include <k3bdatadoc.h>

class KConfig;

class K3bDvdDoc : public K3bDataDoc
{
 public:
  K3bDvdDoc( QObject* parent = 0 );
  virtual ~K3bDvdDoc();

  virtual int docType() const { return DVD; }

  virtual K3bBurnJob* newBurnJob();
  virtual K3bView* newView( QWidget* parent );

 protected:
  virtual QString documentType() const { return "dvd"; }
  virtual void loadDefaultSettings( KConfig* );
};

#endif

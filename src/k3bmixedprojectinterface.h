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


#ifndef _K3B_MIXED_PROJECT_INTERFACE_H_
#define _K3B_MIXED_PROJECT_INTERFACE_H_

#include "k3bprojectinterface.h"

#include <dcopref.h>

class K3bMixedDoc;
class K3bDataProjectInterface;
class K3bAudioProjectInterface;


class K3bMixedProjectInterface : public K3bProjectInterface
{
  K_DCOP

 public:
  K3bMixedProjectInterface( K3bMixedDoc* );

 k_dcop:
  DCOPRef dataPart() const;
  DCOPRef audioPart() const;

 private:
  K3bMixedDoc* m_mixedDoc;

  K3bDataProjectInterface* m_dataInterface;
  K3bAudioProjectInterface* m_audioInterface;
};

#endif

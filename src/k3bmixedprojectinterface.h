/* 
 *
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

namespace K3b {
    class MixedDoc;
}
namespace K3b {
    class DataProjectInterface;
}
namespace K3b {
    class AudioProjectInterface;
}


namespace K3b {
class MixedProjectInterface : public ProjectInterface
{
  K_DCOP

 public:
  MixedProjectInterface( MixedDoc* );

 k_dcop:
  DCOPRef dataPart() const;
  DCOPRef audioPart() const;

 private:
  MixedDoc* m_mixedDoc;

  DataProjectInterface* m_dataInterface;
  AudioProjectInterface* m_audioInterface;
};
}

#endif

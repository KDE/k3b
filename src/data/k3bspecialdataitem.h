/* 
 *
 * $Id: $
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


#ifndef K3BSPECIALDATAITEM_H
#define K3BSPECIALDATAITEM_H

#include "k3bdataitem.h"

class K3bSpecialDataItem : public K3bDataItem
{
 public:
  K3bSpecialDataItem( K3bDataDoc* doc, long size, K3bDataItem* parent = 0 );
  virtual ~K3bSpecialDataItem();

  virtual QString localPath() { return ""; }

  virtual long k3bSize() const { return m_k3bSize; }

 private:
  long m_k3bSize;
};

#endif


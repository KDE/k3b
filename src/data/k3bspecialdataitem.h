/***************************************************************************
                          k3bspecialdataitem.h
                             -------------------
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BSPECIALDATAITEM_H
#define K3BSPECIALDATAITEM_H

#include "k3bdataitem.h"

class K3bSpecialDataItem : public K3bDataItem
{
 public:
  K3bSpecialDataItem( K3bDataDoc* doc, K3bDataItem* parent = 0 );
  virtual ~K3bSpecialDataItem();

  virtual QString localPath() { return ""; }

  virtual long k3bSize() const { return m_k3bSize; }
  void setK3bSize( long size ) { m_k3bSize = size; }

 private:
  long m_k3bSize;
};

#endif


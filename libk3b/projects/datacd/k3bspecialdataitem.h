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


#ifndef K3BSPECIALDATAITEM_H
#define K3BSPECIALDATAITEM_H

#include "k3bdataitem.h"
#include "k3bdiritem.h"

#include <kio/global.h>

/**
 * This can be used to create fake items like the boot catalog
 * It's mainly a K3bDataItem where everything has to be set manually
 */
class K3bSpecialDataItem : public K3bDataItem
{
 public:
  K3bSpecialDataItem( K3bDataDoc* doc, KIO::filesize_t size, K3bDirItem* parent = 0, const QString& k3bName = QString::null )
    : K3bDataItem( doc, parent ),
    m_size( size )
    {
      setK3bName( k3bName );

      // add automagically like a qlistviewitem
      if( parent )
	parent->addDataItem( this );
    }

  ~K3bSpecialDataItem() {
    // remove this from parentdir
    if( parent() )
      parent()->takeDataItem( this );
  }

  void setMimeType( const QString& s ) { m_mimeType = s; }
  const QString& mimeType() const { return m_mimeType; }

  bool isSpecialFile() const { return true; }

 protected:
  /**
   * Normally one does not use this method but K3bDataItem::size()
   */
  KIO::filesize_t itemSize( bool ) const { return m_size; }

 private:
  QString m_mimeType;
  KIO::filesize_t m_size;
};

#endif


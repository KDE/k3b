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


#ifndef _K3B_MOVIX_FILEITEM_H_
#define _K3B_MOVIX_FILEITEM_H_

#include <k3bfileitem.h>

class K3bMovixDoc;


class K3bMovixFileItem : public K3bFileItem
{
 public:
  K3bMovixFileItem( const QString& fileName, K3bMovixDoc* doc, K3bDirItem* dir, const QString& k3bName = 0 );
  ~K3bMovixFileItem();

  K3bFileItem* subTitleItem() const { return m_subTitleItem; }
  void setSubTitleItem( K3bFileItem* i ) { m_subTitleItem = i; }

  /**
   * reimplemented from K3bDataItem
   * also renames the subTitleItem
   */
  void setK3bName( const QString& );

  /**
   * returnes the name that the subtitle file must have in
   * order to work with mplayer
   */
  static QString subTitleFileName( const QString& );

 private:
  K3bMovixDoc* m_doc;

  K3bFileItem* m_subTitleItem;
};

#endif

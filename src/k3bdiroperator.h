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


#ifndef K3BDIROPERATOR_H
#define K3BDIROPERATOR_H

#include <kdiroperator.h>

class QIconViewItem;
class QListViewItem;


/**
  *@author Sebastian Trueg
  */
class K3bDirOperator : public KDirOperator
{
  Q_OBJECT

 public: 
  K3bDirOperator( const KURL& urlName = KURL(), QWidget* parent = 0, const char* name = 0 );
  ~K3bDirOperator();

 signals:
  void doubleClicked( KFileItem* item );

 protected:
  KFileView* createView( QWidget* parent, KFile::FileView view );

 protected slots:
  void slotIconViewItemDoubleClicked( QIconViewItem* );
  void slotListViewItemDoubleClicked( QListViewItem* );
};

#endif

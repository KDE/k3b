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


#ifndef K3BCDDBMULTIENTRIESDIALOG_H
#define K3BCDDBMULTIENTRIESDIALOG_H

#include <kdialogbase.h>

#include "cddb/k3bcddbquery.h"
#include "cddb/k3bcddbresult.h"


class QStringList;
class KListBox;

/**
  *@author Sebastian Trueg
  */
class K3bCddbMultiEntriesDialog : public KDialogBase  
{
  Q_OBJECT

 public:
  ~K3bCddbMultiEntriesDialog();
  
  static const K3bCddbResultHeader& selectCddbEntry( K3bCddbQuery* query, QWidget* parent = 0 );

 protected:
  K3bCddbMultiEntriesDialog( QWidget* parent = 0, const char* name = 0);

 private:
  KListBox *m_listBox;
};

#endif

/***************************************************************************
                          k3bcddbmultientriesdialog.h  -  description
                             -------------------
    begin                : Sun Feb 10 2002
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

#ifndef K3BCDDBMULTIENTRIESDIALOG_H
#define K3BCDDBMULTIENTRIESDIALOG_H

#include <kdialogbase.h>

#include "k3bcddb.h"


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
  
  static int selectCddbEntry( const K3bCddbQuery& query, QWidget* parent = 0 );

 protected:
  K3bCddbMultiEntriesDialog( QWidget* parent = 0, const char* name = 0);

 private:
  KListBox *m_listBox;
};

#endif

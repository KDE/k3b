/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_DATA_URL_ADDING_DIALOG_H_
#define _K3B_DATA_URL_ADDING_DIALOG_H_

#include <kdialogbase.h>
#include <kurl.h>
#include <qvaluelist.h>
#include <qpair.h>

class K3bBusyWidget;
class QLabel;
class K3bDirItem;


class K3bDataUrlAddingDialog : public KDialogBase
{
  Q_OBJECT

 public:
  ~K3bDataUrlAddingDialog();

  /**
   * \return \see QDialog::exec()
   */
  static int addUrls( const KURL::List& urls, K3bDirItem* dir = 0,
		      QWidget* parent = 0 );

 private slots:
  void slotAddUrls();

 private:
  K3bDataUrlAddingDialog( QWidget* parent = 0, const char* name = 0 );

  bool getNewName( const QString& oldName, K3bDirItem* dir, QString& newName );

  K3bBusyWidget* m_busyWidget;
  QLabel* m_infoLabel;

  QValueList< QPair<KURL, K3bDirItem*> > m_urlQueue;

  bool m_bExistingItemsReplaceAll;
  bool m_bExistingItemsIgnoreAll;
};

#endif

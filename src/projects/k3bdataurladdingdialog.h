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
#include <qstringlist.h>
#include <qpair.h>
#include <qdir.h>

class K3bBusyWidget;
class QLabel;
class K3bDataItem;
class K3bDirItem;
class K3bEncodingConverter;


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

  static int moveItems( const QValueList<K3bDataItem*>& items, K3bDirItem* dir,
			QWidget* parent = 0 );

  static int copyItems( const QValueList<K3bDataItem*>& items, K3bDirItem* dir,
			QWidget* parent = 0 );

  static int copyMoveItems( const QValueList<K3bDataItem*>& items, K3bDirItem* dir,
			    QWidget* parent, bool copy );

 private slots:
  void slotAddUrls();
  void slotCopyMoveItems();
  void slotCancel();

 private:
  K3bDataUrlAddingDialog( QWidget* parent = 0, const char* name = 0 );

  bool getNewName( const QString& oldName, K3bDirItem* dir, QString& newName );

  bool checkForHiddenFiles( const QDir& dir );
  bool checkForSystemFiles( const QDir& dir );

  QString resultMessage() const;

  K3bBusyWidget* m_busyWidget;
  QLabel* m_infoLabel;
  K3bEncodingConverter* m_encodingConverter;

  KURL::List m_urls;
  QValueList< QPair<KURL, K3bDirItem*> > m_urlQueue;

  QValueList< QPair<K3bDataItem*, K3bDirItem*> > m_items;

  bool m_bExistingItemsReplaceAll;
  bool m_bExistingItemsIgnoreAll;
  int m_iAddHiddenFiles;
  int m_iAddSystemFiles;

  QStringList m_unreadableFiles;
  QStringList m_notFoundFiles;
  QStringList m_nonLocalFiles;
  QStringList m_tooBigFiles;
  QStringList m_mkisofsLimitationRenamedFiles;
  QStringList m_invalidFilenameEncodingFiles;

  bool m_bCanceled;

  int m_urlCounter;

  bool m_copyItems;
};

#endif

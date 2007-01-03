/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DATA_URL_ADDING_DIALOG_H_
#define _K3B_DATA_URL_ADDING_DIALOG_H_

#include <kdialogbase.h>
#include <kurl.h>
#include <qstringlist.h>
#include <qpair.h>
#include <qdir.h>

class KProgress;
class QLabel;
class K3bDataItem;
class K3bDirItem;
class K3bEncodingConverter;
class K3bDirSizeJob;
class K3bDataDoc;


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
  void slotDirSizeDone( bool );
  void updateProgress();

 private:
  K3bDataUrlAddingDialog( K3bDataDoc* doc, QWidget* parent = 0, const char* name = 0 );

  bool getNewName( const QString& oldName, K3bDirItem* dir, QString& newName );

  bool addHiddenFiles();
  bool addSystemFiles();

  QString resultMessage() const;

  KProgress* m_progressWidget;
  QLabel* m_infoLabel;
  QLabel* m_counterLabel;
  K3bEncodingConverter* m_encodingConverter;

  KURL::List m_urls;
  QValueList< QPair<KURL, K3bDirItem*> > m_urlQueue;

  QValueList< QPair<K3bDataItem*, K3bDirItem*> > m_items;

  QValueList<KURL> m_dirSizeQueue;

  bool m_bExistingItemsReplaceAll;
  bool m_bExistingItemsIgnoreAll;
  bool m_bFolderLinksFollowAll;
  bool m_bFolderLinksAddAll;
  int m_iAddHiddenFiles;
  int m_iAddSystemFiles;

  QStringList m_unreadableFiles;
  QStringList m_notFoundFiles;
  QStringList m_nonLocalFiles;
  QStringList m_tooBigFiles;
  QStringList m_mkisofsLimitationRenamedFiles;
  QStringList m_invalidFilenameEncodingFiles;

  bool m_bCanceled;

  bool m_copyItems;

  KIO::filesize_t m_totalFiles;
  KIO::filesize_t m_filesHandled;
  K3bDirSizeJob* m_dirSizeJob;

  unsigned int m_lastProgress;
};

#endif

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


#ifndef K3BDATADOC_H
#define K3BDATADOC_H

#include <k3bdoc.h>

#include "k3bisooptions.h"

#include <qptrlist.h>
#include <qfileinfo.h>
#include <qstringlist.h>

#include <kurl.h>
#include <kio/global.h>

class K3bDataItem;
class K3bRootItem;
class K3bDirItem;
class K3bFileItem;
class K3bJob;
class K3bBootItem;
class K3bFileCompilationSizeHandler;

class KProgressDialog;
class K3bView;
class KConfig;
class QString;
class QStringList;
class QWidget;
class QDomDocument;
class QDomElement;
class KArchiveDirectory;

namespace K3bCdDevice {
  class CdDevice;
  class DeviceHandler;
}


/**
 *@author Sebastian Trueg
 */

class K3bDataDoc : public K3bDoc
{
  Q_OBJECT

 public:
  K3bDataDoc( QObject* parent = 0 );
  virtual ~K3bDataDoc();

  virtual int docType() const { return DATA; }

  enum mutiSessionModes { NONE, START, CONTINUE, FINISH };

  K3bRootItem* root() const { return m_root; }

  virtual bool newDocument();
  virtual KIO::filesize_t size() const;

  /**
   * This is used for multisession where size() also returnes the imported session's size
   */
  virtual KIO::filesize_t burningSize() const;
  virtual K3b::Msf length() const;

  const QString& name() const { return m_name; }

  void removeItem( K3bDataItem* item );
  void moveItem( K3bDataItem* item, K3bDirItem* newParent );
  void moveItems( QPtrList<K3bDataItem> itemList, K3bDirItem* newParent );

  K3bDirItem* addEmptyDir( const QString& name, K3bDirItem* parent );
	
  /** writes a mkisofs-path-spec file with graft-points **/
  QString writePathSpec( const QString& fileName );

  QString treatWhitespace( const QString& );
	
  /**
   * returns an empty dummy dir for use with K3bDirItems.
   * Creates one if nessessary.
   * The dummy dir is used to create empty dirs on the iso-filesystem! 
   */
  static QString dummyDir();

  virtual K3bBurnJob* newBurnJob();
	
  int multiSessionMode() const { return m_multisessionMode; }
  void setMultiSessionMode( int mode );

  int dataMode() const { return m_dataMode; }
  void setDataMode( int m ) { m_dataMode = m; }

  void setVerifyData( bool b ) { m_verifyData = b; }
  bool verifyData() const { return m_verifyData; }

  static bool nameAlreadyInDir( const QString&, K3bDirItem* );

  K3bIsoOptions& isoOptions() { return m_isoOptions; }

  const QPtrList<K3bBootItem>& bootImages() { return m_bootImages; }
  K3bDataItem* bootCataloge() { return m_bootCataloge; }

  K3bDirItem* bootImageDir();
  K3bBootItem* createBootItem( const QString& filename, K3bDirItem* bootDir = 0 );
  /** this will just remove it from the list of boot items, not remove it from the doc */
  void removeBootItem( K3bBootItem* );

  // This is just a temp solution and will be removed
  K3bFileCompilationSizeHandler* sizeHandler() const { return m_sizeHandler; }

 public slots:
  void slotBurn();

  /** add urls to the compilation.
   * @param dir the directory where to add the urls, by default this is the root directory.
   **/
  void slotAddUrlsToDir( const KURL::List&, K3bDirItem* dir = 0 );
  virtual void addUrls( const KURL::List& urls );

  void importSession( K3bCdDevice::CdDevice* );
  void clearImportedSession();

  virtual void loadDefaultSettings( KConfig* );

 signals:
  void itemRemoved( K3bDataItem* );
  void newFileItems();
	
 private slots:
  void slotAddQueuedItems();
  void slotTocRead( K3bCdDevice::DeviceHandler* dh );

 protected:
  /** reimplemented from K3bDoc */
  virtual bool loadDocumentData( QDomElement* root );
  /** reimplemented from K3bDoc */
  virtual bool saveDocumentData( QDomElement* );

  void saveDocumentDataOptions( QDomElement& optionsElem );
  void saveDocumentDataHeader( QDomElement& headerElem );
  bool loadDocumentDataOptions( QDomElement optionsElem );
  bool loadDocumentDataHeader( QDomElement optionsElem );

  virtual QString documentType() const;

  K3bFileCompilationSizeHandler* m_sizeHandler;
  //  K3bFileCompilationSizeHandler* m_oldSessionSizeHandler;
  KIO::filesize_t m_oldSessionSize;
  KIO::filesize_t m_oldSessionHackSize; // :((

  virtual K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0, const char* name = 0 );

  /** reimplemented from K3bDoc */
  virtual K3bView* newView( QWidget* parent );

 private:
  void createSessionImportItems( const KArchiveDirectory*, K3bDirItem* parent );
  K3bDataItem* createBootCatalogeItem( K3bDirItem* bootDir );

  /**
   * load recursivly
   */
  bool loadDataItem( QDomElement& e, K3bDirItem* parent );
  /**
   * save recursivly
   */
  void saveDataItem( K3bDataItem* item, QDomDocument* doc, QDomElement* parent );

  K3bDirItem* createDirItem( QFileInfo& f, K3bDirItem* parent );
  K3bFileItem* createFileItem( QFileInfo& f, K3bDirItem* parent );

  void informAboutNotFoundFiles();

  class PrivateItemToAdd {
  public:
    PrivateItemToAdd( const QString& p, K3bDirItem* i )
      :fileInfo(p) { parent = i; }
    PrivateItemToAdd( const QFileInfo& f, K3bDirItem* i )
      :fileInfo(f) { parent = i; }
    QFileInfo fileInfo;
    K3bDirItem* parent;
  };

  QPtrList<PrivateItemToAdd> m_queuedToAddItems;
  QTimer* m_queuedToAddItemsTimer;
  int m_numberAddedItems;

  QStringList m_notFoundFiles;
  QStringList m_noPermissionFiles;

  K3bRootItem* m_root;
  QString m_name;

  int m_dataMode;

  bool m_verifyData;

  KIO::filesize_t m_size;
		
  K3bIsoOptions m_isoOptions;

  int m_multisessionMode;
  QPtrList<K3bDataItem> m_oldSession;

  // boot cd stuff
  K3bDataItem* m_bootCataloge;
  QPtrList<K3bBootItem> m_bootImages;

  bool m_bExistingItemsReplaceAll;
  bool m_bExistingItemsIgnoreAll;

  friend class K3bMixedDoc;
};

#endif

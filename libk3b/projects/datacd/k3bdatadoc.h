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
//class K3bView;
class KConfig;
class QString;
class QStringList;
class QWidget;
class QDomDocument;
class QDomElement;
class K3bIso9660Directory;

namespace K3bDevice {
  class Device;
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

  virtual int type() const { return DATA; }

  enum MultiSessionMode { 
    /**
     * Let the K3bDataJob decide if to close the CD or not.
     * The decision is based on the state of the inserted media
     * (appendable/closed), the size of the project (will it fill 
     * up the CD?), and the free space on the inserted media.
     */
    AUTO,
    NONE, 
    START, 
    CONTINUE, 
    FINISH
  };

  K3bRootItem* root() const { return m_root; }

  virtual bool newDocument();
  virtual KIO::filesize_t size() const;

  /**
   * This is used for multisession where size() also returnes the imported session's size
   */
  virtual KIO::filesize_t burningSize() const;
  virtual K3b::Msf length() const;

  const QString& name() const { return m_name; }

  /**
   * Simply deletes the item if it is removable (meaning isRemovable() returns true.
   * Be aware that you can remove items simply by deleting them even if isRemovable()
   * returns false.
   */
  void removeItem( K3bDataItem* item );

  /**
   * Simply calls reparent.
   */
  void moveItem( K3bDataItem* item, K3bDirItem* newParent );
  void moveItems( QPtrList<K3bDataItem> itemList, K3bDirItem* newParent );

  K3bDirItem* addEmptyDir( const QString& name, K3bDirItem* parent );
	
  QString treatWhitespace( const QString& );
	
  virtual K3bBurnJob* newBurnJob( K3bJobHandler* hdl, QObject* parent = 0 );
	
  MultiSessionMode multiSessionMode() const { return m_multisessionMode; }
  void setMultiSessionMode( MultiSessionMode mode );

  int dataMode() const { return m_dataMode; }
  void setDataMode( int m ) { m_dataMode = m; }

  void setVerifyData( bool b ) { m_verifyData = b; }
  bool verifyData() const { return m_verifyData; }

  static bool nameAlreadyInDir( const QString&, K3bDirItem* );

  /**
   * Most of the options that map to the mkisofs parameters are grouped
   * together in the K3bIsoOptions class to allow easy saving to and loading
   * from a KConfig object.
   */
  const K3bIsoOptions& isoOptions() const { return m_isoOptions; }
  void setIsoOptions( const K3bIsoOptions& );

  const QPtrList<K3bBootItem>& bootImages() { return m_bootImages; }
  K3bDataItem* bootCataloge() { return m_bootCataloge; }

  K3bDirItem* bootImageDir();
  K3bBootItem* createBootItem( const QString& filename, K3bDirItem* bootDir = 0 );
  /** this will just remove it from the list of boot items, not remove it from the doc */
  void removeBootItem( K3bBootItem* );

  /**
   * This will prepare the filenames as written to the image.
   * These filenames are saved in K3bDataItem::writtenName
   */
  void prepareFilenames();

  /**
   * Returns true if filenames need to be cut due to the limitations of Joliet.
   *
   * This is only valid after a call to @p prepareFilenames()
   */
  bool needToCutFilenames() const { return m_needToCutFilenames; }

 public slots:
  virtual void addUrls( const KURL::List& urls );

  /**
   * Add urls syncroneously
   * This method adds files recursively including symlinks, hidden, and system files.
   * If a file already exists the new file's name will be appended a number.
   */
  virtual void addUrls( const KURL::List& urls, K3bDirItem* dir );

  void importSession( K3bDevice::Device* );
  void clearImportedSession();

  /**
   * Just a convience method to prevent using setIsoOptions for this
   * often used value.
   */
  void setVolumeID( const QString& );

 signals:
  void itemRemoved( K3bDataItem* );
  void itemAdded( K3bDataItem* );

 private slots:
  void slotTocRead( K3bDevice::DeviceHandler* dh );

 protected:
  /** reimplemented from K3bDoc */
  virtual bool loadDocumentData( QDomElement* root );
  /** reimplemented from K3bDoc */
  virtual bool saveDocumentData( QDomElement* );

  void saveDocumentDataOptions( QDomElement& optionsElem );
  void saveDocumentDataHeader( QDomElement& headerElem );
  bool loadDocumentDataOptions( QDomElement optionsElem );
  bool loadDocumentDataHeader( QDomElement optionsElem );

  virtual QString typeString() const;

  K3bFileCompilationSizeHandler* m_sizeHandler;

  //  K3bFileCompilationSizeHandler* m_oldSessionSizeHandler;
  KIO::filesize_t m_oldSessionSize;

 private:
  void prepareFilenamesInDir( K3bDirItem* dir );
  void createSessionImportItems( const K3bIso9660Directory*, K3bDirItem* parent );
  K3bDataItem* createBootCatalogeItem( K3bDirItem* bootDir );

  /**
   * used by K3bDirItem to inform about removed items.
   */
  void itemRemovedFromDir( K3bDirItem* parent, K3bDataItem* removedItem );
  void itemAddedToDir( K3bDirItem* parent, K3bDataItem* addedItem );

  /**
   * load recursivly
   */
  bool loadDataItem( QDomElement& e, K3bDirItem* parent );
  /**
   * save recursivly
   */
  void saveDataItem( K3bDataItem* item, QDomDocument* doc, QDomElement* parent );

  void informAboutNotFoundFiles();

  QStringList m_notFoundFiles;
  QStringList m_noPermissionFiles;

  K3bRootItem* m_root;
  QString m_name;

  int m_dataMode;

  bool m_verifyData;

  KIO::filesize_t m_size;
		
  K3bIsoOptions m_isoOptions;

  MultiSessionMode m_multisessionMode;
  QPtrList<K3bDataItem> m_oldSession;

  // boot cd stuff
  K3bDataItem* m_bootCataloge;
  QPtrList<K3bBootItem> m_bootImages;

  bool m_bExistingItemsReplaceAll;
  bool m_bExistingItemsIgnoreAll;

  bool m_needToCutFilenames;

  friend class K3bMixedDoc;
  friend class K3bDirItem;
};

#endif

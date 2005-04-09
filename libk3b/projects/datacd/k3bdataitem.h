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


#ifndef K3BDATAITEM_H
#define K3BDATAITEM_H


class K3bDirItem;
class K3bDataDoc;

#include <qstring.h>

#include <kio/global.h>

/**
  *@author Sebastian Trueg
  */

class K3bDataItem
{
 public: 
  K3bDataItem( K3bDataDoc* doc, K3bDataItem* parent = 0 );
  virtual ~K3bDataItem();
	
  K3bDirItem* parent() const { return m_parentDir; }
	
  K3bDataDoc* doc() const { return m_doc; }
  virtual const QString& k3bName() const;
  virtual void setK3bName( const QString& );

  /** 
   * returns the path as defined by the k3b-hierachy, NOT starting with a slash
   * (since this is used for graft-points!) 
   * directories have a trailing "/"
   */
  virtual QString k3bPath() const;

  /**
   * Returns the name of the item as used on the CD or DVD image.
   *
   * Only valid for Rockridge and Joliet since for now K3b does not determine
   * the names as created by mkisofs when creating an ISO9660 only filesystem.
   *
   * This is only valid after a call to @p K3bDataDoc::prepareFilenames()
   */
  const QString& writtenName() const { return m_writtenName; }

  /**
   * Returns the path of the item as written to the CD or DVD image.
   *
   * This is suited to be used for mkisofs graftpoints.
   *
   * This is only valid after a call to @p K3bDataDoc::prepareFilenames()
   */
  virtual QString writtenPath() const;

  /**
   * Used to set the written name by @p K3bDataDoc::prepareFilenames()
   */
  void setWrittenName( const QString& s ) { m_writtenName = s; }

  virtual K3bDataItem* nextSibling();
	
  /** returns the path to the file on the local filesystem */
  virtual QString localPath() const { return QString::null; }

  virtual KIO::filesize_t size() const { return 0; }

  /** returnes the dir of the item (or the item if it's a dir) */
  virtual K3bDirItem* getDirItem() = 0;

  virtual void reparent( K3bDirItem* );

  virtual bool isDir() const { return false; }
  virtual bool isFile() const { return false; }
  virtual bool isSpecialFile() const { return false; }
  virtual bool isSymLink() const { return false; }	
  virtual bool isFromOldSession() const { return false; }

  bool hideOnRockRidge() const;
  bool hideOnJoliet() const;

  virtual void setHideOnRockRidge( bool b );
  virtual void setHideOnJoliet( bool b );

  virtual long sortWeight() const { return m_sortWeigth; }
  virtual void setSortWeigth( long w ) { m_sortWeigth = w; }

  virtual int depth() const;

  virtual bool isValid() const { return true; }

  // these are all needed for special fileitems like
  // imported sessions or the movix filesystem
  virtual bool isRemoveable() const { return m_bRemoveable; }
  virtual bool isMoveable() const { return m_bMovable; }
  virtual bool isRenameable() const { return m_bRenameable; }
  virtual bool isHideable() const { return m_bHideable; }
  virtual bool writeToCd() const { return m_bWriteToCd; }
  virtual const QString& extraInfo() const { return m_extraInfo; }

  void setRenameable( bool b ) { m_bRenameable = b; }
  void setMoveable( bool b ) { m_bMovable = b; }
  void setRemoveable( bool b ) { m_bRemoveable = b; }
  void setHideable( bool b ) { m_bHideable = b; }
  void setWriteToCd( bool b ) { m_bWriteToCd = b; }
  void setExtraInfo( const QString& i ) { m_extraInfo = i; }

 protected:
  QString m_k3bName;

 private:
  QString m_writtenName;

  K3bDataDoc* m_doc;
  K3bDirItem* m_parentDir;

  bool m_bHideOnRockRidge;
  bool m_bHideOnJoliet;
  bool m_bRemoveable;
  bool m_bRenameable;
  bool m_bMovable;
  bool m_bHideable;
  bool m_bWriteToCd;
  QString m_extraInfo;

  long m_sortWeigth;
};

#endif

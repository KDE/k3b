/***************************************************************************
                          k3bdataitem.h  -  description
                             -------------------
    begin                : Wed Apr 25 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3BDATAITEM_H
#define K3BDATAITEM_H


class K3bDirItem;
class K3bDataDoc;

#include <qstring.h>

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
  const QString& k3bName();
  const QString& jolietName();
  void setK3bName( const QString& );
  void setJolietName( const QString& );
  /** 
   * returns the path as defined by the k3b-hierachy, NOT starting with a slash (since this is used for graft-points!) 
   * directories have a trailing "/"
   */
  virtual QString k3bPath();
  virtual QString jolietPath();

  virtual K3bDataItem* nextSibling();
	
  /** returns the path to the file on the local filesystem */
  virtual QString localPath() = 0;
		
  virtual long k3bSize() const { return 0; }

  /** adds the given dataItem to the current parent (can be the item itself if it is a K3bDirItem) */
  virtual K3bDirItem* addDataItem( K3bDataItem* ) = 0;

  virtual void reparent( K3bDirItem* );

  virtual bool isDir() const { return false; }
	
  bool hideOnRockRidge() const;
  bool hideOnJoliet() const;

  virtual void setHideOnRockRidge( bool b );
  virtual void setHideOnJoliet( bool b );

 protected:
  QString m_k3bName;
  /**
   * this is used if Joliet extensions are enabled for cutting the filenames
   */
  QString m_jolietName;

 private:
  K3bDirItem* m_parentDir;
  K3bDataDoc* m_doc;

  bool m_bHideOnRockRidge;
  bool m_bHideOnJoliet;
};

#endif

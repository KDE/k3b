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


#ifndef _K3B_DATA_PROJECT_INTERFACE_H_
#define _K3B_DATA_PROJECT_INTERFACE_H_

#include "k3bprojectinterface.h"

#include <qstringlist.h>

class K3bDataDoc;


class K3bDataProjectInterface : public K3bProjectInterface
{
  K_DCOP

 public:
  K3bDataProjectInterface( K3bDataDoc*, const char* name = 0 );
  ~K3bDataProjectInterface();

 k_dcop:
  /**
   * Create a new folder in the root of the doc.
   * This is the same as calling createFolder( name, "/" )
   */
  bool createFolder( const QString& name );

  /**
   * Create a new folder with name @p name in the folder with the
   * absolute path @p parent. 
   *
   * \return true if the folder was created successfully, false if 
   *         an item with the same name already exists or the parent
   *         directory could not be found.
   *
   * Example: createFolder( "test", "/foo/bar" ) will create the
   *          folder /foo/bar/test.
   */
  bool createFolder( const QString& name, const QString& parent );

  /**
   * Add urls to a specific folder in the project.
   *
   * Example: addUrl( "test.txt", "/foo/bar" ) will add the file test.txt
   *          to folder /foo/bar.
   */
  void addUrl( const KURL& url, const QString& parent );

  void addUrls( const KURL::List& urls, const QString& parent );

  /**
   * Remove an item
   * \return true if the item was successfully removed.
   */
  bool removeItem( const QString& path );

  /**
   * Rename an item
   * \return true if the item was successfully renamed, false if
   *         no item could be found at \p path, \p newName is empty,
   *         or the item cannot be renamed for some reason.
   */
  bool renameItem( const QString& path, const QString& newName );

  /**
   * Set the volume ID of the data project. This is the name shown by Windows
   * when the CD is inserted.
   */
  void setVolumeID( const QString& id );

  /**
   * \return true if the specified path exists in the project and it is a folder.
   */
  bool isFolder( const QString& path ) const;

  /**
   * \return the names of the child elements of the item determined by path.
   */
  QStringList children( const QString& path ) const;

  /**
   * Set the sort weigth of an item
   * \return false if the item at \p could not be found.
   */
  bool setSortWeight( const QString& path, int weight ) const;

 private:
  K3bDataDoc* m_dataDoc;
};

#endif

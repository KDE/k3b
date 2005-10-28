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

#ifndef _K3B_IMAGE_CACHE_H_
#define _K3B_IMAGE_CACHE_H_

#include <k3bjob.h>
#include "k3bimagesink.h"

#include <k3b_export.h>

#include <kio/global.h>


/**
 * The Image Cache writes an image to a local disk. If the image only contains
 * a single track the cache will only write a single cache file. In case the image
 * contains several tracks it will use a folder which contains the tracks named
 * "track01.xxx, track02.xxx". The extension is "wav" for audio tracks and "iso"
 * for MODE1 data tracks. All other track types will have the extension "bin".
 */
class LIBK3B_EXPORT K3bImageCache : public K3bJob, public K3bImageSink
{
  Q_OBJECT

 public:
  K3bImageCache( K3bJobHandler*, QObject* );
  ~K3bImageCache();

 public slots:
  void start();
  void cancel();

  /**
   * Set the path where the image should be cached. This can be a folder or
   * a filename. Depending on the image type the cache will use this path.
   * Missing folders will be created.
   */
  void setCachePath( const QString& path );

  /**
   * Remove the cache files.
   */
  void cleanup();

 private slots:
  void slotTocReady( bool );
  void slotReadAsync();

 private:
  bool initCachePath();
  QString createCacheFilename( unsigned int track ) const;
  void initBuffer( unsigned int track );
  bool openImageFile( unsigned int track );

 private:
  class Private;
  Private* d;
};

#endif

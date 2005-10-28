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

#ifndef _K3B_PLAIN_IMAGE_SOURCE_H_
#define _K3B_PLAIN_IMAGE_SOURCE_H_


#include "k3bsimpleimagesource.h"


/**
 * An image source which stream a single data file as one single data track.
 */
class K3bPlainImageSource : public K3bSimpleImageSource
{
 public:
  K3bPlainImageSource( const QString& filename, K3bJobHandler* jh, QObject* parent );
  K3bPlainImageSource( K3bJobHandler* jh, QObject* parent );
  ~K3bPlainImageSource();

  void setFilename( const QString& name );
  K3bDevice::Toc toc() const;

  void skip( long bytes );

  K3b::SectorSize sectorSize( unsigned int ) const { return K3b::SECTORSIZE_DATA_2048; }

 protected:
  bool init();
  long simpleRead( char* data, long maxLen );

  class Private;
  Private* d;
};

#endif

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

#ifndef _K3B_VIDEODVD_FOLDER_IMAGE_SOURCE_H_
#define _K3B_VIDEODVD_FOLDER_IMAGE_SOURCE_H_

#include "k3bimagesource.h"
#include <k3bmkisofshandler.h>

class KProcess;
class K3bProcess;
class K3bVideoDVDFolderReader;

class K3bVideoDVDFolderImageSource : public K3bImageSource, public K3bMkisofsHandler
{
  Q_OBJECT

 public:
  K3bVideoDVDFolderImageSource( const K3bVideoDVDFolderReader*, K3bJobHandler*, QObject* parent = 0 );
  ~K3bVideoDVDFolderImageSource();

  long read( char* data, long maxLen );

  K3b::SectorSize sectorSize( unsigned int ) const { return K3b::SECTORSIZE_DATA_2048; }

 public slots:
  void start();
  void determineSize();
  void cancel();

  void setVolumeId( const QString& id );

 private slots:
  void slotMkisofsStderr( const QString& );
  void slotMkisofsExited( KProcess* );

 protected:
  void handleMkisofsProgress( int );
  void handleMkisofsInfoMessage( const QString&, int );

 private:
  K3bProcess* createMkisofsProcess( bool printSize );

  class Private;
  Private* d;
};

#endif

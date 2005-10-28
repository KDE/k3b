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


#ifndef _K3B_VIDEODVDFOLDER_READER_H_
#define _K3B_VIDEODVDFOLDER_READER_H_

#include "k3bimagereaderbase.h"



/**
 * An ImageReader which actually converts a Video DVD file structure on the
 * local hd into an udf video dvd image using mkisofs.
 * So imageFileName() points to a directory.
 */
class K3bVideoDVDFolderReader : public K3bImageReaderBase
{
 public:
  K3bVideoDVDFolderReader();
  K3bVideoDVDFolderReader( const QString& );
  ~K3bVideoDVDFolderReader();

  bool open( const QString& file );
  void close();
  bool isOpen() const;

  QString imageType() const;
  QString imageTypeComment() const;

  int mediaType() const { return DVD_IMAGE; }

  QString metaInformation() const;

  K3bImageSource* createImageProvider( K3bJobHandler*, QObject* parent = 0 ) const;

  /**
   * Overide the default volume id of the image. The default is the name of the directory 
   * containing the Video DVD structures.
   */
  void setVolumeId( const QString& id ) { m_volumeId = id; }
  const QString& volumeId() const { return m_volumeId; }

 private:
  bool m_bOpen;
  QString m_volumeId;
};

#endif

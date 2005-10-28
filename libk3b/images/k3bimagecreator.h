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

#ifndef _K3B_IMAGE_CREATOR_H_
#define _K3B_IMAGE_CREATOR_H_

#include <k3bimagesink.h>
#include <k3bjob.h>

class K3bImage;

/**
 * The K3bImageCreator is a K3bImageSink which creates an image file
 * from an arbitrary K3bImageSource.
 *
 * Usage is as simple as setting a destination filename, setting an
 * image source and starting the job.
 */
class K3bImageCreator : public K3bJob, public K3bImageSink
{
  Q_OBJECT

 public:
  K3bImageCreator( K3bJobHandler* handler, QObject* parent );
  ~K3bImageCreator();

 public slots:
  void start();
  void cancel();

  void setImageFilename( const QString& filename );

 private slots:
  void slotTocReady( bool );
  void slotReadAsync();

 private:
  bool startNextTrack();

  K3bImage* m_image;

  class Private;
  Private* d;
};

#endif

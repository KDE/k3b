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

#ifndef _K3B_IMAGE_SOURCE_STREAMER_H_
#define _K3B_IMAGE_SOURCE_STREAMER_H_

#include <k3bthreadjob.h>

class K3bImageSource;


/**
 * The K3bImageSourceStreamer is a helper class which reads the data from a 
 * source's read() method and pipes it into a file descriptor using a thread.
 *
 * This class is intended to be used inside a K3bImageSource class. Do not 
 * use it outside of such a class.
 *
 * When started the K3bImageSourceStreamer simply starrs reading. That means
 * the Source has to be initialized before.
 */
class K3bImageSourceStreamer : public K3bThreadJob
{
 public:
  K3bImageSourceStreamer( K3bJobHandler*, QObject* parent );
  ~K3bImageSourceStreamer();

  void setSource( K3bImageSource* s );

 private:
  class WorkThread;
  WorkThread* m_thread;
};

#endif

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

#ifndef _K3B_IMAGE_SINK_H_
#define _K3B_IMAGE_SINK_H_

#include <k3b_export.h>

class K3bImageSource;


/**
 * Interface for all image sink jobs that work on the data
 * provided by a K3bImageSource.
 *
 * This is implemeted as an interface rather than a K3bJob to allow
 * subclasses to inherit from K3bJob or K3bBurnJob.
 */
class LIBK3B_EXPORT K3bImageSink
{
 public:
  virtual ~K3bImageSink();

  K3bImageSource* source() const { return m_source; }
  void setSource( K3bImageSource* source ) { m_source = source; }

 protected:
  K3bImageSink();

  /**
   * Create a file descriptor pair for communication with the source.
   *
   * Can be used in combination with a QSocketNotifier:
   *
   * <pre>
   * openFdPair();
   * source()->writeToFd( fdIn() );
   * connect( new QSocketNotifier( fdOut(), QSocketNotifier::Read, this ),
   *          SIGNAL(activated(int)),
   *          this,
   *          SLOT(...)) );</pre>
   */
  bool openFdPair();
  void closeFdPair();
  
  /**
   * Fd opened by openFdPair() to write to
   */
  int fdIn() const;

  /**
   * Fd opened by openFdPair() to read from
   */
  int fdOut() const;

 private:
  K3bImageSource* m_source;

  class Private;
  Private* d;
};

#endif

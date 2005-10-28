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

#ifndef _K3B_SIMPLE_IMAGESOURCE_H_
#define _K3B_SIMPLE_IMAGESOURCE_H_

#include "k3bimagesource.h"


/**
 * Base class for simple image sources that are based on sync reading via read().
 * Simple reimplement simpleRead() and if nessessary init() and cleanup().
 *
 * Make sure to do all the error handling in simpleRead() and never emit the
 * finished signal. It is done by the base class.
 *
 * The data streaming is done by a thread using the read method.
 *
 * <b>The K3bSimpleImageSource does not support multiple sessions!</b>
 */
class K3bSimpleImageSource : public K3bImageSource
{
  Q_OBJECT

 public:
  K3bSimpleImageSource( K3bJobHandler*, QObject* parent = 0 );
  virtual ~K3bSimpleImageSource();

  virtual long read( char* data, long maxLen );

 public slots:
  virtual void start();
  virtual void cancel();

 protected:
  virtual bool init();
  virtual void cleanup();

  virtual long simpleRead( char* data, long maxLen ) = 0;

 private:
  class Private;
  Private* d;
};

#endif

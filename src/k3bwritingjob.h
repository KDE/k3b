/* 
 *
 * $Id: $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef K3B_WRITING_JOB__H
#define K3B_WRITING_JOB__H

#include "k3bjob.h"


class K3bWritingJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bWritingJob( QObject* parent = 0 );
  virtual ~K3bWritingJob();

 public slots:
  virtual void start() = 0;
  virtual void cancel() = 0;
  virtual void writeData( char*, int ) {}

 signals:
  void dataWritten();
  void bufferStatus( int );
};


#endif

/* 
 *
 * $Id$
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


#ifndef _K3B_DVD_JOB_H_
#define _K3B_DVD_JOB_H_

#include <k3bdatajob.h>


class K3bDvdDoc;


class K3bDvdJob : public K3bDataJob
{
  Q_OBJECT

 public:
  K3bDvdJob( K3bDvdDoc*, QObject* parent = 0 );
  virtual ~K3bDvdJob();

  virtual QString jobDescription() const;
  virtual QString jobDetails() const;

 protected:
  virtual bool prepareWriterJob();

 protected slots:
  virtual void waitForDisk();

 private:
  K3bDvdDoc* m_doc;
};

#endif

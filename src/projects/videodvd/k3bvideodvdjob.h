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


#ifndef _K3B_VIDEO_DVD_JOB_H_
#define _K3B_VIDEO_DVD_JOB_H_

#include <k3bdvdjob.h>


class K3bVideoDvdDoc;

class K3bVideoDvdJob : public K3bDvdJob
{
  Q_OBJECT

 public:
  K3bVideoDvdJob( K3bVideoDvdDoc*, QObject* parent = 0 );
  virtual ~K3bVideoDvdJob();

  virtual QString jobDescription() const;
  virtual QString jobDetails() const;

 public slots:
  void start();

 private:
  K3bVideoDvdDoc* m_doc;
};

#endif

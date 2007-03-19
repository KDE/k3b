/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_VIDEODVD_DOC_H_
#define _K3B_VIDEODVD_DOC_H_

#include <k3bdvddoc.h>
#include "k3b_export.h"
class KConfig;

class LIBK3B_EXPORT K3bVideoDvdDoc : public K3bDvdDoc
{
 public:
  K3bVideoDvdDoc( QObject* parent = 0 );
  virtual ~K3bVideoDvdDoc();

  virtual int type() const { return VIDEODVD; }

  virtual K3bBurnJob* newBurnJob( K3bJobHandler* hdl, QObject* parent );

  virtual bool newDocument();

  K3bDirItem* videoTsDir() const { return m_videoTsDir; }

  // TODO: implement load- and saveDocumentData since we do not need all those options

 protected:
  virtual QString typeString() const { return "video_dvd"; }

 private:
  K3bDirItem* m_videoTsDir;
};

#endif

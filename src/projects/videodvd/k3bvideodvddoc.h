/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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

class KConfig;

class K3bVideoDvdDoc : public K3bDvdDoc
{
 public:
  K3bVideoDvdDoc( QObject* parent = 0 );
  virtual ~K3bVideoDvdDoc();

  virtual int docType() const { return VIDEODVD; }

  virtual K3bBurnJob* newBurnJob( K3bJobHandler* hdl, QObject* parent );

  virtual bool newDocument();

  K3bDirItem* videoTsDir() const { return m_videoTsDir; }

  // TODO: implement load- and saveDocumentData since we do not need all those options

 protected:
  virtual QString documentType() const { return "video_dvd"; }
  virtual void loadDefaultSettings( KConfig* );

  virtual K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0, const char* name = 0 );
  //  virtual K3bView* newView( QWidget* parent );

 private:
  K3bDirItem* m_videoTsDir;
};

#endif

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


#ifndef _K3B_MOVIX_JOB_H_
#define _K3B_MOVIX_JOB_H_

#include <k3bjob.h>

class K3bMovixDoc;
class K3bCdDevice::CdDevice;
class K3bDataJob;
class KTempFile;
class K3bMovixInstallation;
class K3bDirItem;
class K3bFileItem;

class K3bMovixJob : public K3bBurnJob
{
  Q_OBJECT

 public:
  K3bMovixJob( K3bMovixDoc* doc, QObject* parent = 0 );
  ~K3bMovixJob();

  K3bDoc* doc() const;
  K3bDevice* writer() const;

 public slots:
  void start();
  void cancel();

 private slots:
  void slotDataJobFinished( bool );

 private:
  bool writePlaylistFile();
  bool writeIsolinuxConfigFile();
  bool writeMovixRcFile();
  bool addMovixFiles();

  void cleanUp();

  K3bMovixDoc* m_doc;
  K3bDataJob* m_dataJob;
  K3bMovixInstallation* m_installation;

  bool m_canceled;

  KTempFile* m_playlistFile;
  KTempFile* m_isolinuxConfigFile;
  KTempFile* m_movixRcFile;

  K3bDirItem* m_isolinuxDir;
  K3bDirItem* m_movixDir;
  K3bDirItem* m_mplayerDir;
  K3bFileItem* m_playlistFileItem;
};

#endif

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

#ifndef K3B_MSINFO_FETCHER_H
#define K3B_MSINFO_FETCHER_H

#include <k3bjob.h>


class K3bCdDevice::CdDevice;
class KProcess;

class K3bMsInfoFetcher : public K3bJob
{
  Q_OBJECT

 public:
  K3bMsInfoFetcher( QObject* parent = 0, const char* name = 0 );
  ~K3bMsInfoFetcher();

  const QString& msInfo() const { return m_msInfo; }
  int lastSessionStart() const { return m_lastSessionStart; }
  int nextSessionStart() const { return m_nextSessionStart; }

 public slots:
  void start();
  void cancel();

  void setDevice( K3bCdDevice::CdDevice* dev ) { m_device = dev; }

 private slots:
  void slotProcessExited();
  void slotCollectOutput( KProcess*, char* output, int len );

 private:
  QString m_msInfo;
  int m_lastSessionStart;
  int m_nextSessionStart;
  QString m_collectedOutput;

  KProcess* m_process;
  K3bCdDevice::CdDevice* m_device;

  bool m_canceled;
};

#endif

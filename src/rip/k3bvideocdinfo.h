/*
 *
 * $Id: $
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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


#ifndef K3BVIDEOCDINFO_H
#define K3BVIDEOCDINFO_H

#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>

#include "device/k3btoc.h"
#include <k3bcore.h>

class KProcess;

class K3bVideoCdInfoResult
{
 public:
  K3bVideoCdInfoResult()
  {
  }

  QStringList sequence;
  QStringList sequenceId;
  
  QString volumeId;
  QString type;
  QString version;

  QString rawData;
};


class K3bVideoCdInfo : public QObject
{
  Q_OBJECT

 public:
  K3bVideoCdInfo( QObject* parent = 0, const char* name = 0 );
  ~K3bVideoCdInfo();

  /**
   * Do NOT call this before queryResult has
   * been emitted
   */
  const K3bVideoCdInfoResult& result() const;

  void info( const QString& );

 signals:
  void infoFinished( bool success );

 private slots:
  void slotInfoFinished();
  void slotParseOutput( KProcess*, char* output, int len );

 private:
  void cancelAll();
  
  K3bVideoCdInfoResult m_Result;
  void parseXmlData();

  KProcess* m_process;

  QString m_xmlData;
  bool m_isXml;

};

#endif

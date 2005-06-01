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

#ifndef _K3B_CLONE_JOB_H_
#define _K3B_CLONE_JOB_H_

#include <k3bjob.h>
#include "k3b_export.h"
#include <qstring.h>


namespace K3bDevice {
  class Device;
}
class K3bCdrecordWriter;
class K3bReadcdReader;


class LIBK3B_EXPORT K3bCloneJob : public K3bBurnJob
{
  Q_OBJECT

 public:
  K3bCloneJob( K3bJobHandler*, QObject* parent = 0, const char* name = 0 );
  ~K3bCloneJob();

  K3bDevice::Device* writer() const { return m_writerDevice; }
  K3bDevice::Device* readingDevice() const { return m_readerDevice; }

  QString jobDescription() const;
  QString jobDetails() const;

 public slots:
  void start();
  void cancel();

  void setWriterDevice( K3bDevice::Device* w ) { m_writerDevice = w; }
  void setReaderDevice( K3bDevice::Device* w ) { m_readerDevice = w; }
  void setImagePath( const QString& p ) { m_imagePath = p; }
  void setNoCorrection( bool b ) { m_noCorrection = b; }
  void setRemoveImageFiles( bool b ) { m_removeImageFiles = b; }
  void setOnlyCreateImage( bool b ) { m_onlyCreateImage = b; }
  void setOnlyBurnExistingImage( bool b ) { m_onlyBurnExistingImage = b; }
  void setSimulate( bool b ) { m_simulate = b; }
  void setWriteSpeed( int s ) { m_speed = s; }
  void setCopies( int c ) { m_copies = c; }
  void setReadRetries( int i ) { m_readRetries = i; }

 private slots:
  void slotWriterPercent( int );
  void slotWriterFinished( bool );
  void slotWriterNextTrack( int, int );
  void slotReadingPercent( int );
  void slotReadingFinished( bool );

 private:
  void removeImageFiles();
  void prepareReader();
  void prepareWriter();
  void startWriting();

  K3bDevice::Device* m_writerDevice;
  K3bDevice::Device* m_readerDevice;
  QString m_imagePath;

  K3bCdrecordWriter* m_writerJob;
  K3bReadcdReader* m_readcdReader;

  bool m_noCorrection;
  bool m_removeImageFiles;

  bool m_canceled;
  bool m_running;

  bool m_simulate;
  int m_speed;
  int m_copies;
  bool m_onlyCreateImage;
  bool m_onlyBurnExistingImage;
  int m_readRetries;

  class Private;
  Private* d;
};


#endif

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


#ifndef _K3BCDCOPYJOB_H_
#define _K3BCDCOPYJOB_H_

#include <k3bjob.h>

namespace K3bCdDevice {
  class CdDevice;
  class DeviceHandler;
}


/**
  *@author Sebastian Trueg
  */
class K3bCdCopyJob : public K3bBurnJob
{
  Q_OBJECT

 public:
  K3bCdCopyJob( QObject* parent = 0 );
  ~K3bCdCopyJob();

  K3bCdDevice::CdDevice* writer() const { return m_writerDevice; }
  K3bCdDevice::CdDevice* reader() const { return m_readerDevice; }
	
  QString jobDescription() const;
  QString jobDetails() const;

 public slots:
  void start();
  void cancel();

 public:
  void setWriterDevice( K3bCdDevice::CdDevice* dev ) { m_writerDevice = dev; }
  void setReaderDevice( K3bCdDevice::CdDevice* dev ) { m_readerDevice = dev; }
  void setWritingMode( int m ) { m_writingMode = m; }
  void setSpeed( int s ) { m_speed = s; }
  void setOnTheFly( bool b ) { m_onTheFly = b; }
  void setKeepImage( bool b ) { m_keepImage = b; }
  void setOnlyCreateImage( bool b ) { m_onlyCreateImages = b; }
  void setSimulate( bool b ) { m_simulate = b; }
  void setTempPath( const QString& path ) { m_tempPath= path; }
  void setCopies( unsigned int c ) { m_copies = c; }
  void setParanoiaMode( int i ) { m_paranoiaMode = i; }
  void setIgnoreReadErrors( bool b ) { m_ignoreReadErrors = b; }
  void setReadRetries( int i ) { m_readRetries = i; }
  void setQueryCddb( bool b ) { m_queryCddb =b; }
  void setPreferCdText( bool b ) { m_preferCdText = b; }
  void setBurnfree( bool b ) { m_burnfree = b; }

 private slots:
  void slotDiskInfoReady( K3bCdDevice::DeviceHandler* );
  void slotCdTextReady( K3bCdDevice::DeviceHandler* );
  void slotMediaReloadedForNextSession( K3bCdDevice::DeviceHandler* dh );
  void slotCddbQueryFinished(int);
  void slotWritingNextTrack( int t, int tt );
  void slotReadingNextTrack( int t, int tt );
  void slotSessionReaderFinished( bool success );
  void slotWriterFinished( bool success );
  void slotReaderProgress( int p );
  void slotReaderSubProgress( int p );
  void slotWriterProgress( int p );
  void slotReaderProcessedSize( int p, int pp );

 private:
  void startCopy();
  void queryCddb();
  bool writeNextSession();
  void readNextSession();
  bool prepareImageFiles();
  void cleanup();
  void finishJob( bool canceled, bool error );

  K3bCdDevice::CdDevice* m_writerDevice;
  K3bCdDevice::CdDevice* m_readerDevice;
  bool m_simulate;
  int m_speed;
  int m_paranoiaMode;
  unsigned int m_copies;
  bool m_keepImage;
  bool m_onlyCreateImages;
  bool m_onTheFly;
  bool m_ignoreReadErrors;
  int m_readRetries;
  bool m_queryCddb;
  bool m_preferCdText;
  QString m_tempPath;
  int m_writingMode;
  bool m_burnfree;

  class Private;
  Private* d;
};

#endif

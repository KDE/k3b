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


#ifndef K3BAUDIOJOB_H
#define K3BAUDIOJOB_H

#include <k3bjob.h>


class K3bAudioDoc;
class K3bAudioStreamer;
class QFile;
class QDataStream;
class K3bAbstractWriter;
class K3bWaveFileWriter;
class KTempFile;
class K3bCdrecordWriter;
class K3bAudioNormalizeJob;
class K3bAudioJobTempData;
class K3bCdDevice::CdDevice;

/**
  *@author Sebastian Trueg
  */
class K3bAudioJob : public K3bBurnJob
{
  Q_OBJECT
	
 public:
  K3bAudioJob( K3bAudioDoc*, QObject* parent = 0 );
  ~K3bAudioJob();
	
  K3bDoc* doc() const;
  K3bCdDevice::CdDevice* writer() const;

  QString jobDescription() const;
  QString jobDetails() const;
		
 public slots:
  void cancel();
  void start();

 protected slots:
  // writer slots
  void slotDataWritten();
  void slotWriterFinished( bool success );
  void slotWriterNextTrack(int, int);
  void slotWriterJobPercent(int);

  // audiodecoder slots
  void slotAudioDecoderFinished( bool );
  void slotAudioDecoderNextTrack( int, int );
  void slotAudioDecoderPercent(int);
  void slotAudioDecoderSubPercent( int );
  void slotReceivedAudioDecoderData( const char*, int );

  // normalizing slots
  void slotNormalizeJobFinished( bool );
  void slotNormalizeProgress( int );
  void slotNormalizeSubProgress( int );

 private:
  bool prepareWriter();
  bool startWriting();
  void cleanupAfterError();
  void removeBufferFiles();
  void normalizeFiles();

  K3bAudioDoc* m_doc;
  K3bAudioStreamer* m_audioStreamer;
  K3bWaveFileWriter* m_waveFileWriter;
  K3bAbstractWriter* m_writer;
  K3bAudioNormalizeJob* m_normalizeJob;
  K3bAudioJobTempData* m_tempData;

  KTempFile* m_tocFile;

  bool m_canceled;
  bool m_errorOccuredAndAlreadyReported;

  bool m_written;

  int m_usedWritingApp;
  int m_usedWritingMode;
};

#endif

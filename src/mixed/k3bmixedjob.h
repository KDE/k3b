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


#ifndef K3BMIXEDJOB_H
#define K3BMIXEDJOB_H

#include <k3bjob.h>


class K3bMixedDoc;
class K3bIsoImager;
class K3bAudioDecoder;
class QFile;
class QDataStream;
class K3bAbstractWriter;
class K3bWaveFileWriter;
class KTempFile;
class K3bCdrecordWriter;
class K3bMsInfoFetcher;

/**
  *@author Sebastian Trueg
  */
class K3bMixedJob : public K3bBurnJob
{
  Q_OBJECT
	
 public:
  K3bMixedJob( K3bMixedDoc*, QObject* parent = 0 );
  ~K3bMixedJob();
	
  K3bDoc* doc() const;
  K3bDevice* writer() const;
		
 public slots:
  void cancel();
  void start();

 protected slots:
  void slotSizeCalculationFinished( int, int );
  void slotReceivedIsoImagerData( char*, int );
  void slotIsoImagerFinished( bool success );
  void slotAudioDecoderFinished( bool );
  void slotReceivedAudioDecoderData( const char*, int );
  void slotAudioDecoderNextTrack( int, int );
  void slotDataWritten();
  void slotWriterFinished( bool success );
  void slotWriterNextTrack(int, int);
  void slotWriterJobPercent(int);
  void slotAudioDecoderPercent(int);
  void slotAudioDecoderSubPercent( int );
  void slotIsoImagerPercent(int);
  void slotMsInfoFetched(bool);

 private:
  bool prepareWriter();
  bool writeTocFile();
  bool startWriting();
  void addAudioTracks( K3bCdrecordWriter* writer );
  void addDataTrack( K3bCdrecordWriter* writer );
  void cleanupAfterError();
  void removeBufferFiles();
  void createIsoImage();

  K3bMixedDoc* m_doc;
  K3bIsoImager* m_isoImager;
  K3bAudioDecoder* m_audioDecoder;
  K3bWaveFileWriter* m_waveFileWriter;
  K3bAbstractWriter* m_writer;
  K3bMsInfoFetcher* m_msInfoFetcher;

  QFile* m_isoImageFile;
  QDataStream* m_isoImageFileStream;
  KTempFile* m_tocFile;

  enum Action { CREATING_ISO_IMAGE,
		CREATING_AUDIO_IMAGE,
		WRITING_ISO_IMAGE,
		WRITING_AUDIO_IMAGE,
		FETCHING_MSINFO };

  int m_currentAction;
  double m_audioDocPartOfProcess;

  bool m_canceled;
  bool m_errorOccuredAndAlreadyReported;

  int m_fifo;
  bool m_usingFifo;

  int m_usedWritingApp;
};

#endif

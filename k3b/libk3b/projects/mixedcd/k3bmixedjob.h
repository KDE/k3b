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


#ifndef K3BMIXEDJOB_H
#define K3BMIXEDJOB_H

#include <k3bjob.h>


class K3bMixedDoc;
class K3bIsoImager;
class K3bAudioImager;
class QFile;
class QDataStream;
class K3bAbstractWriter;
class K3bWaveFileWriter;
class KTempFile;
class K3bCdrecordWriter;
class K3bMsInfoFetcher;
class K3bAudioNormalizeJob;
class K3bAudioJobTempData;
class K3bDevice::Device;

/**
  *@author Sebastian Trueg
  */
class K3bMixedJob : public K3bBurnJob
{
  Q_OBJECT
	
 public:
  K3bMixedJob( K3bMixedDoc*, K3bJobHandler*, QObject* parent = 0 );
  ~K3bMixedJob();
	
  K3bDoc* doc() const;
  K3bDevice::Device* writer() const;
		
  QString jobDescription() const;
  QString jobDetails() const;
		
 public slots:
  void cancel();
  void start();

 protected slots:
  // iso imager slots
  void slotSizeCalculationFinished( int, int );
  void slotIsoImagerFinished( bool success );
  void slotIsoImagerPercent(int);

  // ms info fetcher slots
  void slotMsInfoFetched(bool);

  // audio decoder slots
  void slotAudioDecoderFinished( bool );
  void slotAudioDecoderNextTrack( int, int );
  void slotAudioDecoderPercent(int);
  void slotAudioDecoderSubPercent( int );

  // writer slots
  void slotWriterFinished( bool success );
  void slotWriterNextTrack(int, int);
  void slotWriterJobPercent(int);

  // normalizing slots
  void slotNormalizeJobFinished( bool );
  void slotNormalizeProgress( int );
  void slotNormalizeSubProgress( int );

  // misc slots
  void slotMediaReloadedForSecondSession( bool );
  void slotMaxSpeedJobFinished( bool );

 private:
  bool prepareWriter();
  bool writeTocFile();
  bool writeInfFiles();
  bool startWriting();
  void startFirstCopy();
  void addAudioTracks( K3bCdrecordWriter* writer );
  void addDataTrack( K3bCdrecordWriter* writer );
  void cleanupAfterError();
  void removeBufferFiles();
  void createIsoImage();
  void determineWritingMode();
  void normalizeFiles();
  void prepareProgressInformation();
  void writeNextCopy();
  void determinePreliminaryDataImageSize();

  K3bMixedDoc* m_doc;
  K3bIsoImager* m_isoImager;
  K3bAudioImager* m_audioImager;
  K3bAudioJobTempData* m_tempData;
  K3bWaveFileWriter* m_waveFileWriter;
  K3bAbstractWriter* m_writer;
  K3bMsInfoFetcher* m_msInfoFetcher;
  K3bAudioNormalizeJob* m_normalizeJob;

  QString m_isoImageFilePath;

  KTempFile* m_tocFile;

  enum Action { PREPARING_DATA,
		CREATING_ISO_IMAGE,
		CREATING_AUDIO_IMAGE,
		WRITING_ISO_IMAGE,
		WRITING_AUDIO_IMAGE,
		FETCHING_MSINFO };

  int m_currentAction;
  double m_audioDocPartOfProcess;

  bool m_canceled;
  bool m_errorOccuredAndAlreadyReported;

  int m_usedDataMode;
  int m_usedDataWritingApp;
  int m_usedAudioWritingApp;
  int m_usedDataWritingMode;
  int m_usedAudioWritingMode;

  QString m_tempFilePrefix;

  K3b::Msf m_projectSize;

  class Private;
  Private* d;
};

#endif

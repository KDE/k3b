/***************************************************************************
                          k3baudiojob.h  -  description
                             -------------------
    begin                : Fri May 4 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BAUDIOJOB_H
#define K3BAUDIOJOB_H

#include "../k3bjob.h"


class K3bAudioDoc;
class K3bAudioDecoder;
class QFile;
class QDataStream;
class K3bAbstractWriter;
class K3bWaveFileWriter;
class KTempFile;
class K3bCdrecordWriter;

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
  K3bDevice* writer() const;
		
 public slots:
  void cancel();
  void start();

 protected slots:
  void slotAudioDecoderFinished( bool );
  void slotReceivedAudioDecoderData( const char*, int );
  void slotAudioDecoderNextTrack( int, int );
  void slotDataWritten();
  void slotWriterFinished( bool success );
  void slotWriterNextTrack(int, int);
  void slotWriterJobPercent(int);
  void slotAudioDecoderPercent(int);
  void slotAudioDecoderSubPercent( int );

 private:
  bool prepareWriter();
  bool writeTocFile();
  void startWriting();
  void cleanupAfterError();
  void removeBufferFiles();

  K3bAudioDoc* m_doc;
  K3bAudioDecoder* m_audioDecoder;
  K3bWaveFileWriter* m_waveFileWriter;
  K3bAbstractWriter* m_writer;

  KTempFile* m_tocFile;

  bool m_canceled;
  bool m_errorOccuredAndAlreadyReported;

  bool m_written;
};

#endif

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

#include "../tools/k3bwavefilewriter.h"

class K3bAudioDoc;
class K3bAudioTrack;
class QString;
class KProcess;
class QDataStream;

#include <qptrlist.h>
#include <kurl.h>


/**
  *@author Sebastian Trueg
  */

class K3bAudioJob : public K3bBurnJob  {

  Q_OBJECT

 public:
  K3bAudioJob( K3bAudioDoc* doc );
  ~K3bAudioJob();

  K3bDoc* doc() const;
  K3bDevice* writer() const;
	
 public slots:
  void start();
  void cancel();
	
 protected slots:
  void slotParseCdrecordOutput( KProcess*, char*, int );
  void slotCdrecordFinished();
  void slotCdrdaoFinished();
  void slotModuleProgress( int percent );
  void slotModuleFinished( bool success );
  void slotModuleOutput( const unsigned char* data, int len );
  void slotTryStart();
  void slotDecodeNextFile();

  void slotProcessWroteStdin();

  /** reimplemented from K3bBurnJob */
  void createCdrdaoProgress( int made, int size );

  /** reimplemented from K3bBurnJob */
  void startNewCdrdaoTrack();	

 private:
  void clearBufferFiles();
  void startWriting();
  void cancelAll();
  void cdrdaoWrite();
  void cdrecordWrite();

  K3bWaveFileWriter m_waveFileWriter;
	
  KProcess* m_process;
  K3bAudioDoc* m_doc;
  K3bAudioTrack* m_currentDecodedTrack;
  K3bAudioTrack* m_currentWrittenTrack;

  bool firstTrack;
  QString m_tocFile;

  int m_currentDecodedTrackNumber;
  int m_currentWrittenTrackNumber;

  unsigned long m_bytesFinishedTracks;
  unsigned long m_dataToDecode;
  unsigned long m_decodedData;
  unsigned long m_currentModuleDataLength;
  int m_decodingPercentage;

  bool m_working;
  /** we save this here to avoid problems when the settings
   * might be changed in the doc */
  bool m_onTheFly;
  bool m_bLengthInfoEmited;

  int m_writingApp;

  bool m_processWroteStdin;

 signals:
  void writingLeadOut();
};

#endif

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

#ifndef K3BAUDIO_ONTHEFLY_JOB_H
#define K3BAUDIO_ONTHEFLY_JOB_H

#include "../k3bjob.h"

class K3bAudioDoc;
class K3bAudioTrack;
class QString;
class QTimer;


#include <kprocess.h>
#include <qfile.h>

/**
  *@author Sebastian Trueg
  */

class K3bAudioOnTheFlyJob : public K3bBurnJob  {

  Q_OBJECT

 public:
  K3bAudioOnTheFlyJob( K3bAudioDoc* doc );
  ~K3bAudioOnTheFlyJob();

  K3bDoc* doc() const;
	
 public slots:
  void start();
  void cancel();
	
 protected slots:
  void slotParseCdrdaoOutput( KProcess*, char* output, int len );
  void slotCdrdaoFinished();

  void slotWroteData();
  void slotModuleOutput( int );
  void slotModuleFinished( bool );
  void slotTryWritingToProcess();

 private:
  QTimer* m_streamingTimer;
  KProcess m_process;
  K3bAudioDoc* m_doc;

  K3bAudioTrack* m_currentProcessedTrack;
  int m_currentProcessedTrackNumber;

  bool firstTrack;
  QString m_tocFile;
  int m_iNumTracksAlreadyWritten;
  int m_iTracksAlreadyWrittenSize;
  int m_iDocSize;

  char* m_currentWrittenData;
  long m_currentModuleDataLength;
  bool m_streamingStarted;

  // buffer --------------------
/*   char* m_ringBuffer; */
/*   long m_bufferReader; */
/*   long m_bufferWriter; */
/*   long m_bufferSize; */
  // ---------------------------


  // testing
  //  QFile* m_testFile;
		
 signals:
  void writingLeadOut();
};

#endif

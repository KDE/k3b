/***************************************************************************
                             k3b -  description
                             -------------------
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
                          k3bvcdjob.h  -  description
                             -------------------
    begin                : Mon Nov 4 2002
    copyright            : (C) 2002 by Christian Kvasny
    email                : chris@ckvsoft.at
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BVCDJOB_H
#define K3BVCDJOB_H

#include "../k3bjob.h"

class K3bVcdDoc;
class K3bVcdTrack;
class QString;
class KProcess;
class QDataStream;
class K3bAbstractWriter;

class K3bVcdJob : public K3bBurnJob
{
  Q_OBJECT

 public:
  K3bVcdJob( K3bVcdDoc* );
  ~K3bVcdJob();

  K3bDoc* doc() const;
  K3bVcdDoc* vcdDoc() const {return m_doc;};
  K3bDevice* writer() const;

 public slots:
  void start();
  void cancel();

 private slots:
  void cancelAll();

 protected slots:
  void slotCollectOutput( KProcess*, char*, int );
  void slotVcdxGenFinished();
  void slotVcdxBuildFinished();
  void slotParseVcdxBuildOutput( KProcess*, char* output, int len );

  void slotWriterJobPercent( int p );
  void slotWriterNextTrack( int t, int tt);
  void slotWriterJobFinished( bool success );

       
 private:
  bool prepareWriterJob();
  
  void vcdxGen();
  void vcdxBuild();
  
  int m_copies;
  int m_finishedCopies;

  unsigned long m_blocksToCopy;
  unsigned long m_bytesFinishedTracks;
  unsigned long m_bytesFinished;
    
  enum {
    stageUnknown,
    stageScan,
    stageWrite,
    _stage_max
  };

  K3bVcdDoc* m_doc;
  K3bDevice* m_writer;
  K3bDevice* m_reader;
  K3bVcdTrack* m_currentWrittenTrack;
    
  int m_speed;
  int m_stage;
  int m_currentWrittenTrackNumber;

  double m_createimageonlypercent;
    
  bool firstTrack;
  bool m_burnProof;
  bool m_keepImage;
  bool m_onlyCreateImage;
  bool m_onTheFly;
  bool m_dummy;
  bool m_fastToc;
  bool m_readRaw;
  bool m_imageFinished;
  bool m_canceled;
  
  QString m_tempPath;
  QString m_cueFile;
  QString m_xmlFile;
  QString m_collectedOutput;

  K3bAbstractWriter* m_writerJob;        
  KProcess* m_process;

};

#endif

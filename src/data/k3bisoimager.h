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

#ifndef K3B_ISO_IMAGER_H
#define K3B_ISO_IMAGER_H

#include "../k3bjob.h"

#include <qptrqueue.h>

class K3bDataDoc;
class K3bDirItem;
class QTextStream;
class K3bProcess;
class K3bDevice;
class KTempFile;


class K3bIsoImager : public K3bJob
{
 Q_OBJECT

 public:
  K3bIsoImager( K3bDataDoc*, QObject* parent = 0, const char* name = 0 );
  ~K3bIsoImager();

  int size() const { return m_mkisofsPrintSizeResult; }

 public slots:
  void start();
  void cancel();
  void calculateSize();

  /**
   * If dev == 0 K3bIsoImager will ignore the data in the previous session. 
   * This is usable for CD-Extra.
   */
  void setMultiSessionInfo( const QString&, K3bDevice* dev = 0 );

  /**
   * after data has been emitted image creation will
   * be suspended until resume() is called
   */
  void resume();

 signals:
  void sizeCalculated( int exitCode, int size );

  /**
   * after data has been emitted image creation will
   * be suspended until resume() is called
   */
  void data( char* data, int len );

 private slots:
  void slotReceivedStdout( KProcess*, char*, int );
  void slotReceivedStderr( const QString& );
  void slotProcessExited( KProcess* );
  void slotCollectMkisofsPrintSizeStderr(KProcess*, char*, int);
  void slotCollectMkisofsPrintSizeStdout(KProcess*, char*, int);
  void slotMkisofsPrintSizeFinished();

 private:
  K3bDataDoc* m_doc;

  KTempFile* m_pathSpecFile;
  KTempFile* m_rrHideFile;
  KTempFile* m_jolietHideFile;

  bool m_noDeepDirectoryRelocation;

  bool m_importSession;
  QString m_multiSessionInfo;
  K3bDevice* m_device;

  K3bProcess* m_process;
  bool m_processSuspended;
  bool m_processExited;
  QPtrQueue<QByteArray> m_data;
  QByteArray* m_lastOutput;

  // used for mkisofs -print-size parsing
  QString m_collectedMkisofsPrintSizeStdout;
  QString m_collectedMkisofsPrintSizeStderr;
  int m_mkisofsPrintSizeResult;

  bool writePathSpec();
  bool writeRRHideFile();
  bool writeJolietHideFile();
  void writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream );
  QString escapeGraftPoint( const QString& str );
  bool addMkisofsParameters();
  bool prepareMkisofsFiles();
  void outputData();

  void cleanup();

  bool m_canceled;
};


#endif

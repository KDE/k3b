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

#include <k3bjob.h>

#include <qptrqueue.h>
#include <qstringlist.h>

class K3bDataDoc;
class K3bDirItem;
class K3bDataItem;
class QTextStream;
class K3bProcess;
class KProcess;
class K3bCdDevice::CdDevice;
class KTempFile;


class K3bIsoImager : public K3bJob
{
 Q_OBJECT

 public:
  K3bIsoImager( K3bDataDoc*, QObject* parent = 0, const char* name = 0 );
  virtual ~K3bIsoImager();

  int size() const { return m_mkisofsPrintSizeResult; }

 public slots:
  virtual void start();
  virtual void cancel();
  void calculateSize();

  /**
   * lets the isoimager write directly into fd instead of emitting
   * data() signals.
   * Be aware that this only makes sense before starting the job.
   * To disable just set @p fd to -1
   */
  void writeToFd( int fd );

  /**
   * If dev == 0 K3bIsoImager will ignore the data in the previous session. 
   * This is usable for CD-Extra.
   */
  void setMultiSessionInfo( const QString&, K3bCdDevice::CdDevice* dev = 0 );

  /**
   * after data has been emitted image creation will
   * be suspended until resume() is called
   */
  virtual void resume();

  K3bCdDevice::CdDevice* device() const { return m_device; }
  K3bDataDoc* doc() const { return m_doc; }

 signals:
  void sizeCalculated( int exitCode, int size );

  /**
   * after data has been emitted image creation will
   * be suspended until resume() is called
   */
  //  void data( char* data, int len );

 protected:
  /**
   * This method just creates some user information about the filenames
   * that need to be cut in order to fulfill the 64 char restriction of
   * the joliet extensions.
   */
  void informAboutCutJolietNames();

  bool addMkisofsParameters();

  /**
   * calls writePathSpec, writeRRHideFile, and writeJolietHideFile
   */
  bool prepareMkisofsFiles();

  void outputData();
  void init();
  virtual void cleanup();
  bool writePathSpec();
  bool writeRRHideFile();
  bool writeJolietHideFile();
  bool writeSortWeightFile();
  bool writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream );
  QString escapeGraftPoint( const QString& str );

  void parseProgress( const QString& );

  KTempFile* m_pathSpecFile;
  KTempFile* m_rrHideFile;
  KTempFile* m_jolietHideFile;
  KTempFile* m_sortWeightFile;

  K3bProcess* m_process;

  bool m_processSuspended;
  bool m_processExited;
  bool m_canceled;

 protected slots:
  void slotReceivedStdout( KProcess*, char*, int );
  virtual void slotReceivedStderr( const QString& );
  virtual void slotProcessExited( KProcess* );

 private slots:
  void slotCollectMkisofsPrintSizeStderr(KProcess*, char*, int);
  void slotCollectMkisofsPrintSizeStdout(KProcess*, char*, int);
  void slotMkisofsPrintSizeFinished();

 private:
  K3bDataDoc* m_doc;

  bool m_noDeepDirectoryRelocation;

  bool m_importSession;
  QString m_multiSessionInfo;
  K3bCdDevice::CdDevice* m_device;

  QPtrQueue<QByteArray> m_data;
  QByteArray* m_lastOutput;

  // used for mkisofs -print-size parsing
  QString m_collectedMkisofsPrintSizeStdout;
  QString m_collectedMkisofsPrintSizeStderr;
  int m_mkisofsPrintSizeResult;

  QStringList m_tempFiles;

  int m_fdToWriteTo;

  bool m_containsFilesWithMultibleBackslashes;

  double m_firstProgressValue;
};


#endif

/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include "k3bmkisofshandler.h"

#include <qptrqueue.h>
#include <qstringlist.h>

class K3bDataDoc;
class K3bDirItem;
class K3bDataItem;
class K3bFileItem;
class QTextStream;
class K3bProcess;
class KProcess;
class K3bDevice::Device;
class KTempFile;


class K3bIsoImager : public K3bJob, public K3bMkisofsHandler
{
 Q_OBJECT

 public:
  K3bIsoImager( K3bDataDoc*, K3bJobHandler*, QObject* parent = 0, const char* name = 0 );
  virtual ~K3bIsoImager();

  virtual bool active() const;

  int size() const { return m_mkisofsPrintSizeResult; }

  virtual bool hasBeenCanceled() const; 

  /**
   * Get the checksum calculated during the creation of the image.
   */
  QCString checksum() const;

 public slots:
  /**
   * Starts the actual image creation. Always run init() 
   * before starting the image creation
   */
  virtual void start();
  virtual void cancel();

  /**
   * Initialize the image creator. This calculates the image size and performs
   * some checks on the project.
   *
   * The initialization process also finishes with the finished() signal just
   * like a normal job operation. Get the calculated image size via size()
   */
  virtual void init();

  /**
   * Only calculates the size of the image without the additional checks in
   * init()
   *
   * Use this if you need to recalculate the image size for example if the
   * multisession info changed.
   */
  virtual void calculateSize();

  /**
   * lets the isoimager write directly into fd instead of writing
   * to an image file.
   * Be aware that this only makes sense before starting the job.
   * To disable just set @p fd to -1
   */
  void writeToFd( int fd );

  void writeToImageFile( const QString& path );

  /**
   * If dev == 0 K3bIsoImager will ignore the data in the previous session. 
   * This is usable for CD-Extra.
   */
  void setMultiSessionInfo( const QString&, K3bDevice::Device* dev = 0 );

  QString multiSessionInfo() const;
  K3bDevice::Device* multiSessionImportDevice() const;

  K3bDevice::Device* device() const { return m_device; }
  K3bDataDoc* doc() const { return m_doc; }

 protected:
  virtual void handleMkisofsProgress( int );
  virtual void handleMkisofsInfoMessage( const QString&, int );

  virtual bool addMkisofsParameters( bool printSize = false );

  /**
   * calls writePathSpec, writeRRHideFile, and writeJolietHideFile
   */
  bool prepareMkisofsFiles();

  /**
   * The dummy dir is used to create dirs on the iso-filesystem.
   *
   * @return an empty dummy dir for use with K3bDirItems.
   */
  QString dummyDir( K3bDirItem* );

  void outputData();
  void initVariables();
  virtual void cleanup();
  void clearDummyDirs();

  /**
   * @returns The number of entries written or -1 on error
   */
  virtual int writePathSpec();
  bool writeRRHideFile();
  bool writeJolietHideFile();
  bool writeSortWeightFile();

  // used by writePathSpec
  virtual int writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream );
  virtual void writePathSpecForFile( K3bFileItem*, QTextStream& stream );
  QString escapeGraftPoint( const QString& str );

  KTempFile* m_pathSpecFile;
  KTempFile* m_rrHideFile;
  KTempFile* m_jolietHideFile;
  KTempFile* m_sortWeightFile;

  K3bProcess* m_process;

  bool m_processExited;
  bool m_canceled;

 protected slots:
  virtual void slotReceivedStderr( const QString& );
  virtual void slotProcessExited( KProcess* );

 private slots:
  void slotCollectMkisofsPrintSizeStderr(KProcess*, char*, int);
  void slotCollectMkisofsPrintSizeStdout( const QString& );
  void slotMkisofsPrintSizeFinished();
  void slotDataPreparationDone( bool success );

 private:
  void startSizeCalculation();

  class Private;
  Private* d;

  K3bDataDoc* m_doc;

  bool m_noDeepDirectoryRelocation;

  bool m_importSession;
  QString m_multiSessionInfo;
  K3bDevice::Device* m_device;

  // used for mkisofs -print-size parsing
  QString m_collectedMkisofsPrintSizeStdout;
  QString m_collectedMkisofsPrintSizeStderr;
  int m_mkisofsPrintSizeResult;

  QStringList m_tempFiles;

  int m_fdToWriteTo;

  bool m_containsFilesWithMultibleBackslashes;

  // used to create a unique session id
  static int s_imagerSessionCounter;

  int m_sessionNumber;
};


#endif

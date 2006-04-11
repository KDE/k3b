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


#ifndef _K3B_DVD_JOB_H_
#define _K3B_DVD_JOB_H_

#include <k3bjob.h>

#include <qfile.h>

class K3bDataDoc;
class K3bGrowisofsImager;
class K3bGrowisofsWriter;
class K3bIsoImager;
namespace K3bDevice {
  class DeviceHandler;
}


/**
 * Some of this classes methods are made virtual since the K3bVideoDvdJob
 * is derived from this one. This is no clean API at all!
 */
class K3bDvdJob : public K3bBurnJob
{
  Q_OBJECT

 public:
  /**
   * To be more flexible we allow writing of any data doc
   */
  K3bDvdJob( K3bDataDoc*, K3bJobHandler*, QObject* parent = 0 );
  virtual ~K3bDvdJob();

  K3bDoc* doc() const;
  K3bDevice::Device* writer() const;

  virtual bool hasBeenCanceled() const;

  virtual QString jobDescription() const;
  virtual QString jobDetails() const;

 public slots:
  virtual void start();
  virtual void cancel();

 protected:
  virtual bool prepareWriterJob();
  virtual void prepareIsoImager();
  void prepareGrowisofsImager();
  void cleanup();
  void writeImage();
  void determineMultiSessionMode();

  bool waitForDvd();
  bool startWriting();

  int requestMedia( int state );

  K3bIsoImager* m_isoImager;
  K3bGrowisofsImager* m_growisofsImager;
  K3bGrowisofsWriter* m_writerJob;

  bool m_canceled;
  bool m_writingStarted;

 protected slots:
  void slotSizeCalculationFinished(int, int);
  void slotIsoImagerFinished( bool success );
  void slotIsoImagerPercent(int);
  void slotGrowisofsImagerPercent(int);

  void slotWriterJobPercent( int );
  void slotWritingFinished( bool );

  void slotVerificationProgress( int p );
  void slotVerificationFinished( bool success );

  void slotDetermineMultiSessionMode( K3bDevice::DeviceHandler* dh );

 private:
  K3bDataDoc* m_doc;

  class Private;
  Private* d;
};

#endif

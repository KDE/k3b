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


#ifndef K3BISO9660_IMAGE_WRITING_JOB_H
#define K3BISO9660_IMAGE_WRITING_JOB_H

#include <k3bjob.h>

class QString;
class K3bAbstractWriter;
class KTempFile;
namespace K3bCdDevice {
  class CdDevice;
}
class K3bIsoImageVerificationJob;


/**
  *@author Sebastian Trueg
  */
class K3bIso9660ImageWritingJob : public K3bBurnJob
{
  Q_OBJECT
	
 public:
  K3bIso9660ImageWritingJob();
  ~K3bIso9660ImageWritingJob();

  K3bCdDevice::CdDevice* writer() const { return m_device; };
	
  QString jobDescription() const;
  QString jobDetails() const;
		
 public slots:
  void cancel();
  void start();

  void setImagePath( const QString& path ) { m_imagePath = path; }
  void setSpeed( int s ) { m_speed = s; }
  void setBurnDevice( K3bCdDevice::CdDevice* dev ) { m_device = dev; }
  void setBurnproof( bool b ) { m_burnproof = b; }
  void setWritingMode( int mode ) { m_writingMode = mode; }
  void setSimulate( bool b ) { m_simulate = b; }
  void setNoFix( bool b ) { m_noFix = b; }
  void setDataMode( int m ) { m_dataMode = m; }
  void setVerifyData( bool b ) { m_verifyData = b; }

 protected slots:
  void slotWriterJobFinished( bool );
  void slotVerificationFinished( bool );
  void slotVerificationProgress( int );
  void slotWriterPercent( int );
  void slotNextTrack( int, int );
	
 private:
  bool prepareWriter( int mediaType );

  int m_writingMode;
  bool m_simulate;
  bool m_burnproof;
  K3bCdDevice::CdDevice* m_device;
  bool m_noFix;
  int m_speed;
  int m_dataMode;
  bool m_verifyData;

  QString m_imagePath;

  K3bAbstractWriter* m_writer;
  KTempFile* m_tocFile;

  bool m_canceled;
  bool m_finished;

  K3bIsoImageVerificationJob* m_verifyJob;
};

#endif

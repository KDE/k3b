/***************************************************************************
                          k3bdatajob.h  -  description
                             -------------------
    begin                : Tue May 15 2001
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

#ifndef K3BISO9660_IMAGE_WRITING_JOB_H
#define K3BISO9660_IMAGE_WRITING_JOB_H

#include "../k3bjob.h"

class QString;
class K3bDevice;
class K3bAbstractWriter;
class KTempFile;


/**
  *@author Sebastian Trueg
  */

class K3bIso9660ImageWritingJob : public K3bBurnJob
{
  Q_OBJECT
	
 public:
  K3bIso9660ImageWritingJob();
  ~K3bIso9660ImageWritingJob();

  K3bDevice* writer() const { return m_device; };
	
 public slots:
  void cancel();
  void start();

  void setImagePath( const QString& path ) { m_imagePath = path; }
  void setSpeed( int s ) { m_speed = s; }
  void setBurnDevice( K3bDevice* dev ) { m_device = dev; }
  void setBurnproof( bool b ) { m_burnproof = b; }
  void setDao( bool b ) { m_dao = b; }
  void setSimulate( bool b ) { m_simulate = b; }
  void setNoFix( bool b ) { m_noFix = b; }

 protected slots:
  void slotWriterJobFinished( bool );
		
 private:
  bool prepareWriter();

  bool m_dao;
  bool m_simulate;
  bool m_burnproof;
  K3bDevice* m_device;
  bool m_noFix;
  int m_speed;

  QString m_imagePath;

  K3bAbstractWriter* m_writer;
  KTempFile* m_tocFile;
};

#endif

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

#ifndef K3BISOIMAGE_H
#define K3BISOIMAGE_H

#include "../k3bjob.h"

class KProcess;
class QString;
class K3bDevice;


/**
  *@author Sebastian Trueg
  */

class K3bIsoImageJob : public K3bBurnJob
{
  Q_OBJECT
	
 public:
  K3bIsoImageJob();
  ~K3bIsoImageJob();

  K3bDevice* writer() const;
	
 public slots:
  void cancel();
  void start();

  void setImagePath( const QString& );
  void setSpeed( int );
  void setWriter( K3bDevice* );
  void setBurnproof( bool );
  void setDao( bool );
  void setDummy( bool );

 protected slots:
  void slotParseCdrecordOutput( KProcess*, char*, int );
  void slotCdrecordFinished();
  void slotStartWriting();
		
 private:
  KProcess* m_process;

  int m_speed;
  bool m_burnproof;
  bool m_dao;
  bool m_dummy;
  K3bDevice* m_device;
  QString m_imagePath;
};

#endif

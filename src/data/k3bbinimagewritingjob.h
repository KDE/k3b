/***************************************************************************
                          k3bbinimagewritingjob.h  -  description
                             -------------------
    begin                : Mon Jan 13 2003
    copyright            : (C) 2003 by Klaus-Dieter Krannich
    email                : kd@math.tu-cottbus.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BBINIMAGEWRITINGJOB_H
#define K3BBINIMAGEWRITINGJOB_H

#include "../k3bjob.h"
#include "../k3bcdrdaowriter.h"

class K3bProcess;
class K3bDevice;
class K3bDiskInfo;
class QUrlOperator;


/**
  *@author Klaus-Dieter Krannich
  */
class K3bBinImageWritingJob : public K3bBurnJob
{
  Q_OBJECT

 public: 
  K3bBinImageWritingJob( QObject* parent = 0 );
  ~K3bBinImageWritingJob();
  K3bDevice* writer() const { return m_cdrdaowriter->burnDevice(); };

 public slots:
  void start();
  void cancel();

  void setWriter( K3bDevice* dev ) { m_cdrdaowriter->setBurnDevice(dev); }
  void setSpeed( int s ) { m_cdrdaowriter->setBurnSpeed(s); }
  void setSimulate( bool b ) { m_cdrdaowriter->setSimulate(b); }
  void setForce(bool b) { m_cdrdaowriter->setForce(b); };
  void setMulti(bool b) { m_cdrdaowriter->setMulti(b); };
  void setTocFile(const QString& s) { m_cdrdaowriter->setTocFile(s); }
  void setCopies(int c) { m_copies=c; };
 private slots:
  void cdrdaoWrite();  

  void cdrdaoFinished(bool);
  void finishAll();
  void cancelAll();

  void copyPercent(int p);
  void copySubPercent(int p);
  void slotNextTrack( int, int );

 private:
  int m_copies;
  int m_finishedCopies;

  K3bCdrdaoWriter *m_cdrdaowriter;
};

#endif

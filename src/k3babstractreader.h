/***************************************************************************
                          k3babstractreader.h  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
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

#ifndef K3B_ABSTRACT_READER_H
#define K3B_ABSTRACT_READER_H


#include "k3bjob.h"

class K3bDevice;


class K3bAbstractReader : public K3bJob
{
  Q_OBJECT

 public:
  virtual ~K3bAbstractReader();

  K3bDevice* readDevice() const { return m_readDevice; }
  int readSpeed() const { return m_readSpeed; }
  bool readraw() const { return m_readraw; }

 public slots:
  void setReadDevice( K3bDevice* dev ) { m_readDevice = dev; }
  void setReadSpeed( int s ) { m_readSpeed = s; }
  void setReadRaw( bool b ) { m_readraw = b; }

 signals:
  void buffer( int );
  void nextTrack( int, int );

 protected:
  K3bAbstractReader( QObject* parent = 0, const char* name = 0 );

 private:
  K3bDevice* m_readDevice;
  int m_readSpeed;
  bool m_readraw;
};


#endif

/***************************************************************************
                              k3bwritingjob.h
                                   -
         Abstract class that is the basis for all Writer classes like
         K3bCcdrecordJob and K3bCdrdaoJob
                             -------------------
    begin                : Wed Sep  4 12:56:10 CEST 2002
    copyright            : (C) 2002 by Sebastian Trueg
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


#ifndef K3B_WRITING_JOB__H
#define K3B_WRITING_JOB__H

#include "k3bjob.h"


class K3bWritingJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bWritingJob( QObject* parent = 0 );
  virtual ~K3bWritingJob();

 public slots:
  virtual void start() = 0;
  virtual void cancel() = 0;
  virtual void writeData( char*, int ) {}

 signals:
  void dataWritten();
  void bufferStatus( int );
};


#endif

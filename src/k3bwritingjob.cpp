/***************************************************************************
                              k3bwritingjob.cpp
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


#include "k3bwritingjob.h"


K3bWritingJob::K3bWritingJob( QObject* parent )
  : K3bJob( parent )
{
}

K3bWritingJob::~K3bWritingJob()
{
}


#include "k3bwritingjob.moc"

/***************************************************************************
                          k3bjob.cpp  -  description
                             -------------------
    begin                : Thu May 3 2001
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

#include "k3bjob.h"
#include <k3bglobals.h>

#include <klocale.h>
#include <kprocess.h>

#include <qstringlist.h>
#include <kdebug.h>


K3bJob::K3bJob( QObject* parent, const char* name )
  : QObject( parent, name )
{
}

K3bJob::~K3bJob()
{
}


K3bBurnJob::K3bBurnJob( QObject* parent )
  : K3bJob( parent ),
    m_writeMethod( K3b::DEFAULT )
{
}


int K3bBurnJob::supportedWritingApps() const
{
  return K3b::DEFAULT | K3b::CDRDAO | K3b::CDRECORD;
}

#include "k3bjob.moc"

/* 
 *
 * $Id: $
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

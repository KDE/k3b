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


void K3bJob::connectSubJob( K3bJob* subJob,
			    const char* finishedSlot,
			    bool connectProgress,
			    const char* progressSlot,
			    const char* subProgressSlot,
			    const char* processedSizeSlot,
			    const char* processedSubSizeSlot )
{
  connect( subJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( subJob, SIGNAL(newSubTask(const QString&)), this, SLOT(slotNewSubTask(const QString&)) );
  connect( subJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  connect( subJob, SIGNAL(infoMessage(const QString&, int)),
	   this, SIGNAL(infoMessage(const QString&, int)) );
  connect( subJob, SIGNAL(finished(bool)), this, finishedSlot );

  if( connectProgress ) {
    connect( subJob, SIGNAL(percent(int)), 
	       this, progressSlot != 0 ? progressSlot : SIGNAL(subPercent(int)) );
    if( subProgressSlot )
      connect( subJob, SIGNAL(subPercent(int)), this, subProgressSlot );
    connect( subJob, SIGNAL(processedSize(int, int)), 
	     this, processedSizeSlot != 0 ? processedSizeSlot : SIGNAL(processedSubSize(int, int)) );
    if( processedSubSizeSlot )
      connect( subJob, SIGNAL(processedSubSize(int, int)), this, processedSubSizeSlot );
  }
}


void K3bJob::slotNewSubTask( const QString& str )
{
  emit infoMessage( str, INFO );
}


K3bBurnJob::K3bBurnJob( QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_writeMethod( K3b::DEFAULT )
{
}


int K3bBurnJob::supportedWritingApps() const
{
  return K3b::DEFAULT | K3b::CDRDAO | K3b::CDRECORD;
}

#include "k3bjob.moc"

/***************************************************************************
                          k3bbinimagewritingwritingjob.cpp  -  description
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

#include "k3bbinimagewritingjob.h"
#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../k3bemptydiscwaiter.h"
#include "../device/k3bdevice.h"
#include "../cdinfo/k3bdiskinfo.h"
#include "../cdinfo/k3bdiskinfodetector.h"

#include <k3bprocess.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>

#include <qtimer.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qregexp.h>

#include <sys/types.h>
#include <sys/socket.h>



K3bBinImageWritingJob::K3bBinImageWritingJob( QObject* parent )
        : K3bBurnJob( parent ),
        m_copies(1)
{
    m_cdrdaowriter = new K3bCdrdaoWriter(0, this);
    connect(m_cdrdaowriter,SIGNAL(percent(int)),
            this,SLOT(copyPercent(int)));
    connect(m_cdrdaowriter,SIGNAL(subPercent(int)),
            this,SLOT(copySubPercent(int)));
    connect(m_cdrdaowriter,SIGNAL(buffer(int)),
            this,SIGNAL(bufferStatus(int)));
    connect(m_cdrdaowriter,SIGNAL(newSubTask(const QString&)),
            this, SIGNAL(newSubTask(const QString&)) );
    connect(m_cdrdaowriter,SIGNAL(infoMessage(const QString&, int)),
            this, SIGNAL(infoMessage(const QString&, int)) );
    connect(m_cdrdaowriter,SIGNAL(debuggingOutput(const QString&, const QString&)),
            this,SIGNAL(debuggingOutput(const QString&, const QString&)));
    connect(m_cdrdaowriter,SIGNAL(finished(bool)),
            this,SLOT(cdrdaoFinished(bool)));
    connect(m_cdrdaowriter, SIGNAL(nextTrack(int, int)),
            this, SLOT(slotNextTrack(int, int)) );
}


K3bBinImageWritingJob::~K3bBinImageWritingJob() {
    delete m_cdrdaowriter;
}

void K3bBinImageWritingJob::start() {
    if( m_copies < 1 )
        m_copies = 1;
    m_finishedCopies = 0;

    emit newTask( i18n("Write Binary Image") );

    cdrdaoWrite();

    emit started();
}

void K3bBinImageWritingJob::cancel() {
    m_cdrdaowriter->cancel();
    emit canceled();
}


void K3bBinImageWritingJob::cdrdaoWrite() {
    m_cdrdaowriter->setCommand(K3bCdrdaoWriter::WRITE);
    K3bEmptyDiscWaiter waiter( m_cdrdaowriter->burnDevice(), k3bMain() );
    if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
        cancelAll();
        return;
    }
    m_cdrdaowriter->start();
}

void K3bBinImageWritingJob::copyPercent(int p) {
    int x,y;

    x = m_copies;
    y = m_finishedCopies;

    emit percent((100*y + p)/x);
}

void K3bBinImageWritingJob::copySubPercent(int p) {
    emit subPercent(p);
}

void K3bBinImageWritingJob::cdrdaoFinished(bool ok) {
    if (ok) {
        m_finishedCopies++;
        if ( m_finishedCopies == m_copies ) {
            emit infoMessage(
                i18n("%1 copies succsessfully created").arg(m_copies),K3bJob::INFO );
            finishAll(); 
        } else 
            cdrdaoWrite();
    } else
        cancelAll();
}


void K3bBinImageWritingJob::finishAll() {
    if( k3bMain()->eject() )
        m_cdrdaowriter->burnDevice()->eject();

    emit finished( true );
}


void K3bBinImageWritingJob::cancelAll() {
    emit infoMessage( i18n("Canceled"), K3bJob::STATUS );
    emit finished( false );
}


void K3bBinImageWritingJob::slotNextTrack( int t, int tt ) {
    emit newSubTask( i18n("Writing track %1 of %2").arg(t).arg(tt) );
}


#include "k3bbinimagewritingjob.moc"

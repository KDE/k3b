/***************************************************************************
                          k3bcdcopyjob.cpp  -  description
                             -------------------
    begin                : Sun Mar 17 2002
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

#include "k3bcdcopyjob.h"

#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../k3bemptydiscwaiter.h"
#include "../device/k3bdevice.h"
#include "../cdinfo/k3bdiskinfo.h"
#include "../cdinfo/k3bdiskinfodetector.h"

#include "../remote.h"

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



K3bCdCopyJob::K3bCdCopyJob( QObject* parent )
        : K3bBurnJob( parent ),
        m_copies(1),
        m_finishedCopies(0),
        m_onlyCreateImage(false),
        m_onTheFly(true),
        m_tempPath(QString("")),
        m_tocFile(QString("")),
m_job(QString("Reading")) {
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

    m_diskInfoDetector = new K3bDiskInfoDetector( this );
    connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bDiskInfo&)),
             this, SLOT(diskInfoReady(const K3bDiskInfo&)) );
}


K3bCdCopyJob::~K3bCdCopyJob() {
    delete m_cdrdaowriter;
    delete m_diskInfoDetector;
}


void K3bCdCopyJob::start() {
    if( m_copies < 1 )
        m_copies = 1;
    m_finishedCopies = 0;

    m_tempPath = k3bMain()->findTempFile( "img", m_tempPath );
    m_tocFile  = QString(m_tempPath);
    m_tocFile  = m_tocFile.replace(m_tocFile.findRev(".img"),4,".toc");

    //  m_tempPath = QFile::encodeName(m_tempPath);
    //  m_toc  = QFile::encodeName(m_toc);
    emit infoMessage( i18n("Retrieving information about source disk"), K3bJob::PROCESS );
    m_diskInfoDetector->detect( m_cdrdaowriter->sourceDevice() );

}


void K3bCdCopyJob::diskInfoReady( const K3bDiskInfo& info ) {
    if( info.noDisk ) {
        emit infoMessage( i18n("No disk in CD reader"), K3bJob::ERROR );
        cancelAll();
        return;
    }

    if( info.empty ) {
        emit infoMessage( i18n("Source disk is empty"), K3bJob::ERROR );
        cancelAll();
        return;
    }

    if( info.tocType == K3bDiskInfo::DVD ) {
        emit infoMessage( i18n("Source disk seems to be a DVD."), K3bJob::ERROR );
        emit infoMessage( i18n("K3b is not able to copy DVDs yet."), K3bJob::ERROR );
        cancelAll();
        return;
    }


    // TODO: check size and free space on disk


    switch( info.tocType ) {
    case K3bDiskInfo::DATA:
        emit infoMessage( i18n("Source disk seems to be a data CD"), K3bJob::INFO );
        break;
    case K3bDiskInfo::AUDIO:
        emit infoMessage( i18n("Source disk seems to be an audio CD"), K3bJob::INFO );
        break;
    case K3bDiskInfo::MIXED:
        emit infoMessage( i18n("Source disk seems to be a mixed mode CD"), K3bJob::INFO );
        break;
    }

    if( m_onlyCreateImage && !m_onTheFly )
        emit newTask( i18n("Creating CD image: %1 ").arg( m_tempPath ) );
    else if( m_cdrdaowriter->simulate() )
        emit newTask( i18n("CD copy simulation") );
    else
        emit newTask( i18n("CD copy") );

    if ( m_onTheFly )
        cdrdaoDirectCopy();
    else
        cdrdaoRead();

    emit started();
}

void K3bCdCopyJob::cancel() {
    emit canceled();
    m_cdrdaowriter->cancel();
}


void K3bCdCopyJob::cdrdaoRead() {
    m_cdrdaowriter->setCommand(K3bCdrdaoWriter::READ);

    m_cdrdaowriter->setDataFile(m_tempPath);

    m_cdrdaowriter->setTocFile(m_tocFile);

    m_job = QString("Reading");
    m_cdrdaowriter->start();
}

void K3bCdCopyJob::cdrdaoWrite() {
    m_cdrdaowriter->setCommand(K3bCdrdaoWriter::WRITE);
    m_cdrdaowriter->setTocFile(m_tocFile);

    K3bEmptyDiscWaiter waiter( m_cdrdaowriter->burnDevice(), k3bMain() );
    if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
        cancelAll();
        return;
    }
    m_job = QString("Writing");
    m_cdrdaowriter->start();
}

void K3bCdCopyJob::cdrdaoDirectCopy() {
    m_cdrdaowriter->setCommand(K3bCdrdaoWriter::COPY);
    m_cdrdaowriter->setOnTheFly(true);

    K3bEmptyDiscWaiter waiter( m_cdrdaowriter->burnDevice(), k3bMain() );
    if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
        cancelAll();
        return;
    }
    m_job = QString("Writing");
    m_cdrdaowriter->start();
}

void K3bCdCopyJob::copyPercent(int p) {
    int x,y;

    x = m_onTheFly || m_onlyCreateImage ? m_copies : m_copies + 1;
    y = m_finishedCopies;

    emit percent((100*y + p)/x);
}

void K3bCdCopyJob::copySubPercent(int p) {
    emit subPercent(p);
}

void K3bCdCopyJob::cdrdaoFinished(bool ok) {
    if (ok) {
        m_finishedCopies++;
        if ( m_onlyCreateImage ) {
            emit infoMessage(
                i18n("Image '%1' and toc-file '%2' succsessfully created").arg(m_tempPath).arg(m_tocFile),
                K3bJob::INFO );
            finishAll();
        } else if ( m_finishedCopies > m_copies || m_onTheFly && m_finishedCopies == m_copies ) {
            emit infoMessage(
                i18n("%1 copies succsessfully created").arg(m_copies),K3bJob::INFO );
            finishAll(); 
        } else {
            if( m_cdrdaowriter->burnDevice() == m_cdrdaowriter->sourceDevice() )
                m_cdrdaowriter->sourceDevice()->eject();
            else if ( !m_onTheFly ) 
                cdrdaoWrite();
            else
                cdrdaoDirectCopy();
        }
    } else
        cancelAll();
}


void K3bCdCopyJob::finishAll() {
    if( !m_keepImage && !m_onTheFly ) {
        if (QFile::exists(m_tocFile) )
            QFile::remove(m_tocFile);
        if (QFile::exists(m_tempPath))
            QFile::remove(m_tempPath);
        emit infoMessage( i18n("Imagefiles removed"), K3bJob::STATUS );
    }
    if( k3bMain()->eject() )
        m_cdrdaowriter->sourceDevice()->eject();

    emit finished( true );
}


void K3bCdCopyJob::cancelAll() {
    if (QFile::exists(m_tocFile) )
        QFile::remove(m_tocFile);
    if (QFile::exists(m_tempPath))
        QFile::remove(m_tempPath);
    emit infoMessage( i18n("Canceled, temporary files removed"), K3bJob::STATUS );

    emit finished( false );
}


void K3bCdCopyJob::slotNextTrack( int t, int tt ) {
    emit newSubTask( i18n("%1 track %2 of %3").arg(m_job).arg(t).arg(tt) );
}


#include "k3bcdcopyjob.moc"

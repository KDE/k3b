/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *                    Klaus-Dieter Krannich <kd@k3b.org>
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


#include "k3bcdcopythread.h"

#include <k3b.h>
#include <tools/k3bexternalbinmanager.h>
#include <device/k3bdevice.h>
#include <device/k3bdiskinfo.h>
#include <device/k3bdiskinfodetector.h>

#include <remote.h>

#include <k3bprocess.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>

#include <qtimer.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qregexp.h>
#include <qtextstream.h>

#include <sys/types.h>
#include <sys/socket.h>



K3bCdCopyThread::K3bCdCopyThread( QObject* parent )
        : QObject( parent), K3bThread( parent ),
        m_copies(1),
        m_finishedCopies(0),
        m_sessions(1),
        m_finishedSessions(0),
        m_onlyCreateImage(false),
        m_onTheFly(true),
        m_tempPath(QString("")),
        m_tocFile(QString("")),
        m_job(READING)
{
    m_cdrdaowriter = new K3bCdrdaoWriter(0, this);
    connect(m_cdrdaowriter,SIGNAL(percent(int)),
            this,SLOT(copyPercent(int)));
    connect(m_cdrdaowriter,SIGNAL(subPercent(int)),
            this,SLOT(copySubPercent(int)));
    connect(m_cdrdaowriter,SIGNAL(buffer(int)),
            this,SLOT(bufferStatus(int)));
    connect(m_cdrdaowriter,SIGNAL(newSubTask(const QString&)),
            this, SLOT(newSubTask(const QString&)) );
    connect(m_cdrdaowriter,SIGNAL(infoMessage(const QString&, int)),
            this, SLOT(infoMessage(const QString&, int)) );
    connect(m_cdrdaowriter,SIGNAL(debuggingOutput(const QString&, const QString&)),
            this,SLOT(debuggingOutput(const QString&, const QString&)));
    connect(m_cdrdaowriter,SIGNAL(finished(bool)),
            this,SLOT(cdrdaoFinished(bool)));
    connect(m_cdrdaowriter, SIGNAL(nextTrack(int, int)),
            this, SLOT(slotNextTrack(int, int)) );
    connect( m_cdrdaowriter, SIGNAL(writeSpeed(int)),
             this, SLOT(writeSpeed(int)) );

}


K3bCdCopyThread::~K3bCdCopyThread() {
    delete m_cdrdaowriter;
}


void K3bCdCopyThread::run() {
    if( m_copies < 1 )
        m_copies = 1;
    m_finishedCopies = 0;

    emitInfoMessage( i18n("Retrieving information about source disk"), K3bJob::PROCESS );
    getSourceDiskInfo( m_cdrdaowriter->sourceDevice() );
    m_wait.wait();
}


void K3bCdCopyThread::getSourceDiskInfo(K3bDevice *dev) {
    if( dev->isEmpty() == 0 ) {
        emitInfoMessage( i18n("Source disk is empty"), K3bJob::ERROR );
        cancelAll();
        return;
    }
    K3bDiskInfo::type diskType = dev->diskType();

    if( diskType == K3bDiskInfo::NODISC  ) {
        emitInfoMessage( i18n("No disk in CD reader"), K3bJob::ERROR );
        cancelAll();
        return;
    }

    if( diskType == K3bDiskInfo::DVD ) {
        emitInfoMessage( i18n("Source disk seems to be a DVD."), K3bJob::ERROR );
        emitInfoMessage( i18n("K3b is not able to copy DVDs yet."), K3bJob::ERROR );
        cancelAll();
        return;
    }
    m_sessions = dev->numSessions();
    if( m_sessions < 2 ) {
        m_tempPath = k3bMain()->findTempFile( "img", m_tempPath );
        m_tocFile  = m_tempPath;
        m_tocFile  = m_tocFile.replace(m_tocFile.findRev(".img"),4,".toc");;

        switch( diskType ) {
          case K3bDiskInfo::DATA:
               emitInfoMessage( i18n("Source disk seems to be a data CD"), K3bJob::INFO );
               break;
          case K3bDiskInfo::AUDIO:
               emitInfoMessage( i18n("Source disk seems to be an audio CD"), K3bJob::INFO );
               break;
          case K3bDiskInfo::MIXED:
               emitInfoMessage( i18n("Source disk seems to be a mixed mode CD"), K3bJob::INFO );
               break;
          case K3bDiskInfo::UNKNOWN:
          case K3bDiskInfo::NODISC:
          case K3bDiskInfo::DVD:
               break;
        }
    } else {
        m_cdrdaowriter->setMulti(true);
        m_cdrdaowriter->setSession(1);
        m_cdrdaowriter->setEject(false);
        m_tempPath = k3bMain()->findTempFile( "img.1", m_tempPath );
        m_tocFile  = m_tempPath;
        m_tocFile  = m_tocFile.replace(m_tocFile.findRev(".img"),4,".toc");
        emitInfoMessage( i18n("Source disk seems to be a multisession CD"), K3bJob::INFO );
    }

    if( m_onlyCreateImage && !m_onTheFly )
        emitNewTask( i18n("Creating CD image: %1 ").arg( m_tempPath ) );
    else if( m_cdrdaowriter->simulate() )
        emitNewTask( i18n("CD copy simulation") );
    else if( m_sessions > 1 )
        emitNewTask( i18n("Multisession CD copy") );
    else
        emitNewTask( i18n("CD copy") );

    if ( m_onTheFly )
        cdrdaoDirectCopy();
    else
        cdrdaoRead();

    emitStarted();

}

void K3bCdCopyThread::cancel() {
    emitCanceled();
    m_cdrdaowriter->cancel();
    m_wait.wakeAll();
}


void K3bCdCopyThread::cdrdaoRead() {
    m_cdrdaowriter->setCommand(K3bCdrdaoWriter::READ);

    m_cdrdaowriter->setDataFile(m_tempPath);

    m_cdrdaowriter->setTocFile(m_tocFile);

    m_job = READING;
    m_cdrdaowriter->start();
}

void K3bCdCopyThread::cdrdaoWrite() {
    m_cdrdaowriter->setCommand(K3bCdrdaoWriter::WRITE);
    m_cdrdaowriter->setTocFile(m_tocFile);
    m_cdrdaowriter->setDataFile("");

    m_job = WRITING;
    m_cdrdaowriter->start();
}

void K3bCdCopyThread::cdrdaoDirectCopy() {
    m_cdrdaowriter->setCommand(K3bCdrdaoWriter::COPY);
    m_cdrdaowriter->setOnTheFly(true);

    m_job = WRITING;
    m_cdrdaowriter->start();
}

void K3bCdCopyThread::copyPercent(int p) {
    int x,y,z;

    x = (m_onTheFly || m_onlyCreateImage ? m_copies : m_copies + 1);
    y = m_finishedCopies;
    z = m_finishedSessions;
    emitPercent((100*y + (100*z + p)/m_sessions ) / x);
}

void K3bCdCopyThread::copySubPercent(int p) {
    emitSubPercent(p);
}

void K3bCdCopyThread::cdrdaoFinished(bool ok) {
    if (ok) {
        if ( m_onlyCreateImage ) {
            emitInfoMessage(
                i18n("Image '%1' and toc-file '%2' successfully created").arg(m_tempPath).arg(m_tocFile),
                K3bJob::INFO );
            if ( ++m_finishedSessions == m_sessions )
                finishAll();
            else {
                if ( m_finishedSessions == 1)
                    fixTocFile(m_tocFile);
                m_cdrdaowriter->setSession(m_finishedSessions+1);
                m_tempPath = m_tempPath.replace(m_tempPath.findRev("."),2,".%1").arg(m_finishedSessions+1);
                m_tocFile = m_tocFile.replace(m_tocFile.findRev("."),2,".%1").arg(m_finishedSessions+1);
                emitInfoMessage(
                   i18n("Reading session %1").arg(m_finishedSessions+1),
                   K3bJob::INFO );
                cdrdaoRead();
            }
        } else if( m_onTheFly ) {
            if ( ++m_finishedSessions < m_sessions) {
               m_cdrdaowriter->setSession(m_finishedSessions+1);
               cdrdaoDirectCopy();
            } else if ( ++m_finishedCopies == m_copies ) {
                emitInfoMessage(
                    i18n("%1 copies successfully created").arg(m_copies),K3bJob::INFO );
                finishAll();
            } else {
               m_cdrdaowriter->burnDevice()->eject();
               m_finishedSessions = 0;
               m_cdrdaowriter->setSession(m_finishedSessions+1);
               emitInfoMessage(
                    i18n("Start session %1").arg(m_finishedSessions+1),K3bJob::INFO );
                finishAll();
              cdrdaoDirectCopy();
            }
         } else {
            if ( m_finishedCopies == 0) {
               if ( ++m_finishedSessions < m_sessions) {
                   if ( m_finishedSessions == 1)
                      fixTocFile(m_tocFile);
                   m_cdrdaowriter->setSession(m_finishedSessions+1);
                   m_tempPath = m_tempPath.replace(m_tempPath.findRev("."),2,".%1").arg(m_finishedSessions+1);
                   m_tocFile = m_tocFile.replace(m_tocFile.findRev("."),2,".%1").arg(m_finishedSessions+1);
                   emitInfoMessage(
                        i18n("Reading session %1").arg(m_finishedSessions+1),
                        K3bJob::INFO );
                   cdrdaoRead();
               } else {
                   ++m_finishedCopies;
                   m_finishedSessions = 0;
                   if ( m_sessions > 1 ) {
                      m_cdrdaowriter->setSession(m_finishedSessions+1);
                      m_tocFile = m_tocFile.replace(m_tocFile.findRev("."),2,".%1").arg(m_finishedSessions+1);
                      emitInfoMessage(
                                i18n("Start writing session %1").arg(m_finishedSessions+1),
                                K3bJob::INFO );
                  }
                   if( m_cdrdaowriter->burnDevice() == m_cdrdaowriter->sourceDevice() )
                        m_cdrdaowriter->sourceDevice()->eject();
                   cdrdaoWrite();
               }
            } else {
                if ( ++m_finishedSessions < m_sessions) {
                    m_cdrdaowriter->setSession(m_finishedSessions+1);
                    m_tocFile = m_tocFile.replace(m_tocFile.findRev("."),2,".%1").arg(m_finishedSessions+1);
                    emitInfoMessage(
                         i18n("Start writing session %1").arg(m_finishedSessions+1),
                         K3bJob::INFO );
                    cdrdaoWrite();
                } else if ( ++m_finishedCopies > m_copies ) {
                    emitInfoMessage(
                        i18n("%1 copies successfully created").arg(m_copies),K3bJob::INFO );
                    finishAll();
                } else {
                    m_cdrdaowriter->burnDevice()->eject();
                    m_finishedSessions = 0;
                    if ( m_sessions > 1 ) {
                       m_cdrdaowriter->setSession(m_finishedSessions+1);
                       m_tocFile = m_tocFile.replace(m_tocFile.findRev("."),2,".%1").arg(m_finishedSessions+1);
                       emitInfoMessage(
                            i18n("Start writing session %1").arg(m_finishedSessions+1),
                            K3bJob::INFO );
                    }
                    cdrdaoWrite();
                }
           }
        }
    } else
        cancelAll();

}


void K3bCdCopyThread::finishAll() {
    if( !m_keepImage && !m_onTheFly ) {
        removeImages();
        emitInfoMessage( i18n("Image files removed"), K3bJob::STATUS );
    }

    if( k3bMain()->eject() ) {
        m_cdrdaowriter->sourceDevice()->eject();
        if ( !m_onlyCreateImage )
            m_cdrdaowriter->burnDevice()->eject();
    }

    emitFinished( true );
    m_wait.wakeAll();
}


void K3bCdCopyThread::cancelAll() {
    removeImages();
    emitInfoMessage( i18n("Canceled, temporary files removed"), K3bJob::STATUS );

    emitFinished( false );
    m_wait.wakeAll();
}

void K3bCdCopyThread::removeImages() {
    if (m_sessions == 1) {
      if (QFile::exists(m_tocFile) )
            QFile::remove(m_tocFile);
      if (QFile::exists(m_tempPath))
            QFile::remove(m_tempPath);
    } else {
      for ( int i=1; i <= m_sessions; i++ ) {
        m_tocFile = m_tocFile.replace(m_tocFile.findRev("."),2,".%1").arg(i);
        m_tempPath = m_tempPath.replace(m_tempPath.findRev("."),2,".%1").arg(i);
        if (QFile::exists(m_tocFile) )
            QFile::remove(m_tocFile);
        if (QFile::exists(m_tempPath))
            QFile::remove(m_tempPath);
      }
    }
}

void K3bCdCopyThread::fixTocFile(QString &f) {
  QTextStream s(new QFile(f));
  s.device()->open(IO_ReadWrite);
  QString toc = s.read().replace(QRegExp("CD_DA"),"CD_ROM_XA");
  s.device()->reset();
  s << toc;
  s.device()->close();
}

void K3bCdCopyThread::slotNextTrack( int t, int tt ) {
    if( m_job == WRITING )
        emitNewSubTask( i18n("Writing track %1 of %2").arg(t).arg(tt) );
    else
        emitNewSubTask( i18n("Reading track %1 of %2").arg(t).arg(tt) );
}

void K3bCdCopyThread::bufferStatus(int i) {
  emitBufferStatus(i);
}

void K3bCdCopyThread::newSubTask(const QString& t) {
  emitNewSubTask(t);
}

void K3bCdCopyThread::infoMessage(const QString& m, int t) {
  emitInfoMessage(m,t);
}

void K3bCdCopyThread::debuggingOutput(const QString& s1, const QString& s2) {
  emitDebuggingOutput(s1,s2);
}

void K3bCdCopyThread::writeSpeed(int s) {
  emitWriteSpeed(s);
}


#include "k3bcdcopythread.moc"

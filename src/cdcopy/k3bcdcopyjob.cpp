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

#include <qurloperator.h>

K3bCdCopyJob::K3bCdCopyJob( QObject* parent )
  : K3bBurnJob( parent ),
    m_paranoiaMode( 3 )
{
  m_onlyCreateImage = false;
  m_copies = 1;
  m_finishedCopies = 0;
  m_cdrdaowriter = new K3bCdrdaoWriter(0, this);
  connect(m_cdrdaowriter,SIGNAL(percent(int)),this,SLOT(copyPercent(int)));
  connect(m_cdrdaowriter,SIGNAL(subPercent(int)),this,SLOT(copySubPercent(int))); 
  connect(m_cdrdaowriter,SIGNAL(buffer(int)),this,SIGNAL(bufferStatus(int)));
  connect(m_cdrdaowriter,SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect(m_cdrdaowriter,SIGNAL(infoMessage(const QString&, int)), 
	  this, SIGNAL(infoMessage(const QString&, int)) );
  connect(m_cdrdaowriter,SIGNAL(debuggingOutput(const QString&, const QString&)),
              this,SIGNAL(debuggingOutput(const QString&, const QString&)));
  connect(m_cdrdaowriter,SIGNAL(finished(bool)),this,SLOT(cdrdaoFinished(bool)));

  m_cdrdaoreader = new K3bCdrdaoReader(this);
  connect(m_cdrdaoreader,SIGNAL(percent(int)),this,SLOT(copyPercent(int)));
  connect(m_cdrdaoreader,SIGNAL(subPercent(int)),this,SLOT(copySubPercent(int))); 
  connect(m_cdrdaoreader,SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect(m_cdrdaoreader,SIGNAL(infoMessage(const QString&, int)), 
	  this, SIGNAL(infoMessage(const QString&, int)) );
  connect(m_cdrdaoreader,SIGNAL(debuggingOutput(const QString&, const QString&)),
              this,SIGNAL(debuggingOutput(const QString&, const QString&)));
  connect(m_cdrdaoreader,SIGNAL(finished(bool)),this,SLOT(readFinished(bool)));

  m_diskInfoDetector = new K3bDiskInfoDetector( this );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(const K3bDiskInfo&)), 
	   this, SLOT(diskInfoReady(const K3bDiskInfo&)) );
}


K3bCdCopyJob::~K3bCdCopyJob()
{
  delete m_cdrdaowriter;
  delete m_cdrdaoreader;
  delete m_diskInfoDetector;
}


void K3bCdCopyJob::start()
{
  if( m_copies < 1 )
    m_copies = 1;
  m_finishedCopies = 0;

  m_tempPath = k3bMain()->findTempFile( "img", m_tempPath );
  m_tocFile  = QString(m_tempPath);
  m_tocFile  = m_tocFile.replace(QRegExp(".img"),".toc");
  
//  m_tempPath = QFile::encodeName(m_tempPath);
//  m_toc  = QFile::encodeName(m_toc);
  emit infoMessage( i18n("Retrieving information about source disk"), K3bJob::PROCESS );
  m_diskInfoDetector->detect( m_cdrdaoreader->readDevice() );
  
}


void K3bCdCopyJob::diskInfoReady( const K3bDiskInfo& info )
{
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

void K3bCdCopyJob::cancel()
{
  emit canceled();
  m_cdrdaoreader->cancel();
  m_cdrdaowriter->cancel();
}


void K3bCdCopyJob::cdrdaoRead()
{
  m_cdrdaoreader->prepareArgumentList();
  m_cdrdaoreader->addArgument("--source-device")->addArgument(m_cdrdaoreader->readDevice()->busTargetLun());
  
  if( m_cdrdaoreader->readDevice()->cdrdaoDriver() != "auto" )
    m_cdrdaoreader->addArgument("--source-driver")->addArgument(m_cdrdaoreader->readDevice()->cdrdaoDriver());

  if( m_fastToc )
    m_cdrdaoreader->addArgument("--fast-toc");

  if( m_readRaw )
    m_cdrdaoreader->addArgument("--read-raw");

  m_cdrdaoreader->addArgument("--paranoia-mode")->addArgument(QString("%1").arg(m_paranoiaMode));  

  m_cdrdaoreader->addArgument("--datafile")->addArgument(m_tempPath);

  m_cdrdaoreader->addArgument(m_tocFile);

  m_cdrdaoreader->start();
}

void K3bCdCopyJob::cdrdaoWrite()
{ 
  m_cdrdaowriter->prepareArgumentList();

  m_cdrdaowriter->addArgument(m_tocFile);
 
  K3bEmptyDiscWaiter waiter( m_cdrdaowriter->burnDevice(), k3bMain() );
  if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
    cancelAll();
    return;   
  }

  m_cdrdaowriter->start();
}

void K3bCdCopyJob::cdrdaoDirectCopy()
{
  m_cdrdaowriter->prepareArgumentList(true);
  
  addCdrdaoReadArguments();
  
  m_cdrdaowriter->addArgument("--on-the-fly");

  K3bEmptyDiscWaiter waiter( m_cdrdaowriter->burnDevice(), k3bMain() );
  if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
    cancelAll();
    return;
  }

  m_cdrdaowriter->start();
}

void K3bCdCopyJob::addCdrdaoReadArguments()
{
  m_cdrdaowriter->addArgument("--source-device")->addArgument(m_cdrdaoreader->readDevice()->busTargetLun());
  
  if( m_cdrdaoreader->readDevice()->cdrdaoDriver() != "auto" )
    m_cdrdaowriter->addArgument("--source-driver")->addArgument(m_cdrdaoreader->readDevice()->cdrdaoDriver());

  if( m_fastToc )
    m_cdrdaowriter->addArgument("--fast-toc");

  if( m_readRaw )
    m_cdrdaowriter->addArgument("--read-raw");

  m_cdrdaowriter->addArgument("--paranoia-mode")->addArgument(QString("%1").arg(m_paranoiaMode));
}

void K3bCdCopyJob::copyPercent(int p)
{
   int x,y;
   
   x = m_onTheFly || m_onlyCreateImage ? m_copies : m_copies + 1;
   y = m_finishedCopies;

   emit percent((100*y + p)/x);
}

void K3bCdCopyJob::copySubPercent(int p)
{
  emit subPercent(p);
}

void K3bCdCopyJob::readFinished(bool ok)
{
  if (ok) {
     if ( m_copies > 1) { 
       QUrlOperator cp;
       cp.copy(m_tocFile,m_tocFile+QString(".bak"),false,false);
     }
     if( m_cdrdaowriter->burnDevice() == m_cdrdaoreader->readDevice() )
       m_cdrdaoreader->readDevice()->eject();
     if ( !m_onlyCreateImage ) {
        m_finishedCopies++;
        cdrdaoWrite();
     } 
     else {
        emit infoMessage( 
             i18n("Image '%1' and toc-file '%2' succsessfully created").arg(m_tempPath).arg(m_tocFile), 
                  K3bJob::INFO );
        finishAll();
     }
 }
  else
    cancelAll();
}

void K3bCdCopyJob::cdrdaoFinished(bool ok)
{
  if (ok) {
     m_finishedCopies++;
     if ( m_finishedCopies <= m_copies && !m_onTheFly ) {
       QUrlOperator *cp = new QUrlOperator();
       cp->copy(m_tocFile+QString(".bak"),m_tocFile,false,false);
       cdrdaoWrite();
     } 
     else if ( m_finishedCopies < m_copies && m_onTheFly )
       cdrdaoDirectCopy();
     else  
      finishAll();
  }
  else
    cancelAll();
}

void K3bCdCopyJob::finishAll()
{
  if( !m_keepImage && !m_onTheFly ) {
    if (QFile::exists(m_tocFile) ) 
      QFile::remove(m_tocFile);
    if (QFile::exists(m_tocFile+QString(".bak")))
      QFile::remove(m_tocFile+QString(".bak"));
    if (QFile::exists(m_tempPath))
      QFile::remove(m_tempPath);
    emit infoMessage( i18n("Imagefiles removed"), K3bJob::STATUS );
  }
  if( k3bMain()->eject() && m_onTheFly )
	  m_cdrdaoreader->readDevice()->eject();

  emit finished( true );
}


void K3bCdCopyJob::cancelAll()
{
    if (QFile::exists(m_tocFile) ) 
      QFile::remove(m_tocFile);
    if (QFile::exists(m_tocFile+QString(".bak")))
      QFile::remove(m_tocFile+QString(".bak"));
    if (QFile::exists(m_tempPath))
      QFile::remove(m_tempPath);
     emit infoMessage( i18n("Canceled, temporary files removed"), K3bJob::STATUS );


  emit finished( false );
}


#include "k3bcdcopyjob.moc"

/***************************************************************************
                             k3b -  description
                             -------------------
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
                          k3bvcdjob.cpp  -  description
                             -------------------
    begin                : Mon Nov 4 2002
    copyright            : (C) 2002 by Christian Kvasny
    email                : chris@ckvsoft.at
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "k3bvcdjob.h"

#include "k3bvcddoc.h"
#include "k3bvcdtrack.h"
#include "../k3b.h"
#include "../k3bdoc.h"
#include "../k3bprocess.h"
#include "../k3bemptydiscwaiter.h"
#include "../device/k3bdevice.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../tools/k3bglobals.h"

#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <qstring.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qtimer.h>
#include <kdebug.h>
#include <qregexp.h>
#include <qdom.h>


K3bVcdJob::K3bVcdJob( K3bVcdDoc* doc )
  : K3bBurnJob()
{
  m_doc = doc;
  m_process = 0;
  m_currentWrittenTrackNumber = 0;
  m_bytesFinishedTracks = 0;
}


K3bVcdJob::~K3bVcdJob()
{
  delete m_process;
}


K3bDoc* K3bVcdJob::doc() const
{
  return m_doc;
}


K3bDevice* K3bVcdJob::writer() const
{
  return doc()->burner();
}

void K3bVcdJob::cancel()
{
  cancelAll();

  emit infoMessage( i18n("Job canceled by user."), K3bJob::ERROR );
  emit finished( false );
}


void K3bVcdJob::cancelAll()
{
  if( m_process->isRunning() ) {
    m_process->disconnect(this);
    m_process->kill();

    // we need to unlock the writer because cdrdao/cdrecord locked it while writing
    bool block = m_doc->burner()->block( false );
    if( !block )
      emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
    else if ( k3bMain()->eject() )
      m_doc->burner()->eject();
  }
}


void K3bVcdJob::start()
{
  kdDebug() << "(K3bVcdJob) starting job" << endl;

  emit started();

  int pos = QString(m_doc->vcdImage()).find(".bin");
  if (pos > 0) {
    m_cueFile = m_doc->vcdImage().left(pos) + ".cue";
  }
  else {
    m_cueFile = m_doc->vcdImage() + ".cue";
    m_doc->setVcdImage(m_doc->vcdImage() + ".bin");
  }

  vcdxGen();
}

void K3bVcdJob::vcdxGen()
{
  /*
  Usage: vcdxgen [OPTION...]
    -o, --output-file=FILE              specify xml file for output (default:
                                        'videocd.xml')
    -t, --type=TYPE                     select VideoCD type ('vcd11', 'vcd2',
                                        'svcd' or 'hqvcd') (default: 'vcd2')
    -l, --iso-volume-label=LABEL        specify ISO volume label for video cd
                                        (default: 'VIDEOCD')
    --iso-application-id=LABEL          specify ISO application id for video cd
                                        (default: '')
    --info-album-id=LABEL               specify album id for video cd set
                                        (default: '')
    --volume-count=NUMBER               specify number of volumes in album set
    --volume-number=NUMBER              specify album set sequence number (<
                                        volume-count)
    --broken-svcd-mode                  enable non-compliant compatibility mode
                                        for broken devices
    --update-scan-offsets               update scan data offsets in video mpeg2
                                        stream
    --nopbc                             don't create PBC
    --add-dirtree=DIR                   add directory contents recursively to
                                        ISO fs root
    --add-dir=ISO_DIRNAME               add empty dir to ISO fs
    --add-file=FILE,ISO_FILENAME        add single file to ISO fs
    --add-file-2336=FILE,ISO_FILENAME   add file containing full 2336 byte
                                        sectors to ISO fs
    -v, --verbose                       be verbose
    -q, --quiet                         show only critical messages
    -V, --version                       display version and copyright
                                        information and exit
  */
  delete m_process;
  m_process = new KProcess();

  m_xmlFile = locateLocal( "appdata", "temp/k3btempvcd.xml");
  // remove old xml-file
  if( QFile::exists( m_xmlFile ) )
    QFile::remove( m_xmlFile );

  if( !k3bMain()->externalBinManager()->foundBin( "vcdxgen" ) ) {
    kdDebug() << "(K3bVcdJob) could not find vcdxgen executable" << endl;
    emit infoMessage( i18n("vcdxgen executable not found."), K3bJob::ERROR );
    cancelAll();
    return;
  }

  emit infoMessage( i18n("Create XML-file"), K3bJob::STATUS );
  *m_process << k3bMain()->externalBinManager()->binPath( "vcdxgen" );
  // Label
  *m_process << "-l" << QString("%1").arg(m_doc->vcdOptions()->volumeId());
  // AlbumID
  *m_process << QString("--info-album-id=%1").arg(m_doc->vcdOptions()->albumId());

  // set vcdType
  switch( ((K3bVcdDoc*)doc())->vcdType() ) {
  case K3bVcdDoc::VCD11:
        *m_process << "-t" << "vcd11";
    break;
  case K3bVcdDoc::VCD20:
        *m_process << "-t" << "vcd2";
    break;
  case K3bVcdDoc::SVCD10:
        *m_process << "-t" << "svcd";
    break;
  case K3bVcdDoc::HQVCD:
        *m_process << "-t" << "hqvcd";
    break;
  default:
        *m_process << "-t" << "vcd2";
    break;
  }

  kdDebug() << QString("(K3bVcdJob) xmlfile = \"%1\"").arg(QFile::encodeName(m_xmlFile)) << endl;
  *m_process << "-o" << QString("%1").arg(QFile::encodeName(m_xmlFile));

  // Add Tracks to XML
  QListIterator<K3bVcdTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    kdDebug() << QString("%1").arg(QFile::encodeName(it.current()->absPath())) << endl;
    *m_process << QString("%1").arg(QFile::encodeName(it.current()->absPath()));
  }

  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
    this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
    this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
    this, SLOT(slotVcdxGenFinished()) );


  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    kdDebug() << "(K3bVcdJob) could not start vcdxgen" << endl;
    emit infoMessage( i18n("Could not start vcdxgen!"), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
  }
}

void K3bVcdJob::slotVcdxGenFinished()
{
  if( m_process->normalExit() ) {
    if( !QFile::exists( m_xmlFile ) ){
      kdDebug() << QString("(K3bVcdJob) Could not write XML-file.").arg(m_xmlFile) << endl;
      emit infoMessage( i18n("Could not write correct XML-file."), K3bJob::ERROR );
      emit finished( false );
      return;
    }

    // TODO: check the process' exitStatus()
    switch( m_process->exitStatus() ) {
      case 0:
        emit infoMessage( i18n("XML-file successfully created"), K3bJob::STATUS );
        break;
    default:
      // no recording device and also other errors!! :-(
      emit infoMessage( i18n("vcdxgen returned an error! (code %1)").arg(m_process->exitStatus()), K3bJob::ERROR );
      emit infoMessage( i18n("No error handling yet!"), K3bJob::ERROR );
      emit infoMessage( i18n("Please send me an email with the last output..."), K3bJob::ERROR );
      emit finished( false );
      break;
    }
  }
  else {
    emit infoMessage( i18n("vcdxgen not exit cleanly."), K3bJob::ERROR );
    emit finished( false );
  }
  this->vcdxBuild();
}


void K3bVcdJob::vcdxBuild()
{
  /*
  Usage: vcdxbuild [OPTION...]
    -i, --image-type=TYPE          specify image type for output (default:
                                   'bincue')
    -o, --image-option=KEY=VALUE   specify image option
    -c, --cue-file=FILE            specify cue file for output (default:
                                   'videocd.cue')
    -b, --bin-file=FILE            specify bin file for output (default:
                                   'videocd.bin')
    --cdrdao-file=FILE             specify cdrdao-style image filename base
    --sector-2336                  use 2336 byte sectors for output
    -p, --progress                 show progress
    -v, --verbose                  be verbose
    -q, --quiet                    show only critical messages
    --gui                          enable GUI mode
    -V, --version                  display version and copyright information and
                                   exit
  */

  m_stage = stageUnknown;
  firstTrack = true;
  delete m_process;
  m_process = new KProcess();

  emit infoMessage( i18n("Create Cue/Bin files ..."), K3bJob::STATUS );
  if( !k3bMain()->externalBinManager()->foundBin( "vcdxbuild" ) ) {
    kdDebug() << "(K3bVcdJob) could not find vcdxbuild executable" << endl;
    emit infoMessage( i18n("vcdxbuild executable not found."), K3bJob::ERROR );
    cancelAll();
    return;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "vcdxbuild" );

  *m_process << "--progress" << "--gui";

  *m_process << QString("--cue-file=%1").arg( m_cueFile );

  *m_process << QString("--bin-file=%1").arg( m_doc->vcdImage() );

  *m_process << QString("%1").arg(QFile::encodeName(m_xmlFile));;

  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
    this, SLOT(slotParseVcdxBuildOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
    this, SLOT(slotParseVcdxBuildOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
    this, SLOT(slotVcdxBuildFinished()) );


  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    kdDebug() << "(K3bVcdJob) could not start vcdxbuild" << endl;
    emit infoMessage( i18n("Could not start vcdxbuild!"), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
  }
}

void K3bVcdJob::slotParseVcdxBuildOutput( KProcess*, char* output, int len )
{
  QString buffer = QString::fromLocal8Bit( output, len );

  // split to lines
  QStringList lines = QStringList::split( "\n", buffer );

  QDomDocument xml_doc;
  QDomElement xml_root;

  // do every line
  for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
    *str = (*str).stripWhiteSpace();

    emit debuggingOutput( "vcdxbuild", *str );

    xml_doc.setContent(QString("<?xml version='1.0'?><vcdxbuild>") + *str + "</vcdxbuild>");

    xml_root = xml_doc.documentElement();

    // There should be only one... but ...
    for (QDomNode node = xml_root.firstChild(); !node.isNull(); node = node.nextSibling()) {
      QDomElement el = node.toElement();
      if (el.isNull())
        continue;

      const QString tagName = el.tagName().lower();

      if (tagName == "progress") {
        const QString oper = el.attribute("operation").lower();
        const long long pos = el.attribute("position").toLong();
        const long long size = el.attribute("size").toLong();

        if (oper == "scan") {
          // Scan Video Files
          if (m_stage == stageUnknown || pos < m_bytesFinished) {
            const uint index = el.attribute("id").replace(QRegExp("sequence-"), "").toUInt();

            m_currentWrittenTrack = m_doc->at( m_currentWrittenTrackNumber );
            emit newSubTask( i18n("Scanning video file %1 of %2 (%3)").arg(index + 1).arg( doc()->numOfTracks()).arg(m_currentWrittenTrack->fileName()) );
            m_bytesFinished = 0;

            if(!firstTrack) {
              m_bytesFinishedTracks += m_doc->at(m_currentWrittenTrackNumber)->size();
              m_currentWrittenTrackNumber++;
            }
            else
              firstTrack = false;
          }
          emit subPercent( (int) (100.0 * (double)pos / (double)size) );
          emit processedSubSize( pos/1024/1024, size/1024/1024 );

          // this is the first of three processes.
          double relOverallWritten = ( (double)m_bytesFinishedTracks + (double)pos ) / (double)doc()->size();
          emit percent( (int)(33.0 * relOverallWritten)  );

          m_bytesFinished = pos;
          m_stage = stageScan;

        }
        else if (oper == "write") {
          emit subPercent( (int) (100.0 * (double)pos / (double)size) );
          emit processedSubSize( (pos*2352)/1024/1024, (size*2352)/1024/1024 );
          emit percent( 33 + (int) (33.0 * (double)pos / (double)size) );

          m_stage = stageWrite;
        }
        else {
          return;
        }
      }
      else if (tagName == "log") {
        QDomText tel = el.firstChild().toText();
        const QString level = el.attribute("level").lower();
        if (tel.isText()) {
          const QString text = tel.data();
          if (m_stage == stageWrite && level == "information")
            kdDebug() << QString("(K3bVcdJob) VcdxBuild information, %1").arg(text) << endl;
            if( (text).startsWith( "writing track" ) )
              emit newSubTask( i18n("Creating Image for track %1").arg((text).mid(14)) );
          else {
            if (level != "error") {
              kdDebug() << QString("(K3bVcdJob) vcdxbuild warning, %1").arg(text) << endl;
              emit debuggingOutput( "vcdxbuild", text );
            }
            else {
              kdDebug() << QString("(K3bVcdJob) vcdxbuild error, %1").arg(text) << endl;
              emit infoMessage( text, K3bJob::ERROR );
            }
          }
        }
      }
    }
  }
}


void K3bVcdJob::slotVcdxBuildFinished()
{
  if( m_process->normalExit() ) {
    // TODO: check the process' exitStatus()
    switch( m_process->exitStatus() ) {
      case 0:
        emit infoMessage( i18n("Cue/Bin files successfully created."), K3bJob::STATUS );
        break;
    default:
      emit infoMessage( i18n("vcdxbuild returned an error! (code %1)").arg(m_process->exitStatus()), K3bJob::ERROR );
      emit infoMessage( i18n("No error handling yet!"), K3bJob::ERROR );
      emit infoMessage( i18n("Please send me an email with the last output..."), K3bJob::ERROR );
      emit finished( false );
      break;
    }
  }
  else {
    emit infoMessage( i18n("vcdxbuild not exit cleanly."), K3bJob::ERROR );
    emit finished( false );
  }

  //remove xml-file
  if( QFile::exists( m_xmlFile ) )
    QFile::remove( m_xmlFile );

  this->cdrdaoWrite();
}


void K3bVcdJob::slotCollectOutput( KProcess*, char* output, int len )
{
  emit debuggingOutput( "vcdimager", QString::fromLocal8Bit( output, len ) );

  m_collectedOutput += QString::fromLocal8Bit( output, len );
}

// cdrdao stuff
void K3bVcdJob::cdrdaoWrite()
{
  if( !k3bMain()->externalBinManager()->foundBin( "cdrdao" ) ) {
    kdDebug() << "(K3bVcdJob) could not find cdrdao executable" << endl;
    emit infoMessage( i18n("cdrdao executable not found."), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
    return;
  }

  K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );

  if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
    cancel();
    return;
  }

  delete m_process;
  m_process = new KShellProcess();
  *m_process << k3bMain()->externalBinManager()->binPath( "cdrdao" );
  *m_process << "write";

  // device
  // TODO: check if device is in use and throw exception if so

  *m_process << "--device" << m_doc->burner()->busTargetLun();
  if( m_doc->burner()->cdrdaoDriver() != "auto" ) {
    *m_process << "--driver";
    if( m_doc->burner()->cdTextCapable() == 1 )
      *m_process << QString("%1:0x00000010").arg( m_doc->burner()->cdrdaoDriver() );
    else
      *m_process << m_doc->burner()->cdrdaoDriver();
  }

  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "cdrdao parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_process << *it;

  k3bMain()->config()->setGroup( "General Options" );
  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << "--buffers" << QString::number( k3bMain()->config()->readNumEntry( "Cdrdao buffer", 32 ) );
  }
  bool overburn = k3bMain()->config()->readBoolEntry( "Allow overburning", false );

  if( overburn )
    *m_process << "--overburn";

  if( m_doc->dummy() )
    *m_process << "--simulate";
  if( k3bMain()->eject() )
    *m_process << "--eject";

  // writing speed
  *m_process << "--speed" << QString::number(  m_doc->speed() );

  // supress the 10 seconds gap to the writing
  *m_process << "-n";

  // cue-file
  kdDebug() << QString("(K3bVcdJob) cdrdaoWrite cuefile = %1").arg(m_cueFile) << endl;
  *m_process << QString("\"%1\"").arg(QFile::encodeName( m_cueFile ));

  // connect to the cdrdao slots
  connect( m_process, SIGNAL(processExited(KProcess*)),
    this, SLOT(slotCdrdaoFinished()) );
  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
    this, SLOT(parseCdrdaoOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
    this, SLOT(parseCdrdaoOutput(KProcess*, char*, int)) );

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::All ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bVcdJob) could not start cdrdao" << endl;

    emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
    return;
  }

  kdDebug() << "(K3bVcdJob) write process started!" << endl;

  if( m_doc->dummy() )
    emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS );
  else
    emit infoMessage( i18n("Starting recording at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS );
}

void K3bVcdJob::createCdrdaoProgress( int made, int size )
{
  if( size == 0 ) {
    kdDebug() << "(K3bVcdJob) got progress: " << made << ", " << size << endl;
    return;
  }
  
  emit processedSubSize( made, size );
  emit subPercent( 100*made / size );
  emit processedSize( made, size );
  emit percent( 66 + (34*made / size ) );
}

void K3bVcdJob::slotCdrdaoFinished()
{
  if( m_process->normalExit() ) {
      // TODO: check the process' exitStatus()
      switch( m_process->exitStatus() ) {
        case 0:
          if( doc()->dummy() )
            emit infoMessage( i18n("Simulation successfully completed"), K3bJob::STATUS );
          else
            emit infoMessage( i18n("Writing successfully completed"), K3bJob::STATUS );

          emit finished( true );
          break;

        default:
          // no recording device and also other errors!! :-(
          emit infoMessage( i18n("cdrdao returned an error!"), K3bJob::ERROR );
          emit infoMessage( i18n("Error handling not implemented yet!"), K3bJob::ERROR );
          emit infoMessage( i18n("Please send an email to the author with the last output..."), K3bJob::ERROR );

          cancelAll();
          emit finished( false );
          return;
      }
  }
  else {
    emit infoMessage( i18n("cdrdao did not exit cleanly!"), K3bJob::ERROR );

    cancelAll();
    emit finished( false );
    return;
  }

  m_process->disconnect(this);

}

#include "k3bvcdjob.moc"

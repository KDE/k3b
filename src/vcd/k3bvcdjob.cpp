/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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


#include "k3bvcdjob.h"

#include "k3bvcddoc.h"
#include "k3bvcdtrack.h"
#include "k3bvcdxmlview.h"

#include <k3b.h>
#include <k3bdoc.h>
#include <k3bprocess.h>
#include <k3bemptydiscwaiter.h>
#include <device/k3bdevice.h>
#include <tools/k3bexternalbinmanager.h>
#include <tools/k3bglobals.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>

#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <ktempfile.h>
#include <kio/global.h>

#include <qstring.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qtimer.h>
#include <kdebug.h>
#include <qregexp.h>
#include <qdom.h>


K3bVcdJob::K3bVcdJob( K3bVcdDoc* doc, QObject* parent, const char* name )
  : K3bBurnJob(parent, name)
{
  m_doc = doc;
  m_process = 0;
  m_currentWrittenTrackNumber = 0;
  m_bytesFinishedTracks = 0;
  m_writerJob = 0;
  m_createimageonlypercent = 33.3;
  m_imageFinished = false;
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
  m_canceled = true;
  
  if( m_writerJob )
    m_writerJob->cancel();

  if( m_process->isRunning() ) {
    m_process->disconnect(this);
    m_process->kill();
  }

  // remove bin-file if it is unfinished or the user selected to remove image
  if( QFile::exists( m_doc->vcdImage() ) ) {
    if( !m_doc->onTheFly() && m_doc->deleteImage() || !m_imageFinished ) {
      emit infoMessage( i18n("Removing Binary file %1").arg(m_doc->vcdImage()), K3bJob::STATUS );
      QFile::remove( m_doc->vcdImage() );
      m_doc->setVcdImage("");
    }
  }

  // remove cue-file if it is unfinished or the user selected to remove image
  if( QFile::exists( m_cueFile ) ) {
    if( !m_doc->onTheFly() && m_doc->deleteImage() || !m_imageFinished ) {
      emit infoMessage( i18n("Removing Cue file %1").arg(m_cueFile), K3bJob::STATUS );
      QFile::remove( m_cueFile );
      m_cueFile = "";
    }
  }
}


void K3bVcdJob::start()
{
  kdDebug() << "(K3bVcdJob) starting job" << endl;

  emit started();
  m_canceled = false;
  
  int pos = QString(m_doc->vcdImage()).find(".bin", QString(m_doc->vcdImage()).length() - 4);
  if (pos > 0) {
    m_cueFile = m_doc->vcdImage().left(pos) + ".cue";
  }
  else {
    m_cueFile = m_doc->vcdImage() + ".cue";
    m_doc->setVcdImage(m_doc->vcdImage() + ".bin");
  }

  if ( vcdDoc()->onlyCreateImage())
    m_createimageonlypercent = 50.0;

  // vcdxGen();
  xmlGen();
}

void K3bVcdJob::xmlGen()
{
  
  KTempFile tempF;
  m_xmlFile = tempF.name();
  tempF.unlink();

  emit infoMessage( i18n("Create XML-file"), K3bJob::INFO );
  
  K3bVcdXmlView xmlView( m_doc );
  
  if( !xmlView.write( m_xmlFile )) {
    kdDebug() << "(K3bVcdJob) could not write xmlfile." << endl;
    emit infoMessage( i18n("Could not write correct XML-file."), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
  }
  
  emit infoMessage( i18n("XML-file successfully created"), K3bJob::STATUS );
  
  vcdxBuild();
  
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

  emit infoMessage( i18n("Create Cue/Bin files ..."), K3bJob::INFO );
  if( !k3bMain()->externalBinManager()->foundBin( "vcdxbuild" ) ) {
    kdDebug() << "(K3bVcdJob) could not find vcdxbuild executable" << endl;
    emit infoMessage( i18n("vcdxbuild executable not found."), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
    return;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "vcdxbuild" );

  // additional user parameters from config
  QStringList params = k3bMain()->externalBinManager()->program( "vcdxbuild" )->userParameters();
  for( QStringList::Iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;


  if ( vcdDoc()->vcdOptions()->Sector2336() ) {
    kdDebug() << "(K3bVcdJob) Write 2336 Sectors = on" << endl;
    *m_process << "--sector-2336";
  }

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
          emit percent( (int)(m_createimageonlypercent * relOverallWritten)  );

          m_bytesFinished = pos;
          m_stage = stageScan;

        }
        else if (oper == "write") {
          emit subPercent( (int) (100.0 * (double)pos / (double)size) );
          emit processedSubSize( (pos*2048)/1024/1024, (size*2048)/1024/1024 );
          emit percent( (int) (m_createimageonlypercent + (m_createimageonlypercent * (double)pos / (double)size)) );

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
        m_imageFinished = true;
        break;
    default:
      emit infoMessage( i18n("vcdxbuild returned an error! (code %1)").arg(m_process->exitStatus()), K3bJob::ERROR );
      emit infoMessage( i18n("No error handling yet!"), K3bJob::ERROR );
      emit infoMessage( i18n("Please send me an email with the last output..."), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
      return;
    }
  }
  else {
    emit infoMessage( i18n("vcdxbuild not exit cleanly."), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
    return;
  }

  //remove xml-file
  if( QFile::exists( m_xmlFile ) )
    QFile::remove( m_xmlFile );

  kdDebug() << QString("(K3bVcdJob) create only image: %1").arg(vcdDoc()->onlyCreateImage()) << endl;
  if ( !vcdDoc()->onlyCreateImage() ) {
    kdDebug() << "(K3bVcdJob) start writing" << endl;
    if( prepareWriterJob() ) {
      K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );
      if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
        cancel();
        return;
      }
      m_writerJob->start();
    }
  }
  else {
    emit finished( true );
  }
}

bool K3bVcdJob::prepareWriterJob()
{
  if( m_writerJob )
    delete m_writerJob;

  if( writingApp() == K3b::CDRDAO || writingApp() == K3b::DEFAULT ) {
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_doc->burner(), this );
    // create cdrdao job

    writer->setSimulate( m_doc->dummy() );
    writer->setBurnSpeed( m_doc->speed() );

    if( m_doc->onTheFly() ) {
      writer->setProvideStdin(true);
    }

    writer->setTocFile( m_cueFile );

    m_writerJob = writer;

  }
  else {
    cancelAll();
    emit finished( false );
    return false;
  }

  connect( m_writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( m_writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writerJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_writerJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_writerJob, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
  connect( m_writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writerJob, SIGNAL(writeSpeed(int)), this, SIGNAL(writeSpeed(int)) );
  connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
  connect( m_writerJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)), this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}

void K3bVcdJob::slotWriterJobPercent( int p )
{
  emit percent( (int)(66.6 + p / 3) );
}

void K3bVcdJob::slotWriterNextTrack( int t, int tt )
{
  emit newSubTask( i18n("Writing Track %1 of %2").arg(t).arg(tt) );
}

void K3bVcdJob::slotWriterJobFinished( bool success )
{
  if( m_canceled )
    return;

  // remove bin-file if it is unfinished or the user selected to remove image
  if( QFile::exists( m_doc->vcdImage() ) ) {
    if( !m_doc->onTheFly() && m_doc->deleteImage() || !m_imageFinished ) {
      emit infoMessage( i18n("Removing Binary file %1").arg(m_doc->vcdImage()), K3bJob::STATUS );
      QFile::remove( m_doc->vcdImage() );
      m_doc->setVcdImage("");
    }
  }

  // remove cue-file if it is unfinished or the user selected to remove image
  if( QFile::exists( m_cueFile ) ) {
    if( !m_doc->onTheFly() && m_doc->deleteImage() || !m_imageFinished ) {
      emit infoMessage( i18n("Removing Cue file %1").arg(m_cueFile), K3bJob::STATUS );
      QFile::remove( m_cueFile );
      m_cueFile = "";
    }
  }

  if( success ) {
    // allright
    // the writerJob should have emited the "simulation/writing successful" signal
    emit finished(true);
  }
  else {
    cancelAll();
    emit finished(false);
  }
}


QString K3bVcdJob::jobDescription() const
{
  switch( m_doc->vcdType() ) {
  case K3bVcdDoc::VCD11:
    return i18n("Writing Video CD (Version 1.1)");
  case K3bVcdDoc::VCD20:
    return i18n("Writing Video CD (Version 2.0)");
  case K3bVcdDoc::SVCD10:
    return i18n("Writing Super Video CD");
  case K3bVcdDoc::HQVCD:
    return i18n("Writing High Quality Video CD");
  default:
    return i18n("Writing Video CD");
  }
}


QString K3bVcdJob::jobDetails() const
{
  return i18n("1 MPEG (%1)", "%n MPEGs (%1)", m_doc->tracks()->count()).arg(KIO::convertSize(m_doc->size()));
}

#include "k3bvcdjob.moc"

/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
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


#include "k3bdivxencodingprocess.h"
#include "k3bdivxcodecdata.h"
#include <k3bexternalbinmanager.h>
#include <k3bcore.h>

#include <qdir.h>

#include <kprocess.h>
#include <klocale.h>
#include <kdebug.h>
#include <kapplication.h>


K3bDivXEncodingProcess::K3bDivXEncodingProcess( K3bDivxCodecData *data, QWidget* parent, const char *name )
  : K3bJob( parent, name ),
    m_process(0) {
  m_data = data;
}

K3bDivXEncodingProcess::~K3bDivXEncodingProcess() 
{
  if( m_process ) delete m_process;
}

void K3bDivXEncodingProcess::start() {
  m_pass = 0;
  m_speedFlag = 1;
  m_speedTrigger = 0;
  m_speedInitialFlag = 0;
  m_interalInterrupt = false;
  emit started();
  checkVobDirectory(); // checks if other files than .vob are in vob dir
}
void K3bDivXEncodingProcess::slotFinishedCheckVobDirectory() {
    if( m_data->isTcDvdMode() || ( m_data->isNormalize() && !m_data->isAc3Set() ) ) {
	    kdDebug() << "(K3bDivXEncodingProcess) Start transcode in dvd mode or mp3 audio gain mode." << endl;
	    copyIfos(); // old: code directly from slotFinishedCopyIfos, before change to transcode -x dvd
    } else {
        deleteIfos();
    }
}

void K3bDivXEncodingProcess::slotFinishedCopyIfos(KIO::Job* job){
    if ( job->error() > 0 ) {
        kdDebug() << "(K3bDivXEncodingProcess) Job error during copy: " << job->errorString() << endl;
    }
    if ( m_data->getParaAc3().length() > 1 ) {
        // use ac3
        //deleteIfos(); // for using with transcode -x vob
        slotStartEncoding(); 
    } else if ( m_data->isNormalize() ) {
        infoMessage( i18n( "Copy IFO files to vob directory." ), INFO );
        slotStartAudioProcessing(); // -> deleteIfos -> slotStartEncoding
        //copyIfos(); // for using with transcode -x vob
    }
}

void K3bDivXEncodingProcess::slotStartAudioProcessing( ) {
    kdDebug() << "(K3bDivXEncodingProcess) Run transcode." << endl;
    if( m_process ) delete m_process;
    m_process = new KShellProcess;
    const K3bExternalBin *tccatBin = k3bcore->externalBinManager() ->binObject( "tccat" );
    const K3bExternalBin *tcextractBin = k3bcore->externalBinManager() ->binObject( "tcextract" );
    const K3bExternalBin *tcdecodeBin = k3bcore->externalBinManager() ->binObject( "tcdecode" );
    const K3bExternalBin *tcscanBin = k3bcore->externalBinManager() ->binObject( "tcscan" );
    // parse audio for   gain to normalize
    *m_process << "nice" << "-10";
    *m_process << tccatBin->path << " -i" << m_data->getProjectDir() + "/vob" << "-t" << "vob" << "-P" << m_data->getTitle();
    *m_process << "|" << tcextractBin->path << m_data->getParaAudioLanguage() << "-x" << "ac3" << "-t" << "vob";
    *m_process << "|" << tcdecodeBin->path << "-x" << "ac3";
    *m_process << "|" << tcscanBin->path << "-x" << "pcm";

  kdDebug() << "(K3bDivXEncodingProcess)" + tccatBin->path + " -i " + m_data->getProjectDir() + "/vob -t vob -P " + m_data->getTitle()
    + " | " + tcextractBin->path + m_data->getParaAudioLanguage() + " -x ac3  -t vob "
    + " | " + tcdecodeBin->path + " -x ac3 " + " | " + tcscanBin->path + " -x pcm";

  connect( m_process, SIGNAL( receivedStdout( KProcess*, char*, int ) ),
	   this, SLOT( slotParseAudio( KProcess*, char*, int ) ) );
  connect( m_process, SIGNAL( receivedStderr( KProcess*, char*, int ) ),
	   this, SLOT( slotParseAudio( KProcess*, char*, int ) ) );

  connect( m_process, SIGNAL( processExited( KProcess* ) ), this, SLOT( slotAudioExited( KProcess* ) ) );

  if ( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    kdDebug() << "Error audio process starting" << endl;
  }
  //emit started();
  //emit newTask( i18n("Generating video")  );
  emit newSubTask( i18n( "Preprocessing audio" ) );
  infoMessage( i18n( "Search for maximum audio gain to get normalized parameter." ), INFO );
  kdDebug() << "(K3bDivXEncodingProcess) Starting get audio gain." << endl;
}

void K3bDivXEncodingProcess::slotStartEncoding() {
    m_speedFlag = 301;
    m_speedTrigger = 300;
    m_speedInitialFlag = 0;
    QString debugPass( "" );
    kdDebug() << "(K3bDivXEncodingProcess) Run transcode." << endl;
    const K3bExternalBin *transcodeBin = k3bcore->externalBinManager() ->binObject( "transcode" );
    if( m_process ) delete m_process;
    m_process = new KShellProcess;

    *m_process << "nice" << "-10";
    *m_process << transcodeBin->path; //"/usr/local/bin/transcode -i ";
    *m_process << " -i" << m_data->getProjectDir() + "/vob ";
    // -x vob
    if( m_data->isTcDvdMode() ){
  	*m_process << "-x" << "dvd";
	debugPass +="-x dvd ";
    } else {	
	*m_process << "-x" << "vob";
	debugPass +="-x vob ";
    }	
    *m_process << "-T" << m_data->getTitle() + ",-1,1" << m_data->getParaVideoBitrate() + "," + m_data->getKeyframes() + "," + m_data->getCrispness();
    kdDebug() << "(K3bDivXEncodingProcess) Video: " + m_data->getParaVideoBitrate() + "," + m_data->getKeyframes() + "," + m_data->getCrispness() << endl;
    *m_process << m_data->getParaYuv() << m_data->getParaAudioLanguage();
    kdDebug() << "(K3bDivXEncodingProcess) Video: " + m_data->getParaYuv() + m_data->getParaAudioLanguage() << endl;
    if ( m_data->getParaAc3().length() > 1 ) {
        *m_process << m_data->getParaAc3();
        debugPass += m_data->getParaAc3();
    } else {
        *m_process << m_data->getParaAudioResample() << m_data->getParaAudioBitrate() << m_data->getParaAudioGain();
        debugPass = debugPass + m_data->getParaAudioResample() + m_data->getParaAudioBitrate() + m_data->getParaAudioGain();
    }
    *m_process << m_data->getParaDeinterlace();
    kdDebug() << "(K3bDivXEncodingProcess)  Out: " + m_data->getParaDeinterlace() + " -o " + m_data->getAviFile() << endl;

    int top = m_data->getCropTop();
    int left = m_data->getCropLeft();
    int bottom = m_data->getCropBottom();
    int right = m_data->getCropRight();
    kdDebug() << "(K3bDivXEncodingProcess) Crop values t=" << top << ",l=" << left << ",b=" << bottom << ",r=" << right << endl;
    kdDebug() << "(K3bDivXEncodingProcess) Resize factor height=" << m_data->getResizeHeight() << ", width=" << m_data->getResizeWidth() << endl;
    *m_process << "-j" << QString::number( top ) + "," + QString::number( left ) + "," + QString::number( bottom ) + "," + QString::number( right );
    *m_process << "-B" << QString::number( m_data->getResizeHeight() ) + "," + QString::number( m_data->getResizeWidth() ) + ",8";
    if ( m_data->getCodecMode() == 2 ) {
        if ( m_pass == 0 ) {
            m_pass = 1;
            *m_process << " -o /dev/null";
            *m_process << " -R 1," + m_data->getProjectDir() + "/tmp/divx4.log" << m_data->getParaCodec() << ",null";
            debugPass += " -R 1," + m_data->getProjectDir() + "/tmp/divx4.log" + m_data->getParaCodec() + ",null -o /dev/null";
        } else {
            m_pass = 2;
            *m_process << " -o " << KShellProcess::quote( m_data->getAviFile() );
            *m_process << " -R 2," + m_data->getProjectDir() + "/tmp/divx4.log" << m_data->getParaCodec();
            debugPass += " -R 2," + m_data->getProjectDir() + "/tmp/divx4.log" + m_data->getParaCodec() + "-o " + m_data->getAviFile();
        }
    } else {
        *m_process << " -o " << KShellProcess::quote( m_data->getAviFile() );
        *m_process << m_data->getParaCodec();
        debugPass += m_data->getParaCodec();
    }
    // -x vob
    kdDebug() << "(K3bDivXEncodingProcess)" + transcodeBin->path + " -i " + m_data->getProjectDir() +
    "/vob -T" + m_data->getTitle() + ",-1,1" +
    m_data->getParaVideoBitrate() + "," + m_data->getKeyframes() + "," + m_data->getCrispness() +
    m_data->getParaYuv() + m_data->getParaAudioLanguage() +
    m_data->getParaDeinterlace() + " -j " + QString::number( top ) + "," + QString::number( left ) + "," + QString::number( bottom ) + 
    "," + QString::number( right ) + " -B " + QString::number( m_data->getResizeHeight() ) + "," + QString::number( m_data->getResizeWidth() ) + ",8" + debugPass << endl;
    
    connect( m_process, SIGNAL( receivedStdout( KProcess*, char*, int ) ),
             this, SLOT( slotParseEncoding( KProcess*, char*, int ) ) );
    connect( m_process, SIGNAL( receivedStderr( KProcess*, char*, int ) ),
             this, SLOT( slotParseEncoding( KProcess*, char*, int ) ) );

    connect( m_process, SIGNAL( processExited( KProcess* ) ), this, SLOT( slotEncodingExited( KProcess* ) ) );

    if ( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
        kdDebug() << "(K3bDivXEncodingProcess) Error process starting" << endl;
    }
    //emit started();
    if ( m_pass == 1 ) {
        infoMessage( i18n( "Start first pass of video encoding." ), PROCESS );
        emit newSubTask( i18n( "Encoding video (Pass 1)" ) );
    } else if ( m_pass == 2 ) {
        infoMessage( i18n( "Start second pass of video encoding." ), PROCESS );
        emit newSubTask( i18n( "Encoding video (Pass 2)" ) );
    } else {
        infoMessage( i18n( "Start video encoding." ), PROCESS );
        emit newSubTask( i18n( "Encoding video" ) );
    }
    kdDebug() << "(K3bDivXEncodingProcess) Starting encoding." << endl;
}

void K3bDivXEncodingProcess::copyIfos() {
    QDir tmp( m_data->getProjectDir() + "/tmp" );
    if ( tmp.exists() ) {
        QStringList ifos = tmp.entryList( "*.ifo" );
        for ( QStringList::Iterator it = ifos.begin(); it != ifos.end(); ++it ) {
            ( *it ) = m_data->getProjectDir() + "/tmp/" + ( *it );
        }
        KURL::List ifoList( ifos );
        KURL dest( m_data->getProjectDir() + "/vob" );
        // old slot: slotStartAudioProcessing
        connect( KIO::copy( ifoList, dest, false ), SIGNAL( result( KIO::Job * ) ), this, SLOT( slotFinishedCopyIfos( KIO::Job* ) ) );
        kdDebug() << "(K3bDivxEncodingProcess) Copy IFO files to " << dest.path() << endl;
    } else {
        kdDebug() << "(K3bDivXEncodingProcess) Can't find ifo files in tmp directory." << endl;
    }
}

void K3bDivXEncodingProcess::deleteIfos() {
  // delete ifos
  QDir vobs( m_data->getProjectDir() + "/vob" );
  if ( vobs.exists() ) {
    QStringList ifos = vobs.entryList( "*.ifo" );
    for ( QStringList::Iterator it = ifos.begin(); it != ifos.end(); ++it ) {
      ( *it ) = m_data->getProjectDir() + "/vob/" + ( *it );
    }
    KURL::List ifoList( ifos );
    connect( KIO::del( ifoList, false, false ), SIGNAL( result( KIO::Job * ) ), this, SLOT( slotStartEncoding( ) ) );
    kdDebug() << "(K3bDivxEncodingProcess) Delete IFO files in " << vobs.path() << endl;
  }
}

void K3bDivXEncodingProcess::cancel( ) {
  kdDebug() << "(K3bDivXEncodingProcess) Kill shell process." << endl;
  m_process->kill( 9 ); // send SIGTERM doesn't work (?), use now SIGKILL
}

void K3bDivXEncodingProcess::slotParseEncoding( KProcess *, char *buffer, int len ) {
  if ( m_speedFlag > m_speedTrigger ) {
    if ( m_speedInitialFlag < 50 ) {
      m_speedTrigger = 0;
      m_speedInitialFlag++;
      if ( m_speedInitialFlag == 50 )
	m_speedTrigger = 400;
    }
    m_speedFlag = 0;
    QString tmp = QString::fromLocal8Bit( buffer, len );
    if ( tmp.contains( "file read error" ) ) {
      infoMessage( i18n( "Starting transcode failed. K3b hasn't successful backuped unused files.Verify that there are no other files than *.vob-files in %1. K3b should have backuped all files in %2. " ).arg( m_data->getProjectDir() + "/vob" ).arg( m_data->getProjectDir() + "/tmp" ), K3bJob::ERROR );
      cancel( );
      m_interalInterrupt = true;
      return ;
    }
    m_debugBuffer += tmp;
    kdDebug() << tmp << endl;
    int index = tmp.find( "]" );
    tmp = tmp.mid( index - 6, 6 ).stripWhiteSpace();
    long f = tmp.toLong();
    int p = f * 100 / m_data->getFramesValue();
    emit subPercent( p );
    if ( m_pass == 1 ) {
      emit percent( (int)(( (double)p * 47.5 / 100.0 )) + 5 );
    } else if ( m_pass == 2 ) {
      emit percent( (int)(( (double)p * 47.5 / 100.0 ) + 52.5) );
    } else {
      emit percent( (int)( (double)p * 95.0 / 100.0 ) + 5 );
    }
  }
  m_speedFlag++;
}

void K3bDivXEncodingProcess::slotEncodingExited( KProcess *p ) {
  kdDebug() << "(K3bDivxEncodingProcess) Encoding finished" << endl;
  if ( m_interalInterrupt ) {
    restoreBackupFiles();
    emit debuggingOutput( "Videoencoding (transcode)", m_debugBuffer );
    emit finished( false );
  } else if ( !p->normalExit() ) {
    infoMessage( i18n( "Video generation aborted by user." ), STATUS );
    kdDebug() << "(K3bDivxEncodingProcess) Aborted encoding" << endl;
    restoreBackupFiles();
    emit debuggingOutput( "Videoencoding (transcode)", m_debugBuffer );
    emit finished( false );
  } else {
    if ( m_pass == 1 ) {
      kdDebug() << "(K3bDivxEncodingProcess) Start second pass." << endl;
      slotStartEncoding();
    } else {
        if ( m_pass == 1 ) { // one pass mode is m_pass=0
            kdDebug() << "(K3bDivxEncodingProcess) Start second pass." << endl;
            slotStartEncoding();
        } else {
            emit debuggingOutput( "videoencoding (transcode)", m_debugBuffer );
            restoreBackupFiles();
            if ( m_data->isShutdown() ) {
                kdDebug() << "(K3bDivxEncodingProcess) Encoding finished. Shutting down now." << endl;
                if ( !shutdown() ) {
                    infoMessage( i18n( "Couldn't shutdown the system." ), ERROR );
                    emit finished( true );
                }
            } else {
                infoMessage( i18n( "Video generating successful finished." ), STATUS );
                emit finished( true );
            }
            infoMessage( i18n("Video generating successfully finished."), STATUS );
            emit debuggingOutput("videoencoding (transcode)", m_debugBuffer);
            emit finished( true );
        }
    }
  }
}

void K3bDivXEncodingProcess::slotParseAudio( KProcess*, char * buffer, int len ) {
  if ( m_speedFlag > m_speedTrigger ) {
    m_speedFlag = 0;
    if ( m_speedInitialFlag < 50 ) {
      m_speedTrigger = 0;
      m_speedInitialFlag++;
      if ( m_speedInitialFlag == 50 )
	m_speedTrigger = 40;
    }
    QString tmp = QString::fromLocal8Bit( buffer, len );
    m_debugBuffer += tmp;
    if ( tmp.contains( "rescale" ) ) {
      int scale = tmp.find( "rescale" );
      tmp = tmp.mid( scale + 8 ).stripWhiteSpace();
      float f = tmp.toFloat();
      m_data->setAudioGain( QString::number( f, 'f', 3 ) );
      kdDebug() << "K3bDivxEncodingProcess) Audio gain: " + m_data->getAudioGain() << endl;
      infoMessage( i18n( "Gain for normalizing is: %1" ).arg( m_data->getAudioGain() ), INFO );
    }
    kdDebug() << "(K3bDivxEncodingProcess) Audio gain output: " << tmp << endl;
    int index = tmp.find( "%" );
    tmp = tmp.mid( index - 5, 5 ).stripWhiteSpace();
    float f = tmp.toFloat() + 0.5;
    if ( f > 95 )
      m_speedTrigger = 0;
    int p = ( int ) f;
    emit subPercent( p );
    emit percent( p / 20 );
  }
  m_speedFlag++;
}
void K3bDivXEncodingProcess::slotAudioExited( KProcess * p ) {
  infoMessage( i18n( "Preprocessing audio completed." ), STATUS );
  kdDebug() << "(K3bDivxEncodingProcess) Audio gain detection finished" << endl;
  if ( p->normalExit() ) {
    deleteIfos(); // delete ifos and slotStartEncoding( );
  } else {
    slotEncodingExited( p );
  }
}


bool K3bDivXEncodingProcess::shutdown() {
  // not sure if KApplication::ShutdownModeSchedule is the one to use...
  return kapp->requestShutDown( KApplication::ShutdownConfirmNo, 
				KApplication::ShutdownTypeHalt, 
				KApplication::ShutdownModeInteractive );
}

void K3bDivXEncodingProcess::checkVobDirectory() {
  QDir vobs( m_data->getProjectDir() + "/vob" );
  if ( vobs.exists() ) {
    kdDebug() << "(K3bDivXEncodingProcess) Backup files in vob dir" << endl;
    QStringList movefiles;
    m_movefiles.clear();
    QStringList files = vobs.entryList( QDir::Files );
    for ( QStringList::Iterator it = files.begin(); it != files.end(); ++it ) {
      kdDebug() << "(K3bDivXEncodingProcess) Check file <" << ( *it ) << ">." << endl;
      if ( !( ( *it ).endsWith( ".vob" ) ) ) {
	kdDebug() << "(K3bDivXEncodingProcess) Backup file <" << ( *it ) << ">." << endl;
	infoMessage( i18n( "Backup file %1 to %2." ).arg( m_data->getProjectDir() + "/vob/" + ( *it ) ).arg( m_data->getProjectDir() + "/tmp/" + ( *it ) ), INFO );
	m_movefiles.append( *it );
	( *it ) = m_data->getProjectDir() + "/vob/" + ( *it );
	movefiles.append( *it );
      }
    }
    if ( !movefiles.empty() ) {
      KURL::List fileList( movefiles );
      KURL dest( m_data->getProjectDir() + "/tmp/" );
      connect( KIO::move( fileList, dest, false ), SIGNAL( result( KIO::Job * ) ), this, SLOT( slotFinishedCheckVobDirectory( ) ) );
      kdDebug() << "(K3bDivxEncodingProcess) Backup wrong files in vob directory." << endl;
    } else {
      slotFinishedCheckVobDirectory();
    }
  } else {
    infoMessage( i18n( "Found no movie data (VOB-files)." ), K3bJob::ERROR );
  }
}

void K3bDivXEncodingProcess::restoreBackupFiles() {
  QStringList files;
  for ( QStringList::Iterator it = m_movefiles.begin(); it != m_movefiles.end(); ++it ) {
    kdDebug() << "(K3bDivXEncodingProcess) Check file <" << ( *it ) << ">." << endl;
    infoMessage( i18n( "Restore backuped file %1 to %2." ).arg( *it ).arg( m_data->getProjectDir() + "/vob/" + ( *it ) ), INFO );
    ( *it ) = m_data->getProjectDir() + "/tmp/" + ( *it );
    files.append( *it );
  }
  if ( !files.empty() ) {
    KURL::List fileList( files );
    KURL dest( m_data->getProjectDir() + "/vob/" );
    connect( KIO::move( fileList, dest, false ), SIGNAL( result( KIO::Job * ) ), this, SLOT( slotFinishedRestoreBackup() ) );
    infoMessage( i18n( "Restore backuped files to %1." ).arg( m_data->getProjectDir() + "/vob" ), INFO );
    kdDebug() << "(K3bDivxEncodingProcess) Restore backuped files to vob directory." << endl;
  }
}

void K3bDivXEncodingProcess::slotFinishedRestoreBackup(){
}


QString K3bDivXEncodingProcess::jobDescription() const
{
  return i18n("Encoding video");
}

QString K3bDivXEncodingProcess::jobDetails() const
{
  // TODO: here we could show a lot of info!
  //       choose wisely!
  return m_data->getSize();
}

#include "k3bdivxencodingprocess.moc"

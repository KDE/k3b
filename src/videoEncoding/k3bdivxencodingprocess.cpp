/***************************************************************************
                          k3bdivxencodingprocess.cpp  -  description
                             -------------------
    begin                : Sun Apr 28 2002
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

#include "k3bdivxencodingprocess.h"
#include "k3bdivxcodecdata.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../k3b.h"

#include <qdir.h>

#include <kprocess.h>
#include <klocale.h>
#include <kdebug.h>


K3bDivXEncodingProcess::K3bDivXEncodingProcess(K3bDivxCodecData *data, QWidget *parent, const char *name ) : K3bJob( ) {
    m_data = data;
    connect( this, SIGNAL( finished( bool )), SLOT( slotShutdown( bool ) ));
}

K3bDivXEncodingProcess::~K3bDivXEncodingProcess(){
}

void K3bDivXEncodingProcess::start(){
    m_pass = 0;
    m_speedFlag = 1;
    m_speedTrigger = 0;
    m_speedInitialFlag = 0;
    emit started();
    if( m_data->isNormalize() ){
        infoMessage( i18n("Copy IFO files to vob directory."), INFO );
        copyIfos(); // copy IFO files to enable dvd import mode of transcode
    } else {
        infoMessage( i18n("Disable audio normalizing"), INFO );
        deleteIfos();
    }
}

void K3bDivXEncodingProcess::slotStartAudioProcessing(KIO::Job *job){
    if( job->error() > 0 ){
        kdDebug() << "(K3bDivXEncodingProcess) Job error during copy: " << job->errorString() << endl;
    }
     kdDebug() << "(K3bDivXEncodingProcess) Run transcode." << endl;
     m_process = new KShellProcess;
     K3bExternalBin *tccatBin = k3bMain()->externalBinManager()->binObject("tccat");
     K3bExternalBin *tcextractBin = k3bMain()->externalBinManager()->binObject("tcextract");
     K3bExternalBin *tcdecodeBin = k3bMain()->externalBinManager()->binObject("tcdecode");
     K3bExternalBin *tcscanBin = k3bMain()->externalBinManager()->binObject("tcscan");
      // parse audio for gain to normalize
     *m_process << "nice" << "-10";
     *m_process << tccatBin->path << " -i" << m_data->getProjectDir() + "/vob" << "-t" << "vob" <<"-P" << m_data->getTitle();
     *m_process << "|" << tcextractBin->path << m_data->getParaAudioLanguage() << "-x" << "ac3" << "-t" << "vob";
     *m_process << "|" << tcdecodeBin->path << "-x" << "ac3";
     *m_process << "|" << tcscanBin->path << "-x" << "pcm";

     kdDebug() << "(K3bDivXEncodingProcess)" +  tccatBin->path + " -i " + m_data->getProjectDir() + "/vob -t vob -P " + m_data->getTitle()
     + " | " + tcextractBin->path + m_data->getParaAudioLanguage() + " -x ac3  -t vob "
     + " | " + tcdecodeBin->path + " -x ac3 " + " | " + tcscanBin->path + " -x pcm";

     connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
         this, SLOT(slotParseAudio(KProcess*, char*, int)) );
     connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
         this, SLOT(slotParseAudio(KProcess*, char*, int)) );

     connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotAudioExited( KProcess* )) );

     if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ){
         kdDebug() << "Error audio process starting" << endl;
     }
     //emit started();
     //emit newTask( i18n("Generating video")  );
     emit newSubTask( i18n("Preprocessing audio")  );
     infoMessage( i18n("Search for maximum audio gain to get normalized parameter."), INFO );
     kdDebug() <<"(K3bDivXEncodingProcess) Starting get audio gain." << endl;
}

void K3bDivXEncodingProcess::slotStartEncoding(){
     m_speedFlag = 301;
     m_speedTrigger = 300;
     m_speedInitialFlag = 0;
     kdDebug() << "(K3bDivXEncodingProcess) Run transcode." << endl;
     K3bExternalBin *transcodeBin = k3bMain()->externalBinManager()->binObject("transcode");
     m_process = new KShellProcess;

     *m_process << "nice" << "-10";
     *m_process << transcodeBin->path; //"/usr/local/bin/transcode -i ";
     *m_process << " -i" << m_data->getProjectDir() + "/vob ";

     *m_process << "-x" << "vob" << "-T" << m_data->getTitle() +",-1,1" << m_data->getParaVideoBitrate() + "," + m_data->getKeyframes() + ","+ m_data->getCrispness();
     kdDebug() << "(K3bDivXEncodingProcess) Video: "  + m_data->getParaVideoBitrate() + "," + m_data->getKeyframes() + ","+ m_data->getCrispness()<< endl;
     *m_process << m_data->getParaYuv() << m_data->getParaAudioLanguage();
     kdDebug() << "(K3bDivXEncodingProcess) Video: " + m_data->getParaYuv() + m_data->getParaAudioLanguage()<< endl;
     QString debugPass("");
     if( m_data->getParaAc3().length() > 1 ){
         *m_process << m_data->getParaAc3();
         debugPass += m_data->getParaAc3();
     } else {
         *m_process << m_data->getParaAudioResample()  << m_data->getParaAudioBitrate() << m_data->getParaAudioGain();
         debugPass = debugPass + m_data->getParaAudioResample() + m_data->getParaAudioBitrate() + m_data->getParaAudioGain();
     }
     *m_process << m_data->getParaDeinterlace();
     *m_process << " -o " << m_data->getAviFile();
     kdDebug() << "(K3bDivXEncodingProcess)  Out: " + m_data->getParaDeinterlace() + " -o " + m_data->getAviFile()<< endl;

     int top = m_data->getCropTop();
     int left = m_data->getCropLeft();
     int bottom = m_data->getCropBottom();
     int right = m_data->getCropRight();
     kdDebug() << "(K3bDivXEncodingProcess) Crop values t=" << top << ",l=" << left << ",b=" << bottom << ",r=" << right << endl;
     kdDebug() << "(K3bDivXEncodingProcess) Resize factor height=" << m_data->getResizeHeight() << ", width=" << m_data->getResizeWidth() << endl;
     *m_process << "-j" << QString::number(top) +","+ QString::number(left) +","+ QString::number(bottom)+","+QString::number( right );
     *m_process << "-B" << QString::number(m_data->getResizeHeight()) + "," + QString::number(m_data->getResizeWidth()) + ",8";
     if( m_data->getCodecMode() == 2 ){
         if( m_pass == 0 ){
             m_pass = 1;
             *m_process << " -R 1," + m_data->getProjectDir() + "/tmp/divx4.log" <<  m_data->getParaCodec() << ",null";
             debugPass += " -R 1,"+ m_data->getProjectDir() + "/tmp/divx4.log" + m_data->getParaCodec() + ",null";
         } else {
             m_pass = 2;
             *m_process << " -R 2,"+ m_data->getProjectDir() + "/tmp/divx4.log" <<  m_data->getParaCodec();
             debugPass += " -R 2,"+ m_data->getProjectDir() + "/tmp/divx4.log" + m_data->getParaCodec();
         }
     } else {
         *m_process << m_data->getParaCodec();
         debugPass += m_data->getParaCodec();
     }
     kdDebug() << "(K3bDivXEncodingProcess)" + transcodeBin->path + " -i " + m_data->getProjectDir() +
     "/vob -x vob -T" + m_data->getTitle() +",-1,1" +
     m_data->getParaVideoBitrate() + "," + m_data->getKeyframes() + ","+ m_data->getCrispness() +
     m_data->getParaYuv() + m_data->getParaAudioLanguage() +
     m_data->getParaDeinterlace() + " -o " + m_data->getAviFile() +
     " -j " + QString::number(top) + "," + QString::number( left) +","+ QString::number( bottom ) + "," + QString::number( right ) +
     " -B " + QString::number(m_data->getResizeHeight()) + "," + QString::number(m_data->getResizeWidth()) + ",8" + debugPass << endl;
     connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
         this, SLOT(slotParseEncoding(KProcess*, char*, int)) );
     connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
         this, SLOT(slotParseEncoding(KProcess*, char*, int)) );

     connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotEncodingExited( KProcess* )) );

     if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ){
         kdDebug() << "Error process starting" << endl;
     }
     //emit started();
     if( m_pass == 1 ){
         infoMessage( i18n("Start first pass of video encoding."), PROCESS );
         emit newSubTask( i18n("Encoding video (Pass 1)")  );
     } else if( m_pass == 2 ) {
         infoMessage( i18n("Start second pass of video encoding."), PROCESS );
         emit newSubTask( i18n("Encoding video (Pass 2)")  );
     } else {
         infoMessage( i18n("Start video encoding."), PROCESS );
         emit newSubTask( i18n("Encoding video")  );
     }
     kdDebug() << "(K3bDivXEncodingProcess) Starting encoding." << endl;
}

void K3bDivXEncodingProcess::copyIfos(){
    QDir vobs( m_data->getProjectDir() + "/tmp");
    if( vobs.exists() ){
        QStringList ifos = vobs.entryList("*.ifo");
        for ( QStringList::Iterator it = ifos.begin(); it != ifos.end(); ++it ) {
            (*it) = m_data->getProjectDir() + "/tmp/" +(*it);
        }
        KURL::List ifoList( ifos );
        KURL dest( m_data->getProjectDir() + "/vob" );
        connect( KIO::copy( ifoList, dest, false ), SIGNAL( result( KIO::Job *) ), this, SLOT( slotStartAudioProcessing( KIO::Job* ) ) );
        kdDebug() << "(K3bDivxEncodingProcess) Copy IFO files to " << dest.path() << endl;
    }
}

void K3bDivXEncodingProcess::deleteIfos(){
    // delete ifos
    QDir vobs( m_data->getProjectDir() + "/vob");
    if( vobs.exists() ){
        QStringList ifos = vobs.entryList("*.ifo");
        for ( QStringList::Iterator it = ifos.begin(); it != ifos.end(); ++it ) {
            (*it) = m_data->getProjectDir() + "/vob/" +(*it);
        }
        KURL::List ifoList( ifos );
        connect( KIO::del( ifoList, false, false ), SIGNAL( result( KIO::Job *) ), this, SLOT( slotStartEncoding( ) ) );
        kdDebug() << "(K3bDivxEncodingProcess) Delete IFO files in " << vobs.path() << endl;
    }
}

void K3bDivXEncodingProcess::cancel( ){
    m_process->kill(); // send SIGTERM
}

void K3bDivXEncodingProcess::slotParseEncoding( KProcess *p, char *buffer, int len){
    if( m_speedFlag > m_speedTrigger ){
        if( m_speedInitialFlag < 50 ){
            m_speedTrigger = 0;
            m_speedInitialFlag++;
            if( m_speedInitialFlag == 50)
               m_speedTrigger = 400;
        }
        m_speedFlag = 0;
        QString tmp = QString::fromLocal8Bit( buffer, len );
        m_debugBuffer += tmp;
        kdDebug() << tmp << endl;
        int index = tmp.find( "]" );
        tmp = tmp.mid( index - 6, 6).stripWhiteSpace();
        long f = tmp.toLong();
        int p = f *100 / m_data->getFramesValue();
        emit subPercent( p );
        if( m_pass == 1 ){
            emit percent( (p*47.5/100) + 5 );
        } else if (m_pass == 2 ){
            emit percent( (p*47.5/100) + 52.5 );
        } else {
            emit percent( (p*95/100) + 5 );
        }
    }
    m_speedFlag++;
}

void K3bDivXEncodingProcess::slotEncodingExited( KProcess *p ){
    kdDebug() << "(K3bDivxEncodingProcess) Encoding finished" << endl;
    if( !p->normalExit() ){
        infoMessage( i18n("Video generation aborted by user."), STATUS );
        kdDebug() << "(K3bDivxEncodingProcess) Aborted encoding" << endl;
        delete m_process;
        emit debuggingOutput("Videoencoding (transcode)", m_debugBuffer);
        emit finished( false );
    } else {
        delete m_process;
        if( m_pass == 1 ){
            kdDebug() << "(K3bDivxEncodingProcess) Start second pass." << endl;
            slotStartEncoding();
        } else {
            infoMessage( i18n("Video generating successful finished."), STATUS );
            emit debuggingOutput("videoencoding (transcode)", m_debugBuffer);
            emit finished( true );
        }
    }
}

void K3bDivXEncodingProcess::slotParseAudio( KProcess *p, char *buffer, int len){
    if( m_speedFlag > m_speedTrigger ){
        m_speedFlag = 0;
        if( m_speedInitialFlag < 50 ){
            m_speedTrigger = 0;
            m_speedInitialFlag++;
            if( m_speedInitialFlag == 50)
               m_speedTrigger = 40;
        }
        QString tmp = QString::fromLocal8Bit( buffer, len );
        m_debugBuffer += tmp;
        if( tmp.contains("rescale") ){
            int scale = tmp.find( "rescale" );
            tmp = tmp.mid( scale + 8).stripWhiteSpace();
            float f = tmp.toFloat();
            m_data->setAudioGain( QString::number( f, 'f', 3 ) );
            kdDebug() << "K3bDivxEncodingProcess) Audio gain: " + m_data->getAudioGain() << endl;
            infoMessage( i18n("Gain for normalizing is: %1").arg(m_data->getAudioGain()), INFO );
        }
        //kdDebug() <<  tmp << endl;
        int index = tmp.find( "%" );
        tmp = tmp.mid( index - 5, 5).stripWhiteSpace();
        float f = tmp.toFloat()+ 0.5;
        if( f > 95 )
            m_speedTrigger = 0;
        int p = (int) f;
        emit subPercent( p );
        emit percent( p / 20 );
    }
    m_speedFlag++;
}
void K3bDivXEncodingProcess::slotAudioExited( KProcess *p ){
    infoMessage( i18n("Preprocessing audio completed."), STATUS );
    kdDebug() << "(K3bDivxEncodingProcess) Audio gain detection finished" << endl;
    if( p->normalExit() ){
        delete m_process;
        deleteIfos(); // delete ifos and slotStartEncoding( );
    } else {
        slotEncodingExited( p );
    }
}

void K3bDivXEncodingProcess::slotShutdown( bool b ){
    if( b ){ // successful finished
        m_process = new KShellProcess;
        *m_process << "shutdown" << "-h" << "now";
        if( !m_process->start( KProcess::DontCare ) ){
            kdDebug() << "(K3bDivXEncodingProcess) Shutdown failed." << endl;
        }
    }
}
#include "k3bdivxencodingprocess.moc"

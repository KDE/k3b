/*
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
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


#include "k3bdvdrippingprocess.h"
#include "k3bdvdcontent.h"
#include "k3bdvdcopy.h"
#include <k3bexternalbinmanager.h>
#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bcore.h>
#include <kstdguiitem.h>
#include "k3bdvdaudiogain.h"

#include <qstring.h>
#include <qdatastream.h>
#include <qtextstream.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qregexp.h>
#include <qapplication.h>

#include <kdebug.h>
#include <kprocess.h>
#include <klocale.h>
#include <kio/job.h>
#include <kmessagebox.h>

K3bDvdRippingProcess::K3bDvdRippingProcess( K3bJobHandler* hdl, QObject *parent )
  : K3bJob( hdl, parent ) {
    //m_processAudio = processAudio;
  //    m_parent = parent;
}

K3bDvdRippingProcess::~K3bDvdRippingProcess() {
    /*
    if( m_delAudioProcess )
        delete m_audioProcess;
    */
}

void K3bDvdRippingProcess::start( ) {
    // split of process and view (m_ripJob) due to m_ripJob cannot receive signals,
    // they will always connect to k3bjob instead to k3bdvdcopy. Why ?
    m_maxTitle=m_titles.count();
    kdDebug() << "(K3bDvdRippingProcess) Starting rip " << m_maxTitle << " titles." << endl;
    m_currentRipTitle=0;
    m_currentVobIndex = 1;
    m_summaryBytes = 0;
    m_dataRateBytes = 0;
    m_rippedBytes = 0;
    m_percent = 0;
    m_interrupted = false;
    m_delAudioProcess = false;
    m_dvdOrgFilenameDetected = false;
    m_dvdAlreadyMounted = false;
    m_preProcessingFailed = false;
    m_udfMount = false;
    if( m_maxTitle > 0 ) {
        checkRippingMode();
    } else {
        kdDebug() << "(K3bDvdRippingProcess) Nothing to rip." << endl;
    }

}

void K3bDvdRippingProcess::checkRippingMode() {
    m_rippedBytes = 0;
    m_dvd = m_titles.at( m_currentRipTitle );
    m_currentRipTitle++;
    m_title = QString::number( (*m_dvd).getTitleNumber() );
    if( (*m_dvd).getMaxAngle() == 1 ) {
        kdDebug() << "K3bDvdRippingProcess) Rip Title" << endl;
        m_ripMode = "-P " + m_title;
    } else {
        kdDebug() << "K3bDvdRippingProcess) Rip Angle" << endl;
        // workaround due to buggy transcode, angle selection doesn't work with -1 (all chapters)
        // and it doesn't work with loop alone, chapters must be from - to = 0-4 or so.
        // combination of loop and chapter range works with transcode 0.6.2-20021107
        m_ripMode = "-T " + m_title + ",0-1," + m_angle + " -L";
        //QStringList::Iterator titleList = (*m_dvd).getAngles()->at( m_currentRipAngle );
        //m_currentRipAngle++;
        //m_angle = *titleList;
    }
    preProcessingDvd();
}

void K3bDvdRippingProcess::startRippingProcess( ) {
    QString f = getFilename();
    if( !f.isNull() ) {
        m_outputFile.setName( f );
    } else {
        kdDebug() << "(K3bDvdRippingProcess) Ripping not started due to vob already exists." << endl;
        //emit interrupted();
        emit finished( false );
        return;
    }
    m_outputFile.open( IO_WriteOnly );                     // open file for writing
    m_stream = new QDataStream( &m_outputFile );                        // serialize using f

    /*
    m_audioProcess = new K3bDvdAudioGain( m_dirtmp );
    if( !m_audioProcess->start() ){
        emit interrupted();
        return;
}
    connect( m_audioProcess, SIGNAL( finished() ), this, SLOT( slotAudioProcessFinished() ) );
    m_delAudioProcess = true;
    */
    const K3bExternalBin *m_tccatBin = k3bcore->externalBinManager()->binObject("tccat");
    m_ripProcess = new KShellProcess();
    kdDebug() << "(K3bDvdRippingProcess)" << m_tccatBin->path << " -i " << m_device <<" "<< m_ripMode << endl;
    *m_ripProcess << m_tccatBin->path << "-i" <<  m_device << m_ripMode;
    //*m_ripProcess << m_tccatBin->path << "-d 1" << "-i" <<  m_device << m_ripMode << m_title << ",-1," << m_angle;
    //*m_ripProcess << m_tccatBin->path << "-i" <<  m_device << m_ripMode << m_title << ",-1," << m_angle;
    connect( m_ripProcess, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(slotParseOutput(KProcess*, char*, int)) );
    //connect( m_ripProcess, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(slotParseError(KProcess*, char*, int)) );
    connect( m_ripProcess, SIGNAL(processExited(KProcess*)), this, SLOT(slotExited( KProcess* )) );
    if( !m_ripProcess->start( KProcess::NotifyOnExit, KProcess::Stdout ) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        kdDebug() << "(K3bDvdRippingProcess) Error starting rip." << endl;
        m_outputFile.close();
    }
    // successful start
    //    m_parent->close();
}

void K3bDvdRippingProcess::setDvdTitle( const QValueList<K3bDvdContent> &titles ) {
    m_titles = titles;
}

void K3bDvdRippingProcess::setRipSize( double size ) {
    //kdDebug() << "RipSize: " << (float) size << endl;
    m_titleBytes = size;
}

void K3bDvdRippingProcess::cancel( ) {
    m_interrupted = true;
}

void K3bDvdRippingProcess::slotExited( KProcess* ) {
    kdDebug() << "(K3bDvdRippingProcess) Ripping task finsihed." << endl;
    m_outputFile.close();
    delete m_ripProcess;
    kdDebug() << "(K3bDvdRippingProcess) Ripped bytes: " << m_summaryBytes << endl;
    //m_audioProcess->closeStdin();
    if( m_currentRipTitle < m_maxTitle ) {
        checkRippingMode();
    } else {
        kdDebug() << "(K3bDvdRippingProcess) Copy IFO files for audio gain processing." << endl;
        //postProcessingDvd();
        saveConfig();
	emit infoMessage( i18n("Successfully ripped all video titles to %1.").arg(m_dirvob), SUCCESS );
        emit finished( true );
        //  postProcessingFinished();
    }
}

void K3bDvdRippingProcess::slotParseOutput( KProcess *p, char *text, int len) {
    if( m_interrupted ) {
        m_outputFile.close();
        p->kill();
        //m_audioProcess->kill();
        emit finished( false );
        return;
    }
    m_stream->writeRawBytes( text, len );
    //m_audioProcess->writeStdin( text, len );
    m_rippedBytes += len;
    m_summaryBytes += len;
    if( m_titleBytes > 0 ) {
        unsigned int pc = (unsigned int) ((( m_summaryBytes / m_titleBytes) *100)+0.5);
        if ( pc > m_percent ) {
            emit percent( pc );
            m_percent = pc;
            unsigned long b = (unsigned long) ( m_summaryBytes - m_dataRateBytes );
            m_dataRateBytes = m_summaryBytes;
            emit rippedBytesPerPercent( b );
        }
    }
    if( m_rippedBytes >= 1073741824 ) {  // 24
        m_rippedBytes = 0;
        p->suspend();
        m_outputFile.close();
        delete m_stream;
        QString index;
        QString f = getFilename();
        if( !f.isNull() ) {
            m_outputFile.setName( f );
        } else {
            kdDebug() << "(K3bDvdRippingProcess) Cancel due to vob already exists." << endl;
            p->kill();
            //m_audioProcess->kill();
            //Whats the difference ?? finished/canceled
            //emit finished( false );
            emit canceled();
            return;
        }
        p->resume(); // restart process
        m_outputFile.open( IO_WriteOnly );                     // open file for writing
        m_stream = new QDataStream( &m_outputFile );                        // serialize using f
    }


}

float K3bDvdRippingProcess::tccatParsedBytes( char *text, int len) {
    QString tmp = QString::fromLocal8Bit( text, len );
    float blocks = 0;
    if( tmp.contains("blocks (") ) {
        int index = tmp.find("blocks (");
        tmp = tmp.mid(index+8); // start range i.e 3645-214245)
        int end = tmp.find( "-" );
        QString start= tmp.mid( 0, end); // first value
        float startBlocks = start.toFloat();
        tmp=tmp.mid(end+1);
        end = tmp.find( ")" );
        tmp= tmp.mid( 0, end); // end value
        blocks = (tmp.toFloat()-startBlocks)*2048;
    }
    kdDebug() << "(K3bDvdRippingProcess) Parsed bytes: " << QString::number(blocks, 'f') << endl;
    return blocks;
}

void K3bDvdRippingProcess::preProcessingDvd( ) {
    kdDebug() << "(K3bDvdRippingProcess::preProcessingDVD) Copy IFO files from device: <" << m_device << ">."<< endl;
    if( !m_dvdOrgFilenameDetected ) {
        K3bDevice *dev = k3bcore->deviceManager()->deviceByName( m_device );
        m_mountPoint = dev->mountPoint();
        if( !m_mountPoint.isEmpty() ) {
            QString mount = KIO::findDeviceMountPoint( m_device );
            kdDebug() << "(K3bDvdRippingProcess) Is mounted device:  <" << m_device << "> on <" << mount << ">." << endl;
            if( mount.isEmpty() ) {
                kdDebug() << "(K3bDvdRippingProcess) Try to mount <" << m_mountPoint << ">." << endl;
		emit newSubTask( i18n("Mounting media") );
                connect( KIO::mount( true, "autofs", "", m_mountPoint, true ),
                         SIGNAL(result(KIO::Job*)), this, SLOT( slotPreProcessingDvd(KIO::Job*) ) );
            } else {
                m_mountPoint = mount;
                m_dvdAlreadyMounted = true;
                slotPreProcessingDvd();
            }
        } else {
            KMessageBox::error(qApp->activeWindow(), i18n("K3b could not mount <%1>. Please run K3bSetup.").arg(dev->mountDevice()),
                               i18n("I/O Error") );
            emit finished( false );
        }
    }
}

void K3bDvdRippingProcess::slotPreProcessingDvd( KIO::Job *resultJob) {
  if( resultJob->error() > 0 ) {
    // do show a dialog instead of an infoMessage since the errorString spreads over multible lines. :(
    resultJob->showErrorDialog( qApp->activeWindow() );
    emit infoMessage( i18n("Mounting failed."), ERROR );

    m_preProcessingFailed = true;
    emit finished( false );
  } else {
    emit infoMessage( i18n("Successfully mounted media. Starting DVD Ripping."), INFO );
    slotPreProcessingDvd();
  }
}

void K3bDvdRippingProcess::slotPreProcessingDvd() {
    QString video;
    QDir video_ts( m_mountPoint + "/VIDEO_TS");
    if( video_ts.exists() ) {
        m_udfMount = true;
        kdDebug() << "(K3bDvdRippingProcess) <" << m_mountPoint << "> has UDF filesystem." << endl;
    }
    video_ts.setPath( m_mountPoint + "/video_ts");
    if( !video_ts.exists() && !m_udfMount){
        m_preProcessingFailed = true;
        KMessageBox::error(qApp->activeWindow(), i18n("K3b could not mount the DVD-device. Ensure that you have the rights to mount the DVD-drive and that it supports either iso9660 or udf filesystem."),
                           i18n("I/O Error") );
        kdDebug() << "(K3bDvdRippingProcess::slotPreProcessingDvD) Mount DVD-device failed." << endl;
        emit finished( false );
        return;
    }
    bool result = false;
    // read directory from /dev/dvd
    if( !m_mountPoint.isEmpty() && !m_udfMount) {
        result = copyIfoFiles("video_ts", "vts", "ifo");
        video = "video_ts";
    } else if (!m_mountPoint.isEmpty() && m_udfMount) {
        kdDebug() << "(K3bDvdRippingProcess) UDF DVD mount filessystem." << endl;
        video = "VIDEO_TS";
        result = copyIfoFiles("VIDEO_TS", "VTS", "IFO");
    }
    if( !result ) {
        m_preProcessingFailed = true;
        KMessageBox::error(qApp->activeWindow(), i18n("K3b could not copy the ifo-files from %1.").arg( m_mountPoint + "/" + video),
                           i18n("I/O Error") );
        kdDebug() << "(K3bDvdRippingProcess::slotPreProcessingDvD) Copy IFO files failed." << endl;
        emit finished( false );
    }
}

bool K3bDvdRippingProcess::copyIfoFiles( const QString& video, const QString& vts, const QString& ifo) {
    bool result = false;
    QDir video_ts( m_mountPoint + "/" + video, "*." +ifo);
    if( video_ts.exists() ) {
        QStringList ifos;  // = video_ts.entryList();
        m_videoCaseSensitive = QString(video + "." +ifo); // dvd norm
        ifos << m_mountPoint + "/" + video + "/" + m_videoCaseSensitive;
        m_vobCaseSensitive = vts + "_"+formattedTitleset()+"_0." + ifo; // titleset ifo for the current ripped title
        ifos << m_mountPoint + "/" + video + "/" + m_vobCaseSensitive;
        KURL::List ifoList = KURL::List( ifos );
        KURL dest( m_dirtmp );
        connect( KIO::copy( ifoList, dest, false ), SIGNAL( result( KIO::Job *) ), this, SLOT( slotIfoCopyFinished( KIO::Job* ) ) );
        kdDebug() << "(K3bDvdRippingProcess) Copy IFO files to " << dest.path() << endl;
        result = true;
    }
    m_dvdOrgFilenameDetected = true;
    return result;
}

void K3bDvdRippingProcess::slotIfoCopyFinished( KIO::Job *job ) {
    if( job->error() > 0 ) {
        kdDebug() << "(K3bDvdRippingProcess) Job error IFO copy: " << job->errorString() << endl;
        emit finished( false );
        KIO::unmount( m_mountPoint, true );
        return;
    }
    kdDebug() << "(K3bDvdRippingProcess) Chmod: " << m_dirtmp << "/" << m_videoCaseSensitive << endl;
    kdDebug() << "(K3bDvdRippingProcess) Chmod: " << m_dirtmp << "/" << m_vobCaseSensitive << endl;
    KIO::chmod( KURL::fromPathOrURL(m_dirtmp + "/" + m_videoCaseSensitive), 0644 );
    KIO::chmod( KURL::fromPathOrURL(m_dirtmp + "/" + m_vobCaseSensitive), 0644 );
    connect( KIO::unmount( m_mountPoint, true ), SIGNAL(result(KIO::Job*)), this, SLOT( slotPreProcessingFinished( KIO::Job* )) );
}

void K3bDvdRippingProcess::slotPreProcessingFinished( KIO::Job *job ) {
    if( job->error() > 0 ) {
        kdDebug() << "(K3bDvdRippingProcess) WARNING: Unmount failed, Job error: " << job->errorString() << endl;
    }
    startRippingProcess();
}

QString K3bDvdRippingProcess::formattedTitleset() {
    QString titleset;
    int s = (*m_dvd).getTitleSet();
    if( s < 10 )
        titleset = "0" + QString::number( s );
    else
        titleset = QString::number( s );
    return titleset;
}

QString K3bDvdRippingProcess::getFilename() {
    QString result;
    result = m_dirvob + "/vts_" + formattedTitleset() + "_" + QString::number( m_currentVobIndex ) + ".vob";
    kdDebug() << "(K3bDvdRippingProcess) Vob filename: " << result << endl;
    m_currentVobIndex++;

    QFile destFile( result );
    if( destFile.exists() ) {
        int button = QMessageBox::critical( 0, i18n("Ripping Error"), i18n("%1 already exists." ).arg(result), i18n("Overwrite"), KStdGuiItem::cancel().text() );
        if( button == 0 )
            return result;
        else
            return QString::null;
    }
    return result;
}

void K3bDvdRippingProcess::slotIfoRename( KIO::Job* job){
    if( job->error() > 0 ){
        kdDebug() << "(K3bDvdRippingProcess) Renaming IFO Files failed. Can't move files form upper to lower case."
            << "You should enable your dvd drive to mount as iso9660 filesystem instead of udf." << endl;
    }
}

void K3bDvdRippingProcess::saveConfig() {
    QFile f( m_dirname + "/k3bDVDRip.xml" );
    if( f.exists() ) {
        QString dontAskAgainName = "Overwrite k3bDVDRip.xml";
        int button = KMessageBox::questionYesNo(qApp->activeWindow(), i18n("Log file already exists. Overwrite?"), i18n("Ripping Error"),
        KStdGuiItem::yes(), KStdGuiItem::no(), dontAskAgainName) ;
        if( button != KMessageBox::Yes ) {
            kdDebug() << "(K3bDvdRippingProcess) Couldn't save ripping datas." << endl;
            return;
        }
    }
    if( m_currentRipTitle == 1 ) {
        f.open( IO_WriteOnly );
    } else {
        f.open( IO_WriteOnly | IO_Append );
    }
    QTextStream t( &f );
    /*
    QString extension( (*m_dvd).getStrAspectExtension() );
    if( extension.contains( "&" ) ){
        extension.replace( QRegExp("&"), "&amp" );
} */
    t << "<k3bDVDTitles>\n";
    t << "    <title number=\"" << (*m_dvd).getTitleNumber() << "\">\n";
    t << "        " << "<frames>" << (*m_dvd).getFrames() << "</frames>\n";
    t << "        " << "<fps>" << (*m_dvd).getFramerate() << "</fps>\n";
    t << "        " << "<time>" << (*m_dvd).getStrTime() << "</time>\n";
    // can't be detected with using kprocess
    t << "        " << "<audiogain>" << "1.0" << "</audiogain>\n";
    //    t << "        " << "<audiogain>" << getAudioGain() << "</audiogain>\n";
    t << "        " << "<aspectratio>" << (*m_dvd).getStrAspect() << "</aspectratio>\n";
    t << "        " << "<aspectratioAnamorph>" << (*m_dvd).getStrAspectAnamorph() << "</aspectratioAnamorph>\n";
    // & not supported, TODO    t << "        " << "<aspectratioExtension>" << extension << "</aspectratioExtension>\n";
    t << "        " << "<width>" << (*m_dvd).getRes().width() << "</width>\n";
    t << "        " << "<height>" << (*m_dvd).getRes().height() << "</height>\n";
    t << "        " << "<chapters>" << (*m_dvd).getMaxChapters() << "</chapters>\n";
    QStringList *list=(*m_dvd).getAudioList();
    for ( QStringList::Iterator it = list->begin(); it != list->end(); ++it ) {
        t << "        " << "<audiolanguage>" << (*it).latin1() << "</audiolanguage>\n";
    }
    t << "    </title>\n";
    t << "</k3bDVDTitles>\n";

    f.close();
}

float K3bDvdRippingProcess::getAudioGain() {
    QFile f( m_dirtmp + "/audioGain.log" );
    float result = 1.0;
    if( f.open( IO_ReadOnly ) ) {
        QTextStream t( &f );
        QString tmp;
        for( int i=0; i < 5; i++ ) {
            tmp = t.readLine();
            kdDebug() << tmp << endl;
            if( tmp.contains("rescale") ) {
                int index = tmp.find( "rescale=" );
                tmp = tmp.mid(index+8);
                result = tmp.stripWhiteSpace().toFloat();
                kdDebug() << "(K3bDvdRippingProcess) Found audio gain: " << result << endl;
                break;
            }
        }
        f.close();
    } else {
        QMessageBox::critical( 0, i18n("Ripping Error"), i18n("Unable to get data for audio normalizing. Use default of 1.0."), KStdGuiItem::ok().text() );
    }
    return result;
}

#include "k3bdvdrippingprocess.moc"

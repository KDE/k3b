/***************************************************************************
                          k3bdvdrippingprocess.cpp  -  description
                             -------------------
    begin                : Sun Mar 3 2002
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

#include "k3bdvdrippingprocess.h"
#include "k3bdvdcontent.h"
#include "k3bdvdcopy.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../k3b.h"
#include "k3bdvdaudiogain.h"

#include <qstring.h>
#include <qdatastream.h>
#include <qtextstream.h>
#include <qdir.h>
#include <qmessagebox.h>

#include <kprocess.h>
#include <klocale.h>

K3bDvdRippingProcess::K3bDvdRippingProcess( QWidget *parent) : QObject() {
    //m_processAudio = processAudio;
    m_parent = parent;
}

K3bDvdRippingProcess::~K3bDvdRippingProcess(){
    if( m_delAudioProcess )
        delete m_audioProcess;
}

void K3bDvdRippingProcess::start( ){
    // split of process and view (m_ripJob) due to m_ripJob cannot receive signals,
    // they will always connect to k3bjob instead to k3bdvdcopy. Why ?
    m_maxTitle = m_titles.count();
    qDebug("(K3bDvdRippingProcess) Starting rip %i titles.", m_maxTitle );
    m_currentRipTitle = 0;
    m_currentRipAngle = 0;
    m_currentVobIndex = 1;
    m_summaryBytes = 0;
    m_titleBytes = 0;
    m_interrupted = false;
    m_delAudioProcess = false;
    if( m_maxTitle > 0 ){
        checkRippingMode();
    } else {
        qDebug("(K3bDvdRippingProcess) Nothing to rip.");
    }
}

void K3bDvdRippingProcess::checkRippingMode(){
    m_rippedBytes = 0;
    m_dvd = m_titles.at( m_currentRipTitle );
    m_currentRipTitle++;
    m_title = QString::number( (*m_dvd).getTitleNumber() );
    if( (*m_dvd).isAllAngle() ){
        qDebug("K3bDvdRippingProcess) Rip Title");
        m_ripMode = "-P";
    } else {
        qDebug("K3bDvdRippingProcess) Rip Angle");
        m_ripMode = "-T";
        QStringList::Iterator titleList = (*m_dvd).getSelectedAngle()->at( m_currentRipAngle );
        m_currentRipAngle++;
        m_angle = *titleList;
    }
    rip();
}

void K3bDvdRippingProcess::rip( ){
    QString f = prepareFilename();
    if( !f.isNull() ){
        m_outputFile.setName( f );
    } else {
        emit interrupted();
        return;
    }
    m_outputFile.open( IO_WriteOnly );                     // open file for writing
    m_stream = new QDataStream( &m_outputFile );                        // serialize using f

    m_audioProcess = new K3bDvdAudioGain( m_dirtmp );
    if( !m_audioProcess->start() ){
        emit interrupted();
        return;
    }
    connect( m_audioProcess, SIGNAL( finished() ), this, SLOT( slotAudioProcessFinished() ) );
    m_delAudioProcess = true;
    K3bExternalBin *bin = k3bMain()->externalBinManager()->binObject("tccat");
    m_ripProcess = new KShellProcess();
    qDebug("(K3bDvdRippingProcess) tccat -d 1 -i " + m_device +" "+ m_ripMode +" "+ m_title +",-1," +m_angle);
    *m_ripProcess << bin->path << "-d 1" << "-i" <<  m_device << m_ripMode << m_title << ",-1," << m_angle;
    connect( m_ripProcess, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(slotParseOutput(KProcess*, char*, int)) );
    connect( m_ripProcess, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(slotParseError(KProcess*, char*, int)) );
    connect( m_ripProcess, SIGNAL(processExited(KProcess*)), this, SLOT(slotExited( KProcess* )) );
    if( !m_ripProcess->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        qDebug("(K3bDvdRippingProcess) Error starting rip.");
        m_outputFile.close();
    }
    delete bin;
    // successful start
    m_parent->close();
}

void K3bDvdRippingProcess::setDvdTitle( const QValueList<K3bDvdContent> &titles ){
    m_titles = titles;
}
/*
void K3bDvdRippingProcess::setJob( K3bDvdCopy *job ){
    m_ripJob = job;
}
*/
void K3bDvdRippingProcess::cancel( ){
    m_interrupted = true;
}

void K3bDvdRippingProcess::slotExited( KProcess* ){
    qDebug("(K3bDvdRippingProcess) Ripping task finsihed.");
    m_outputFile.close();
    delete m_ripProcess;
    qDebug("(K3bDvdRippingProcess) Ripped bytes: " + QString::number( m_summaryBytes ) );
    m_audioProcess->closeStdin();
    if( m_currentRipTitle < m_maxTitle ){
        checkRippingMode();
    } else {
        qDebug("(K3bDvdRippingProcess) Send finish.");
        //m_ripJob->ripFinished( true );
        emit finished( true );
    }
}

void K3bDvdRippingProcess::slotAudioProcessFinished(){
    qDebug("(K3bDvdRippingProcess) Save audio/config data.");
    delete m_audioProcess;
    saveConfig();
}

void K3bDvdRippingProcess::slotParseOutput( KProcess *p, char *text, int len){
    if( m_interrupted ){
        m_outputFile.close();
        p->kill();
        //m_audioProcess->kill();
        return;
    }
    m_stream->writeRawBytes( text, len );
    m_audioProcess->writeStdin( text, len );
    m_rippedBytes += len;
    m_summaryBytes += len;
    if( m_titleBytes > 0 ){
        int pc = (int) ((((double) m_rippedBytes / (double) m_titleBytes) *100)+0.5);
        //qDebug("percent %i" , pc );
        emit progressPercent( pc );
    }
    if( m_rippedBytes >= 1073741824 ) {  // 24
        m_rippedBytes = 0;
        p->suspend();
        m_outputFile.close();
        delete m_stream;
        QString index;
        QString f = prepareFilename();
        if( !f.isNull() ){
            m_outputFile.setName( f );
        } else {
            p->kill();
            m_audioProcess->kill();
            emit interrupted();
            return;
        }
        p->resume(); // stop process
        m_outputFile.open( IO_WriteOnly );                     // open file for writing
        m_stream = new QDataStream( &m_outputFile );                        // serialize using f
    }
}

void K3bDvdRippingProcess::slotParseError( KProcess *p, char *text, int len){
    qDebug("(K3bDvdRippingProcess) Calculate bytes to rip.");
    m_titleBytes = tccatParsedBytes( text, len );
    m_ripProcess->disconnect( SIGNAL(receivedStderr(KProcess*, char*, int)), this );
}

long K3bDvdRippingProcess::tccatParsedBytes( char *text, int len){
    QString tmp = QString::fromLatin1( text, len );
    long blocks = 0;
    if( tmp.contains("blocks (0") ){
        int index = tmp.find("blocks (0");
        tmp = tmp.mid(index +10);
        int end = tmp.find( ")" );
        tmp= tmp.mid( 0, end);
        blocks = tmp.toLong()*2048;
    }
    return blocks;
}

QString K3bDvdRippingProcess::prepareFilename( ){
    QString index;
    index = QString::number( m_currentRipTitle );
    if( index.length() == 1 )
        index = "0" + index;
    QString result = m_dirvob + "/" + "vts_"+index+ "_" + QString::number( m_currentVobIndex ) + ".vob";
    m_currentVobIndex++;
    QFile destFile( result );
    if( destFile.exists() ){
        int button = QMessageBox::critical( 0, i18n("Ripping Error"), result + i18n(" already exists." ), i18n("Overwrite"), i18n("Cancel") );
        if( button == 0 )
            return result;
        else
            return QString::null;
    }
    return result;
}

void K3bDvdRippingProcess::saveConfig(){
    QFile f( m_dirname + "/k3bDVDRip.log" );
    if( f.exists() ){
        int button = QMessageBox::critical( 0, i18n("Ripping Error"), i18n("Log file already exists."), i18n("Overwrite"), i18n("Cancel") );
        if( button != 0 ){
            qDebug("(K3bDvdRippingProcess) Couldn't save ripping datas.");
            return;
        }
    }
    if( m_currentRipTitle == 1 ){
        f.open( IO_WriteOnly );
    } else {
        f.open( IO_WriteOnly | IO_Append );
    }
    QTextStream t( &f );
    t << "<k3bDVDTitles>\n";
    t << "    <title number=" << (*m_dvd).getTitleNumber() << ">\n";
    t << "        " << "<frames>" << (*m_dvd).getFrames() << "</frames>\n";
    t << "        " << "<fps>" << (*m_dvd).getFramerate() << "</fps>\n";
    t << "        " << "<time>" << (*m_dvd).getStrTime() << "</time>\n";
    t << "        " << "<audiogain>" << getAudioGain() << "</audiogain>\n";
    t << "        " << "<aspectratio>" << (*m_dvd).getStrAspect() << "<aspectratio>\n";
    t << "        " << "<width>" << (*m_dvd).getRes().width() << "</widht>\n";
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

float K3bDvdRippingProcess::getAudioGain(){
    QFile f( m_dirtmp + "/audioGain.log" );
    float result = 1.0;
    if( f.open( IO_ReadOnly ) ){
        QTextStream t( &f );
        QString tmp;
        for( int i=0; i < 5; i++ ){
            tmp = t.readLine();
            qDebug(tmp);
            if( tmp.contains("rescale") ){
                int index = tmp.find( "rescale=" );
                tmp = tmp.mid(index+8);
                result = tmp.stripWhiteSpace().toFloat();
                qDebug("(K3bDvdRippingProcess) Found audio gain: %f", result );
                break;
            }
        }
        f.close();
    } else {
        QMessageBox::critical( 0, i18n("Ripping Error"), i18n("Couldn't get data for audio normalizeing, use default of 1.0."), i18n("Ok") );
    }
    return result;
}

#include "k3bdvdrippingprocess.moc"
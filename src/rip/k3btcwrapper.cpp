/***************************************************************************
                          k3btcwrapper.cpp  -  description
                             -------------------
    begin                : Sat Feb 23 2002
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

#include "k3btcwrapper.h"
#include "k3bdvdcontent.h"
#include "../device/k3bdevice.h"
#include "../device/k3bdevicemanager.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../k3b.h"

#include <qstring.h>

#include <kprocess.h>
#include <klocale.h>

K3bTcWrapper::K3bTcWrapper(){
}
K3bTcWrapper::~K3bTcWrapper(){
}

bool K3bTcWrapper::supportDvd(){
    return k3bMain()->externalBinManager()->foundBin("tcprobe");
}

void K3bTcWrapper::checkDvd( const QString& device) {
    m_firstProbeDone = false;
    m_currentTitle = 1;
    m_device = device;
    runTcprobe( device );
    m_dvdTitles.clear();
}

void K3bTcWrapper::runTcprobe( const QString& device ){
    m_outputBuffer = QString::null;
    m_errorBuffer = QString::null;
    K3bDevice *dev = k3bMain()->deviceManager()->deviceByName( device );
    K3bExternalBin *bin = k3bMain()->externalBinManager()->binObject("tcprobe");
    KShellProcess *p = new KShellProcess();
    *p << bin->path << "-i" <<  dev->ioctlDevice() << "-T" << QString::number(m_currentTitle);
    connect( p, SIGNAL(receivedStderr(KProcess*, char*, int)), this, SLOT(slotParseTcprobeError(KProcess*, char*, int)) );
    connect( p, SIGNAL(receivedStdout(KProcess*, char*, int)), this, SLOT(slotParseTcprobeOutput(KProcess*, char*, int)) );
    connect( p, SIGNAL(processExited(KProcess*)), this, SLOT(slotTcprobeExited( KProcess* )) );

    if( !p->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        qDebug("(K3bDirView) Error during checking drive for DVD.");
    }
}

void K3bTcWrapper::slotParseTcprobeOutput( KProcess *p, char *text, int len){
    m_outputBuffer += QString::fromLatin1( text, len );
}

void K3bTcWrapper::slotParseTcprobeError( KProcess *p, char *text, int len){
    m_errorBuffer += QString::fromLatin1( text, len );
}

void K3bTcWrapper::slotTcprobeExited( KProcess *p){
    qDebug("(K3bTcWrapper) Tcprobe output\n" + m_outputBuffer);
    qDebug("(K3bTcWrapper) Tcprobe error\n" + m_errorBuffer);
    qDebug("(K3bTcWrapper) Tcprobe finished");
    // split to lines
    QStringList errorLines = QStringList::split( "\n", m_errorBuffer );
    if( !m_firstProbeDone ) {
        // check dvd
        QStringList::Iterator str = errorLines.begin();
        if( !(*str).contains("tcprobe") && !(*str).contains("DVD image/device") ) {
            emit notSupportedDisc( );
            return;
        }
        // chekc
        QString titles = errorLines[ 1 ];
        int index = titles.find("title");
        titles = titles.mid(index+6);
        index= titles.find("/");
        m_currentTitle = titles.left(index).toInt();
        titles= titles.mid(index+1);
        index = titles.find(":");
        m_allTitle = titles.left(index).toInt();
        m_firstProbeDone = true;
        qDebug("(K3bTcWrapper) Found titles %i/%i", m_currentTitle, m_allTitle);
    }
    if( m_currentTitle <= m_allTitle ){
        K3bDvdContent con( *parseTcprobe() );
        QString titles = errorLines[ 1 ];
        int index = titles.find(":");
        int end = titles.find("chap");
        qDebug("Title: " + titles.mid(index+1, end-index ));
        qDebug("Chapters " + QString::number(titles.mid(index+1, end-index-1 ).stripWhiteSpace().toInt() ) );
        con.chapters = (titles.mid(index+1, end-index-1).stripWhiteSpace()).toInt();
        m_dvdTitles.append( con );
        m_currentTitle++;
        if( m_currentTitle <= m_allTitle) {
            runTcprobe( m_device );
        } else {
            emit successfulDvdCheck( true );
        }
    }
}

K3bDvdContent* K3bTcWrapper::parseTcprobe(){
    QStringList outputLines = QStringList::split( "\n", m_outputBuffer );
    // check content
    int dvdreaderIndex = 0;
    K3bDvdContent *title = new K3bDvdContent();
    for( QStringList::Iterator str = outputLines.begin(); str != outputLines.end(); str++ ) {
        if( (*str).contains( "dvd_reader.c" ) ) {
            int index = (*str).find( ")" );
            QString tmp = (*str).mid( index+1 );
            if( dvdreaderIndex > 0 ){
                // audio channels
                title->m_audioList.append( tmp );
            } else {
                // input mode
                QStringList mode = QStringList::split( " ", tmp );
                title->m_input = mode[0];
                title->m_mode =  mode[1];
            }
            dvdreaderIndex++;
        } else if( (*str).contains("frame size") ){
            int index = (*str).find(": -g");
            int end = (*str).find( " ", index+5);
            title->m_res = (*str).mid( index+5, end-index-5 );
        } else if( (*str).contains("aspect") ){
            int index = (*str).find("ratio:");
            int end = (*str).find( " ", index+7);
            title->m_aspect = (*str).mid( index+7, end-index-7 );
        } else if( (*str).contains("frame rate") ){
            int index = (*str).find(": -f");
            int end = (*str).find( " ", index+5);
            title->m_framerate = (*str).mid( index+5, end-index-5 );
        } else if( (*str).contains("[tcprobe] V:") ){
            int index = (*str).find("V:");
            int end = (*str).find( " ", index+3);
            title->m_frames = (*str).mid( index+3, end-index-3 );
            index = (*str).find("frames,");
            end = (*str).find( " ", index+8);
            title->m_time = (*str).mid( index+8, end-index-8 );
        } else if( (*str).contains("[tcprobe] A:") ){
            int index = (*str).find("A:");
            title->m_audio = (*str).mid( index+3 );
        } else if( (*str).contains("700 MB |") ){
            int index = (*str).find("700");
            title->m_video = (*str).mid( index+13 );
        }
    }
    return title;
}

QValueList<K3bDvdContent> K3bTcWrapper::getDvdTitles( ) const {
    return m_dvdTitles;
}

#include "k3btcwrapper.moc"
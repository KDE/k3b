/***************************************************************************
                          k3bdivxtcprobeac3.cpp  -  description
                             -------------------
    begin                : Sun Jul 28 2002
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

#include "k3bdivxtcprobeac3.h"
#include "k3bdivxcodecdata.h"
#include "k3bdivxhelper.h"
#include "../tools/k3bexternalbinmanager.h"
#include "../k3b.h"

#include <kprocess.h>
#include <kdebug.h>

K3bDivXTcprobeAc3::K3bDivXTcprobeAc3(){
}
K3bDivXTcprobeAc3::~K3bDivXTcprobeAc3(){
}

void K3bDivXTcprobeAc3::parseAc3Bitrate( K3bDivxCodecData* data){
    m_data = data;
    m_util = new K3bDivxHelper;
    m_util->deleteIfos( m_data );
    connect( m_util, SIGNAL( finished( bool ) ), this, SLOT( slotInternalParsing() ));
}

void K3bDivXTcprobeAc3::slotInternalParsing(){
     kdDebug() << "(K3bDivXTcprobeAc3:parseAc3Bitrate) Search ac3 bitrate." << endl;
     delete m_util;
     m_buffer = "";
     m_process = new KShellProcess;
     const K3bExternalBin *tcprobeBin = k3bMain()->externalBinManager()->binObject("tcprobe");

     *m_process << tcprobeBin->path << " -i" << m_data->getProjectDir() + "/vob";
     kdDebug() << "(K3bDivXTcprobeAc3)" +  tcprobeBin->path + " -i " + m_data->getProjectDir() + "/vob" << endl;

     connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
         this, SLOT(slotParseOutput(KProcess*, char*, int)) );
     //connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
     //    this, SLOT(slotParseError(KProcess*, char*, int)) );

     connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotParsingExited( KProcess* )) );

     if( !m_process->start( KProcess::NotifyOnExit, KProcess::Stdout ) ){
         kdDebug() << "Error probing ac3 bitrate process starting" << endl;
     }
}

void K3bDivXTcprobeAc3::slotParseOutput( KProcess*, char* buffer, int length ){
    QString tmp = QString::fromLocal8Bit( buffer, length );
    //kdDebug() << "(K3bDivXTcprobeAc3) " << tmp << endl;
    m_buffer += tmp;
}

void K3bDivXTcprobeAc3::slotParsingExited( KProcess* ){
    kdDebug() << "(K3bDivXTcprobeAc3) Exited:" << m_buffer << endl;
    QStringList lines = QStringList::split( "\n", m_buffer );
    int lindex = 0;
    bool nextTrack = false;
    QString bitrate = "";
    for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
        if ( nextTrack ){
            nextTrack = false;
            int index = (*str).find( "bitrate=" );
            bitrate = (*str).mid( index+8, 3).stripWhiteSpace();
            kdDebug() << "(K3bDivXTcprobeAc3) Detect bitrate ("<< bitrate << " kbps) for languange " << lindex << endl;
            m_data->addLanguageAc3Bitrate( bitrate );
            lindex++;
        }
        if( (*str).contains("audio track:") ) {
            nextTrack = true;
        }
    }
    delete m_process;
    emit finished();
}

#include "k3bdivxtcprobeac3.moc"

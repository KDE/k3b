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


#include "k3bdivxtcprobeac3.h"
#include "k3bdivxcodecdata.h"
#include "k3bdivxhelper.h"
#include <k3bexternalbinmanager.h>
#include <k3bcore.h>

#include <kprocess.h>
#include <kdebug.h>

K3bDivXTcprobeAc3::K3bDivXTcprobeAc3()
  : m_process(0),
    m_util(0)
{
}

K3bDivXTcprobeAc3::~K3bDivXTcprobeAc3()
{
  delete m_process;
  delete m_util;
}

void K3bDivXTcprobeAc3::parseAc3Bitrate( K3bDivxCodecData* data){
    m_data = data;
    if( !m_util ) {
      m_util = new K3bDivxHelper;
      connect( m_util, SIGNAL( finished( bool ) ), this, SLOT( slotInternalParsing() ));
    }
    m_util->deleteIfos( m_data );
}

void K3bDivXTcprobeAc3::slotInternalParsing(){
     kdDebug() << "(K3bDivXTcprobeAc3:parseAc3Bitrate) Search ac3 bitrate." << endl;
     m_buffer = "";
     delete m_process;
     m_process = new KShellProcess;
     const K3bExternalBin *tcprobeBin = k3bcore->externalBinManager()->binObject("tcprobe");

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
    emit finished();
}

#include "k3bdivxtcprobeac3.moc"

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
#include "../tools/k3bexternalbinmanager.h"
#include "../k3b.h"

#include <kprocess.h>
#include <kdebug.h>

K3bDivXTcprobeAc3::K3bDivXTcprobeAc3(){
}
K3bDivXTcprobeAc3::~K3bDivXTcprobeAc3(){
}

void K3bDivXTcprobeAc3::parseAc3Bitrate( K3bDivxCodecData* data){
     kdDebug() << "(K3bDivXTcprobeAc3:parseAc3Bitrate) Search ac3 bitrate." << endl;
     m_data = data;
     m_buffer = "";
     m_process = new KShellProcess;
     const K3bExternalBin *tcprobeBin = k3bMain()->externalBinManager()->binObject("tcprobe");
      // parse audio for gain to normalize
     *m_process << tcprobeBin->path << " -i" << m_data->getProjectDir() + "/vob";
     kdDebug() << "(K3bDivXTcprobeAc3)" +  tcprobeBin->path + " -i " + m_data->getProjectDir() + "/vob" << endl;

     connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
         this, SLOT(slotParseOutput(KProcess*, char*, int)) );
     connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
         this, SLOT(slotParseError(KProcess*, char*, int)) );

     connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotParsingExited( KProcess* )) );

     if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ){
         kdDebug() << "Error probing ac3 bitrate process starting" << endl;
     }
}

void K3bDivXTcprobeAc3::slotParseOutput( KProcess* p, char* buffer, int length ){
    QString tmp = QString::fromLocal8Bit( buffer, length );
    m_buffer += tmp;
    kdDebug() << "(K3bDivXTcprobeAc3) Output:" << tmp << endl;
}

void K3bDivXTcprobeAc3::slotParseError( KProcess* p, char* buffer, int length ){
    QString tmp = QString::fromLocal8Bit( buffer, length );
    m_buffer += tmp;
    kdDebug() << "(K3bDivXTcprobeAc3) Error:" <<tmp << endl;
}

void K3bDivXTcprobeAc3::slotParsingExited( KProcess* p){
    kdDebug() << "(K3bDivXTcprobeAc3) Exited:" << m_buffer << endl;
    QStringList lines = QStringList::split( "\n", m_buffer );
    delete m_process;
}

#include "k3bdivxtcprobeac3.moc"

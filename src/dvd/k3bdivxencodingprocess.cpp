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
#include "k3bdvdcodecdata.h"

#include "kprocess.h"
#include "klocale.h"

K3bDivXEncodingProcess::K3bDivXEncodingProcess(K3bDvdCodecData *data, QWidget *parent, const char *name ) : K3bJob( ) {
     m_data = data;
}

K3bDivXEncodingProcess::~K3bDivXEncodingProcess(){
}

void K3bDivXEncodingProcess::start(){
     qDebug("(K3bDivXEncodingProcess) Run transcode.");
     m_process = new KShellProcess;
     *m_process << "/usr/local/bin/transcode -i ";
     qDebug("Projectdir: %s/vob", m_data->getProjectDir().latin1());
     *m_process << m_data->getProjectDir() + "/vob ";
     *m_process << " -x vob -y xvid -w 1200 -V -a 0 -c 0-1000 -L 450000 ";
     *m_process << " -o " + m_data->getProjectDir() + "/k3btest.avi ";
     //p << m_data->getAviFile();
     int top = m_data->getCropTop();
     int left = m_data->getCropLeft();
     int bottom = m_data->getCropBottom();
     int right = m_data->getCropRight();
     qDebug("(K3bDivXEncodingProcess) Crop values t=%i,l=%i,b=%i,r=%i",top, left, bottom, right);
     qDebug("(K3bDivXEncodingProcess) Resize factor height=%i, width=%i", m_data->getResizeHeight(), m_data->getResizeWidth());
     *m_process << "-j " + QString::number(top) +","+ QString::number(left) +","+ QString::number(bottom)+","+QString::number( right );
     *m_process << "-B " + QString::number(m_data->getResizeHeight()) + "," + QString::number(m_data->getResizeWidth()) + ",8";
     connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
         this, SLOT(slotParseProcess(KProcess*, char*, int)) );
     connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
         this, SLOT(slotParseProcess(KProcess*, char*, int)) );

     connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited( KProcess* )) );

     if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ){
         qDebug("Error process starting");
     }
     emit started();
     emit newTask( i18n("Encoding video")  );
     qDebug("(K3bDivXEncodingProcess) Starting encoding.");
}

void K3bDivXEncodingProcess::cancel( ){
}

void K3bDivXEncodingProcess::slotParseProcess( KProcess *p, char *buffer, int len){
    QString tmp = QString::fromLatin1( buffer, len );
    qDebug( "%s", tmp.latin1() );
}

void K3bDivXEncodingProcess::slotProcessExited( KProcess *p ){
    qDebug("finished");
    delete m_process;
    emit finished( true );
}

#include "k3bdivxencodingprocess.moc"

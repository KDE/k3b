/***************************************************************************
                          k3bdvdaudiogain.cpp  -  description
                             -------------------
    begin                : Sun Mar 10 2002
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

#include "k3bdvdaudiogain.h"
#include "../k3b.h"
#include "../tools/k3bexternalbinmanager.h"

#include <qstring.h>
#include <qdir.h>

#include <kdebug.h>
#include <kprocess.h>

K3bDvdAudioGain::K3bDvdAudioGain( const QString &dir) : QObject() {
    m_dirname = dir;
}

K3bDvdAudioGain::~K3bDvdAudioGain(){
}

bool K3bDvdAudioGain::start(){
    const K3bExternalBin *tcextract = k3bMain()->externalBinManager()->binObject("tcextract");
    if( tcextract == 0 ){
        kdDebug() << "(K3bDvdAudioGain) Fatal Error, couldn't find tcextract tools." << endl;
        return false;
    }
    const K3bExternalBin *tcdecode = k3bMain()->externalBinManager()->binObject("tcdecode");
    if( tcdecode == 0 ){
        kdDebug() << "(K3bDvdAudioGain) Fatal Error, couldn't find tcdecode tools." << endl;
        return false;
    }
    const K3bExternalBin *tcscan = k3bMain()->externalBinManager()->binObject("tcscan");
    if( tcscan == 0 ){
        kdDebug() << "(K3bDvdAudioGain) Fatal Error, couldn't find tcscan tools." << endl;
        return false;
    }
    m_audioProcess = new KShellProcess();
    QDir testDir( m_dirname );
    if( !testDir.exists() ){
        bool result = testDir.mkdir( m_dirname );
        if( result ){
            kdDebug() << "(K3bDvdAudioGain) Fatal Error, couldn't create directory <" << m_dirname << ">." << endl;
            return false;
        }
    }
    QString outputfile = m_dirname + "/audioGain.log";
    kdDebug() << "K3bDvdAudioGain) Audio processing " << tcextract->path << " -x ac3 -t vob | " << tcdecode->path << " -x ac3 | " << tcscan->path << " -x pcm ";
    *m_audioProcess << tcextract->path << " -x ac3 -t vob |" << tcdecode->path << " -x ac3 |" << tcscan->path << " -x pcm " << "&>" << outputfile;
    //connect( m_audioProcess, SIGNAL( receivedStdout( KProcess*, char*, int)), this, SLOT(slotParseOutput(KProcess*, char*, int)) );
    //connect( m_audioProcess, SIGNAL( receivedStderr( KProcess*, char*, int)), this, SLOT(slotParseError(KProcess*, char*, int)) );
    //connect( m_audioProcess, SIGNAL( wroteStdin( KProcess* )), this, SLOT(slotWroteStdin( KProcess* )) );
    connect( m_audioProcess, SIGNAL( processExited(KProcess*)), this, SLOT(slotExited( KProcess* )) );
    if( !m_audioProcess->start( KProcess::NotifyOnExit, KProcess::Stdin ) ){
        kdDebug() << "(K3bDvdAudioGain) Error starting audio processing." << endl;
        return false;
    }
    return true;
}

void K3bDvdAudioGain::writeStdin( const char *buffer, int len ){
    //kdDebug() << "(K3bDvdAudioGain) Write stdin." << endl;
    m_audioProcess->writeStdin( buffer, len );
}

void K3bDvdAudioGain::kill( ){
    kdDebug() << "(K3bDvdAudioGain) Kill AudioProcessing." << endl;
    m_audioProcess->kill();
}

void K3bDvdAudioGain::closeStdin( ){
    kdDebug() << "(K3bDvdAudioGain) Close Stdin." << endl;
    if( !m_audioProcess->closeStdin()){
        kdDebug() << "(K3bDvdAudioGain) Close failed." << endl;
    }
}

void K3bDvdAudioGain::slotParseOutput( KProcess *, char *buffer, int len ){
    QString tmp = QString::fromLocal8Bit( buffer, len );
    kdDebug() << "(K3bDvdAudioGain) AudioProcessing output: " << tmp << endl;
}

void K3bDvdAudioGain::slotParseError( KProcess *, char *buffer, int len ){
    QString tmp = QString::fromLocal8Bit( buffer, len );
    kdDebug() << "(K3bDvdAudioGain) AudioProcessing error: " << tmp << endl;
}

void K3bDvdAudioGain::slotExited( KProcess *){
    kdDebug() << "(K3bDvdAudioGain) AudioProcessing finished." << endl;
    emit finished();
}

void K3bDvdAudioGain::slotWroteStdin( KProcess *p){
    //kdDebug() << "(K3bDvdAudioGain) Wrote Stdin." << endl;
}

#include "k3bdvdaudiogain.moc"
